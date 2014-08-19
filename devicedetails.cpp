#include "devicedetails.h"

DeviceDetails::DeviceDetails(QObject *parent) :
	QObject(parent)
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

int DeviceDetails::ccrMode() const
{
	return m_ccrMode;
}

void DeviceDetails::setCcrMode(int ccrMode)
{
	m_ccrMode = ccrMode;
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

int DeviceDetails::pp02Max() const
{
	return m_pp02Max;
}

void DeviceDetails::setPp02Max(int pp02Max)
{
	m_pp02Max = pp02Max;
}

int DeviceDetails::pp02Min() const
{
	return m_pp02Min;
}

void DeviceDetails::setPp02Min(int pp02Min)
{
	m_pp02Min = pp02Min;
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


















































