#include "configuredivecomputer.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
#include <QFile>

ConfigureDiveComputer::ConfigureDiveComputer(QObject *parent) :
	QObject(parent),
	readThread(0),
	writeThread(0)
{
	setState(INITIAL);
}

void ConfigureDiveComputer::readSettings(device_data_t *data)
{
	setState(READING);

	if (readThread)
		readThread->deleteLater();

	readThread = new ReadSettingsThread(this, data);
	connect (readThread, SIGNAL(finished()),
		 this, SLOT(readThreadFinished()), Qt::QueuedConnection);
	connect (readThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));
	connect (readThread, SIGNAL(devicedetails(DeviceDetails*)), this,
		 SIGNAL(deviceDetailsChanged(DeviceDetails*)));

	readThread->start();
}

void ConfigureDiveComputer::saveDeviceDetails(DeviceDetails *details, device_data_t *data)
{
	setState(WRITING);

	if (writeThread)
		writeThread->deleteLater();

	writeThread = new WriteSettingsThread(this, data);
	connect (writeThread, SIGNAL(finished()),
		 this, SLOT(writeThreadFinished()), Qt::QueuedConnection);
	connect (writeThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));

	writeThread->setDeviceDetails(details);
	writeThread->start();
}

bool ConfigureDiveComputer::saveXMLBackup(QString fileName, DeviceDetails *details, device_data_t *data, QString errorText)
{
	QString xml = "";
	QString vendor = data->vendor;
	QString product = data->product;
	xml += "<backup>";
	xml += "\n<divecomputer vendor='" + vendor
			+ "' model = '" + product + "'"
			+ " />";
	xml += "\n<settings>";
	xml += "\n<setting name='CustomText' value = '" + details->customText() + "' />";
	xml += "\n<setting name='Brightness' value = '" + QString::number(details->brightness()) + "' />";
	xml += "\n<setting name='Language' value = '" + QString::number(details->language()) + "' />";
	xml += "\n<setting name='DateFormat' value = '" + QString::number(details->dateFormat()) + "' />";
	xml += "\n</settings>";
	xml += "\n</backup>";
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) {
		errorText = tr("Could not save the backup file %1. Error Message: %2")
				.arg(fileName, file.errorString());
		return false;
	}
	//file open successful. write data and save.
	QTextStream out(&file);
	out << xml;

	file.close();
	return true;
}

void ConfigureDiveComputer::setState(ConfigureDiveComputer::states newState)
{
	currentState = newState;
	emit stateChanged(currentState);
}

void ConfigureDiveComputer::setError(QString err)
{
	lastError = err;
	emit error(err);
}

void ConfigureDiveComputer::readThreadFinished()
{
	setState(DONE);
	emit readFinished();
}

void ConfigureDiveComputer::writeThreadFinished()
{
	setState(DONE);
	if (writeThread->lastError.isEmpty()) {
		//No error
		emit message(tr("Setting successfully written to device"));
	}
}
