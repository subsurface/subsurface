// SPDX-License-Identifier: GPL-2.0
#include "devicedetails.h"

gas::gas(unsigned char oxygen, unsigned char helium, unsigned char type, unsigned char depth) :
	oxygen(oxygen), helium(helium), type(type), depth(depth)
{
}

setpoint::setpoint(unsigned char sp, unsigned char depth) :
	sp(sp), depth(depth)
{
}

std::array<QByteArray, 0x35> DeviceDetails::ostc4LegacyEdits() const
{
	std::array<QByteArray, 0x35> desired = ostc4Snapshot.displayRecords;
	for (size_t n = 0; n < Ostc4SettingsTransaction::ids.size(); ++n)
		desired[Ostc4SettingsTransaction::ids[n] - 0x10] = ostc4Snapshot.records[n].original;
	const gas gases[] = { gas1, gas2, gas3, gas4, gas5, dil1, dil2, dil3, dil4, dil5 };
	for (int n = 0; n < 10; ++n) {
		QByteArray &raw = desired[n];
		if (raw.size() != 4 || !ostc4Editable[n])
			continue;
		raw[0] = static_cast<char>(gases[n].oxygen);
		raw[1] = static_cast<char>(gases[n].helium);
		raw[2] = static_cast<char>(gases[n].type);
		raw[3] = static_cast<char>(gases[n].depth);
	}
	const setpoint points[] = { sp1, sp2, sp3, sp4, sp5 };
	for (int n = 0; n < 5; ++n) {
		QByteArray &raw = desired[0x1a + n - 0x10];
		if (raw.size() == 4 && ostc4Editable[0x1a + n - 0x10]) {
			raw[0] = static_cast<char>(points[n].sp);
			raw[1] = static_cast<char>(points[n].depth);
		}
	}
	const std::pair<int, int> scalars[] = {
		{0x1f, ccrMode}, {0x20, diveMode}, {0x21, decoType}, {0x22, ppO2Max},
		{0x23, ppO2Min}, {0x24, futureTTS}, {0x25, gfLow}, {0x26, gfHigh},
		{0x27, aGFLow}, {0x28, aGFHigh}, {0x29, vpmConservatism},
		{0x2c, lastDeco}, {0x2d, brightness}, {0x2e, units}, {0x30, salinity},
		{0x31, diveModeColor}, {0x32, language}, {0x33, dateFormat},
		{0x34, compassGain}, {0x35, pressureSensorOffset},
		{0x38, setPointFallback}, {0x39, flipScreen}, {0x3a, buttonSensitivity},
		{0x3d, decoGasConsumption}, {0x3e, travelGasConsumption},
		{0x41, alwaysShowppO2}, {0x42, tempSensorOffset},
		{0x43, safetyStopLength}, {0x44, safetyStopStartDepth}
	};
	for (const auto &field : scalars) {
		QByteArray &raw = desired[field.first - 0x10];
		if (raw.size() != 4 || !ostc4Editable[field.first - 0x10])
			continue;
		if (field.second < (field.first == 0x34 || field.first == 0x35 || field.first == 0x42 ? -128 : 0) ||
		    field.second > (field.first == 0x34 || field.first == 0x35 || field.first == 0x42 ? 127 : 255)) {
			raw = QByteArray(1, '\0'); // rejected by applyLegacy before any writes
			continue;
		}
		raw[0] = static_cast<char>(field.second);
	}
	return desired;
}

DeviceDetails::DeviceDetails() :
	setPointFallback(0),
	ccrMode(0),
	calibrationGas(0),
	diveMode(0),
	decoType(0),
	ppO2Max(0),
	ppO2Min(0),
	futureTTS(0),
	gfLow(0),
	gfHigh(0),
	aGFLow(0),
	aGFHigh(0),
	aGFSelectable(0),
	vpmConservatism(0),
	saturation(0),
	desaturation(0),
	lastDeco(0),
	brightness(0),
	units(0),
	samplingRate(0),
	salinity(0),
	diveModeColor(0),
	language(0),
	dateFormat(0),
	compassGain(0),
	pressureSensorOffset(0),
	flipScreen(0),
	safetyStop(0),
	totalTime(0),
	numberOfDives(0),
	altitude(0),
	personalSafety(0),
	timeFormat(0),
	lightEnabled(false),
	light(0),
	alarmTimeEnabled(false),
	alarmTime(0),
	alarmDepthEnabled(false),
	leftButtonSensitivity(0),
	rightButtonSensitivity(0),
	buttonSensitivity(0),
	buttonBalance(0),
	bottomGasConsumption(0),
	decoGasConsumption(0),
	travelGasConsumption(0),
	modWarning(false),
	dynamicAscendRate(false),
	graphicalSpeedIndicator(false),
	alwaysShowppO2(false),
	tempSensorOffset(0),
	safetyStopLength(0),
	safetyStopStartDepth(0),
	safetyStopEndDepth(0),
	safetyStopResetDepth(0)
{
}
