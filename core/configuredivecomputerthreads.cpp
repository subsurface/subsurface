// SPDX-License-Identifier: GPL-2.0
#include "configuredivecomputerthreads.h"
#include "libdivecomputer/hw_ostc.h"
#include "libdivecomputer/hw_ostc3.h"
#include "libdivecomputer.h"
#include "units.h"
#include "errorhelper.h"

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
#define OSTC4_VPM_CONSERVATISM		0x29
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
#define OSTC3_LEFT_BUTTON_SENSIVITY	0x3A
#define OSTC3_RIGHT_BUTTON_SENSIVITY	0x3B
#define OSTC4_BUTTON_SENSIVITY		0x3A
#define OSTC3_BOTTOM_GAS_CONSUMPTION	0x3C
#define OSTC3_DECO_GAS_CONSUMPTION	0x3D
#define OSTC3_MOD_WARNING		0x3E
#define OSTC4_TRAVEL_GAS_CONSUMPTION	0x3E
#define OSTC3_DYNAMIC_ASCEND_RATE	0x3F
#define OSTC3_GRAPHICAL_SPEED_INDICATOR	0x40
#define OSTC3_ALWAYS_SHOW_PPO2		0x41
#define OSTC3_TEMP_SENSOR_OFFSET	0x42
#define OSTC3_SAFETY_STOP_LENGTH	0x43
#define OSTC3_SAFETY_STOP_START_DEPTH	0x44
#define OSTC3_SAFETY_STOP_END_DEPTH	0x45
#define OSTC3_SAFETY_STOP_RESET_DEPTH	0x46

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
#define SUUNTO_VYPER_CUSTOM_TEXT_LENGTH   30

#ifdef DEBUG_OSTC
// Fake io to ostc memory banks
#define hw_ostc_device_eeprom_read local_hw_ostc_device_eeprom_read
#define hw_ostc_device_eeprom_write local_hw_ostc_device_eeprom_write
#define OSTC_FILE "../OSTC-data-dump.bin"

// Fake the open function.
static dc_status_t local_dc_device_open(dc_device_t **out, dc_context_t *context, dc_descriptor_t *descriptor, const char *name)
{
	if (strcmp(dc_descriptor_get_vendor(descriptor), "Heinrichs Weikamp") == 0 &&strcmp(dc_descriptor_get_product(descriptor), "OSTC 2N") == 0)
		return DC_STATUS_SUCCESS;
	else
		return dc_device_open(out, context, descriptor, name);
}
#define dc_device_open local_dc_device_open

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

#endif

static int read_ostc_cf(unsigned char data[], unsigned char cf)
{
	return data[128 + (cf % 32) * 4 + 3] << 8 ^ data[128 + (cf % 32) * 4 + 2];
}

static void write_ostc_cf(unsigned char data[], unsigned char cf, unsigned char max_CF, unsigned int value)
{
	// Only write settings supported by this firmware.
	if (cf > max_CF)
		return;

	data[128 + (cf % 32) * 4 + 3] = (value & 0xff00) >> 8;
	data[128 + (cf % 32) * 4 + 2] = (value & 0x00ff);
}

#define EMIT_PROGRESS()	do { \
		progress.current++; \
		progress_cb(device, DC_EVENT_PROGRESS, &progress, userdata); \
	} while (0)

static dc_status_t read_suunto_vyper_settings(dc_device_t *device, DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	unsigned char data[SUUNTO_VYPER_CUSTOM_TEXT_LENGTH + 1];
	dc_status_t rc;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 16;

	rc = dc_device_read(device, SUUNTO_VYPER_COMPUTER_TYPE, data, 1);
	if (rc == DC_STATUS_SUCCESS) {
		dc_descriptor_t *desc = get_descriptor(DC_FAMILY_SUUNTO_VYPER, data[0]);

		if (desc) {
			// We found a supported device
			// we can safely proceed with reading/writing to this device.
			deviceDetails.model = dc_descriptor_get_product(desc);
			dc_descriptor_free(desc);
		} else {
			return DC_STATUS_UNSUPPORTED;
		}
	}
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_MAXDEPTH, data, 2);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	// in ft * 128.0
	int depth = feet_to_mm(data[0] << 8 ^ data[1]) / 128;
	deviceDetails.maxDepth = depth;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_TOTAL_TIME, data, 2);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	int total_time = data[0] << 8 ^ data[1];
	deviceDetails.totalTime = total_time;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_NUMBEROFDIVES, data, 2);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	int number_of_dives = data[0] << 8 ^ data[1];
	deviceDetails.numberOfDives = number_of_dives;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_FIRMWARE, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.firmwareVersion = QString::number(data[0]) + ".0.0";
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_SERIALNUMBER, data, 4);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	int serial_number = data[0] * 1000000 + data[1] * 10000 + data[2] * 100 + data[3];
	deviceDetails.serialNo = QString::number(serial_number);
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_CUSTOM_TEXT, data, SUUNTO_VYPER_CUSTOM_TEXT_LENGTH);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	data[SUUNTO_VYPER_CUSTOM_TEXT_LENGTH] = 0;
	deviceDetails.customText = (const char *)data;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_SAMPLING_RATE, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.samplingRate = (int)data[0];
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_ALTITUDE_SAFETY, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.altitude = data[0] & 0x03;
	deviceDetails.personalSafety = data[0] >> 2 & 0x03;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_TIMEFORMAT, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.timeFormat = data[0] & 0x01;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_UNITS, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.units = data[0] & 0x01;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_MODEL, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.diveMode = data[0] & 0x03;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_LIGHT, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.lightEnabled = data[0] >> 7;
	deviceDetails.light = data[0] & 0x7F;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_ALARM_DEPTH_TIME, data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	deviceDetails.alarmTimeEnabled = data[0] & 0x01;
	deviceDetails.alarmDepthEnabled = data[0] >> 1 & 0x01;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_ALARM_TIME, data, 2);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	int time = data[0] << 8 ^ data[1];
	// The stinger stores alarm time in seconds instead of minutes.
	if (deviceDetails.model == "Stinger")
		time /= 60;
	deviceDetails.alarmTime = time;
	EMIT_PROGRESS();

	rc = dc_device_read(device, SUUNTO_VYPER_ALARM_DEPTH, data, 2);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	depth = feet_to_mm(data[0] << 8 ^ data[1]) / 128;
	deviceDetails.alarmDepth = depth;
	EMIT_PROGRESS();

	return DC_STATUS_SUCCESS;
}

