// SPDX-License-Identifier: GPL-2.0
#ifndef CONFIGUREDIVECOMPUTERTHREADS_H
#define CONFIGUREDIVECOMPUTERTHREADS_H

#include <QObject>
#include <QThread>
#include "libdivecomputer.h"
#include "devicedetails.h"

class DeviceThread : public QThread {
	Q_OBJECT
public:
	DeviceThread(QObject *parent, device_data_t *data);
	virtual void run() = 0;
signals:
	void error(QString err);
	void progress(int value);
protected:
	device_data_t *m_data;
	void progressCB(int value);
	static void event_cb(dc_device_t *device, dc_event_type_t event, const void *data, void *userdata);
};

class ReadSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	ReadSettingsThread(QObject *parent, device_data_t *data);
	void run();
signals:
	void devicedetails(DeviceDetails newDeviceDetails);
};

class WriteSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	WriteSettingsThread(QObject *parent, device_data_t *data);
	void setDeviceDetails(const DeviceDetails &details);
	void run();

private:
	DeviceDetails m_deviceDetails;
};

class FirmwareUpdateThread : public DeviceThread {
	Q_OBJECT
public:
	FirmwareUpdateThread(QObject *parent, device_data_t *data, QString fileName, bool forceUpdate);
	void run();

private:
	QString m_fileName;
	bool m_forceUpdate;
};

class ResetSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	ResetSettingsThread(QObject *parent, device_data_t *data);
	void run();
};

#endif // CONFIGUREDIVECOMPUTERTHREADS_H
