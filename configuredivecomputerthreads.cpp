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

#ifdef DEBUG_OSTC
// Fake io to ostc memory banks
#define hw_ostc_device_eeprom_read local_hw_ostc_device_eeprom_read
#define hw_ostc_device_eeprom_write local_hw_ostc_device_eeprom_write
#define hw_ostc_device_clock local_hw_ostc_device_clock
#define OSTC_FILE "../OSTC-data-dump.bin"

static dc_status_t local_hw_ostc_device_eeprom_read(void *ignored, unsigned char bank, unsigned char data[], unsigned int data_size)
{
	FILE *f;
	if ((f = fopen(OSTC_FILE, "r")) == NULL)
		return DC_STATUS_NODEVICE;
	fseek(f, bank * 256, SEEK_SET);
	if (fread(data, sizeof(unsigned char), data_size, f) != data_size) {
		fclose(f);
		return DC_STATUS_IO;
	}
	fclose(f);

	return DC_STATUS_SUCCESS;
}

static dc_status_t local_hw_ostc_device_eeprom_write(void *ignored, unsigned char bank, unsigned char data[], unsigned int data_size)
{
	FILE *f;
	if ((f = fopen(OSTC_FILE, "r+")) == NULL)
		f = fopen(OSTC_FILE, "w");
	fseek(f, bank * 256, SEEK_SET);
	fwrite(data, sizeof(unsigned char), data_size, f);
	fclose(f);

	return DC_STATUS_SUCCESS;
}

static dc_status_t local_hw_ostc_device_clock(void *ignored, dc_datetime_t *time)
{
	qDebug() << "Setting OSTC time";
	return DC_STATUS_SUCCESS;
}
#endif

ReadSettingsThread::ReadSettingsThread(QObject *parent, device_data_t *data)
	: QThread(parent), m_data(data)
{

}

static int read_ostc_cf(unsigned char data[], unsigned char cf) {
	return data[128 + (cf % 32) * 4 + 3] << 8 ^ data[128 + (cf % 32) * 4 + 2];
}

static void write_ostc_cf(unsigned char data[], unsigned char cf, unsigned char max_CF, unsigned int value) {
	// Only write settings supported by this firmware.
	if (cf > max_CF)
		return;

	data[128 + (cf % 32) * 4 + 3] = (value & 0xff00) >> 8;
	data[128 + (cf % 32) * 4 + 2] = (value & 0x00ff);
}

void ReadSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
#ifdef DEBUG_OSTC
	if (strcmp(m_data->vendor, "Heinrichs Weikamp") == 0 && strcmp(m_data->product, "OSTC 2N") == 0)
		rc = DC_STATUS_SUCCESS;
	else
