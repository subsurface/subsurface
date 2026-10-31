// SPDX-License-Identifier: GPL-2.0
#ifndef CONFIGUREDIVECOMPUTER_H
#define CONFIGUREDIVECOMPUTER_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include "configuredivecomputerthreads.h"
#include <QDateTime>
#include <functional>

#include "libxml/xmlreader.h"

class ConfigureDiveComputer : public QObject {
	Q_OBJECT
public:
	explicit ConfigureDiveComputer();
	~ConfigureDiveComputer() override;
	void readSettings(device_data_t *data);

	enum states {
		OPEN,
		INITIAL,
		READING,
		WRITING,
		RESETTING,
		FWUPDATE,
		CANCELLING,
		CANCELLED,
		ERRORED,
		DONE,
	};

	QString lastError;
	states currentState;
	void saveDeviceDetails(const DeviceDetails &details, device_data_t *data);
	void setOstc4BackupPath(const QString &path) { ostc4BackupPath = path; }
	bool validateOstc4Backup(const QString &path, QString &description) const;
	void fetchDeviceDetails();
	bool saveXMLBackup(const QString &fileName, const DeviceDetails &details, device_data_t *data);
	bool restoreXMLBackup(const QString &fileName, DeviceDetails &details);
	void startFirmwareUpdate(const QString &fileName, device_data_t *data, bool forceUpdate);
	void resetSettings(device_data_t *data);

	QString dc_open(device_data_t *data);
	QString openForOtherOperation(device_data_t *data);
	// Test seam for the controller-to-worker transport lifecycle.
	void setReadTransportForTesting(std::function<QString(device_data_t *)> open,
		std::function<dc_status_t(device_data_t *)> close, std::function<bool(device_data_t *)> operation);
	void setReadDetailsForTesting(const DeviceDetails &details) { testDetails = details; }
	void setOstc4IOForTesting(std::shared_ptr<const Ostc4SettingsIO> io) { testIO = std::move(io); }
	// Transport-only operations: caller must create, use and close the stream in one thread.
	static QString openTransport(device_data_t *data);
	static dc_status_t closeTransport(device_data_t *data);
public
slots:
	void dc_close(device_data_t *data);
signals:
	void progress(int percent);
	void message(QString msg);
	void error(QString err);
	void stateChanged(states oldState, states newState);
	void deviceDetailsChanged(DeviceDetails newDetails);

private:
	Ostc4SettingsTransaction ostc4Snapshot;
	QString ostc4BackupPath;
	bool ostc4Writing = false;
	ReadSettingsThread *readThread;
	WriteSettingsThread *writeThread;
	ResetSettingsThread *resetThread;
	FirmwareUpdateThread *firmwareThread;
	std::function<QString(device_data_t *)> readOpen = openTransport;
	std::function<dc_status_t(device_data_t *)> readClose = closeTransport;
	std::function<bool(device_data_t *)> readOperation;
	DeviceDetails testDetails;
	std::shared_ptr<const Ostc4SettingsIO> testIO;
	void stopReadSession();
	void connectThreadSignals(DeviceThread *thread);
	void setState(states newState);
private
slots:
	void progressEvent(int percent);
	void threadFinished();
	void setError(QString err);
};

#endif // CONFIGUREDIVECOMPUTER_H
