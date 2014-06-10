#include "configuredivecomputer.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
ConfigureDiveComputer::ConfigureDiveComputer(QObject *parent) :
	QObject(parent),
	readThread(0),
	writeThread(0),
	m_deviceDetails(0)
{
	setState(INITIAL);
}

void ConfigureDiveComputer::readSettings(DeviceDetails *deviceDetails, device_data_t *data)
{
	setState(READING);
	m_deviceDetails = deviceDetails;

	if (readThread)
		readThread->deleteLater();

	readThread = new ReadSettingsThread(this, deviceDetails, data);
	connect (readThread, SIGNAL(finished()),
		 this, SLOT(readThreadFinished()), Qt::QueuedConnection);
	connect (readThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));

	readThread->start();
}

void ConfigureDiveComputer::saveDeviceDetails()
{

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
