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
#define OSTC3_PPO2_MAX			0x22
#define OSTC3_PPO2_MIN			0x23
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
#define OSTC3_CALIBRATION_GAS_O2	0x37
#define OSTC3_SETPOINT_FALLBACK	0x38
#define OSTC3_FLIP_SCREEN	0x39

#define SUUNTO_VYPER_MAXDEPTH             0x1e
#define SUUNTO_VYPER_TOTAL_TIME           0x20
#define SUUNTO_VYPER_NUMBEROFDIVES        0x22
#define SUUNTO_VYPER_COMPUTER_TYPE        0x24
#define SUUNTO_VYPER_FIRMWARE             0x25
#define SUUNTO_VYPER_SERIALNUMBER         0x26
#define SUUNTO_VYPER_CUSTOM_TEXT          0x2c
#define SUUNTO_VYPER_SAMPLING_RATE        0x53
#define SUUNTO_VYPER_ALTITUDE_SAFETY      0x54
#define SUUNTO_VYPER_TIMEFORMAT           0x60
#define SUUNTO_VYPER_UNITS                0x62
#define SUUNTO_VYPER_MODEL                0x63
#define SUUNTO_VYPER_LIGHT                0x64
#define SUUNTO_VYPER_ALARM_DEPTH_TIME     0x65
#define SUUNTO_VYPER_ALARM_TIME           0x66
#define SUUNTO_VYPER_ALARM_DEPTH          0x68
#define SUUNTO_VYPER_CUSTOM_TEXT_LENGHT   30


ReadSettingsThread::ReadSettingsThread(QObject *parent, device_data_t *data)
	: QThread(parent), m_data(data)
{

}

void ReadSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		DeviceDetails *m_deviceDetails = new DeviceDetails(0);
		switch (dc_device_get_type(m_data->device)) {
		case DC_FAMILY_SUUNTO_VYPER:
			unsigned char data[SUUNTO_VYPER_CUSTOM_TEXT_LENGHT + 1];
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_COMPUTER_TYPE, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				const char *model;
				// FIXME: grab this info from libdivecomputer descriptor
				// instead of hard coded here
				switch(data[0]) {
				case 0x03:
					model = "Stinger";
					break;
				case 0x04:
					model = "Mosquito";
					break;
				case 0x05:
					model = "D3";
					break;
				case 0x0A:
					model = "Vyper";
					break;
				case 0x0B:
					model = "Vytec";
					break;
				case 0x0C:
					model = "Cobra";
					break;
				case 0x0D:
					model = "Gekko";
					break;
				case 0x16:
					model = "Zoop";
					break;
				case 20:
				case 30:
				case 60:
					// Suunto Spyder have there sample interval at this position
					// Fallthrough
				default:
					supported  = false;
					goto unsupported_dc_error;
				}
				// We found a supported device
				// we can safely proceed with reading/writing to this device.
				supported = true;
				m_deviceDetails->setModel(model);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_MAXDEPTH, data, 2);
			if (rc == DC_STATUS_SUCCESS) {
				// in ft * 128.0
				int depth = feet_to_mm(data[0] << 8 ^ data[1]) / 128;
				m_deviceDetails->setMaxDepth(depth);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_TOTAL_TIME, data, 2);
			if (rc == DC_STATUS_SUCCESS) {
				int total_time = data[0] << 8 ^ data[1];
				m_deviceDetails->setTotalTime(total_time);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_NUMBEROFDIVES, data, 2);
			if (rc == DC_STATUS_SUCCESS) {
				int number_of_dives = data[0] << 8 ^ data[1];
				m_deviceDetails->setNumberOfDives(number_of_dives);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_FIRMWARE, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setFirmwareVersion(QString::number(data[0]) + ".0.0");
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_SERIALNUMBER, data, 4);
			if (rc == DC_STATUS_SUCCESS) {
				int serial_number = data[0] * 1000000 + data[1] * 10000 + data[2] * 100 + data[3];
				m_deviceDetails->setSerialNo(QString::number(serial_number));
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_CUSTOM_TEXT, data, SUUNTO_VYPER_CUSTOM_TEXT_LENGHT);
			if (rc == DC_STATUS_SUCCESS) {
				data[SUUNTO_VYPER_CUSTOM_TEXT_LENGHT] = 0;
				m_deviceDetails->setCustomText((const char*) data);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_SAMPLING_RATE, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setSamplingRate((int) data[0]);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_ALTITUDE_SAFETY, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setAltitude(data[0] & 0x03);
				m_deviceDetails->setPersonalSafety(data[0] >> 2 & 0x03);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_TIMEFORMAT, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setTimeFormat(data[0] & 0x01);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_UNITS, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setUnits(data[0] & 0x01);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_MODEL, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setDiveMode(data[0] & 0x03);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_LIGHT, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setLightEnabled(data[0] >> 7);
				m_deviceDetails->setLight(data[0] & 0x7F);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_ALARM_DEPTH_TIME, data, 1);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setAlarmTimeEnabled(data[0] & 0x01);
				m_deviceDetails->setAlarmDepthEnabled(data[0] >> 1 & 0x01);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_ALARM_TIME, data, 2);
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setAlarmTime(data[0] << 8 ^ data[1]);
			}
			rc = dc_device_read(m_data->device, SUUNTO_VYPER_ALARM_DEPTH, data, 2);
			if (rc == DC_STATUS_SUCCESS) {
				int depth = feet_to_mm(data[0] << 8 ^ data[1]) / 128;
				m_deviceDetails->setAlarmDepth(depth);
			}
			emit devicedetails(m_deviceDetails);
            break;
