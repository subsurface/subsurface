#ifndef DEVICEDETAILS_H
#define DEVICEDETAILS_H

#include <QObject>
#include <QDateTime>
#include "libdivecomputer.h"

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

private:
	device_data_t *m_data;
	QString m_serialNo;
	QString m_firmwareVersion;
	QString m_customText;
	int m_brightness;
	int m_diveModeColor;
	int m_language;
	int m_dateFormat;
	int m_lastDeco;
	bool m_syncTime;
};


#endif // DEVICEDETAILS_H
