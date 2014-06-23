#ifndef CONFIGUREDIVECOMPUTER_H
#define CONFIGUREDIVECOMPUTER_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include "configuredivecomputerthreads.h"
#include <QDateTime>

#include "libxml/xmlreader.h"

class ConfigureDiveComputer : public QObject
{
	Q_OBJECT
public:
	explicit ConfigureDiveComputer(QObject *parent = 0);
	void readSettings(device_data_t *data);

	enum states {
			INITIAL,
			READING,
			WRITING,
			CANCELLING,
			CANCELLED,
			ERROR,
			DONE,
		};

	QString lastError;
	states currentState;
	device_data_t *m_data;
	void saveDeviceDetails(DeviceDetails *details, device_data_t *data);
	void fetchDeviceDetails();
	bool saveXMLBackup(QString fileName, DeviceDetails *details, device_data_t *data, QString errorText);
	bool restoreXMLBackup(QString fileName, DeviceDetails *details, QString errorText);
	void startFirmwareUpdate(QString fileName, device_data_t *data, QString errorText);
signals:
	void message(QString msg);
	void error(QString err);
	void readFinished();
	void writeFinished();
	void stateChanged(states newState);
	void deviceDetailsChanged(DeviceDetails *newDetails);

private:
	ReadSettingsThread *readThread;
	WriteSettingsThread *writeThread;
	void setState(states newState);
	QString addSettingToXML(QString settingName, QVariant value);
private slots:
	void readThreadFinished();
	void writeThreadFinished();
	void setError(QString err);
};

#endif // CONFIGUREDIVECOMPUTER_H
