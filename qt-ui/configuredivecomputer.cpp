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

	readThread->start();
}

void ConfigureDiveComputer::setDeviceName(device_data_t *data, QString newName)
{
	writeSettingToDevice(data, "Name", newName);
}

void ConfigureDiveComputer::setDeviceDateAndTime(device_data_t *data, QDateTime dateAndTime)
{
	writeSettingToDevice(data, "DateAndTime", dateAndTime);
}

void ConfigureDiveComputer::setState(ConfigureDiveComputer::states newState)
{
	currentState = newState;
	emit stateChanged(currentState);
}

void ConfigureDiveComputer::writeSettingToDevice(device_data_t *data, QString settingName, QVariant settingValue)
{
	setState(READING);

	if (writeThread)
		writeThread->deleteLater();

	writeThread = new WriteSettingsThread(this, data, settingName, settingValue);
	connect (writeThread, SIGNAL(error(QString)), this, SLOT(setError(QString)));
	connect (writeThread, SIGNAL(finished()), this, SLOT(writeThreadFinished()));

	writeThread->start();
}

void ConfigureDiveComputer::setError(QString err)
{
	lastError = err;
	emit error(err);
}

void ConfigureDiveComputer::readThreadFinished()
{
	setState(DONE);
	emit deviceSettings(readThread->result);
}

void ConfigureDiveComputer::writeThreadFinished()
{
	setState(DONE);
	if (writeThread->lastError.isEmpty()) {
		//No error
		emit message(tr("Setting successfully written to device"));
	}
}

ReadSettingsThread::ReadSettingsThread(QObject *parent, device_data_t *data)
	: QThread(parent), data(data)
{

}

void ReadSettingsThread::run()
{
	dc_status_t rc;
	rc = dc_device_open(&data->device, data->context, data->descriptor, data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		if (dc_device_get_type(data->device) == DC_FAMILY_HW_OSTC3) {
			unsigned char hw_data[10];
			hw_ostc3_device_version(data->device, hw_data, 10);
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

WriteSettingsThread::WriteSettingsThread(QObject *parent, device_data_t *data, QString settingName, QVariant settingValue)
	: QThread(parent), data(data), m_settingName(settingName), m_settingValue(settingValue)
{

}

void WriteSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = dc_device_open(&data->device, data->context, data->descriptor, data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		dc_status_t result;
		if (dc_device_get_type(data->device) == DC_FAMILY_HW_OSTC3) {
			if (m_settingName == "Name") {
				supported = true;
				result = hw_ostc3_device_customtext(data->device, m_settingValue.toByteArray().data());
			}
		} else if ( dc_device_get_type(data->device) == DC_FAMILY_HW_FROG ) {
			if (m_settingName == "Name") {
				supported = true;
				result = hw_frog_device_customtext(data->device, m_settingValue.toByteArray().data());
			}
		}
		if ( dc_device_get_type(data->device) == DC_FAMILY_HW_OSTC3 && m_settingName == "DateTime" ) {
			supported = true;
			QDateTime timeToSet = m_settingValue.toDateTime();
			dc_datetime_t time;
			time.year = timeToSet.date().year();
			time.month = timeToSet.date().month();
			time.day = timeToSet.date().day();
			time.hour = timeToSet.time().hour();
			time.minute = timeToSet.time().minute();
			time.second = timeToSet.time().second();
			result = hw_ostc_device_clock(data->device, &time); //Toto fix error here
		}
		if (result !=  DC_STATUS_SUCCESS) {
			qDebug() << result;
			lastError = tr("An error occurred while sending data to the dive computer.");
			//Todo Update this message to change depending on actual result.

			emit error(lastError);
		}
		dc_device_close(data->device);
	} else {
		lastError = tr("Could not a establish connection to the dive computer.");
		emit error(lastError);
	}
	if (!supported) {
		lastError = tr("This feature is not yet available for the selected dive computer.");
		emit error(lastError);
	}
}
