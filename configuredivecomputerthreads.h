#ifndef CONFIGUREDIVECOMPUTERTHREADS_H
#define CONFIGUREDIVECOMPUTERTHREADS_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include <QDateTime>
#include "devicedetails.h"

class ReadSettingsThread : public QThread
{
	Q_OBJECT
public:
	ReadSettingsThread(QObject *parent, device_data_t *data);
	virtual void run();
	QString result;
	QString lastError;
signals:
	void error(QString err);
	void devicedetails(DeviceDetails *newDeviceDetails);
private:
	device_data_t *m_data;
};

class WriteSettingsThread : public QThread
{
	Q_OBJECT
public:
	WriteSettingsThread(QObject *parent, device_data_t *data);
	void setDeviceDetails(DeviceDetails *details);
	virtual void run();
	QString result;
	QString lastError;
signals:
	void error(QString err);
private:
	device_data_t *m_data;
	DeviceDetails *m_deviceDetails;
};

class FirmwareUpdateThread : public QThread
{
	Q_OBJECT
public:
	FirmwareUpdateThread(QObject *parent, device_data_t *data, QString fileName);
	virtual void run();
	QString lastError;
signals:
	void progress(int percent);
	void message(QString msg);
	void error(QString err);
private:
	device_data_t *m_data;
	QString m_fileName;
};

class ResetSettingsThread : public QThread
{
	Q_OBJECT
public:
	ResetSettingsThread(QObject *parent, device_data_t *data);
	virtual void run();
	QString lastError;
signals:
	void progress(int percent);
	void message(QString msg);
	void error(QString err);
private:
	device_data_t *m_data;
};

#endif // CONFIGUREDIVECOMPUTERTHREADS_H
