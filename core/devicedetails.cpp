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

DeviceDetails::DeviceDetails() :
	syncTime(false),
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
	maxDepth(0),
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
	alarmDepth(0),
	leftButtonSensitivity(0),
	rightButtonSensitivity(0),
	buttonSensitivity(0),
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