#if DC_VERSION_CHECK(0, 5, 0)
		case DC_FAMILY_HW_OSTC3:
		{
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
				sp1.sp = spData[0];
				sp1.depth = spData[1];
			}
			//Sp 2
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP2, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp2.sp = spData[0];
				sp2.depth = spData[1];
			}
			//Sp 3
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP3, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp3.sp = spData[0];
				sp3.depth = spData[1];
			}
			//Sp 4
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP4, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp4.sp = spData[0];
				sp4.depth = spData[1];
			}
			//Sp 5
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_SP5, spData, sizeof(spData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				sp5.sp = spData[0];
				sp5.depth = spData[1];
			}

			m_deviceDetails->setSp1(sp1);
			m_deviceDetails->setSp2(sp2);
			m_deviceDetails->setSp3(sp3);
			m_deviceDetails->setSp4(sp4);
			m_deviceDetails->setSp5(sp5);

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
			//Desaturation
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DESATURATION, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setDesaturation(uData[0]);
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
			//Compass gain
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_COMPASS_GAIN, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS)
				m_deviceDetails->setCompassGain(uData[0]);


			//read firmware settings
			unsigned char fData[64] = {0};
			rc = hw_ostc3_device_version (m_data->device, fData, sizeof (fData));
			if (rc == DC_STATUS_SUCCESS) {
				int serial = fData[0] + (fData[1] << 8);
				m_deviceDetails->setSerialNo(QString::number(serial));
				m_deviceDetails->setFirmwareVersion(QString::number(fData[2]) + "." + QString::number(fData[3]));
				QByteArray ar((char *)fData + 4, 60);
				m_deviceDetails->setCustomText(ar.trimmed());
			}

			emit devicedetails(m_deviceDetails);
		}
		break;
#endif	// divecomputer 0.5.0
		default:
			supported = false;
			break;
		}
unsupported_dc_error:
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
	rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		switch (dc_device_get_type(m_data->device)) {
		case DC_FAMILY_SUUNTO_VYPER:
			unsigned char data;
			unsigned char data2[2];
			// Maybee we should read the model from the device to sanity check it here too..
			// For now we just check that we actually read a device before writing to one.
			if (m_deviceDetails->model() == "")
				break;
			else
				supported = true;

			dc_device_write(m_data->device, SUUNTO_VYPER_CUSTOM_TEXT,
					// Convert the customText to a 30 char wide padded with " "
					(const unsigned char *) QString("%1").arg(m_deviceDetails->customText(), -30, QChar(' ')).toUtf8().data(),
					SUUNTO_VYPER_CUSTOM_TEXT_LENGHT);
			data = m_deviceDetails->samplingRate();
			dc_device_write(m_data->device, SUUNTO_VYPER_SAMPLING_RATE, &data, 1);
			data = m_deviceDetails->personalSafety() << 2 ^ m_deviceDetails->altitude();
			dc_device_write(m_data->device, SUUNTO_VYPER_ALTITUDE_SAFETY, &data, 1);
			data = m_deviceDetails->timeFormat();
			dc_device_write(m_data->device, SUUNTO_VYPER_TIMEFORMAT, &data, 1);
			data = m_deviceDetails->units();
			dc_device_write(m_data->device, SUUNTO_VYPER_UNITS, &data, 1);
			data = m_deviceDetails->diveMode();
			dc_device_write(m_data->device, SUUNTO_VYPER_MODEL, &data, 1);
			data = m_deviceDetails->lightEnabled() << 7 ^ (m_deviceDetails->light() & 0x7F);
			dc_device_write(m_data->device, SUUNTO_VYPER_LIGHT, &data, 1);
			data = m_deviceDetails->alarmDepthEnabled() << 1 ^ m_deviceDetails->alarmTimeEnabled();
			dc_device_write(m_data->device, SUUNTO_VYPER_ALARM_DEPTH_TIME, &data, 1);
			data2[0] = m_deviceDetails->alarmTime() >> 8;
			data2[1] = m_deviceDetails->alarmTime() & 0xFF;
			dc_device_write(m_data->device, SUUNTO_VYPER_ALARM_TIME, data2, 2);
			data2[0] = (int)(mm_to_feet(m_deviceDetails->alarmDepth()) * 128) >> 8;
			data2[1] = (int)(mm_to_feet(m_deviceDetails->alarmDepth()) * 128) & 0x0FF;
			dc_device_write(m_data->device, SUUNTO_VYPER_ALARM_DEPTH, data2, 2);
			break;
#if DC_VERSION_CHECK(0,5,0)
		case DC_FAMILY_HW_OSTC3:
		{
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

			//desaturation
			data[0] = m_deviceDetails->desaturation();
			hw_ostc3_device_config_write(m_data->device, OSTC3_DESATURATION, data, sizeof(data));

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
		}
#endif	// divecomputer 0.5.0
		default:
			supported = false;
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


FirmwareUpdateThread::FirmwareUpdateThread(QObject *parent, device_data_t *data, QString fileName)
: QThread(parent), m_data(data), m_fileName(fileName)
{

}

void FirmwareUpdateThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
#if DC_VERSION_CHECK(0, 5, 0)
		if (dc_device_get_type(m_data->device) == DC_FAMILY_HW_OSTC3) {
			supported = true;
			//hw_ostc3_device_fwupdate(m_data->device, m_fileName.toUtf8().data());
		}
#endif	// divecomputer 0.5.0
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
