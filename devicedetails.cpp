#include "devicedetails.h"

// This can probably be done better by someone with better c++-FU
const struct gas zero_gas = {0};
const struct setpoint zero_setpoint = {0};

DeviceDetails::DeviceDetails(QObject *parent) :
	QObject(parent),
	m_data(0),
	m_serialNo(""),
	m_firmwareVersion(""),
	m_customText(""),
	m_model(""),
	m_syncTime(false),
	m_gas1(zero_gas),
	m_gas2(zero_gas),
	m_gas3(zero_gas),
	m_gas4(zero_gas),
	m_gas5(zero_gas),
	m_dil1(zero_gas),
	m_dil2(zero_gas),
	m_dil3(zero_gas),
	m_dil4(zero_gas),
	m_dil5(zero_gas),
	m_sp1(zero_setpoint),
	m_sp2(zero_setpoint),
	m_sp3(zero_setpoint),
	m_sp4(zero_setpoint),
	m_sp5(zero_setpoint),
	m_setPointFallback(0),
	m_ccrMode(0),
	m_calibrationGas(0),
	m_diveMode(0),
	m_decoType(0),
	m_ppO2Max(0),
	m_ppO2Min(0),
	m_futureTTS(0),
	m_gfLow(0),
	m_gfHigh(0),
	m_aGFLow(0),
	m_aGFHigh(0),
	m_aGFSelectable(0),
	m_saturation(0),
	m_desaturation(0),
	m_lastDeco(0),
	m_brightness(0),
	m_units(0),
	m_samplingRate(0),
	m_salinity(0),
	m_diveModeColor(0),
	m_language(0),
	m_dateFormat(0),
	m_compassGain(0),
	m_pressureSensorOffset(0),
	m_flipScreen(0),
	m_safetyStop(0),
	m_maxDepth(0),
	m_totalTime(0),
	m_numberOfDives(0),
	m_altitude(0),
	m_personalSafety(0),
	m_timeFormat(0),
	m_lightEnabled(false),
	m_light(0),
	m_alarmTimeEnabled(false),
	m_alarmTime(0),
	m_alarmDepthEnabled(false),
	m_alarmDepth(0)
{
}

device_data_t *DeviceDetails::data() const
{
	return m_data;
}

void DeviceDetails::setData(device_data_t *data)
{
	m_data = data;
}

QString DeviceDetails::serialNo() const
{
	return m_serialNo;
}

void DeviceDetails::setSerialNo(const QString &serialNo)
{
	m_serialNo = serialNo;
}

QString DeviceDetails::firmwareVersion() const
{
	return m_firmwareVersion;
}

void DeviceDetails::setFirmwareVersion(const QString &firmwareVersion)
{
	m_firmwareVersion = firmwareVersion;
}

QString DeviceDetails::customText() const
{
	return m_customText;
}

void DeviceDetails::setCustomText(const QString &customText)
{
	m_customText = customText;
}

QString DeviceDetails::model() const
{
	return m_model;
}

void DeviceDetails::setModel(const QString &model)
{
	m_model = model;
}

int DeviceDetails::brightness() const
{
	return m_brightness;
}

void DeviceDetails::setBrightness(int brightness)
{
	m_brightness = brightness;
}

int DeviceDetails::diveModeColor() const
{
	return m_diveModeColor;
}

void DeviceDetails::setDiveModeColor(int diveModeColor)
{
	m_diveModeColor = diveModeColor;
}

int DeviceDetails::language() const
{
	return m_language;
}

void DeviceDetails::setLanguage(int language)
{
	m_language = language;
}

int DeviceDetails::dateFormat() const
{
	return m_dateFormat;
}

void DeviceDetails::setDateFormat(int dateFormat)
{
	m_dateFormat = dateFormat;
}

int DeviceDetails::lastDeco() const
{
	return m_lastDeco;
}

void DeviceDetails::setLastDeco(int lastDeco)
{
	m_lastDeco = lastDeco;
}

bool DeviceDetails::syncTime() const
{
	return m_syncTime;
}

void DeviceDetails::setSyncTime(bool syncTime)
{
	m_syncTime = syncTime;
}

gas DeviceDetails::gas1() const
{
	return m_gas1;
}

void DeviceDetails::setGas1(const gas &gas1)
{
	m_gas1 = gas1;
}

gas DeviceDetails::gas2() const
{
	return m_gas2;
}

void DeviceDetails::setGas2(const gas &gas2)
{
	m_gas2 = gas2;
}

