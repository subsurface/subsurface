#include "configuredivecomputerthreads.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
#include <QDateTime>
#include <QStringList>

ReadSettingsThread::ReadSettingsThread(QObject *parent, device_data_t *data)
	: QThread(parent), m_data(data)
{

}

void ReadSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		DeviceDetails *m_deviceDetails = new DeviceDetails(0);
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
			m_deviceDetails->setCompassGain(0);
			m_deviceDetails->setSalinity(0);
			m_deviceDetails->setSamplingRate(0);
			m_deviceDetails->setUnits(0);


			//Gread gas mixes
			gas gas1;
			gas gas2;
			gas gas3;
			gas gas4;
			gas gas5;
			//Gas 1
			unsigned char gasData[4] = {0,0,0,0};
			rc = hw_ostc3_device_config_read(m_data->device, 0x10, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas1.depth = gasData[3];
				gas1.oxygen = gasData[0];
				gas1.helium = gasData[1];
				gas1.type = gasData[2];
			}
			//Gas 2
			rc = hw_ostc3_device_config_read(m_data->device, 0x11, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas2.depth = gasData[3];
				gas2.oxygen = gasData[0];
				gas2.helium = gasData[1];
				gas2.type = gasData[2];
			}
			//Gas 3
			rc = hw_ostc3_device_config_read(m_data->device, 0x12, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas3.depth = gasData[3];
				gas3.oxygen = gasData[0];
				gas3.helium = gasData[1];
				gas3.type = gasData[2];
			}
			//Gas 4
			rc = hw_ostc3_device_config_read(m_data->device, 0x13, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas4.depth = gasData[3];
				gas4.oxygen = gasData[0];
				gas4.helium = gasData[1];
				gas4.type = gasData[2];
			}
			//Gas 5
			rc = hw_ostc3_device_config_read(m_data->device, 0x14, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas5.depth = gasData[3];
				gas5.oxygen = gasData[0];
				gas5.helium = gasData[1];
				gas5.type = gasData[2];
			}

			m_deviceDetails->setGas1(gas1);
			m_deviceDetails->setGas2(gas2);
			m_deviceDetails->setGas3(gas3);
			m_deviceDetails->setGas4(gas4);
			m_deviceDetails->setGas5(gas5);

			//Read other settings
			unsigned char uData[1] = {0};
			//DiveMode
			rc = hw_ostc3_device_config_read(m_data->device, 0x20, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDiveMode(uData[0]);
			//Saturation
			rc = hw_ostc3_device_config_read(m_data->device, 0x2A, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setSaturation(uData[0]);
			//LastDeco
			rc = hw_ostc3_device_config_read(m_data->device, 0x2D, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setLastDeco(uData[0]);
			//Brightness
			rc = hw_ostc3_device_config_read(m_data->device, 0x2D, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setBrightness(uData[0]);
			//Units
			rc = hw_ostc3_device_config_read(m_data->device, 0x2E, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setUnits(uData[0]);
			//Sampling Rate
			rc = hw_ostc3_device_config_read(m_data->device, 0x2F, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setSamplingRate(uData[0]);
			//Salinity
			rc = hw_ostc3_device_config_read(m_data->device, 0x30, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setSalinity(uData[0]);
			//Dive mode colour
			rc = hw_ostc3_device_config_read(m_data->device, 0x31, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDiveModeColor(uData[0]);
			//Language
			rc = hw_ostc3_device_config_read(m_data->device, 0x32, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setLanguage(uData[0]);
			//Date Format
			rc = hw_ostc3_device_config_read(m_data->device, 0x33, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDateFormat(uData[0]);


			//read firmware settings
			unsigned char fData[64] = {0};
			rc = hw_ostc3_device_version (m_data->device, fData, sizeof (fData));
			if (rc == DC_STATUS_SUCCESS) {
				int serial = fData[0] + (fData[1] << 8);
				m_deviceDetails->setSerialNo(QString::number(serial));
				int fw = (fData[2] << 8) + fData[3];
				m_deviceDetails->setFirmwareVersion(QString::number(fw));
				QByteArray ar((char *)fData + 4, 60);
				m_deviceDetails->setCustomText(ar.trimmed());
			}

			emit devicedetails(m_deviceDetails);
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

WriteSettingsThread::WriteSettingsThread(QObject *parent, device_data_t *data)
	: QThread(parent), m_data(data) {

}

void WriteSettingsThread::setDeviceDetails(DeviceDetails *details)
{
	m_deviceDetails = details;
}

void WriteSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		switch (dc_device_get_type(m_data->device)) {
		case DC_FAMILY_HW_OSTC3:
			supported = true;
			//write gas values
			unsigned char gas1Data[4] = {m_deviceDetails->gas1().oxygen,
						     m_deviceDetails->gas1().helium,
						     m_deviceDetails->gas1().type,
						     m_deviceDetails->gas1().depth};

			unsigned char gas2Data[4] = {m_deviceDetails->gas2().oxygen,
						     m_deviceDetails->gas2().helium,
						     m_deviceDetails->gas2().type,
						     m_deviceDetails->gas2().depth};

			unsigned char gas3Data[4] = {m_deviceDetails->gas3().oxygen,
						     m_deviceDetails->gas3().helium,
						     m_deviceDetails->gas3().type,
						     m_deviceDetails->gas3().depth};

			unsigned char gas4Data[4] = {m_deviceDetails->gas4().oxygen,
						     m_deviceDetails->gas4().helium,
						     m_deviceDetails->gas4().type,
						     m_deviceDetails->gas4().depth};

			unsigned char gas5Data[4] = {m_deviceDetails->gas5().oxygen,
						     m_deviceDetails->gas5().helium,
						     m_deviceDetails->gas5().type,
						     m_deviceDetails->gas5().depth};
			//gas 1
			hw_ostc3_device_config_write(m_data->device, 0x10, gas1Data, sizeof(gas1Data));
			//gas 2
			hw_ostc3_device_config_write(m_data->device, 0x11, gas2Data, sizeof(gas2Data));
			//gas 3
			hw_ostc3_device_config_write(m_data->device, 0x12, gas3Data, sizeof(gas3Data));
			//gas 4
			hw_ostc3_device_config_write(m_data->device, 0x13, gas4Data, sizeof(gas4Data));
			//gas 5
			hw_ostc3_device_config_write(m_data->device, 0x14, gas5Data, sizeof(gas5Data));


			//write general settings
			//custom text
			hw_ostc3_device_customtext(m_data->device, m_deviceDetails->customText().toUtf8().data());
			unsigned char data[1] = {0};

			//dive mode
			data[0] = m_deviceDetails->diveMode();
			hw_ostc3_device_config_write(m_data->device, 0x20, data, sizeof(data));

			//saturation
			data[0] = m_deviceDetails->saturation();
			hw_ostc3_device_config_write(m_data->device, 0x2A, data, sizeof(data));

			//last deco
			data[0] = m_deviceDetails->lastDeco();
			hw_ostc3_device_config_write(m_data->device, 0x2C, data, sizeof(data));

			//brightness
			data[0] = m_deviceDetails->brightness();
			hw_ostc3_device_config_write(m_data->device, 0x2D, data, sizeof(data));

			//units
			data[0] = m_deviceDetails->units();
			hw_ostc3_device_config_write(m_data->device, 0x2E, data, sizeof(data));

			//sampling rate
			data[0] = m_deviceDetails->samplingRate();
			hw_ostc3_device_config_write(m_data->device, 0x2F, data, sizeof(data));

			//salinity
			data[0] = m_deviceDetails->salinity();
			hw_ostc3_device_config_write(m_data->device, 0x30, data, sizeof(data));

			//dive mode colour
			data[0] = m_deviceDetails->diveModeColor();
			hw_ostc3_device_config_write(m_data->device, 0x31, data, sizeof(data));

			//language
			data[0] = m_deviceDetails->language();
			hw_ostc3_device_config_write(m_data->device, 0x32, data, sizeof(data));

			//date format
			data[0] = m_deviceDetails->dateFormat();
			hw_ostc3_device_config_write(m_data->device, 0x33, data, sizeof(data));

			//compass gain
			data[0] = m_deviceDetails->compassGain();
			hw_ostc3_device_config_write(m_data->device, 0x34, data, sizeof(data));

			//sync date and time
			if (m_deviceDetails->syncTime()) {
				QDateTime timeToSet = QDateTime::currentDateTime();
				dc_datetime_t time;
				time.year = timeToSet.date().year();
				time.month = timeToSet.date().month();
				time.day = timeToSet.date().day();
				time.hour = timeToSet.time().hour();
				time.minute = timeToSet.time().minute();
				time.second = timeToSet.time().second();
				hw_ostc3_device_clock(m_data->device, &time);
			}

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
