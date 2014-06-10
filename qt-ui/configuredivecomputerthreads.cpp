#include "configuredivecomputerthreads.h"
#include "libdivecomputer/hw.h"
#include <QDebug>

ReadSettingsThread::ReadSettingsThread(QObject *parent, DeviceDetails *deviceDetails, device_data_t *data)
	: QThread(parent), m_deviceDetails(deviceDetails), m_data(data)
{

}

void ReadSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		switch (dc_device_get_type(m_data->device)) {
		case DC_FAMILY_HW_OSTC3:
			supported = true;
			m_deviceDetails->setBrightness(0);
			m_deviceDetails->setCustomText("");
			m_deviceDetails->setDateFormat(0);
			m_deviceDetails->setDiveModeColor(0);
			m_deviceDetails->setFirmwareVersion("");
			m_deviceDetails->setLanguage(0);
			m_deviceDetails->setLastDeco(0);
			m_deviceDetails->setSerialNo("");
			unsigned char uData[1];
			rc = hw_ostc3_device_config_read(m_data->device, 0x2D, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setBrightness(uData[0]);
			rc = hw_ostc3_device_config_read(m_data->device, 0x32, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setLanguage(uData[0]);
			rc = hw_ostc3_device_config_read(m_data->device, 0x33, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDateFormat(uData[0]);
			break;

		}
		dc_device_close(m_data->device);

		if (!supported) {
			lastError = tr("This feature is not yet available for the selected dive computer.");
			emit error(lastError);
		}
	}
	else {
		lastError = tr("Could not a establish connection to the dive computer.");
		emit error(lastError);
	}
}

WriteSettingsThread::WriteSettingsThread(QObject *parent, DeviceDetails *deviceDetails, QString settingName, QVariant settingValue)
	: QThread(parent), m_deviceDetails(deviceDetails), m_settingName(settingName), m_settingValue(settingValue)
{

}

void WriteSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;

	switch (dc_device_get_type(data->device)) {
	case DC_FAMILY_HW_OSTC3:
		rc = dc_device_open(&data->device, data->context, data->descriptor, data->devname);
		if (rc == DC_STATUS_SUCCESS) {

		} else {
			lastError = tr("Could not a establish connection to the dive computer.");
			emit error(lastError);
		}
		break;

		if (!supported) {
			lastError = tr("This feature is not yet available for the selected dive computer.");
			emit error(lastError);
		}
	}
}