#endif
	rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
		DeviceDetails *m_deviceDetails = new DeviceDetails(0);
		switch (dc_device_get_type(m_data->device)) {
		case DC_FAMILY_SUUNTO_VYPER: {
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
					supported = false;
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
		}
#if DC_VERSION_CHECK(0, 5, 0)
		case DC_FAMILY_HW_OSTC3: {
			supported = true;
			//Read gas mixes
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
				gas1.oxygen = gasData[0];
				gas1.helium = gasData[1];
				gas1.type = gasData[2];
				gas1.depth = gasData[3];
			}
			//Gas 2
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS2, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas2.oxygen = gasData[0];
				gas2.helium = gasData[1];
				gas2.type = gasData[2];
				gas2.depth = gasData[3];
			}
			//Gas 3
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS3, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas3.oxygen = gasData[0];
				gas3.helium = gasData[1];
				gas3.type = gasData[2];
				gas3.depth = gasData[3];
			}
			//Gas 4
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS4, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas4.oxygen = gasData[0];
				gas4.helium = gasData[1];
				gas4.type = gasData[2];
				gas4.depth = gasData[3];
			}
			//Gas 5
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_GAS5, gasData, sizeof(gasData));
			if (rc == DC_STATUS_SUCCESS) {
				//Gas data read successful
				gas5.oxygen = gasData[0];
				gas5.helium = gasData[1];
				gas5.type = gasData[2];
				gas5.depth = gasData[3];
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
				dil1.oxygen = dilData[0];
				dil1.helium = dilData[1];
				dil1.type = dilData[2];
				dil1.depth = dilData[3];
			}
			//Dil 2
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL2, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil2.oxygen = dilData[0];
				dil2.helium = dilData[1];
				dil2.type = dilData[2];
				dil2.depth = dilData[3];
			}
			//Dil 3
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL3, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil3.oxygen = dilData[0];
				dil3.helium = dilData[1];
				dil3.type = dilData[2];
				dil3.depth = dilData[3];
			}
			//Dil 4
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL4, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil4.oxygen = dilData[0];
				dil4.helium = dilData[1];
				dil4.type = dilData[2];
				dil4.depth = dilData[3];
			}
			//Dil 5
			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_DIL5, dilData, sizeof(dilData));
			if (rc == DC_STATUS_SUCCESS) {
				//Data read successful
				dil5.oxygen = dilData[0];
				dil5.helium = dilData[1];
				dil5.type = dilData[2];
				dil5.depth = dilData[3];
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

#define READ_SETTING(_OSTC3_SETTING, _DEVICE_DETAIL) \
			do { \
				rc = hw_ostc3_device_config_read(m_data->device, _OSTC3_SETTING, uData, sizeof(uData)); \
				if (rc == DC_STATUS_SUCCESS) \
					m_deviceDetails->_DEVICE_DETAIL(uData[0]); \
			} while (0)

			READ_SETTING(OSTC3_DIVE_MODE, setDiveMode);
			READ_SETTING(OSTC3_SATURATION, setSaturation);
			READ_SETTING(OSTC3_DESATURATION, setDesaturation);
			READ_SETTING(OSTC3_LAST_DECO, setLastDeco);
			READ_SETTING(OSTC3_BRIGHTNESS, setBrightness);
			READ_SETTING(OSTC3_UNITS, setUnits);
			READ_SETTING(OSTC3_SAMPLING_RATE,setSamplingRate);
			READ_SETTING(OSTC3_SALINITY, setSalinity);
			READ_SETTING(OSTC3_DIVEMODE_COLOR,setDiveModeColor);
			READ_SETTING(OSTC3_LANGUAGE, setLanguage);
			READ_SETTING(OSTC3_DATE_FORMAT, setDateFormat);
			READ_SETTING(OSTC3_COMPASS_GAIN, setCompassGain);
			READ_SETTING(OSTC3_SAFETY_STOP, setSafetyStop);
			READ_SETTING(OSTC3_GF_HIGH, setGfHigh);
			READ_SETTING(OSTC3_GF_LOW, setGfLow);
			READ_SETTING(OSTC3_PPO2_MIN, setPpO2Min);
			READ_SETTING(OSTC3_PPO2_MAX, setPpO2Max);
			READ_SETTING(OSTC3_FUTURE_TTS, setFutureTTS);
			READ_SETTING(OSTC3_CCR_MODE, setCcrMode);
			READ_SETTING(OSTC3_DECO_TYPE, setDecoType);
			READ_SETTING(OSTC3_AGF_SELECTABLE, setAGFSelectable);
			READ_SETTING(OSTC3_AGF_HIGH, setAGFHigh);
			READ_SETTING(OSTC3_AGF_LOW, setAGFLow);
			READ_SETTING(OSTC3_CALIBRATION_GAS_O2, setCalibrationGas);
			READ_SETTING(OSTC3_FLIP_SCREEN, setFlipScreen);
			READ_SETTING(OSTC3_SETPOINT_FALLBACK, setSetPointFallback);

#undef READ_SETTING

			rc = hw_ostc3_device_config_read(m_data->device, OSTC3_PRESSURE_SENSOR_OFFSET, uData, sizeof(uData));
			if (rc == DC_STATUS_SUCCESS) {
				// OSTC3 stores the pressureSensorOffset in two-complement
				m_deviceDetails->setPressureSensorOffset((signed char) uData[0]);
			}

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
			break;
		}
#ifdef DEBUG_OSTC
		case DC_FAMILY_NULL:
