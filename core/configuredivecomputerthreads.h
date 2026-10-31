// SPDX-License-Identifier: GPL-2.0
#ifndef CONFIGUREDIVECOMPUTERTHREADS_H
#define CONFIGUREDIVECOMPUTERTHREADS_H

#include <QObject>
#include <QThread>
#include "libdivecomputer.h"
#include "devicedetails.h"
#include <functional>
#include <QMutex>
#include <memory>

// Replace only protocol I/O at the transaction boundary. Backup persistence,
// validation and result reporting are shared by device and simulated sessions.
struct Ostc4SettingsIO {
	Ostc4SettingsTransaction::Identify identify;
	Ostc4SettingsTransaction::Read read;
	Ostc4SettingsTransaction::Write write;
	std::function<bool(QByteArray &)> readText;
	std::function<bool(const QByteArray &)> writeText;
};

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
	void setTransport(std::function<QString(device_data_t *)> open,
		std::function<void(device_data_t *)> close);
	void setTestOperation(std::function<bool(device_data_t *)> operation) { m_testOperation = std::move(operation); }
	void setTestDetails(const DeviceDetails &details) { m_testDetails = details; }
	void setSaveIO(std::shared_ptr<const Ostc4SettingsIO> io) { m_saveIO = std::move(io); }
	void setSession(bool enabled) { m_session = enabled; }
	void save(const DeviceDetails &details, const QString &backupPath);
	void stopSession();
	void run();
signals:
	void devicedetails(DeviceDetails newDeviceDetails);
	void operationFinished(bool writing);
	void result(QString text);
	void sessionCommand();
private:
	std::function<QString(device_data_t *)> m_open;
	std::function<void(device_data_t *)> m_close;
	std::function<bool(device_data_t *)> m_testOperation;
	DeviceDetails m_testDetails;
	std::shared_ptr<const Ostc4SettingsIO> m_saveIO;
	bool m_session = false;
	QMutex m_mutex;
	bool m_stop = false;
	bool m_save = false;
	DeviceDetails m_saveDetails;
	QString m_backupPath;
};

class WriteSettingsThread : public DeviceThread {
	Q_OBJECT
public:
	WriteSettingsThread(QObject *parent, device_data_t *data);
	void setDeviceDetails(const DeviceDetails &details);
	void setIO(std::shared_ptr<const Ostc4SettingsIO> io) { m_io = std::move(io); }
	void setTransport(std::function<QString(device_data_t *)> open,
		std::function<void(device_data_t *)> close);
	// Test seam for exercising the actual worker lifecycle without device I/O.
	void setTestOperation(std::function<bool(device_data_t *)> operation) { m_testOperation = std::move(operation); }
	void run();
	void perform(); // Called by the persistent OSTC settings worker on its transport thread.
signals:
	void result(QString text);

private:
	DeviceDetails m_deviceDetails;
	std::shared_ptr<const Ostc4SettingsIO> m_io;
	QString m_backupPath;
	std::function<QString(device_data_t *)> m_open;
	std::function<void(device_data_t *)> m_close;
	std::function<bool(device_data_t *)> m_testOperation;
public:
	void setBackupPath(const QString &path) { m_backupPath = path; }
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
