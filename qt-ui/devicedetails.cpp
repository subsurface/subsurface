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
