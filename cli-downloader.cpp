// SPDX-License-Identifier: GPL-2.0
#include "qt-models/diveimportedmodel.h"
#include "core/configuredivecomputer.h"
#include "core/downloadfromdcthread.h"
#include "core/subsurfacestartup.h"
#include <QCoreApplication>
#include <QThread>
#include <QDir>

// Expand tilde in filenames
static QString expandTilde(const QString &path)
{
	if (path.startsWith("~/")) {
		return QDir::homePath() + path.mid(1);
	}
	return path;
}

// Perform a firmware update from the command line. Returns 0 on success, non-zero on failure.
int cliFirmwareUpdate(const std::string &vendor, const std::string &product, const std::string &device, const std::string &filename, bool force)
{
	ConfigureDiveComputer config;
	DCDeviceData localData;

	// prepare device_data
	localData.setVendor(QString::fromStdString(vendor));
	localData.setProduct(QString::fromStdString(product));
	localData.setDevName(QString::fromStdString(device));
	localData.setBluetoothMode(false);
	device_data_t *data = localData.internalData();

	// Set up the descriptor (needed for dc_open)
	QString vendorLower = QString::fromStdString(vendor).toLower();
	QString productLower = QString::fromStdString(product).toLower();
	data->descriptor = descriptorLookup[vendorLower + productLower];
	if (!data->descriptor) {
		fprintf(stderr, "Unknown dive computer: %s %s\n", vendor.c_str(), product.c_str());
		return 1;
	}

	// If descriptor not filled yet, call fill_computer_list() in caller before this
	QString err = config.dc_open(data);
	if (!err.isEmpty()) {
		fprintf(stderr, "Failed to open device: %s\n", qPrintable(err));
		return 2;
	}

	// Expand tilde in filename
	QString expandedFilename = expandTilde(QString::fromStdString(filename));

	// Start firmware update and wait for thread to finish via state changes/signals.
	// The ConfigureDiveComputer starts a thread; we'll connect to signals and wait in a local event loop.
	bool hadError = false;
	QObject::connect(&config, &ConfigureDiveComputer::error, [&hadError](QString e){
		fprintf(stderr, "Firmware update error: %s\n", e.toLocal8Bit().constData());
		hadError = true;
	});
	QObject::connect(&config, &ConfigureDiveComputer::message, [](QString m){
		fprintf(stdout, "%s\n", m.toLocal8Bit().constData());
	});
	QObject::connect(&config, &ConfigureDiveComputer::progress, [](int percent){
		fprintf(stdout, "Progress: %d%%\r", percent);
		fflush(stdout);
	});

	config.startFirmwareUpdate(expandedFilename, data, force);

	// Wait until state becomes DONE or ERRORED
	while (config.currentState != ConfigureDiveComputer::DONE && config.currentState != ConfigureDiveComputer::ERRORED) {
		QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
		QThread::msleep(50);
	}

	// Let any remaining events be processed (important for thread cleanup)
	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	QThread::msleep(100);

	// Check if we had errors or if the state indicates failure
	if (hadError || config.currentState == ConfigureDiveComputer::ERRORED || !config.lastError.isEmpty()) {
		if (!config.lastError.isEmpty()) {
			fprintf(stderr, "Firmware update failed: %s\n", qPrintable(config.lastError));
		} else if (!hadError) {
			fprintf(stderr, "Firmware update failed with unknown error\n");
		}
		// Don't call dc_close on error - the thread may have already closed it or left it in a bad state
		return 3;
	}

	fprintf(stdout, "\nFirmware update completed successfully\n");
	// Don't call dc_close() - the firmware update thread has already closed the device
	// Calling it again causes errors like "Failed to receive the echo"
	return 0;
}

void cliDownloader(const std::string &vendor, const std::string &product, const std::string &device)
{
	DiveImportedModel diveImportedModel;
	DiveImportedModel::connect(&diveImportedModel, &DiveImportedModel::downloadFinished, [] {
		// do something useful at the end of the download
		printf("Finished\n");
	});

	auto data = diveImportedModel.thread.data();
	data->setVendor(QString::fromStdString(vendor));
	data->setProduct(QString::fromStdString(product));
	data->setBluetoothMode(false);
	if (data->vendor() == "Uemis") {
		QString devname = QString::fromStdString(device);
		int colon = devname.indexOf(QStringLiteral(":\\ (UEMISSDA)"));
		if (colon >= 0) {
			devname.truncate(colon + 2);
			fprintf(stderr, "shortened devname to \"%s\"", qPrintable(devname));
		}
		data->setDevName(devname);
	} else {
		data->setDevName(QString::fromStdString(device));
	}

	// some assumptions - should all be configurable
	data->setForceDownload(false);
	data->setSaveLog(true);
	data->setSaveDump(false);
	data->setSyncTime(false);

	diveImportedModel.startDownload();
	diveImportedModel.waitForDownload();
	diveImportedModel.recordDives();
}
