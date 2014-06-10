#include "configuredivecomputer.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
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