#endif
		case DC_FAMILY_HW_OSTC: {
			supported = true;
			unsigned char data[256] = {};
#ifdef DEBUG_OSTC_CF
			// FIXME: how should we report settings not supported back?
			unsigned char max_CF = 0;
#endif
			rc = hw_ostc_device_eeprom_read(m_data->device, 0, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS) {
				m_deviceDetails->setSerialNo(QString::number(data[1] << 8 ^ data[0]));
				m_deviceDetails->setNumberOfDives(data[3] << 8 ^ data[2]);
				//Byte5-6:
				//Gas 1 default (%O2=21, %He=0)
				gas gas1;
				gas1.oxygen = data[6];
				gas1.helium = data[7];
				//Byte9-10:
				//Gas 2 default (%O2=21, %He=0)
				gas gas2;
				gas2.oxygen = data[10];
				gas2.helium = data[11];
				//Byte13-14:
				//Gas 3 default (%O2=21, %He=0)
				gas gas3;
				gas3.oxygen = data[14];
				gas3.helium = data[15];
				//Byte17-18:
				//Gas 4 default (%O2=21, %He=0)
				gas gas4;
				gas4.oxygen = data[18];
				gas4.helium = data[19];
				//Byte21-22:
				//Gas 5 default (%O2=21, %He=0)
				gas gas5;
				gas5.oxygen = data[22];
				gas5.helium = data[23];
				//Byte25-26:
				//Gas 6 current (%O2, %He)
				m_deviceDetails->setSalinity(data[26]);
				// Active Gas Flag Register
				gas1.type = data[27] & 0x01;
				gas2.type = (data[27] & 0x02) >> 1;
				gas3.type = (data[27] & 0x04) >> 2;
				gas4.type = (data[27] & 0x08) >> 3;
				gas5.type = (data[27] & 0x10) >> 4;

				// Gas switch depths
				gas1.depth = data[28];
				gas2.depth = data[29];
				gas3.depth = data[30];
				gas4.depth = data[31];
				gas5.depth = data[32];
				// 33 which gas is Fist gas
				switch(data[33]) {
					case 1:
						gas1.type = 2;
						break;
					case 2:
						gas2.type = 2;
						break;
					case 3:
						gas3.type = 2;
						break;
					case 4:
						gas4.type = 2;
						break;
					case 5:
						gas5.type = 2;
						break;
					default:
						//Error?
						break;
				}
				// Data filled up, set the gases.
				m_deviceDetails->setGas1(gas1);
				m_deviceDetails->setGas2(gas2);
				m_deviceDetails->setGas3(gas3);
				m_deviceDetails->setGas4(gas4);
				m_deviceDetails->setGas5(gas5);
				m_deviceDetails->setDecoType(data[34]);
				//Byte36:
				//Use O2 Sensor Module in CC Modes (0= OFF, 1= ON) (Only available in old OSTC1 - unused for OSTC Mk.2/2N)
				//m_deviceDetails->setCcrMode(data[35]);
				setpoint sp1;
				sp1.sp = data[36];
				sp1.depth = 0;
				setpoint sp2;
				sp2.sp = data[37];
				sp2.depth = 0;
				setpoint sp3;
				sp3.sp = data[38];
				sp3.depth = 0;
				m_deviceDetails->setSp1(sp1);
				m_deviceDetails->setSp2(sp2);
				m_deviceDetails->setSp3(sp3);
				// Byte41-42:
				// Lowest Battery voltage seen (in mV)
				// Byte43:
				// Lowest Battery voltage seen at (Month)
				// Byte44:
				// Lowest Battery voltage seen at (Day)
				// Byte45:
				// Lowest Battery voltage seen at (Year)
				// Byte46-47:
				// Lowest Battery voltage seen at (Temperature in 0.1 °C)
				// Byte48:
				// Last complete charge at (Month)
				// Byte49:
				// Last complete charge at (Day)
				// Byte50:
				// Last complete charge at (Year)
				// Byte51-52:
				// Total charge cycles
				// Byte53-54:
				// Total complete charge cycles
				// Byte55-56:
				// Temperature Extrema minimum (Temperature in 0.1 °C)
				// Byte57:
				// Temperature Extrema minimum at (Month)
				// Byte58:
				// Temperature Extrema minimum at (Day)
				// Byte59:
				// Temperature Extrema minimum at (Year)
				// Byte60-61:
				// Temperature Extrema maximum (Temperature in 0.1 °C)
				// Byte62:
				// Temperature Extrema maximum at (Month)
				// Byte63:
				// Temperature Extrema maximum at (Day)
				// Byte64:
				// Temperature Extrema maximum at (Year)
				// Byte65:
				// Custom Text active (=1), Custom Text Disabled (<>1)
				// Byte66-90:
				// TO FIX EDITOR SYNTAX/INDENT {
				// (25Bytes): Custom Text for Surfacemode (Real text must end with "}")
				// Example: OSTC Dive Computer} (19 Characters incl. "}") Bytes 85-90 will be ignored.
				if (data[64] == 1) {
					// Make shure the data is null-terminated
					data[89] = 0;
					// Find the internal termination and replace it with 0
					char *term = strchr((char *) data + 65, (int)'}');
					if (term)
						*term = 0;
					m_deviceDetails->setCustomText((const char*) data + 65);
				}
				// Byte91:
				// Dim OLED in Divemode (>0), Normal mode (=0)
				// Byte92:
				// Date format for all outputs:
				// =0: MM/DD/YY
				// =1: DD/MM/YY
				// =2: YY/MM/DD
				m_deviceDetails->setDateFormat(data[91]);
				// Byte93:
				// Total number of CF used in installed firmware
#ifdef DEBUG_OSTC_CF
				max_CF = data[92];
#endif
				// Byte94:
				// Last selected view for customview area in surface mode
				// Byte95:
				// Last selected view for customview area in dive mode
				// Byte96-97:
				// Diluent 1 Default (%O2,%He)
				// Byte98-99:
				// Diluent 1 Current (%O2,%He)
				gas dil1 = {};
				dil1.oxygen = data[97];
				dil1.helium = data[98];
				// Byte100-101:
				// Gasuent 2 Default (%O2,%He)
				// Byte102-103:
				// Gasuent 2 Current (%O2,%He)
				gas dil2 = {};
				dil2.oxygen = data[101];
				dil2.helium = data[102];
				// Byte104-105:
				// Gasuent 3 Default (%O2,%He)
				// Byte106-107:
				// Gasuent 3 Current (%O2,%He)
				gas dil3 = {};
				dil3.oxygen = data[105];
				dil3.helium = data[106];
				// Byte108-109:
				// Gasuent 4 Default (%O2,%He)
				// Byte110-111:
				// Gasuent 4 Current (%O2,%He)
				gas dil4 = {};
				dil4.oxygen = data[109];
				dil4.helium = data[110];
				// Byte112-113:
				// Gasuent 5 Default (%O2,%He)
				// Byte114-115:
				// Gasuent 5 Current (%O2,%He)
				gas dil5 = {};
				dil5.oxygen = data[113];
				dil5.helium = data[114];
				// Byte116:
				// First Diluent (1-5)
				switch(data[115]) {
					case 1:
						dil1.type = 2;
						break;
					case 2:
						dil2.type = 2;
						break;
					case 3:
						dil3.type = 2;
						break;
					case 4:
						dil4.type = 2;
						break;
					case 5:
						dil5.type = 2;
						break;
					default:
						//Error?
						break;
				}
				m_deviceDetails->setDil1(dil1);
				m_deviceDetails->setDil2(dil2);
				m_deviceDetails->setDil3(dil3);
				m_deviceDetails->setDil4(dil4);
				m_deviceDetails->setDil5(dil5);
				// Byte117-128:
				// not used/reserved
				// Byte129-256:
				// 32 custom Functions (CF0-CF31)

				// Decode the relevant ones
				// CF11: Factor for saturation processes
				m_deviceDetails->setSaturation(read_ostc_cf(data, 11));
				// CF12: Factor for desaturation processes
				m_deviceDetails->setDesaturation(read_ostc_cf(data, 12));
				// CF17: Lower threshold for ppO2 warning
				m_deviceDetails->setPpO2Min(read_ostc_cf(data, 17));
				// CF18: Upper threshold for ppO2 warning
				m_deviceDetails->setPpO2Max(read_ostc_cf(data, 18));
				// CF20: Depth sampling rate for Profile storage
				m_deviceDetails->setSamplingRate(read_ostc_cf(data, 20));
				// CF29: Depth of last decompression stop
				m_deviceDetails->setLastDeco(read_ostc_cf(data, 29));

#ifdef DEBUG_OSTC_CF
				for(int cf = 0; cf <= 31 && cf <= max_CF; cf++)
					printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
			}
			rc = hw_ostc_device_eeprom_read(m_data->device, 1, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS) {
				// Byte1:
				// Logbook version indicator (Not writable!)
				// Byte2-3:
				// Last Firmware installed, 1st Byte.2nd Byte (e.g. „1.90“) (Not writable!)
				m_deviceDetails->setFirmwareVersion(QString::number(data[1]) + "." + QString::number(data[2]));
				// Byte4:
				// OLED brightness (=0: Eco, =1 High) (Not writable!)
				// Byte5-11:
				// Time/Date vault during firmware updates
				// Byte12-128
				// not used/reserved
				// Byte129-256:
				// 32 custom Functions (CF 32-63)

				// Decode the relevant ones
				// CF32: Gradient Factor low
				m_deviceDetails->setGfLow(read_ostc_cf(data, 32));
				// CF33: Gradient Factor high
				m_deviceDetails->setGfHigh(read_ostc_cf(data, 33));
				// CF58: Future time to surface setFutureTTS
				m_deviceDetails->setFutureTTS(read_ostc_cf(data, 58));
#ifdef DEBUG_OSTC_CF
				for(int cf = 32; cf <= 63 && cf <= max_CF; cf++)
					printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
			}
			rc = hw_ostc_device_eeprom_read(m_data->device, 2, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS) {
				// Byte1-4:
				// not used/reserved (Not writable!)
				// Byte5-128:
				// not used/reserved
				// Byte129-256:
				// 32 custom Functions (CF 64-95)

				// Decode the relevant ones
				// CF65: Show safety stop
				m_deviceDetails->setSafetyStop(read_ostc_cf(data, 65));
				// CF67: Alternaitve Gradient Factor low
				m_deviceDetails->setAGFLow(read_ostc_cf(data, 67));
				// CF68: Alternative Gradient Factor high
				m_deviceDetails->setAGFHigh(read_ostc_cf(data, 68));
				// CF69: Allow Gradient Factor change
				m_deviceDetails->setAGFSelectable(read_ostc_cf(data, 69));
#ifdef DEBUG_OSTC_CF
				for(int cf = 64; cf <= 95 && cf <= max_CF; cf++)
					printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
			}
			emit devicedetails(m_deviceDetails);
			break;
		}
