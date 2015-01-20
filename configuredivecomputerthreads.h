#ifndef CONFIGUREDIVECOMPUTERTHREADS_H
#define CONFIGUREDIVECOMPUTERTHREADS_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include <QDateTime>
#include "devicedetails.h"

class DeviceThread : public QThread {
	Q_OBJECT
public:
	DeviceThread(QObject *parent, device_data_t *data);
	virtual void run() = 0;
	QString result;
	QString lastError;
signals:
	void error(QString err);
	void message(QString msg);
protected:
	device_data_t *m_data;
};

class ReadSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	ReadSettingsThread(QObject *parent, device_data_t *data);
	void run();
signals:
	void devicedetails(DeviceDetails *newDeviceDetails);
};

class WriteSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	WriteSettingsThread(QObject *parent, device_data_t *data);
	void setDeviceDetails(DeviceDetails *details);
	void run();

private:
	DeviceDetails *m_deviceDetails;
};

class FirmwareUpdateThread : public DeviceThread {
	Q_OBJECT
public:
	FirmwareUpdateThread(QObject *parent, device_data_t *data, QString fileName);
	void run();

private:
	QString m_fileName;
};

class ResetSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	ResetSettingsThread(QObject *parent, device_data_t *data);
	void run();
};

#endif // CONFIGUREDIVECOMPUTERTHREADS_H
