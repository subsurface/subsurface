#include "configuredivecomputerthreads.h"
#include "libdivecomputer/hw.h"
#include <QDebug>
#include <QDateTime>
#include <QStringList>

#define OSTC3_GAS1			0x10
#define OSTC3_GAS2			0x11
#define OSTC3_GAS3			0x12
#define OSTC3_GAS4			0x13
#define OSTC3_GAS5			0x14
#define OSTC3_DIL1			0x15
#define OSTC3_DIL2			0x16
#define OSTC3_DIL3			0x17
#define OSTC3_DIL4			0x18
#define OSTC3_DIL5			0x19
#define OSTC3_SP1			0x1A
#define OSTC3_SP2			0x1B
#define OSTC3_SP3			0x1C
#define OSTC3_SP4			0x1D
#define OSTC3_SP5			0x1E
#define OSTC3_CCR_MODE			0x1F
#define OSTC3_DIVE_MODE			0x20
#define OSTC3_DECO_TYPE			0x21
#define OSTC3_PP02_MAX			0x22
#define OSTC3_PP02_MIN			0x23
#define OSTC3_FUTURE_TTS		0x24
#define OSTC3_GF_LOW			0x25
#define OSTC3_GF_HIGH			0x26
#define OSTC3_AGF_LOW			0x27
#define OSTC3_AGF_HIGH			0x28
#define OSTC3_AGF_SELECTABLE		0x29
#define OSTC3_SATURATION		0x2A
#define OSTC3_DESATURATION		0x2B
#define OSTC3_LAST_DECO			0x2C
#define OSTC3_BRIGHTNESS		0x2D
#define OSTC3_UNITS			0x2E
#define OSTC3_SAMPLING_RATE		0x2F
#define OSTC3_SALINITY			0x30
#define OSTC3_DIVEMODE_COLOR		0x31
#define OSTC3_LANGUAGE			0x32
#define OSTC3_DATE_FORMAT		0x33
#define OSTC3_COMPASS_GAIN		0x34
#define OSTC3_PRESSURE_SENSOR_OFFSET	0x35
#define OSTC3_SAFETY_STOP		0x36

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
#if DC_VERSION_CHECK(0, 5, 0)
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
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS1, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas1.depth = gasData[3];
				gas1.oxygen = gasData[0];
				gas1.helium = gasData[1];
				gas1.type = gasData[2];
			}
			//Gas 2
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS2, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas2.depth = gasData[3];
				gas2.oxygen = gasData[0];
				gas2.helium = gasData[1];
				gas2.type = gasData[2];
			}
			//Gas 3
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS3, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas3.depth = gasData[3];
				gas3.oxygen = gasData[0];
				gas3.helium = gasData[1];
				gas3.type = gasData[2];
			}
			//Gas 4
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS4, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas4.depth = gasData[3];
				gas4.oxygen = gasData[0];
				gas4.helium = gasData[1];
				gas4.type = gasData[2];
			}
			//Gas 5
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS5, gasData, sizeof(gasData));
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

			//Read Dil Values
			gas dil1;
			gas dil2;
			gas dil3;
			gas dil4;
			gas dil5;
			//Dil 1
			unsigned char dilData[4] = {0,0,0,0};
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL1, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil1.depth = dilData[3];
				dil1.oxygen = dilData[0];
				dil1.helium = dilData[1];
				dil1.type = dilData[2];
			}
			//Dil 2
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL2, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil2.depth = dilData[3];
				dil2.oxygen = dilData[0];
				dil2.helium = dilData[1];
				dil2.type = dilData[2];
			}
			//Dil 3
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL3, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil3.depth = dilData[3];
				dil3.oxygen = dilData[0];
				dil3.helium = dilData[1];
				dil3.type = dilData[2];
			}
			//Dil 4
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL4, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil4.depth = dilData[3];
				dil4.oxygen = dilData[0];
				dil4.helium = dilData[1];
				dil4.type = dilData[2];
			}
			//Dil 5
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL5, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil5.depth = dilData[3];
				dil5.oxygen = dilData[0];
				dil5.helium = dilData[1];
				dil5.type = dilData[2];
			}

			m_deviceDetails->setDil1(dil1);
			m_deviceDetails->setDil2(dil2);
			m_deviceDetails->setDil3(dil3);
			m_deviceDetails->setDil4(dil4);
			m_deviceDetails->setDil5(dil5);

			//Read set point Values
			setpoint sp1;
			setpoint sp2;
			setpoint sp3;
			setpoint sp4;
			setpoint sp5;

			unsigned char spData[2] = {0,0};

			//Sp 1
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP1, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp1.sp = dilData[0];
				sp1.depth = dilData[1];
			}
			//Sp 2
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP2, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp2.sp = dilData[0];
				sp2.depth = dilData[1];
			}
			//Sp 3
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP3, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp3.sp = dilData[0];
				sp3.depth = dilData[1];
			}
			//Sp 4
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP4, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp4.sp = dilData[0];
				sp4.depth = dilData[1];
			}
			//Sp 5
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP5, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp5.sp = dilData[0];
				sp5.depth = dilData[1];
			}


			//Read other settings
			unsigned char uData[1] = {0};
			//DiveMode
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIVE_MODE, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDiveMode(uData[0]);
			//Saturation
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SATURATION, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setSaturation(uData[0]);
			//LastDeco
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_LAST_DECO, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setLastDeco(uData[0]);
			//Brightness
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_BRIGHTNESS, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setBrightness(uData[0]);
			//Units
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_UNITS, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setUnits(uData[0]);
			//Sampling Rate
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SAMPLING_RATE, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setSamplingRate(uData[0]);
			//Salinity
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SALINITY, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setSalinity(uData[0]);
			//Dive mode colour
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIVEMODE_COLOR, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDiveModeColor(uData[0]);
			//Language
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_LANGUAGE, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setLanguage(uData[0]);
			//Date Format
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DATE_FORMAT, uData, sizeof(uData));
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
#endif	// divecomputer 0.5.0
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
#if DC_VERSION_CHECK(0,5,0)
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
			hw_ostc3_device_config_write(m_data->device, OSTC3_GAS1, gas1Data, sizeof(gas1Data));
			//gas 2
			hw_ostc3_device_config_write(m_data->device, OSTC3_GAS2, gas2Data, sizeof(gas2Data));
			//gas 3
			hw_ostc3_device_config_write(m_data->device, OSTC3_GAS3, gas3Data, sizeof(gas3Data));
			//gas 4
			hw_ostc3_device_config_write(m_data->device, OSTC3_GAS4, gas4Data, sizeof(gas4Data));
			//gas 5
			hw_ostc3_device_config_write(m_data->device, OSTC3_GAS5, gas5Data, sizeof(gas5Data));

			//write set point values
			unsigned char sp1Data[2] = {m_deviceDetails->sp1().sp,
						     m_deviceDetails->sp1().depth};

			unsigned char sp2Data[2] = {m_deviceDetails->sp2().sp,
						     m_deviceDetails->sp2().depth};

			unsigned char sp3Data[2] = {m_deviceDetails->sp3().sp,
						     m_deviceDetails->sp3().depth};

			unsigned char sp4Data[2] = {m_deviceDetails->sp4().sp,
						     m_deviceDetails->sp4().depth};

			unsigned char sp5Data[2] = {m_deviceDetails->sp5().sp,
						     m_deviceDetails->sp5().depth};

			//sp 1
			hw_ostc3_device_config_write(m_data->device, OSTC3_SP1, sp1Data, sizeof(sp1Data));
			//sp 2
			hw_ostc3_device_config_write(m_data->device, OSTC3_SP2, sp2Data, sizeof(sp2Data));
			//sp 3
			hw_ostc3_device_config_write(m_data->device, OSTC3_SP3, sp3Data, sizeof(sp3Data));
			//sp 4
			hw_ostc3_device_config_write(m_data->device, OSTC3_SP4, sp4Data, sizeof(sp4Data));
			//sp 5
			hw_ostc3_device_config_write(m_data->device, OSTC3_SP5, sp5Data, sizeof(sp5Data));

			//write dil values
			unsigned char dil1Data[4] = {m_deviceDetails->dil1().oxygen,
						     m_deviceDetails->dil1().helium,
						     m_deviceDetails->dil1().type,
						     m_deviceDetails->dil1().depth};

			unsigned char dil2Data[4] = {m_deviceDetails->dil2().oxygen,
						     m_deviceDetails->dil2().helium,
						     m_deviceDetails->dil2().type,
						     m_deviceDetails->dil2().depth};

			unsigned char dil3Data[4] = {m_deviceDetails->dil3().oxygen,
						     m_deviceDetails->dil3().helium,
						     m_deviceDetails->dil3().type,
						     m_deviceDetails->dil3().depth};

			unsigned char dil4Data[4] = {m_deviceDetails->dil4().oxygen,
						     m_deviceDetails->dil4().helium,
						     m_deviceDetails->dil4().type,
						     m_deviceDetails->dil4().depth};

			unsigned char dil5Data[4] = {m_deviceDetails->dil5().oxygen,
						     m_deviceDetails->dil5().helium,
						     m_deviceDetails->dil5().type,
						     m_deviceDetails->dil5().depth};
			//dil 1
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIL1, dil1Data, sizeof(gas1Data));
			//dil 2
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIL2, dil2Data, sizeof(dil2Data));
			//dil 3
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIL3, dil3Data, sizeof(dil3Data));
			//dil 4
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIL4, dil4Data, sizeof(dil4Data));
			//dil 5
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIL5, dil5Data, sizeof(dil5Data));


			//write general settings
			//custom text
			hw_ostc3_device_customtext(m_data->device, m_deviceDetails->customText().toUtf8().data());
			unsigned char data[1] = {0};

			//dive mode
			data[0] = m_deviceDetails->diveMode();
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIVE_MODE, data, sizeof(data));

			//saturation
			data[0] = m_deviceDetails->saturation();
			hw_ostc3_device_config_write(m_data->device, OSTC3_SATURATION, data, sizeof(data));

			//last deco
			data[0] = m_deviceDetails->lastDeco();
			hw_ostc3_device_config_write(m_data->device, OSTC3_LAST_DECO, data, sizeof(data));

			//brightness
			data[0] = m_deviceDetails->brightness();
			hw_ostc3_device_config_write(m_data->device, OSTC3_BRIGHTNESS, data, sizeof(data));

			//units
			data[0] = m_deviceDetails->units();
			hw_ostc3_device_config_write(m_data->device, OSTC3_UNITS, data, sizeof(data));

			//sampling rate
			data[0] = m_deviceDetails->samplingRate();
			hw_ostc3_device_config_write(m_data->device, OSTC3_SAMPLING_RATE, data, sizeof(data));

			//salinity
			data[0] = m_deviceDetails->salinity();
			hw_ostc3_device_config_write(m_data->device, OSTC3_SALINITY, data, sizeof(data));

			//dive mode colour
			data[0] = m_deviceDetails->diveModeColor();
			hw_ostc3_device_config_write(m_data->device, OSTC3_DIVEMODE_COLOR, data, sizeof(data));

			//language
			data[0] = m_deviceDetails->language();
			hw_ostc3_device_config_write(m_data->device, OSTC3_LANGUAGE, data, sizeof(data));

			//date format
			data[0] = m_deviceDetails->dateFormat();
			hw_ostc3_device_config_write(m_data->device, OSTC3_DATE_FORMAT, data, sizeof(data));

			//compass gain
			data[0] = m_deviceDetails->compassGain();
			hw_ostc3_device_config_write(m_data->device, OSTC3_COMPASS_GAIN, data, sizeof(data));

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
#endif	// divecomputer 0.5.0
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


FirmwareUpdateThread::FirmwareUpdateThread(QObject *parent, device_data_t *data, QString fileName)
: QThread(parent), m_data(data), m_fileName(fileName)
{

}

void FirmwareUpdateThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		switch (dc_device_get_type(m_data->device)) {
#if DC_VERSION_CHECK(0, 5, 0)
		case DC_FAMILY_HW_OSTC3:
			supported = true;
			//hw_ostc3_device_fwupdate(m_data->device, m_fileName.toUtf8().data());
			break;
#endif	// divecomputer 0.5.0
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
