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
};

struct setpoint {
	unsigned char sp;
	unsigned char depth;
};

class DeviceDetails : public QObject
{
	Q_OBJECT
public:
	explicit DeviceDetails(QObject *parent = 0);

	device_data_t *data() const;
	void setData(device_data_t *data);

	QString serialNo() const;
	void setSerialNo(const QString &serialNo);

	QString firmwareVersion() const;
	void setFirmwareVersion(const QString &firmwareVersion);

	QString customText() const;
	void setCustomText(const QString &customText);

	QString model() const;
	void setModel(const QString &model);

	int brightness() const;
	void setBrightness(int brightness);

	int diveModeColor() const;
	void setDiveModeColor(int diveModeColor);

	int language() const;
	void setLanguage(int language);

	int dateFormat() const;
	void setDateFormat(int dateFormat);

	int lastDeco() const;
	void setLastDeco(int lastDeco);

	bool syncTime() const;
	void setSyncTime(bool syncTime);

	gas gas1() const;
	void setGas1(const gas &gas1);

	gas gas2() const;
	void setGas2(const gas &gas2);

	gas gas3() const;
	void setGas3(const gas &gas3);

	gas gas4() const;
	void setGas4(const gas &gas4);

	gas gas5() const;
	void setGas5(const gas &gas5);

	gas dil1() const;
	void setDil1(const gas &dil1);

	gas dil2() const;
	void setDil2(const gas &dil2);

	gas dil3() const;
	void setDil3(const gas &dil3);

	gas dil4() const;
	void setDil4(const gas &dil4);

	gas dil5() const;
	void setDil5(const gas &dil5);

	setpoint sp1() const;
	void setSp1(const setpoint &sp1);

	setpoint sp2() const;
	void setSp2(const setpoint &sp2);

	setpoint sp3() const;
	void setSp3(const setpoint &sp3);

	setpoint sp4() const;
	void setSp4(const setpoint &sp4);

	setpoint sp5() const;
	void setSp5(const setpoint &sp5);

	bool setPointFallback() const;
	void setSetPointFallback(bool setSetPointFallback);

	int ccrMode() const;
	void setCcrMode(int ccrMode);

	int calibrationGas() const;
	void setCalibrationGas(int calibrationGas);

	int diveMode() const;
	void setDiveMode(int diveMode);

	int decoType() const;
	void setDecoType(int decoType);

	int ppO2Max() const;
	void setPpO2Max(int ppO2Max);

	int ppO2Min() const;
	void setPpO2Min(int ppO2Min);

	int futureTTS() const;
	void setFutureTTS(int futureTTS);

	int gfLow() const;
	void setGfLow(int gfLow);

	int gfHigh() const;
	void setGfHigh(int gfHigh);

	int aGFLow() const;
	void setAGFLow(int aGFLow);

	int aGFHigh() const;
	void setAGFHigh(int aGFHigh);

	int aGFSelectable() const;
	void setAGFSelectable(int aGFSelectable);

	int saturation() const;
	void setSaturation(int saturation);

	int desaturation() const;
	void setDesaturation(int desaturation);

	int units() const;
	void setUnits(int units);

	int samplingRate() const;
	void setSamplingRate(int samplingRate);

	int salinity() const;
	void setSalinity(int salinity);

	int compassGain() const;
	void setCompassGain(int compassGain);

	int pressureSensorOffset() const;
	void setPressureSensorOffset(int pressureSensorOffset);

	bool flipScreen() const;
	void setFlipScreen(bool flipScreen);

	bool safetyStop() const;
	void setSafetyStop(bool safetyStop);

	int maxDepth() const;
	void setMaxDepth(int maxDepth);

	int totalTime() const;
	void setTotalTime(int totalTime);

	int numberOfDives() const;
	void setNumberOfDives(int numberOfDives);

	int altitude() const;
	void setAltitude(int altitude);

	int personalSafety() const;
	void setPersonalSafety(int personalSafety);

	int timeFormat() const;
	void setTimeFormat(int timeFormat);

	bool lightEnabled() const;
	void setLightEnabled(bool lightEnabled);

	int light() const;
	void setLight(int light);

	bool alarmTimeEnabled() const;
	void setAlarmTimeEnabled(bool alarmTimeEnabled);

	int alarmTime() const;
	void setAlarmTime(int alarmTime);

	bool alarmDepthEnabled() const;
	void setAlarmDepthEnabled(bool alarmDepthEnabled);

	int alarmDepth() const;
	void setAlarmDepth(int alarmDepth);

private:
	device_data_t *m_data;
	QString m_serialNo;
	QString m_firmwareVersion;
	QString m_customText;
	QString m_model;
	bool m_syncTime;
	gas m_gas1;
	gas m_gas2;
	gas m_gas3;
	gas m_gas4;
	gas m_gas5;
	gas m_dil1;
	gas m_dil2;
	gas m_dil3;
	gas m_dil4;
	gas m_dil5;
	setpoint m_sp1;
	setpoint m_sp2;
	setpoint m_sp3;
	setpoint m_sp4;
	setpoint m_sp5;
	bool m_setPointFallback;
	int m_ccrMode;
	int m_calibrationGas;
	int m_diveMode;
	int m_decoType;
	int m_ppO2Max;
	int m_ppO2Min;
	int m_futureTTS;
	int m_gfLow;
	int m_gfHigh;
	int m_aGFLow;
	int m_aGFHigh;
	int m_aGFSelectable;
	int m_saturation;
	int m_desaturation;
	int m_lastDeco;
	int m_brightness;
	int m_units;
	int m_samplingRate;
	int m_salinity;
	int m_diveModeColor;
	int m_language;
	int m_dateFormat;
	int m_compassGain;
	int m_pressureSensorOffset;
	bool m_flipScreen;
	bool m_safetyStop;
	int m_maxDepth;
	int m_totalTime;
	int m_numberOfDives;
	int m_altitude;
	int m_personalSafety;
	int m_timeFormat;
	bool m_lightEnabled;
	int m_light;
	bool m_alarmTimeEnabled;
	int m_alarmTime;
	bool m_alarmDepthEnabled;
	int m_alarmDepth;
};


#endif // DEVICEDETAILS_H
