// SPDX-License-Identifier: GPL-2.0
#ifndef DEVICEDETAILS_H
#define DEVICEDETAILS_H

#include <QObject>
#include <QDateTime>
#include "libdivecomputer.h"

struct gas {
	unsigned char oxygen;
	unsigned char helium;
	unsigned char type;
	unsigned char depth;
	gas(unsigned char oxygen = 0, unsigned char helium = 0, unsigned char type = 0, unsigned char depth = 0);
};

struct setpoint {
	unsigned char sp;
	unsigned char depth;
	setpoint(unsigned char sp = 0, unsigned char depth = 0);
};

class DeviceDetails
{
public:
	DeviceDetails();

	QString serialNo;
	QString firmwareVersion;
	QString customText;
	QString model;
	bool syncTime;
	gas gas1;
	gas gas2;
	gas gas3;
	gas gas4;
	gas gas5;
	gas dil1;
	gas dil2;
	gas dil3;
	gas dil4;
	gas dil5;
	setpoint sp1;
	setpoint sp2;
	setpoint sp3;
	setpoint sp4;
	setpoint sp5;
	bool setPointFallback;
	int ccrMode;
	int calibrationGas;
	int diveMode;
	int decoType;
	int ppO2Max;
	int ppO2Min;
	int futureTTS;
	int gfLow;
	int gfHigh;
	int aGFLow;
	int aGFHigh;
	int aGFSelectable;
	int vpmConservatism;
	int saturation;
	int desaturation;
	int lastDeco;
	int brightness;
	int units;
	int samplingRate;
	int salinity;
	int diveModeColor;
	int language;
	int dateFormat;
	int compassGain;
	int pressureSensorOffset;
	bool flipScreen;
	bool safetyStop;
	int maxDepth;
	int totalTime;
	int numberOfDives;
	int altitude;
	int personalSafety;
	int timeFormat;
	bool lightEnabled;
	int light;
	bool alarmTimeEnabled;
	int alarmTime;
	bool alarmDepthEnabled;
	int alarmDepth;
	int leftButtonSensitivity;
	int rightButtonSensitivity;
	int buttonSensitivity;
	int bottomGasConsumption;
	int decoGasConsumption;
	int travelGasConsumption;
	bool modWarning;
	bool dynamicAscendRate;
	bool graphicalSpeedIndicator;
	bool alwaysShowppO2;
	int tempSensorOffset;
	unsigned safetyStopLength;
	unsigned safetyStopStartDepth;
	unsigned safetyStopEndDepth;
	unsigned safetyStopResetDepth;
};

Q_DECLARE_METATYPE(DeviceDetails);

#endif // DEVICEDETAILS_H
