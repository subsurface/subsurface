#ifndef CONFIGUREDIVECOMPUTERTHREADS_H
#define CONFIGUREDIVECOMPUTERTHREADS_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include <QDateTime>
#include "devicedetails.h"

class ReadSettingsThread : public QThread {
	Q_OBJECT
public:
	ReadSettingsThread(QObject *parent, DeviceDetails *deviceDetails, device_data_t *data);
	virtual void run();
	QString result;
	QString lastError;
signals:
	void error(QString err);
private:
	DeviceDetails *m_deviceDetails;
	device_data_t *m_data;
};

class WriteSettingsThread : public QThread {
	Q_OBJECT
public:
	WriteSettingsThread(QObject *parent, DeviceDetails *deviceDetails, QString settingName, QVariant settingValue);
	virtual void run();
	QString result;
	QString lastError;
signals:
	void error(QString err);
private:
	device_data_t *data;
	QString m_settingName;
	QVariant m_settingValue;

	DeviceDetails *m_deviceDetails;
};

#endif // CONFIGUREDIVECOMPUTERTHREADS_H