gas DeviceDetails::gas3() const
{
	return m_gas3;
}

void DeviceDetails::setGas3(const gas &gas3)
{
	m_gas3 = gas3;
}

gas DeviceDetails::gas4() const
{
	return m_gas4;
}

void DeviceDetails::setGas4(const gas &gas4)
{
	m_gas4 = gas4;
}

gas DeviceDetails::gas5() const
{
	return m_gas5;
}

void DeviceDetails::setGas5(const gas &gas5)
{
	m_gas5 = gas5;
}

gas DeviceDetails::dil1() const
{
	return m_dil1;
}

void DeviceDetails::setDil1(const gas &dil1)
{
	m_dil1 = dil1;
}

gas DeviceDetails::dil2() const
{
	return m_dil2;
}

void DeviceDetails::setDil2(const gas &dil2)
{
	m_dil2 = dil2;
}

gas DeviceDetails::dil3() const
{
	return m_dil3;
}

void DeviceDetails::setDil3(const gas &dil3)
{
	m_dil3 = dil3;
}

gas DeviceDetails::dil4() const
{
	return m_dil4;
}

void DeviceDetails::setDil4(const gas &dil4)
{
	m_dil4 = dil4;
}

gas DeviceDetails::dil5() const
{
	return m_dil5;
}

void DeviceDetails::setDil5(const gas &dil5)
{
	m_dil5 = dil5;
}

setpoint DeviceDetails::sp1() const
{
	return m_sp1;
}

void DeviceDetails::setSp1(const setpoint &sp1)
{
	m_sp1 = sp1;
}

setpoint DeviceDetails::sp2() const
{
	return m_sp2;
}

void DeviceDetails::setSp2(const setpoint &sp2)
{
	m_sp2 = sp2;
}

setpoint DeviceDetails::sp3() const
{
	return m_sp3;
}

void DeviceDetails::setSp3(const setpoint &sp3)
{
	m_sp3 = sp3;
}

setpoint DeviceDetails::sp4() const
{
	return m_sp4;
}

void DeviceDetails::setSp4(const setpoint &sp4)
{
	m_sp4 = sp4;
}

setpoint DeviceDetails::sp5() const
{
	return m_sp5;
}

void DeviceDetails::setSp5(const setpoint &sp5)
{
	m_sp5 = sp5;
}

bool DeviceDetails::setPointFallback() const
{
	return m_setPointFallback;
}

void DeviceDetails::setSetPointFallback(bool setSetPointFallback)
{
	m_setPointFallback = setSetPointFallback;
}

int DeviceDetails::ccrMode() const
{
	return m_ccrMode;
}

void DeviceDetails::setCcrMode(int ccrMode)
{
	m_ccrMode = ccrMode;
}

int DeviceDetails::calibrationGas() const
{
	return m_calibrationGas;
}

void DeviceDetails::setCalibrationGas(int calibrationGas)
{
	m_calibrationGas = calibrationGas;
}

int DeviceDetails::diveMode() const
{
	return m_diveMode;
}

void DeviceDetails::setDiveMode(int diveMode)
{
	m_diveMode = diveMode;
}

int DeviceDetails::decoType() const
{
	return m_decoType;
}

void DeviceDetails::setDecoType(int decoType)
{
	m_decoType = decoType;
}

int DeviceDetails::ppO2Max() const
{
	return m_ppO2Max;
}

void DeviceDetails::setPpO2Max(int ppO2Max)
{
	m_ppO2Max = ppO2Max;
}

int DeviceDetails::ppO2Min() const
{
	return m_ppO2Min;
}

void DeviceDetails::setPpO2Min(int ppO2Min)
{
	m_ppO2Min = ppO2Min;
}

int DeviceDetails::futureTTS() const
{
	return m_futureTTS;
}

void DeviceDetails::setFutureTTS(int futureTTS)
{
	m_futureTTS = futureTTS;
}

int DeviceDetails::gfLow() const
{
	return m_gfLow;
}

void DeviceDetails::setGfLow(int gfLow)
{
	m_gfLow = gfLow;
}

int DeviceDetails::gfHigh() const
{
	return m_gfHigh;
}

void DeviceDetails::setGfHigh(int gfHigh)
{
	m_gfHigh = gfHigh;
}

int DeviceDetails::aGFLow() const
{
	return m_aGFLow;
}

void DeviceDetails::setAGFLow(int aGFLow)
{
	m_aGFLow = aGFLow;
}