#endif	// divecomputer 0.5.0
		default:
			supported = false;
			break;
		}
	} else {
		lastError = tr("Could not a establish connection to the dive computer.");
		emit error(lastError);
	}
unsupported_dc_error:
	dc_device_close(m_data->device);

	if (!supported) {
		lastError = tr("This feature is not yet available for the selected dive computer.");
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
#ifdef DEBUG_OSTC
	rc = DC_STATUS_SUCCESS;
#else
	rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
#endif
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
		case DC_FAMILY_HW_OSTC3: {
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
#define WRITE_SETTING(_OSTC3_SETTING, _DEVICE_DETAIL) \
			do { \
				data[0] = m_deviceDetails->_DEVICE_DETAIL(); \
				hw_ostc3_device_config_write(m_data->device, _OSTC3_SETTING, data, sizeof(data)); \
			} while (0)

			WRITE_SETTING(OSTC3_DIVE_MODE, diveMode);
			WRITE_SETTING(OSTC3_SATURATION, saturation);
			WRITE_SETTING(OSTC3_DESATURATION, desaturation);
			WRITE_SETTING(OSTC3_LAST_DECO, lastDeco);
			WRITE_SETTING(OSTC3_BRIGHTNESS, brightness);
			WRITE_SETTING(OSTC3_UNITS, units);
			WRITE_SETTING(OSTC3_SAMPLING_RATE, samplingRate);
			WRITE_SETTING(OSTC3_SALINITY, salinity);
			WRITE_SETTING(OSTC3_DIVEMODE_COLOR, diveModeColor);
			WRITE_SETTING(OSTC3_LANGUAGE, language);
			WRITE_SETTING(OSTC3_DATE_FORMAT, dateFormat);
			WRITE_SETTING(OSTC3_COMPASS_GAIN, compassGain);
			WRITE_SETTING(OSTC3_SAFETY_STOP, safetyStop);
			WRITE_SETTING(OSTC3_GF_HIGH, gfHigh);
			WRITE_SETTING(OSTC3_GF_LOW, gfLow);
			WRITE_SETTING(OSTC3_PPO2_MIN, ppO2Min);
			WRITE_SETTING(OSTC3_PPO2_MAX, ppO2Max);
			WRITE_SETTING(OSTC3_FUTURE_TTS, futureTTS);
			WRITE_SETTING(OSTC3_CCR_MODE, ccrMode);
			WRITE_SETTING(OSTC3_DECO_TYPE, decoType);
			WRITE_SETTING(OSTC3_AGF_SELECTABLE, aGFSelectable);
			WRITE_SETTING(OSTC3_AGF_HIGH, aGFHigh);
			WRITE_SETTING(OSTC3_AGF_LOW, aGFLow); WRITE_SETTING(OSTC3_CALIBRATION_GAS_O2, calibrationGas);
			WRITE_SETTING(OSTC3_FLIP_SCREEN, flipScreen);
			WRITE_SETTING(OSTC3_SETPOINT_FALLBACK, setPointFallback);

#undef WRITE_SETTING

			// OSTC3 stores the pressureSensorOffset in two-complement
			data[0] = (unsigned char) m_deviceDetails->pressureSensorOffset();
			hw_ostc3_device_config_write(m_data->device, OSTC3_PRESSURE_SENSOR_OFFSET, data, sizeof(data));

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
#ifdef DEBUG_OSTC
		case DC_FAMILY_NULL:
#endif
		case DC_FAMILY_HW_OSTC: {
			supported = true;
			unsigned char data[256] = {};
			unsigned char max_CF = 0;

			// Because we write whole memory blocks, we read all the current
			// values out and then change then ones we should change.
			rc = hw_ostc_device_eeprom_read(m_data->device, 0, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS) {
				//Byte5-6:
				//Gas 1 default (%O2=21, %He=0)
				gas gas1 = m_deviceDetails->gas1();
				data[6] = gas1.oxygen;
				data[7] = gas1.helium;
				//Byte9-10:
				//Gas 2 default (%O2=21, %He=0)
				gas gas2 = m_deviceDetails->gas2();
				data[10] = gas2.oxygen;
				data[11] = gas2.helium;
				//Byte13-14:
				//Gas 3 default (%O2=21, %He=0)
				gas gas3 = m_deviceDetails->gas3();
				data[14] = gas3.oxygen;
				data[15] = gas3.helium;
				//Byte17-18:
				//Gas 4 default (%O2=21, %He=0)
				gas gas4 = m_deviceDetails->gas4();
				data[18] = gas4.oxygen;
				data[19] = gas4.helium;
				//Byte21-22:
				//Gas 5 default (%O2=21, %He=0)
				gas gas5 = m_deviceDetails->gas5();
				data[22] = gas5.oxygen;
				data[23] = gas5.helium;
				//Byte25-26:
				//Gas 6 current (%O2, %He)
				data[26] = m_deviceDetails->salinity();
				// Gas types, 0=Disabled, 1=Active, 2=Fist
				// Active Gas Flag Register
				data[27] = 0;
				if (gas1.type)
					data[27] ^= 0x01;
				if (gas2.type)
					data[27] ^= 0x02;
				if (gas3.type)
					data[27] ^= 0x04;
				if (gas4.type)
					data[27] ^= 0x08;
				if (gas5.type)
					data[27] ^= 0x10;

				// Gas switch depths
				data[28] = gas1.depth;
				data[29] = gas2.depth;
				data[30] = gas3.depth;
				data[31] = gas4.depth;
				data[32] = gas5.depth;
				// 33 which gas is Fist gas
				if (gas1.type == 2)
					data[33] = 1;
				else if (gas2.type == 2)
					data[33] = 2;
				else if (gas3.type == 2)
					data[33] = 3;
				else if (gas4.type == 2)
					data[33] = 4;
				else if (gas5.type == 2)
					data[33] = 5;
				else {
					// FIXME: No gas was First?
					// Set gas 1 to first
					data[33] = 1;
				}
				data[34] = m_deviceDetails->decoType();
				//Byte36:
				//Use O2 Sensor Module in CC Modes (0= OFF, 1= ON) (Only available in old OSTC1 - unused for OSTC Mk.2/2N)
				//m_deviceDetails->setCcrMode(data[35]);
				data[36] = m_deviceDetails->sp1().sp;
				data[37] = m_deviceDetails->sp2().sp;
				data[38] = m_deviceDetails->sp3().sp;
				// Byte41-42:
				// Lowest Battery voltage seen (in mV)
				// Byte43:
				// Lowest Battery voltage seen at (Month)
				// Byte44:
				// Lowest Battery voltage seen at (Day)
				// Byte45:
				// Lowest Battery voltage seen at (Year)
				// Byte46-47:
				// Lowest Battery voltage seen at (Temperature in 0.1 °C)
				// Byte48:
				// Last complete charge at (Month)
				// Byte49:
				// Last complete charge at (Day)
				// Byte50:
				// Last complete charge at (Year)
				// Byte51-52:
				// Total charge cycles
				// Byte53-54:
				// Total complete charge cycles
				// Byte55-56:
				// Temperature Extrema minimum (Temperature in 0.1 °C)
				// Byte57:
				// Temperature Extrema minimum at (Month)
				// Byte58:
				// Temperature Extrema minimum at (Day)
				// Byte59:
				// Temperature Extrema minimum at (Year)
				// Byte60-61:
				// Temperature Extrema maximum (Temperature in 0.1 °C)
				// Byte62:
				// Temperature Extrema maximum at (Month)
				// Byte63:
				// Temperature Extrema maximum at (Day)
				// Byte64:
				// Temperature Extrema maximum at (Year)
				// Byte65:
				// Custom Text active (=1), Custom Text Disabled (<>1)
				// Byte66-90:
				// (25Bytes): Custom Text for Surfacemode (Real text must end with "}")
				// Example: OSTC Dive Computer} (19 Characters incl. "}") Bytes 85-90 will be ignored.
				if (m_deviceDetails->customText() == "")
					data[64] = 0;
				else {
					data[64] = 1;
					// Copy the string to the right place in the memory, padded with 0x20 (" ")
					strncpy((char *) data + 65, QString("%1").arg(m_deviceDetails->customText(), -23, QChar(' ')).toUtf8().data(), 23);
					// And terminate the string.
					if (m_deviceDetails->customText().length() <= 23)
						data[65 + m_deviceDetails->customText().length()] = '}';
					else
						data[90] = '}';
				}
				// Byte91:
				// Dim OLED in Divemode (>0), Normal mode (=0)
				// Byte92:
				// Date format for all outputs:
				// =0: MM/DD/YY
				// =1: DD/MM/YY
				// =2: YY/MM/DD
				data[91] = m_deviceDetails->dateFormat();
				// Byte93:
				// Total number of CF used in installed firmware
				max_CF = data[92];
				// Byte94:
				// Last selected view for customview area in surface mode
				// Byte95:
				// Last selected view for customview area in dive mode
				// Byte96-97:
				// Diluent 1 Default (%O2,%He)
				// Byte98-99:
				// Diluent 1 Current (%O2,%He)
				gas dil1 = m_deviceDetails->dil1();
				data[97] = dil1.oxygen;
				data[98] = dil1.helium;
				// Byte100-101:
				// Gasuent 2 Default (%O2,%He)
				// Byte102-103:
				// Gasuent 2 Current (%O2,%He)
				gas dil2 = m_deviceDetails->dil2();
				data[101] = dil2.oxygen;
				data[102] = dil2.helium;
				// Byte104-105:
				// Gasuent 3 Default (%O2,%He)
				// Byte106-107:
				// Gasuent 3 Current (%O2,%He)
				gas dil3 = m_deviceDetails->dil3();
				data[105] = dil3.oxygen;
				data[106] = dil3.helium;
				// Byte108-109:
				// Gasuent 4 Default (%O2,%He)
				// Byte110-111:
				// Gasuent 4 Current (%O2,%He)
				gas dil4 = m_deviceDetails->dil4();
				data[109] = dil4.oxygen;
				data[110] = dil4.helium;
				// Byte112-113:
				// Gasuent 5 Default (%O2,%He)
				// Byte114-115:
				// Gasuent 5 Current (%O2,%He)
				gas dil5 = m_deviceDetails->dil5();
				data[113] = dil5.oxygen;
				data[114] = dil5.helium;
				// Byte116:
				// First Diluent (1-5)
				if (dil1.type == 2)
					data[115] = 1;
				else if (dil2.type == 2)
					data[115] = 2;
				else if (dil3.type == 2)
					data[115] = 3;
				else if (dil4.type == 2)
					data[115] = 4;
				else if (dil5.type == 2)
					data[115] = 5;
				else {
					// FIXME: No first diluent?
					// Set gas 1 to fist
					data[115] = 1;
				}
				// Byte117-128:
				// not used/reserved
				// Byte129-256:
				// 32 custom Functions (CF0-CF31)

				// Write the relevant ones
				// CF11: Factor for saturation processes
				write_ostc_cf(data, 11, max_CF, m_deviceDetails->saturation());
				// CF12: Factor for desaturation processes
				write_ostc_cf(data, 12, max_CF, m_deviceDetails->desaturation());
				// CF17: Lower threshold for ppO2 warning
				write_ostc_cf(data, 17, max_CF, m_deviceDetails->ppO2Min());
				// CF18: Upper threshold for ppO2 warning
				write_ostc_cf(data, 18, max_CF, m_deviceDetails->ppO2Max());
				// CF20: Depth sampling rate for Profile storage
				write_ostc_cf(data, 20, max_CF, m_deviceDetails->samplingRate());
				// CF29: Depth of last decompression stop
				write_ostc_cf(data, 29, max_CF, m_deviceDetails->lastDeco());

#ifdef DEBUG_OSTC_CF
				for(int cf = 0; cf <= 31 && cf <= max_CF; cf++)
					printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
				rc = hw_ostc_device_eeprom_write(m_data->device, 0, data, sizeof(data));
				if (rc != DC_STATUS_SUCCESS) {
					//FIXME: ERROR!
				}
			}
			rc = hw_ostc_device_eeprom_read(m_data->device, 1, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS) {
				// Byte1:
				// Logbook version indicator (Not writable!)
				// Byte2-3:
				// Last Firmware installed, 1st Byte.2nd Byte (e.g. „1.90“) (Not writable!)
				// Byte4:
				// OLED brightness (=0: Eco, =1 High) (Not writable!)
				// Byte5-11:
				// Time/Date vault during firmware updates
				// Byte12-128
				// not used/reserved
				// Byte129-256:
				// 32 custom Functions (CF 32-63)

				// Decode the relevant ones
				// CF32: Gradient Factor low
				write_ostc_cf(data, 32, max_CF, m_deviceDetails->gfLow());
				// CF33: Gradient Factor high
				write_ostc_cf(data, 33, max_CF, m_deviceDetails->gfHigh());
				// CF58: Future time to surface setFutureTTS
				write_ostc_cf(data, 58, max_CF, m_deviceDetails->futureTTS());
#ifdef DEBUG_OSTC_CF
				for(int cf = 32; cf <= 63 && cf <= max_CF; cf++)
					printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
				rc = hw_ostc_device_eeprom_write(m_data->device, 1, data, sizeof(data));
				if (rc != DC_STATUS_SUCCESS) {
					//FIXME: ERROR!
				}
			}
			rc = hw_ostc_device_eeprom_read(m_data->device, 2, data, sizeof(data));
			if (rc == DC_STATUS_SUCCESS) {
				// Byte1-4:
				// not used/reserved (Not writable!)
				// Byte5-128:
				// not used/reserved
				// Byte129-256:
				// 32 custom Functions (CF 64-95)

				// Decode the relevant ones
				// CF65: Show safety stop
				write_ostc_cf(data, 65, max_CF, m_deviceDetails->safetyStop());
				// CF67: Alternaitve Gradient Factor low
				write_ostc_cf(data, 67, max_CF, m_deviceDetails->aGFLow());
				// CF68: Alternative Gradient Factor high
				write_ostc_cf(data, 68, max_CF, m_deviceDetails->aGFHigh());
				// CF69: Allow Gradient Factor change
				write_ostc_cf(data, 69, max_CF, m_deviceDetails->aGFSelectable());
#ifdef DEBUG_OSTC_CF
				for(int cf = 64; cf <= 95 && cf <= max_CF; cf++)
					printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
				rc = hw_ostc_device_eeprom_write(m_data->device, 2, data, sizeof(data));
				if (rc != DC_STATUS_SUCCESS) {
					//FIXME: ERROR!
				}
			}
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
				rc = hw_ostc_device_clock(m_data->device, &time);
				if (rc != DC_STATUS_SUCCESS) {
					//FIXME: ERROR!
				}
			}
			break;
		}
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
		switch (dc_device_get_type(m_data->device)) {
#if DC_VERSION_CHECK(0, 5, 0)
		case DC_FAMILY_HW_OSTC3:
			//Not Yet supported
			//supported = true;
			//rc = hw_ostc3_device_fwupdate(m_data->device, m_fileName.toUtf8().data());
			break;
		case DC_FAMILY_HW_OSTC:
			supported = true;
			rc = hw_ostc_device_fwupdate(m_data->device, m_fileName.toUtf8().data());
			break;
#endif	// divecomputer 0.5.0
		default:
			supported = false;
			break;
		}
		dc_device_close(m_data->device);

		if (!supported) {
			lastError = tr("This feature is not yet available for the selected dive computer.");
			emit error(lastError);
		} else if (rc != DC_STATUS_SUCCESS) {
			lastError = tr("Firmware update failed!");
		}
	}
	else {
		lastError = tr("Could not a establish connection to the dive computer.");
		emit error(lastError);
	}
}


ResetSettingsThread::ResetSettingsThread(QObject *parent, device_data_t *data)
: QThread(parent), m_data(data)
{
}

void ResetSettingsThread::run()
{
	bool supported = false;
	dc_status_t rc;
	rc = dc_device_open(&m_data->device, m_data->context, m_data->descriptor, m_data->devname);
	if (rc == DC_STATUS_SUCCESS) {
#if DC_VERSION_CHECK(0, 5, 0)
		if (dc_device_get_type(m_data->device) == DC_FAMILY_HW_OSTC3) {
			supported = true;
			hw_ostc3_device_config_reset(m_data->device);
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
