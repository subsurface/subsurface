// SPDX-License-Identifier: GPL-2.0
#ifndef CONFIGUREDIVECOMPUTER_H
#define CONFIGUREDIVECOMPUTER_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include "configuredivecomputerthreads.h"
#include <QDateTime>

#include "libxml/xmlreader.h"

class ConfigureDiveComputer : public QObject {
	Q_OBJECT
public:
	explicit ConfigureDiveComputer();
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
	void saveDeviceDetails(DeviceDetails *details, device_data_t *data);
	void fetchDeviceDetails();
	bool saveXMLBackup(const QString &fileName, DeviceDetails *details, device_data_t *data);
	bool restoreXMLBackup(const QString &fileName, DeviceDetails *details);
	void startFirmwareUpdate(const QString &fileName, device_data_t *data, bool forceUpdate);
	void resetSettings(device_data_t *data);

	QString dc_open(device_data_t *data);
public
slots:
	void dc_close(device_data_t *data);
signals:
	void progress(int percent);
	void message(QString msg);
	void error(QString err);
	void stateChanged(states newState);
	void deviceDetailsChanged(DeviceDetails *newDetails);

private:
	ReadSettingsThread *readThread;
	WriteSettingsThread *writeThread;
	ResetSettingsThread *resetThread;
	FirmwareUpdateThread *firmwareThread;
	void connectThreadSignals(DeviceThread *thread);
	void setState(states newState);
private
slots:
	void progressEvent(int percent);
	void readThreadFinished();
	void writeThreadFinished();
	void resetThreadFinished();
	void firmwareThreadFinished();
	void setError(QString err);
};

#endif // CONFIGUREDIVECOMPUTER_H