int DeviceDetails::aGFHigh() const
{
	return m_aGFHigh;
}

void DeviceDetails::setAGFHigh(int aGFHigh)
{
	m_aGFHigh = aGFHigh;
}

int DeviceDetails::aGFSelectable() const
{
	return m_aGFSelectable;
}

void DeviceDetails::setAGFSelectable(int aGFSelectable)
{
	m_aGFSelectable = aGFSelectable;
}

int DeviceDetails::saturation() const
{
	return m_saturation;
}

void DeviceDetails::setSaturation(int saturation)
{
	m_saturation = saturation;
}

int DeviceDetails::desaturation() const
{
	return m_desaturation;
}

void DeviceDetails::setDesaturation(int desaturation)
{
	m_desaturation = desaturation;
}

int DeviceDetails::units() const
{
	return m_units;
}

void DeviceDetails::setUnits(int units)
{
	m_units = units;
}

int DeviceDetails::samplingRate() const
{
	return m_samplingRate;
}

void DeviceDetails::setSamplingRate(int samplingRate)
{
	m_samplingRate = samplingRate;
}

int DeviceDetails::salinity() const
{
	return m_salinity;
}

void DeviceDetails::setSalinity(int salinity)
{
	m_salinity = salinity;
}

int DeviceDetails::compassGain() const
{
	return m_compassGain;
}

void DeviceDetails::setCompassGain(int compassGain)
{
	m_compassGain = compassGain;
}

int DeviceDetails::pressureSensorOffset() const
{
	return m_pressureSensorOffset;
}

void DeviceDetails::setPressureSensorOffset(int pressureSensorOffset)
{
	m_pressureSensorOffset = pressureSensorOffset;
}

bool DeviceDetails::flipScreen() const
{
	return m_flipScreen;
}

void DeviceDetails::setFlipScreen(bool flipScreen)
{
	m_flipScreen = flipScreen;
}

bool DeviceDetails::safetyStop() const
{
	return m_safetyStop;
}

void DeviceDetails::setSafetyStop(bool safetyStop)
{
	m_safetyStop = safetyStop;
}

int DeviceDetails::maxDepth() const
{
	return m_maxDepth;
}

void DeviceDetails::setMaxDepth(int maxDepth)
{
	m_maxDepth = maxDepth;
}

int DeviceDetails::totalTime() const
{
	return m_totalTime;
}

void DeviceDetails::setTotalTime(int totalTime)
{
	m_totalTime = totalTime;
}

int DeviceDetails::numberOfDives() const
{
	return m_numberOfDives;
}

void DeviceDetails::setNumberOfDives(int numberOfDives)
{
	m_numberOfDives = numberOfDives;
}

int DeviceDetails::altitude() const
{
	return m_altitude;
}

void DeviceDetails::setAltitude(int altitude)
{
	m_altitude = altitude;
}

int DeviceDetails::personalSafety() const
{
	return m_personalSafety;
}

void DeviceDetails::setPersonalSafety(int personalSafety)
{
	m_personalSafety = personalSafety;
}

int DeviceDetails::timeFormat() const
{
	return m_timeFormat;
}

void DeviceDetails::setTimeFormat(int timeFormat)
{
	m_timeFormat = timeFormat;
}

bool DeviceDetails::lightEnabled() const
{
	return m_lightEnabled;
}

void DeviceDetails::setLightEnabled(bool lightEnabled)
{
	m_lightEnabled = lightEnabled;
}

int DeviceDetails::light() const
{
	return m_light;
}

void DeviceDetails::setLight(int light)
{
	m_light = light;
}

bool DeviceDetails::alarmTimeEnabled() const
{
	return m_alarmTimeEnabled;
}

void DeviceDetails::setAlarmTimeEnabled(bool alarmTimeEnabled)
{
	m_alarmTimeEnabled = alarmTimeEnabled;
}

int DeviceDetails::alarmTime() const
{
	return m_alarmTime;
}

void DeviceDetails::setAlarmTime(int alarmTime)
{
	m_alarmTime = alarmTime;
}

bool DeviceDetails::alarmDepthEnabled() const
{
	return m_alarmDepthEnabled;
}

void DeviceDetails::setAlarmDepthEnabled(bool alarmDepthEnabled)
{
	m_alarmDepthEnabled = alarmDepthEnabled;
}

int DeviceDetails::alarmDepth() const
{
	return m_alarmDepth;
}

void DeviceDetails::setAlarmDepth(int alarmDepth)
{
	m_alarmDepth = alarmDepth;
}
