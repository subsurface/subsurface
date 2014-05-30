#include "configuredivecomputer.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
ConfigureDiveComputer::ConfigureDiveComputer(QObject *parent) :
	QObject(parent),
	readThread(0)
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

	readThread->start();
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

void ConfigureDiveComputer::readHWSettings(device_data_t *data)
{

}

void ConfigureDiveComputer::readThreadFinished()
{
	setState(DONE);
	emit deviceSettings(readThread->result);
}

ReadSettingsThread::ReadSettingsThread(QObject *parent, device_data_t *data)
	: QThread(parent), data(data)
{

}

void ReadSettingsThread::run()
{
	QString vendor = data->vendor;
	dc_status_t rc;
	rc = dc_device_open(&data->device, data->context, data->descriptor, data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		if (vendor.trimmed() == "Heinrichs Weikamp") {
			unsigned char hw_data[10];
			hw_frog_device_version(data->device, hw_data, 10);
			QTextStream (&result) << "Device Version: " << hw_data; //just a test. I will work on decoding this
		} else {
			lastError = tr("This feature is not yet available for the selected dive computer.");
			emit error(lastError);
		}
		dc_device_close(data->device);
	} else {
		lastError = tr("Could not a establish connection to the dive computer.");
		emit error(lastError);
	}
}
