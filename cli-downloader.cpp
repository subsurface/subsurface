// SPDX-License-Identifier: GPL-2.0
#include "qt-models/diveimportedmodel.h"
#include "core/configuredivecomputer.h"
#include "core/downloadfromdcthread.h"
#include "core/libdivecomputer.h"
#include "core/bluetoothaddress.h"
#include "core/subsurfacestartup.h"
#include <QCoreApplication>
#include <QEventLoop>
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
	QString deviceName = QString::fromStdString(device);
	QString bluetoothAddress = extractBluetoothAddress(deviceName);

	// prepare device_data
	localData.setVendor(QString::fromStdString(vendor));
	localData.setProduct(QString::fromStdString(product));
	localData.setBluetoothMode(!bluetoothAddress.isEmpty());
	localData.setDevName(bluetoothAddress.isEmpty() ? deviceName : bluetoothAddress);
	localData.setSaveLog(!logfile_name.empty());
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
	// Close during the same FWUPDATE -> DONE transition as the GUI. For OSTC 4/5,
	// leaving service mode promptly is what lets the device continue into the
	// firmware installation step after the upload has completed.
	bool hadError = false;
	bool closedDevice = false;
	QEventLoop loop;
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
	QObject::connect(&config, &ConfigureDiveComputer::stateChanged,
			 [&config, data, &closedDevice, &loop](ConfigureDiveComputer::states oldState, ConfigureDiveComputer::states newState) {
		if (oldState == ConfigureDiveComputer::FWUPDATE && newState == ConfigureDiveComputer::DONE && !closedDevice) {
			config.dc_close(data);
			closedDevice = true;
		}
		if (newState == ConfigureDiveComputer::DONE || newState == ConfigureDiveComputer::ERRORED)
			loop.quit();
	});

	config.startFirmwareUpdate(expandedFilename, data, force);

	if (config.currentState != ConfigureDiveComputer::DONE && config.currentState != ConfigureDiveComputer::ERRORED)
		loop.exec();

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

	if (!closedDevice)
		config.dc_close(data);
	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
	QThread::msleep(250);

	fprintf(stdout, "\nFirmware update completed successfully\n");
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
	QString deviceName = QString::fromStdString(device);
	QString bluetoothAddress = extractBluetoothAddress(deviceName);
	data->setVendor(QString::fromStdString(vendor));
	data->setProduct(QString::fromStdString(product));
	data->setBluetoothMode(!bluetoothAddress.isEmpty());
	if (data->vendor() == "Uemis") {
		QString devname = deviceName;
		int colon = devname.indexOf(QStringLiteral(":\\ (UEMISSDA)"));
		if (colon >= 0) {
			devname.truncate(colon + 2);
			fprintf(stderr, "shortened devname to \"%s\"", qPrintable(devname));
		}
		data->setDevName(devname);
	} else {
		data->setDevName(bluetoothAddress.isEmpty() ? deviceName : bluetoothAddress);
	}

	// some assumptions - should all be configurable
	data->setForceDownload(false);
	data->setSaveLog(!logfile_name.empty());
	data->setSaveDump(false);
	data->setSyncTime(false);

	diveImportedModel.startDownload();
	diveImportedModel.waitForDownload();
	diveImportedModel.recordDives();
}