static dc_status_t write_suunto_vyper_settings(dc_device_t *device, DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	dc_status_t rc;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 10;
	unsigned char data;
	unsigned char data2[2];
	int time;

	// Maybee we should read the model from the device to sanity check it here too..
	// For now we just check that we actually read a device before writing to one.
	if (deviceDetails.model == "")
		return DC_STATUS_UNSUPPORTED;

	rc = dc_device_write(device, SUUNTO_VYPER_CUSTOM_TEXT,
			     // Convert the customText to a 30 char wide padded with " "
			     (const unsigned char *)qPrintable(QString("%1").arg(deviceDetails.customText, -30, QChar(' '))),
			     SUUNTO_VYPER_CUSTOM_TEXT_LENGTH);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.samplingRate;
	rc = dc_device_write(device, SUUNTO_VYPER_SAMPLING_RATE, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.personalSafety << 2 ^ deviceDetails.altitude;
	rc = dc_device_write(device, SUUNTO_VYPER_ALTITUDE_SAFETY, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.timeFormat;
	rc = dc_device_write(device, SUUNTO_VYPER_TIMEFORMAT, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.units;
	rc = dc_device_write(device, SUUNTO_VYPER_UNITS, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.diveMode;
	rc = dc_device_write(device, SUUNTO_VYPER_MODEL, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.lightEnabled << 7 ^ (deviceDetails.light & 0x7F);
	rc = dc_device_write(device, SUUNTO_VYPER_LIGHT, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data = deviceDetails.alarmDepthEnabled << 1 ^ deviceDetails.alarmTimeEnabled;
	rc = dc_device_write(device, SUUNTO_VYPER_ALARM_DEPTH_TIME, &data, 1);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	// The stinger stores alarm time in seconds instead of minutes.
	time = deviceDetails.alarmTime;
	if (deviceDetails.model == "Stinger")
		time *= 60;
	data2[0] = time >> 8;
	data2[1] = time & 0xFF;
	rc = dc_device_write(device, SUUNTO_VYPER_ALARM_TIME, data2, 2);
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	data2[0] = (int)(mm_to_feet(deviceDetails.alarmDepth) * 128) >> 8;
	data2[1] = (int)(mm_to_feet(deviceDetails.alarmDepth) * 128) & 0x0FF;
	rc = dc_device_write(device, SUUNTO_VYPER_ALARM_DEPTH, data2, 2);
	EMIT_PROGRESS();
	return rc;
}

static dc_status_t read_ostc4_settings(dc_device_t *device, DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	// This code is really similar to the OSTC3 code, but there are minor
	// differences in what the data means, and how to communicate with the
	// device. If anyone can find a good way to harmonize the two, be my guest.
	dc_status_t rc = DC_STATUS_SUCCESS;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 47;

	EMIT_PROGRESS();

	//Read gas mixes
	gas gas1;
	gas gas2;
	gas gas3;
	gas gas4;
	gas gas5;
	unsigned char gasData[4] = { 0, 0, 0, 0 };

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS1, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas1.oxygen = gasData[0];
	gas1.helium = gasData[1];
	gas1.type = gasData[2];
	gas1.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS2, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas2.oxygen = gasData[0];
	gas2.helium = gasData[1];
	gas2.type = gasData[2];
	gas2.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS3, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas3.oxygen = gasData[0];
	gas3.helium = gasData[1];
	gas3.type = gasData[2];
	gas3.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS4, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas4.oxygen = gasData[0];
	gas4.helium = gasData[1];
	gas4.type = gasData[2];
	gas4.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS5, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas5.oxygen = gasData[0];
	gas5.helium = gasData[1];
	gas5.type = gasData[2];
	gas5.depth = gasData[3];
	EMIT_PROGRESS();

	deviceDetails.gas1 = gas1;
	deviceDetails.gas2 = gas2;
	deviceDetails.gas3 = gas3;
	deviceDetails.gas4 = gas4;
	deviceDetails.gas5 = gas5;
	EMIT_PROGRESS();

	//Read Dil Values
	gas dil1;
	gas dil2;
	gas dil3;
	gas dil4;
	gas dil5;
	unsigned char dilData[4] = { 0, 0, 0, 0 };

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL1, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil1.oxygen = dilData[0];
	dil1.helium = dilData[1];
	dil1.type = dilData[2];
	dil1.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL2, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil2.oxygen = dilData[0];
	dil2.helium = dilData[1];
	dil2.type = dilData[2];
	dil2.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL3, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil3.oxygen = dilData[0];
	dil3.helium = dilData[1];
	dil3.type = dilData[2];
	dil3.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL4, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil4.oxygen = dilData[0];
	dil4.helium = dilData[1];
	dil4.type = dilData[2];
	dil4.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL5, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil5.oxygen = dilData[0];
	dil5.helium = dilData[1];
	dil5.type = dilData[2];
	dil5.depth = dilData[3];
	EMIT_PROGRESS();

	deviceDetails.dil1 = dil1;
	deviceDetails.dil2 = dil2;
	deviceDetails.dil3 = dil3;
	deviceDetails.dil4 = dil4;
	deviceDetails.dil5 = dil5;

	//Read setpoint Values
	setpoint sp1;
	setpoint sp2;
	setpoint sp3;
	setpoint sp4;
	setpoint sp5;
	unsigned char spData[4] = { 0, 0, 0, 0};

	rc = hw_ostc3_device_config_read(device, OSTC3_SP1, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp1.sp = spData[0];
	sp1.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP2, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp2.sp = spData[0];
	sp2.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP3, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp3.sp = spData[0];
	sp3.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP4, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp4.sp = spData[0];
	sp4.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP5, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp5.sp = spData[0];
	sp5.depth = spData[1];
	EMIT_PROGRESS();

	deviceDetails.sp1 = sp1;
	deviceDetails.sp2 = sp2;
	deviceDetails.sp3 = sp3;
	deviceDetails.sp4 = sp4;
	deviceDetails.sp5 = sp5;

	//Read other settings
	unsigned char uData[4] = { 0 };

#define READ_SETTING(_OSTC4_SETTING, _DEVICE_DETAIL)                                            \
	do {                                                                                    \
		rc = hw_ostc3_device_config_read(device, _OSTC4_SETTING, uData, sizeof(uData)); \
		if (rc != DC_STATUS_SUCCESS)                                                    \
			return rc;                                                              \
		deviceDetails._DEVICE_DETAIL = uData[0];                                        \
		EMIT_PROGRESS();                                                                \
	} while (0)

	READ_SETTING(OSTC3_DIVE_MODE, diveMode);
	READ_SETTING(OSTC3_LAST_DECO, lastDeco);
	READ_SETTING(OSTC3_BRIGHTNESS, brightness);
	READ_SETTING(OSTC3_UNITS, units);
	READ_SETTING(OSTC3_SALINITY, salinity);
	READ_SETTING(OSTC3_DIVEMODE_COLOR, diveModeColor);
	READ_SETTING(OSTC3_LANGUAGE, language);
	READ_SETTING(OSTC3_DATE_FORMAT, dateFormat);
	READ_SETTING(OSTC3_SAFETY_STOP, safetyStop);
	READ_SETTING(OSTC3_GF_HIGH, gfHigh);
	READ_SETTING(OSTC3_GF_LOW, gfLow);
	READ_SETTING(OSTC3_PPO2_MIN, ppO2Min);
	READ_SETTING(OSTC3_PPO2_MAX, ppO2Max);
	READ_SETTING(OSTC3_FUTURE_TTS, futureTTS);
	READ_SETTING(OSTC3_CCR_MODE, ccrMode);
	READ_SETTING(OSTC3_DECO_TYPE, decoType);
	READ_SETTING(OSTC3_AGF_HIGH, aGFHigh);
	READ_SETTING(OSTC3_AGF_LOW, aGFLow);
	READ_SETTING(OSTC4_VPM_CONSERVATISM, vpmConservatism);
	READ_SETTING(OSTC3_SETPOINT_FALLBACK, setPointFallback);
	READ_SETTING(OSTC4_BUTTON_SENSIVITY, buttonSensitivity);
	READ_SETTING(OSTC3_BOTTOM_GAS_CONSUMPTION, bottomGasConsumption);
	READ_SETTING(OSTC3_DECO_GAS_CONSUMPTION, decoGasConsumption);
	READ_SETTING(OSTC4_TRAVEL_GAS_CONSUMPTION, travelGasConsumption);
	READ_SETTING(OSTC3_ALWAYS_SHOW_PPO2, alwaysShowppO2);
	READ_SETTING(OSTC3_SAFETY_STOP_LENGTH, safetyStopLength);
	READ_SETTING(OSTC3_SAFETY_STOP_START_DEPTH, safetyStopStartDepth);
	/*
	 * Settings not yet implemented
	 *
	 * logbook offset 0x47 0..9000 low byte 0..9000 high byte
	 * Extra display 0x71 0=0ff, 1=BigFont
	 * Custom View Center 0x72 0..8 (..9 Bonex Version)
	 * CV Center Fallback 0x73 0..20 sec
	 * Custom View Corner 0x74 1..7
	 * CV Corner Fallback 0x75 0..20 sec
	 */

#undef READ_SETTING

	rc = hw_ostc3_device_config_read(device, OSTC3_PRESSURE_SENSOR_OFFSET, uData, sizeof(uData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	// OSTC3 stores the pressureSensorOffset in two-complement
	deviceDetails.pressureSensorOffset = (signed char)uData[0];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_TEMP_SENSOR_OFFSET, uData, sizeof(uData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	// OSTC3 stores the tempSensorOffset in two-complement
	deviceDetails.tempSensorOffset = (signed char)uData[0];
	EMIT_PROGRESS();

	//read firmware settings
	unsigned char fData[64] = { 0 };
	rc = hw_ostc3_device_version(device, fData, sizeof(fData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	int serial = fData[0] + (fData[1] << 8);
	deviceDetails.serialNo = QString::number(serial);
	unsigned char X, Y, Z, beta;
	unsigned int firmwareOnDevice = (fData[3] << 8) + fData[2];
	X = (firmwareOnDevice & 0xF800) >> 11;
	Y = (firmwareOnDevice & 0x07C0) >> 6;
	Z = (firmwareOnDevice & 0x003E) >> 1;
	beta = firmwareOnDevice & 0x0001;
	deviceDetails.firmwareVersion = QString("%1.%2.%3%4").arg(X).arg(Y).arg(Z).arg(beta?" beta":"");
	QByteArray ar((char *)fData + 4, 60);
	deviceDetails.customText = ar.trimmed();
	EMIT_PROGRESS();

	return rc;
}

static dc_status_t write_ostc4_settings(dc_device_t *device, const DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	// This code is really similar to the OSTC3 code, but there are minor
	// differences in what the data means, and how to communicate with the
	// device. If anyone can find a good way to harmonize the two, be my guest.
	dc_status_t rc = DC_STATUS_SUCCESS;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 21;

	//write gas values
	unsigned char gas1Data[4] = {
		deviceDetails.gas1.oxygen,
		deviceDetails.gas1.helium,
		deviceDetails.gas1.type,
		deviceDetails.gas1.depth
	};

	unsigned char gas2Data[4] = {
		deviceDetails.gas2.oxygen,
		deviceDetails.gas2.helium,
		deviceDetails.gas2.type,
		deviceDetails.gas2.depth
	};

	unsigned char gas3Data[4] = {
		deviceDetails.gas3.oxygen,
		deviceDetails.gas3.helium,
		deviceDetails.gas3.type,
		deviceDetails.gas3.depth
	};

	unsigned char gas4Data[4] = {
		deviceDetails.gas4.oxygen,
		deviceDetails.gas4.helium,
		deviceDetails.gas4.type,
		deviceDetails.gas4.depth
	};

	unsigned char gas5Data[4] = {
		deviceDetails.gas5.oxygen,
		deviceDetails.gas5.helium,
		deviceDetails.gas5.type,
		deviceDetails.gas5.depth
	};
	//gas 1
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS1, gas1Data, sizeof(gas1Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 2
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS2, gas2Data, sizeof(gas2Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 3
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS3, gas3Data, sizeof(gas3Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 4
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS4, gas4Data, sizeof(gas4Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 5
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS5, gas5Data, sizeof(gas5Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//write setpoint values
	unsigned char sp1Data[4] = {
		deviceDetails.sp1.sp,
		deviceDetails.sp1.depth
	};

	unsigned char sp2Data[4] = {
		deviceDetails.sp2.sp,
		deviceDetails.sp2.depth
	};

	unsigned char sp3Data[4] = {
		deviceDetails.sp3.sp,
		deviceDetails.sp3.depth
	};

	unsigned char sp4Data[4] = {
		deviceDetails.sp4.sp,
		deviceDetails.sp4.depth
	};

	unsigned char sp5Data[4] = {
		deviceDetails.sp5.sp,
		deviceDetails.sp5.depth
	};

	//sp 1
	rc = hw_ostc3_device_config_write(device, OSTC3_SP1, sp1Data, sizeof(sp1Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 2
	rc = hw_ostc3_device_config_write(device, OSTC3_SP2, sp2Data, sizeof(sp2Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 3
	rc = hw_ostc3_device_config_write(device, OSTC3_SP3, sp3Data, sizeof(sp3Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 4
	rc = hw_ostc3_device_config_write(device, OSTC3_SP4, sp4Data, sizeof(sp4Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 5
	rc = hw_ostc3_device_config_write(device, OSTC3_SP5, sp5Data, sizeof(sp5Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//write dil values
	unsigned char dil1Data[4] = {
		deviceDetails.dil1.oxygen,
		deviceDetails.dil1.helium,
		deviceDetails.dil1.type,
		deviceDetails.dil1.depth
	};

	unsigned char dil2Data[4] = {
		deviceDetails.dil2.oxygen,
		deviceDetails.dil2.helium,
		deviceDetails.dil2.type,
		deviceDetails.dil2.depth
	};

	unsigned char dil3Data[4] = {
		deviceDetails.dil3.oxygen,
		deviceDetails.dil3.helium,
		deviceDetails.dil3.type,
		deviceDetails.dil3.depth
	};

	unsigned char dil4Data[4] = {
		deviceDetails.dil4.oxygen,
		deviceDetails.dil4.helium,
		deviceDetails.dil4.type,
		deviceDetails.dil4.depth
	};

	unsigned char dil5Data[4] = {
		deviceDetails.dil5.oxygen,
		deviceDetails.dil5.helium,
		deviceDetails.dil5.type,
		deviceDetails.dil5.depth
	};
	//dil 1
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL1, dil1Data, sizeof(gas1Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 2
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL2, dil2Data, sizeof(dil2Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 3
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL3, dil3Data, sizeof(dil3Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 4
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL4, dil4Data, sizeof(dil4Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 5
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL5, dil5Data, sizeof(dil5Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//write general settings
	//custom text
	rc = hw_ostc3_device_customtext(device, qPrintable(deviceDetails.customText));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	unsigned char data[4] = { 0 };
#define WRITE_SETTING(_OSTC4_SETTING, _DEVICE_DETAIL)                                          \
	do {                                                                                   \
		data[0] = deviceDetails._DEVICE_DETAIL;                                        \
		rc = hw_ostc3_device_config_write(device, _OSTC4_SETTING, data, sizeof(data)); \
		if (rc != DC_STATUS_SUCCESS)                                                   \
			return rc;                                                             \
		EMIT_PROGRESS();                                                               \
	} while (0)

	WRITE_SETTING(OSTC3_DIVE_MODE, diveMode);
	WRITE_SETTING(OSTC3_LAST_DECO, lastDeco);
	WRITE_SETTING(OSTC3_BRIGHTNESS, brightness);
	WRITE_SETTING(OSTC3_UNITS, units);
	WRITE_SETTING(OSTC3_SALINITY, salinity);
	WRITE_SETTING(OSTC3_DIVEMODE_COLOR, diveModeColor);
	WRITE_SETTING(OSTC3_LANGUAGE, language);
	WRITE_SETTING(OSTC3_DATE_FORMAT, dateFormat);
	WRITE_SETTING(OSTC3_SAFETY_STOP, safetyStop);
	WRITE_SETTING(OSTC3_GF_HIGH, gfHigh);
	WRITE_SETTING(OSTC3_GF_LOW, gfLow);
	WRITE_SETTING(OSTC3_PPO2_MIN, ppO2Min);
	WRITE_SETTING(OSTC3_PPO2_MAX, ppO2Max);
	WRITE_SETTING(OSTC3_FUTURE_TTS, futureTTS);
	WRITE_SETTING(OSTC3_CCR_MODE, ccrMode);
	WRITE_SETTING(OSTC3_DECO_TYPE, decoType);
	WRITE_SETTING(OSTC3_AGF_HIGH, aGFHigh);
	WRITE_SETTING(OSTC3_AGF_LOW, aGFLow);
	WRITE_SETTING(OSTC4_VPM_CONSERVATISM, vpmConservatism);
	WRITE_SETTING(OSTC3_SETPOINT_FALLBACK, setPointFallback);
	WRITE_SETTING(OSTC4_BUTTON_SENSIVITY, buttonSensitivity);
	WRITE_SETTING(OSTC3_BOTTOM_GAS_CONSUMPTION, bottomGasConsumption);
	WRITE_SETTING(OSTC3_DECO_GAS_CONSUMPTION, decoGasConsumption);
	WRITE_SETTING(OSTC4_TRAVEL_GAS_CONSUMPTION, travelGasConsumption);
	WRITE_SETTING(OSTC3_ALWAYS_SHOW_PPO2, alwaysShowppO2);
	WRITE_SETTING(OSTC3_SAFETY_STOP_LENGTH, safetyStopLength);
	WRITE_SETTING(OSTC3_SAFETY_STOP_START_DEPTH, safetyStopStartDepth);
	/*
	 * Settings not yet implemented
	 *
	 * logbook offset 0x47 0..9000 low byte 0..9000 high byte
	 * Extra display 0x71 0=0ff, 1=BigFont
	 * Custom View Center 0x72 0..8 (..9 Bonex Version)
	 * CV Center Fallback 0x73 0..20 sec
	 * Custom View Corner 0x74 1..7
	 * CV Corner Fallback 0x75 0..20 sec
	 */

#undef WRITE_SETTING

	// OSTC3 stores the pressureSensorOffset in two-complement
	data[0] = (unsigned char)deviceDetails.pressureSensorOffset;
	rc = hw_ostc3_device_config_write(device, OSTC3_PRESSURE_SENSOR_OFFSET, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	// OSTC3 stores the tempSensorOffset in two-complement
	data[0] = (unsigned char)deviceDetails.tempSensorOffset;
	rc = hw_ostc3_device_config_write(device, OSTC3_TEMP_SENSOR_OFFSET, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();


	//sync date and time
	if (deviceDetails.syncTime) {
		dc_datetime_t now;
		dc_datetime_localtime(&now, dc_datetime_now());

		rc = dc_device_timesync(device, &now);
	}

	EMIT_PROGRESS();

	return rc;
}

static dc_status_t read_ostc3_settings(dc_device_t *device, DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	dc_status_t rc;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 56;
	unsigned char hardware[1];

	//Read hardware type
	rc = hw_ostc3_device_hardware (device, hardware, sizeof (hardware));
	if (rc != DC_STATUS_SUCCESS)
		return rc;

	dc_descriptor_t *desc = get_descriptor(DC_FAMILY_HW_OSTC3, hardware[0]);
	if (desc) {
		deviceDetails.model = dc_descriptor_get_product(desc);
		dc_descriptor_free(desc);
	} else {
		return DC_STATUS_UNSUPPORTED;
	}

	if (deviceDetails.model == "OSTC 4")
		return read_ostc4_settings(device, deviceDetails, progress_cb, userdata);

	EMIT_PROGRESS();

	//Read gas mixes
	gas gas1;
	gas gas2;
	gas gas3;
	gas gas4;
	gas gas5;
	unsigned char gasData[4] = { 0, 0, 0, 0 };

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS1, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas1.oxygen = gasData[0];
	gas1.helium = gasData[1];
	gas1.type = gasData[2];
	gas1.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS2, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas2.oxygen = gasData[0];
	gas2.helium = gasData[1];
	gas2.type = gasData[2];
	gas2.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS3, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas3.oxygen = gasData[0];
	gas3.helium = gasData[1];
	gas3.type = gasData[2];
	gas3.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS4, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas4.oxygen = gasData[0];
	gas4.helium = gasData[1];
	gas4.type = gasData[2];
	gas4.depth = gasData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_GAS5, gasData, sizeof(gasData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	gas5.oxygen = gasData[0];
	gas5.helium = gasData[1];
	gas5.type = gasData[2];
	gas5.depth = gasData[3];
	EMIT_PROGRESS();

	deviceDetails.gas1 = gas1;
	deviceDetails.gas2 = gas2;
	deviceDetails.gas3 = gas3;
	deviceDetails.gas4 = gas4;
	deviceDetails.gas5 = gas5;
	EMIT_PROGRESS();

	//Read Dil Values
	gas dil1;
	gas dil2;
	gas dil3;
	gas dil4;
	gas dil5;
	unsigned char dilData[4] = { 0, 0, 0, 0 };

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL1, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil1.oxygen = dilData[0];
	dil1.helium = dilData[1];
	dil1.type = dilData[2];
	dil1.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL2, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil2.oxygen = dilData[0];
	dil2.helium = dilData[1];
	dil2.type = dilData[2];
	dil2.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL3, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil3.oxygen = dilData[0];
	dil3.helium = dilData[1];
	dil3.type = dilData[2];
	dil3.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL4, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil4.oxygen = dilData[0];
	dil4.helium = dilData[1];
	dil4.type = dilData[2];
	dil4.depth = dilData[3];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_DIL5, dilData, sizeof(dilData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	dil5.oxygen = dilData[0];
	dil5.helium = dilData[1];
	dil5.type = dilData[2];
	dil5.depth = dilData[3];
	EMIT_PROGRESS();

	deviceDetails.dil1 = dil1;
	deviceDetails.dil2 = dil2;
	deviceDetails.dil3 = dil3;
	deviceDetails.dil4 = dil4;
	deviceDetails.dil5 = dil5;

	//Read setpoint Values
	setpoint sp1;
	setpoint sp2;
	setpoint sp3;
	setpoint sp4;
	setpoint sp5;
	unsigned char spData[2] = { 0, 0 };

	rc = hw_ostc3_device_config_read(device, OSTC3_SP1, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp1.sp = spData[0];
	sp1.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP2, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp2.sp = spData[0];
	sp2.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP3, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp3.sp = spData[0];
	sp3.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP4, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp4.sp = spData[0];
	sp4.depth = spData[1];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_SP5, spData, sizeof(spData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	sp5.sp = spData[0];
	sp5.depth = spData[1];
	EMIT_PROGRESS();

	deviceDetails.sp1 = sp1;
	deviceDetails.sp2 = sp2;
	deviceDetails.sp3 = sp3;
	deviceDetails.sp4 = sp4;
	deviceDetails.sp5 = sp5;

	//Read other settings
	unsigned char uData[1] = { 0 };

#define READ_SETTING(_OSTC3_SETTING, _DEVICE_DETAIL)                                            \
	do {                                                                                    \
		rc = hw_ostc3_device_config_read(device, _OSTC3_SETTING, uData, sizeof(uData)); \
		if (rc != DC_STATUS_SUCCESS)                                                    \
			return rc;                                                              \
		deviceDetails._DEVICE_DETAIL = uData[0];                                        \
		EMIT_PROGRESS();                                                                \
	} while (0)

	READ_SETTING(OSTC3_DIVE_MODE, diveMode);
	READ_SETTING(OSTC3_SATURATION, saturation);
	READ_SETTING(OSTC3_DESATURATION, desaturation);
	READ_SETTING(OSTC3_LAST_DECO, lastDeco);
	READ_SETTING(OSTC3_BRIGHTNESS, brightness);
	READ_SETTING(OSTC3_UNITS, units);
	READ_SETTING(OSTC3_SAMPLING_RATE, samplingRate);
	READ_SETTING(OSTC3_SALINITY, salinity);
	READ_SETTING(OSTC3_DIVEMODE_COLOR, diveModeColor);
	READ_SETTING(OSTC3_LANGUAGE, language);
	READ_SETTING(OSTC3_DATE_FORMAT, dateFormat);
	READ_SETTING(OSTC3_COMPASS_GAIN, compassGain);
	READ_SETTING(OSTC3_SAFETY_STOP, safetyStop);
	READ_SETTING(OSTC3_GF_HIGH, gfHigh);
	READ_SETTING(OSTC3_GF_LOW, gfLow);
	READ_SETTING(OSTC3_PPO2_MIN, ppO2Min);
	READ_SETTING(OSTC3_PPO2_MAX, ppO2Max);
	READ_SETTING(OSTC3_FUTURE_TTS, futureTTS);
	READ_SETTING(OSTC3_CCR_MODE, ccrMode);
	READ_SETTING(OSTC3_DECO_TYPE, decoType);
	READ_SETTING(OSTC3_AGF_SELECTABLE, aGFSelectable);
	READ_SETTING(OSTC3_AGF_HIGH, aGFHigh);
	READ_SETTING(OSTC3_AGF_LOW, aGFLow);
	READ_SETTING(OSTC3_CALIBRATION_GAS_O2, calibrationGas);
	READ_SETTING(OSTC3_FLIP_SCREEN, flipScreen);
	READ_SETTING(OSTC3_LEFT_BUTTON_SENSIVITY, leftButtonSensitivity);
	READ_SETTING(OSTC3_RIGHT_BUTTON_SENSIVITY, rightButtonSensitivity);
	READ_SETTING(OSTC3_BOTTOM_GAS_CONSUMPTION, bottomGasConsumption);
	READ_SETTING(OSTC3_DECO_GAS_CONSUMPTION, decoGasConsumption);
	READ_SETTING(OSTC3_MOD_WARNING, modWarning);
	READ_SETTING(OSTC3_DYNAMIC_ASCEND_RATE, dynamicAscendRate);
	READ_SETTING(OSTC3_GRAPHICAL_SPEED_INDICATOR, graphicalSpeedIndicator);
	READ_SETTING(OSTC3_ALWAYS_SHOW_PPO2, alwaysShowppO2);
	READ_SETTING(OSTC3_SAFETY_STOP_LENGTH, safetyStopLength);
	READ_SETTING(OSTC3_SAFETY_STOP_START_DEPTH, safetyStopStartDepth);
	READ_SETTING(OSTC3_SAFETY_STOP_END_DEPTH, safetyStopEndDepth);
	READ_SETTING(OSTC3_SAFETY_STOP_RESET_DEPTH, safetyStopResetDepth);

#undef READ_SETTING

	rc = hw_ostc3_device_config_read(device, OSTC3_PRESSURE_SENSOR_OFFSET, uData, sizeof(uData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	// OSTC3 stores the pressureSensorOffset in two-complement
	deviceDetails.pressureSensorOffset = (signed char)uData[0];
	EMIT_PROGRESS();

	rc = hw_ostc3_device_config_read(device, OSTC3_TEMP_SENSOR_OFFSET, uData, sizeof(uData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	// OSTC3 stores the tempSensorOffset in two-complement
	deviceDetails.tempSensorOffset = (signed char)uData[0];
	EMIT_PROGRESS();

	//read firmware settings
	unsigned char fData[64] = { 0 };
	rc = hw_ostc3_device_version(device, fData, sizeof(fData));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	int serial = fData[0] + (fData[1] << 8);
	deviceDetails.serialNo = QString::number(serial);
	deviceDetails.firmwareVersion = QString::number(fData[2]) + "." + QString::number(fData[3]);
	QByteArray ar((char *)fData + 4, 60);
	deviceDetails.customText = ar.trimmed();
	EMIT_PROGRESS();

	return rc;
}

static dc_status_t write_ostc3_settings(dc_device_t *device, const DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	dc_status_t rc;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 55;

	//write gas values
	unsigned char gas1Data[4] = {
		deviceDetails.gas1.oxygen,
		deviceDetails.gas1.helium,
		deviceDetails.gas1.type,
		deviceDetails.gas1.depth
	};

	unsigned char gas2Data[4] = {
		deviceDetails.gas2.oxygen,
		deviceDetails.gas2.helium,
		deviceDetails.gas2.type,
		deviceDetails.gas2.depth
	};

	unsigned char gas3Data[4] = {
		deviceDetails.gas3.oxygen,
		deviceDetails.gas3.helium,
		deviceDetails.gas3.type,
		deviceDetails.gas3.depth
	};

	unsigned char gas4Data[4] = {
		deviceDetails.gas4.oxygen,
		deviceDetails.gas4.helium,
		deviceDetails.gas4.type,
		deviceDetails.gas4.depth
	};

	unsigned char gas5Data[4] = {
		deviceDetails.gas5.oxygen,
		deviceDetails.gas5.helium,
		deviceDetails.gas5.type,
		deviceDetails.gas5.depth
	};
	//gas 1
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS1, gas1Data, sizeof(gas1Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 2
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS2, gas2Data, sizeof(gas2Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 3
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS3, gas3Data, sizeof(gas3Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 4
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS4, gas4Data, sizeof(gas4Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//gas 5
	rc = hw_ostc3_device_config_write(device, OSTC3_GAS5, gas5Data, sizeof(gas5Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//write setpoint values
	unsigned char sp1Data[2] = {
		deviceDetails.sp1.sp,
		deviceDetails.sp1.depth
	};

	unsigned char sp2Data[2] = {
		deviceDetails.sp2.sp,
		deviceDetails.sp2.depth
	};

	unsigned char sp3Data[2] = {
		deviceDetails.sp3.sp,
		deviceDetails.sp3.depth
	};

	unsigned char sp4Data[2] = {
		deviceDetails.sp4.sp,
		deviceDetails.sp4.depth
	};

	unsigned char sp5Data[2] = {
		deviceDetails.sp5.sp,
		deviceDetails.sp5.depth
	};

	//sp 1
	rc = hw_ostc3_device_config_write(device, OSTC3_SP1, sp1Data, sizeof(sp1Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 2
	rc = hw_ostc3_device_config_write(device, OSTC3_SP2, sp2Data, sizeof(sp2Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 3
	rc = hw_ostc3_device_config_write(device, OSTC3_SP3, sp3Data, sizeof(sp3Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 4
	rc = hw_ostc3_device_config_write(device, OSTC3_SP4, sp4Data, sizeof(sp4Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//sp 5
	rc = hw_ostc3_device_config_write(device, OSTC3_SP5, sp5Data, sizeof(sp5Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//write dil values
	unsigned char dil1Data[4] = {
		deviceDetails.dil1.oxygen,
		deviceDetails.dil1.helium,
		deviceDetails.dil1.type,
		deviceDetails.dil1.depth
	};

	unsigned char dil2Data[4] = {
		deviceDetails.dil2.oxygen,
		deviceDetails.dil2.helium,
		deviceDetails.dil2.type,
		deviceDetails.dil2.depth
	};

	unsigned char dil3Data[4] = {
		deviceDetails.dil3.oxygen,
		deviceDetails.dil3.helium,
		deviceDetails.dil3.type,
		deviceDetails.dil3.depth
	};

	unsigned char dil4Data[4] = {
		deviceDetails.dil4.oxygen,
		deviceDetails.dil4.helium,
		deviceDetails.dil4.type,
		deviceDetails.dil4.depth
	};

	unsigned char dil5Data[4] = {
		deviceDetails.dil5.oxygen,
		deviceDetails.dil5.helium,
		deviceDetails.dil5.type,
		deviceDetails.dil5.depth
	};
	//dil 1
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL1, dil1Data, sizeof(gas1Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 2
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL2, dil2Data, sizeof(dil2Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 3
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL3, dil3Data, sizeof(dil3Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 4
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL4, dil4Data, sizeof(dil4Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//dil 5
	rc = hw_ostc3_device_config_write(device, OSTC3_DIL5, dil5Data, sizeof(dil5Data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//write general settings
	//custom text
	rc = hw_ostc3_device_customtext(device, qPrintable(deviceDetails.customText));
	if (rc != DC_STATUS_SUCCESS)
		return rc;

	unsigned char data[1] = { 0 };
#define WRITE_SETTING(_OSTC3_SETTING, _DEVICE_DETAIL)                                          \
	do {                                                                                   \
		data[0] = deviceDetails._DEVICE_DETAIL;                                        \
		rc = hw_ostc3_device_config_write(device, _OSTC3_SETTING, data, sizeof(data)); \
		if (rc != DC_STATUS_SUCCESS)                                                   \
			return rc;                                                             \
		EMIT_PROGRESS();                                                               \
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
	WRITE_SETTING(OSTC3_AGF_LOW, aGFLow);
	WRITE_SETTING(OSTC3_CALIBRATION_GAS_O2, calibrationGas);
	WRITE_SETTING(OSTC3_FLIP_SCREEN, flipScreen);
	WRITE_SETTING(OSTC3_LEFT_BUTTON_SENSIVITY, leftButtonSensitivity);
	WRITE_SETTING(OSTC3_RIGHT_BUTTON_SENSIVITY, rightButtonSensitivity);
	WRITE_SETTING(OSTC3_BOTTOM_GAS_CONSUMPTION, bottomGasConsumption);
	WRITE_SETTING(OSTC3_DECO_GAS_CONSUMPTION, decoGasConsumption);
	WRITE_SETTING(OSTC3_MOD_WARNING, modWarning);
	WRITE_SETTING(OSTC3_DYNAMIC_ASCEND_RATE, dynamicAscendRate);
	WRITE_SETTING(OSTC3_GRAPHICAL_SPEED_INDICATOR, graphicalSpeedIndicator);
	WRITE_SETTING(OSTC3_ALWAYS_SHOW_PPO2, alwaysShowppO2);
	WRITE_SETTING(OSTC3_SAFETY_STOP_LENGTH, safetyStopLength);
	WRITE_SETTING(OSTC3_SAFETY_STOP_START_DEPTH, safetyStopStartDepth);
	WRITE_SETTING(OSTC3_SAFETY_STOP_END_DEPTH, safetyStopEndDepth);
	WRITE_SETTING(OSTC3_SAFETY_STOP_RESET_DEPTH, safetyStopResetDepth);

#undef WRITE_SETTING

	// OSTC3 stores the pressureSensorOffset in two-complement
	data[0] = (unsigned char)deviceDetails.pressureSensorOffset;
	rc = hw_ostc3_device_config_write(device, OSTC3_PRESSURE_SENSOR_OFFSET, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	// OSTC3 stores the tempSensorOffset in two-complement
	data[0] = (unsigned char)deviceDetails.tempSensorOffset;
	rc = hw_ostc3_device_config_write(device, OSTC3_TEMP_SENSOR_OFFSET, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//sync date and time
	if (deviceDetails.syncTime) {
		dc_datetime_t now;
		dc_datetime_localtime(&now, dc_datetime_now());

		rc = dc_device_timesync(device, &now);
	}
	EMIT_PROGRESS();

	return rc;
}

static dc_status_t read_ostc_settings(dc_device_t *device, DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	dc_status_t rc;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 3;

	unsigned char data[256] = {};
#ifdef DEBUG_OSTC_CF
	// open question: how should we report settings not supported back?
	unsigned char max_CF = 0;
#endif
	rc = hw_ostc_device_eeprom_read(device, 0, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	deviceDetails.serialNo = QString::number(data[1] << 8 ^ data[0]);
	deviceDetails.numberOfDives = data[3] << 8 ^ data[2];
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
	deviceDetails.salinity = data[26];
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
	switch (data[33]) {
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
	deviceDetails.gas1 = gas1;
	deviceDetails.gas2 = gas2;
	deviceDetails.gas3 = gas3;
	deviceDetails.gas4 = gas4;
	deviceDetails.gas5 = gas5;
	deviceDetails.decoType = data[34];
	//Byte36:
	//Use O2 Sensor Module in CC Modes (0= OFF, 1= ON) (Only available in old OSTC1 - unused for OSTC Mk.2/2N)
	//deviceDetails.ccrMode = data[35];
	setpoint sp1;
	sp1.sp = data[36];
	sp1.depth = 0;
	setpoint sp2;
	sp2.sp = data[37];
	sp2.depth = 0;
	setpoint sp3;
	sp3.sp = data[38];
	sp3.depth = 0;
	deviceDetails.sp1 = sp1;
	deviceDetails.sp2 = sp2;
	deviceDetails.sp3 = sp3;
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
		char *term = strchr((char *)data + 65, (int)'}');
		if (term)
			*term = 0;
		deviceDetails.customText = (const char *)data + 65;
	}
	// Byte91:
	// Dim OLED in Divemode (>0), Normal mode (=0)
	// Byte92:
	// Date format for all outputs:
	// =0: MM/DD/YY
	// =1: DD/MM/YY
	// =2: YY/MM/DD
	deviceDetails.dateFormat = data[91];
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
	gas dil1(data[97], data[98]);
	// Byte100-101:
	// Gasuent 2 Default (%O2,%He)
	// Byte102-103:
	// Gasuent 2 Current (%O2,%He)
	gas dil2(data[101], data[102]);
	// Byte104-105:
	// Gasuent 3 Default (%O2,%He)
	// Byte106-107:
	// Gasuent 3 Current (%O2,%He)
	gas dil3(data[105], data[106]);
	// Byte108-109:
	// Gasuent 4 Default (%O2,%He)
	// Byte110-111:
	// Gasuent 4 Current (%O2,%He)
	gas dil4(data[109], data[110]);
	// Byte112-113:
	// Gasuent 5 Default (%O2,%He)
	// Byte114-115:
	// Gasuent 5 Current (%O2,%He)
	gas dil5(data[113], data[114]);
	// Byte116:
	// First Diluent (1-5)
	switch (data[115]) {
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
	deviceDetails.dil1 = dil1;
	deviceDetails.dil2 = dil2;
	deviceDetails.dil3 = dil3;
	deviceDetails.dil4 = dil4;
	deviceDetails.dil5 = dil5;
	// Byte117-128:
	// not used/reserved
	// Byte129-256:
	// 32 custom Functions (CF0-CF31)

	// Decode the relevant ones
	// CF11: Factor for saturation processes
	deviceDetails.saturation = read_ostc_cf(data, 11);
	// CF12: Factor for desaturation processes
	deviceDetails.desaturation = read_ostc_cf(data, 12);
	// CF17: Lower threshold for ppO2 warning
	deviceDetails.ppO2Min = read_ostc_cf(data, 17);
	// CF18: Upper threshold for ppO2 warning
	deviceDetails.ppO2Max = read_ostc_cf(data, 18);
	// CF20: Depth sampling rate for Profile storage
	deviceDetails.samplingRate = read_ostc_cf(data, 20);
	// CF29: Depth of last decompression stop
	deviceDetails.lastDeco = read_ostc_cf(data, 29);

#ifdef DEBUG_OSTC_CF
	for (int cf = 0; cf <= 31 && cf <= max_CF; cf++)
		printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif

	rc = hw_ostc_device_eeprom_read(device, 1, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	// Byte1:
	// Logbook version indicator (Not writable!)
	// Byte2-3:
	// Last Firmware installed, 1st Byte.2nd Byte (e.g. „1.90“) (Not writable!)
	deviceDetails.firmwareVersion = QString::number(data[1]) + "." + QString::number(data[2]);
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
	deviceDetails.gfLow = read_ostc_cf(data, 32);
	// CF33: Gradient Factor high
	deviceDetails.gfHigh = read_ostc_cf(data, 33);
	// CF56: Bottom gas consumption
	deviceDetails.bottomGasConsumption = read_ostc_cf(data, 56);
	// CF57: Ascent gas consumption
	deviceDetails.decoGasConsumption = read_ostc_cf(data, 57);
	// CF58: Future time to surface setFutureTTS
	deviceDetails.futureTTS = read_ostc_cf(data, 58);

#ifdef DEBUG_OSTC_CF
	for (int cf = 32; cf <= 63 && cf <= max_CF; cf++)
		printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif

	rc = hw_ostc_device_eeprom_read(device, 2, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	// Byte1-4:
	// not used/reserved (Not writable!)
	// Byte5-128:
	// not used/reserved
	// Byte129-256:
	// 32 custom Functions (CF 64-95)

	// Decode the relevant ones
	// CF60: Graphic velocity
	deviceDetails.graphicalSpeedIndicator = read_ostc_cf(data, 60);
	// CF65: Show safety stop
	deviceDetails.safetyStop = read_ostc_cf(data, 65);
	// CF67: Alternaitve Gradient Factor low
	deviceDetails.aGFLow = read_ostc_cf(data, 67);
	// CF68: Alternative Gradient Factor high
	deviceDetails.aGFHigh = read_ostc_cf(data, 68);
	// CF69: Allow Gradient Factor change
	deviceDetails.aGFSelectable = read_ostc_cf(data, 69);
	// CF70: Safety Stop Duration [s]
	deviceDetails.safetyStopLength = read_ostc_cf(data, 70);
	// CF71: Safety Stop Start Depth [m]
	deviceDetails.safetyStopStartDepth = read_ostc_cf(data, 71);
	// CF72: Safety Stop End Depth [m]
	deviceDetails.safetyStopEndDepth = read_ostc_cf(data, 72);
	// CF73: Safety Stop Reset Depth [m]
	deviceDetails.safetyStopResetDepth = read_ostc_cf(data, 73);
	// CF74: Battery Timeout [min]

#ifdef DEBUG_OSTC_CF
	for (int cf = 64; cf <= 95 && cf <= max_CF; cf++)
		printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif

	return rc;
}

static dc_status_t write_ostc_settings(dc_device_t *device, const DeviceDetails &deviceDetails, dc_event_callback_t progress_cb, void *userdata)
{
	dc_status_t rc;
	dc_event_progress_t progress;
	progress.current = 0;
	progress.maximum = 7;
	unsigned char data[256] = {};
	unsigned char max_CF = 0;

	// Because we write whole memory blocks, we read all the current
	// values out and then change then ones we should change.
	rc = hw_ostc_device_eeprom_read(device, 0, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	//Byte5-6:
	//Gas 1 default (%O2=21, %He=0)
	gas gas1 = deviceDetails.gas1;
	data[6] = gas1.oxygen;
	data[7] = gas1.helium;
	//Byte9-10:
	//Gas 2 default (%O2=21, %He=0)
	gas gas2 = deviceDetails.gas2;
	data[10] = gas2.oxygen;
	data[11] = gas2.helium;
	//Byte13-14:
	//Gas 3 default (%O2=21, %He=0)
	gas gas3 = deviceDetails.gas3;
	data[14] = gas3.oxygen;
	data[15] = gas3.helium;
	//Byte17-18:
	//Gas 4 default (%O2=21, %He=0)
	gas gas4 = deviceDetails.gas4;
	data[18] = gas4.oxygen;
	data[19] = gas4.helium;
	//Byte21-22:
	//Gas 5 default (%O2=21, %He=0)
	gas gas5 = deviceDetails.gas5;
	data[22] = gas5.oxygen;
	data[23] = gas5.helium;
	//Byte25-26:
	//Gas 6 current (%O2, %He)
	data[26] = deviceDetails.salinity;
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
	else
		// odd: No gas was First?
		// Set gas 1 to first
		data[33] = 1;

	data[34] = deviceDetails.decoType;
	//Byte36:
	//Use O2 Sensor Module in CC Modes (0= OFF, 1= ON) (Only available in old OSTC1 - unused for OSTC Mk.2/2N)
	//deviceDetails.ccrMode = data[35];
	data[36] = deviceDetails.sp1.sp;
	data[37] = deviceDetails.sp2.sp;
	data[38] = deviceDetails.sp3.sp;
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
	// Example: "OSTC Dive Computer}" (19 Characters incl. "}") Bytes 85-90 will be ignored.
	if (deviceDetails.customText == "") {
		data[64] = 0;
	} else {
		data[64] = 1;
		// Copy the string to the right place in the memory, padded with 0x20 (" ")
		strncpy((char *)data + 65, qPrintable(QString("%1").arg(deviceDetails.customText, -23, QChar(' '))), 23);
		// And terminate the string.
		if (deviceDetails.customText.length() <= 23)
			data[65 + deviceDetails.customText.length()] = '}';
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
	data[91] = deviceDetails.dateFormat;
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
	gas dil1 = deviceDetails.dil1;
	data[97] = dil1.oxygen;
	data[98] = dil1.helium;
	// Byte100-101:
	// Gasuent 2 Default (%O2,%He)
	// Byte102-103:
	// Gasuent 2 Current (%O2,%He)
	gas dil2 = deviceDetails.dil2;
	data[101] = dil2.oxygen;
	data[102] = dil2.helium;
	// Byte104-105:
	// Gasuent 3 Default (%O2,%He)
	// Byte106-107:
	// Gasuent 3 Current (%O2,%He)
	gas dil3 = deviceDetails.dil3;
	data[105] = dil3.oxygen;
	data[106] = dil3.helium;
	// Byte108-109:
	// Gasuent 4 Default (%O2,%He)
	// Byte110-111:
	// Gasuent 4 Current (%O2,%He)
	gas dil4 = deviceDetails.dil4;
	data[109] = dil4.oxygen;
	data[110] = dil4.helium;
	// Byte112-113:
	// Gasuent 5 Default (%O2,%He)
	// Byte114-115:
	// Gasuent 5 Current (%O2,%He)
	gas dil5 = deviceDetails.dil5;
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
	else
		// odd: No first diluent?
		// Set gas 1 to fist
		data[115] = 1;

	// Byte117-128:
	// not used/reserved
	// Byte129-256:
	// 32 custom Functions (CF0-CF31)

	// Write the relevant ones
	// CF11: Factor for saturation processes
	write_ostc_cf(data, 11, max_CF, deviceDetails.saturation);
	// CF12: Factor for desaturation processes
	write_ostc_cf(data, 12, max_CF, deviceDetails.desaturation);
	// CF17: Lower threshold for ppO2 warning
	write_ostc_cf(data, 17, max_CF, deviceDetails.ppO2Min);
	// CF18: Upper threshold for ppO2 warning
	write_ostc_cf(data, 18, max_CF, deviceDetails.ppO2Max);
	// CF20: Depth sampling rate for Profile storage
	write_ostc_cf(data, 20, max_CF, deviceDetails.samplingRate);
	// CF29: Depth of last decompression stop
	write_ostc_cf(data, 29, max_CF, deviceDetails.lastDeco);

#ifdef DEBUG_OSTC_CF
	for (int cf = 0; cf <= 31 && cf <= max_CF; cf++)
		printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
	rc = hw_ostc_device_eeprom_write(device, 0, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	rc = hw_ostc_device_eeprom_read(device, 1, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
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
	write_ostc_cf(data, 32, max_CF, deviceDetails.gfLow);
	// CF33: Gradient Factor high
	write_ostc_cf(data, 33, max_CF, deviceDetails.gfHigh);
	// CF56: Bottom gas consumption
	write_ostc_cf(data, 56, max_CF, deviceDetails.bottomGasConsumption);
	// CF57: Ascent gas consumption
	write_ostc_cf(data, 57, max_CF, deviceDetails.decoGasConsumption);
	// CF58: Future time to surface setFutureTTS
	write_ostc_cf(data, 58, max_CF, deviceDetails.futureTTS);
#ifdef DEBUG_OSTC_CF
	for (int cf = 32; cf <= 63 && cf <= max_CF; cf++)
		printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
	rc = hw_ostc_device_eeprom_write(device, 1, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	rc = hw_ostc_device_eeprom_read(device, 2, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();
	// Byte1-4:
	// not used/reserved (Not writable!)
	// Byte5-128:
	// not used/reserved
	// Byte129-256:
	// 32 custom Functions (CF 64-95)

	// Decode the relevant ones
	// CF60: Graphic velocity
	write_ostc_cf(data, 60, max_CF, deviceDetails.graphicalSpeedIndicator);
	// CF65: Show safety stop
	write_ostc_cf(data, 65, max_CF, deviceDetails.safetyStop);
	// CF67: Alternaitve Gradient Factor low
	write_ostc_cf(data, 67, max_CF, deviceDetails.aGFLow);
	// CF68: Alternative Gradient Factor high
	write_ostc_cf(data, 68, max_CF, deviceDetails.aGFHigh);
	// CF69: Allow Gradient Factor change
	write_ostc_cf(data, 69, max_CF, deviceDetails.aGFSelectable);
	// CF70: Safety Stop Duration [s]
	write_ostc_cf(data, 70, max_CF, deviceDetails.safetyStopLength);
	// CF71: Safety Stop Start Depth [m]
	write_ostc_cf(data, 71, max_CF, deviceDetails.safetyStopStartDepth);
	// CF72: Safety Stop End Depth [m]
	write_ostc_cf(data, 72, max_CF, deviceDetails.safetyStopEndDepth);
	// CF73: Safety Stop Reset Depth [m]
	write_ostc_cf(data, 73, max_CF, deviceDetails.safetyStopResetDepth);
	// CF74: Battery Timeout [min]

#ifdef DEBUG_OSTC_CF
	for (int cf = 64; cf <= 95 && cf <= max_CF; cf++)
		printf("CF %d: %d\n", cf, read_ostc_cf(data, cf));
#endif
	rc = hw_ostc_device_eeprom_write(device, 2, data, sizeof(data));
	if (rc != DC_STATUS_SUCCESS)
		return rc;
	EMIT_PROGRESS();

	//sync date and time
	if (deviceDetails.syncTime) {
		QDateTime timeToSet = QDateTime::currentDateTime();
		dc_datetime_t time = { 0 };
		time.year = timeToSet.date().year();
		time.month = timeToSet.date().month();
		time.day = timeToSet.date().day();
		time.hour = timeToSet.time().hour();
		time.minute = timeToSet.time().minute();
		time.second = timeToSet.time().second();
		time.timezone = DC_TIMEZONE_NONE;
		rc = dc_device_timesync(device, &time);
	}
	EMIT_PROGRESS();
	return rc;
}

#undef EMIT_PROGRESS

DeviceThread::DeviceThread(QObject *parent, device_data_t *data) : QThread(parent), m_data(data)
{
}

void DeviceThread::progressCB(int percent)
{
	emit progress(percent);
}

void DeviceThread::event_cb(dc_device_t *, dc_event_type_t event, const void *data, void *userdata)
{
	const dc_event_progress_t *progress = (dc_event_progress_t *) data;
	DeviceThread *dt = static_cast<DeviceThread*>(userdata);

	switch (event) {
	case DC_EVENT_PROGRESS:
		dt->progressCB(lrint(100.0 * (double)progress->current / (double)progress->maximum));
		break;
	default:
		emit dt->error("Unexpected event recived");
		break;
	}
}

ReadSettingsThread::ReadSettingsThread(QObject *parent, device_data_t *data) : DeviceThread(parent, data)
{
}

void ReadSettingsThread::run()
{
	dc_status_t rc;

	DeviceDetails deviceDetails;
	switch (dc_device_get_type(m_data->device)) {
	case DC_FAMILY_SUUNTO_VYPER:
		rc = read_suunto_vyper_settings(m_data->device, deviceDetails, DeviceThread::event_cb, this);
		if (rc == DC_STATUS_SUCCESS) {
			emit devicedetails(std::move(deviceDetails));
		} else if (rc == DC_STATUS_UNSUPPORTED) {
			emit error(tr("This feature is not yet available for the selected dive computer."));
		} else {
			emit error(tr("Failed!"));
		}
		break;
	case DC_FAMILY_HW_OSTC3:
		rc = read_ostc3_settings(m_data->device, deviceDetails, DeviceThread::event_cb, this);
		if (rc == DC_STATUS_SUCCESS)
			emit devicedetails(std::move(deviceDetails));
		else
			emit error(tr("Failed!"));
		break;

#ifdef DEBUG_OSTC
	case DC_FAMILY_NULL:
#endif
	case DC_FAMILY_HW_OSTC:
		rc = read_ostc_settings(m_data->device, deviceDetails, DeviceThread::event_cb, this);
		if (rc == DC_STATUS_SUCCESS)
			emit devicedetails(std::move(deviceDetails));
		else
			emit error(tr("Failed!"));
		break;
	default:
		emit error(tr("This feature is not yet available for the selected dive computer."));
		break;
	}
}

WriteSettingsThread::WriteSettingsThread(QObject *parent, device_data_t *data) :
	DeviceThread(parent, data)
{
}

void WriteSettingsThread::setDeviceDetails(const DeviceDetails &details)
{
	m_deviceDetails = details;
}

void WriteSettingsThread::run()
{
	dc_status_t rc;

	switch (dc_device_get_type(m_data->device)) {
	case DC_FAMILY_SUUNTO_VYPER:
		rc = write_suunto_vyper_settings(m_data->device, m_deviceDetails, DeviceThread::event_cb, this);
		if (rc == DC_STATUS_UNSUPPORTED) {
			emit error(tr("This feature is not yet available for the selected dive computer."));
		} else if (rc != DC_STATUS_SUCCESS) {
			emit error(tr("Failed!"));
		}
		break;
	case DC_FAMILY_HW_OSTC3:
		// Is this the best way?
		if (m_deviceDetails.model == "OSTC 4")
			rc = write_ostc4_settings(m_data->device, m_deviceDetails, DeviceThread::event_cb, this);
		else
			rc = write_ostc3_settings(m_data->device, m_deviceDetails, DeviceThread::event_cb, this);
		if (rc != DC_STATUS_SUCCESS)
			emit error(tr("Failed!"));
		break;
#ifdef DEBUG_OSTC
	case DC_FAMILY_NULL:
#endif
	case DC_FAMILY_HW_OSTC:
		rc = write_ostc_settings(m_data->device, m_deviceDetails, DeviceThread::event_cb, this);
		if (rc != DC_STATUS_SUCCESS)
			emit error(tr("Failed!"));
		break;
	default:
		emit error(tr("This feature is not yet available for the selected dive computer."));
		break;
	}
}


FirmwareUpdateThread::FirmwareUpdateThread(QObject *parent, device_data_t *data, QString fileName, bool forceUpdate) : DeviceThread(parent, data), m_fileName(fileName), m_forceUpdate(forceUpdate)
{
}

void FirmwareUpdateThread::run()
{
	dc_status_t rc;

	rc = dc_device_set_events(m_data->device, DC_EVENT_PROGRESS, DeviceThread::event_cb, this);
	if (rc != DC_STATUS_SUCCESS) {
		emit error("Error registering the event handler.");
		return;
	}
	switch (dc_device_get_type(m_data->device)) {
	case DC_FAMILY_HW_OSTC3:
		rc = hw_ostc3_device_fwupdate(m_data->device, qPrintable(m_fileName), m_forceUpdate);
		break;
	case DC_FAMILY_HW_OSTC:
		rc = hw_ostc_device_fwupdate(m_data->device, qPrintable(m_fileName));
		break;
	default:
		emit error(tr("This feature is not yet available for the selected dive computer."));
		return;
	}

	if (rc != DC_STATUS_SUCCESS) {
		emit error(tr("Firmware update failed!"));
	}
}


ResetSettingsThread::ResetSettingsThread(QObject *parent, device_data_t *data) : DeviceThread(parent, data)
{
}

void ResetSettingsThread::run()
{
	dc_status_t rc = DC_STATUS_SUCCESS;

	if (dc_device_get_type(m_data->device) == DC_FAMILY_HW_OSTC3) {
		rc = hw_ostc3_device_config_reset(m_data->device);
		emit progress(100);
	}
	if (rc != DC_STATUS_SUCCESS) {
		emit error(tr("Reset settings failed!"));
	}
}
