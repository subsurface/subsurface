#ifndef CONFIGUREDIVECOMPUTER_H
#define CONFIGUREDIVECOMPUTER_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include "configuredivecomputerthreads.h"
#include <QDateTime>

class ConfigureDiveComputer : public QObject
{
	Q_OBJECT
public:
	explicit ConfigureDiveComputer(QObject *parent = 0);
	void readSettings(DeviceDetails *deviceDetails, device_data_t *data);

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
	DeviceDetails *m_deviceDetails;
	device_data_t *m_data;
	void saveDeviceDetails();
	void fetchDeviceDetails();

signals:
	void message(QString msg);
	void error(QString err);
	void readFinished();
	void writeFinished();
	void stateChanged(states newState);

private:
	ReadSettingsThread *readThread;
	WriteSettingsThread *writeThread;
	void setState(states newState);

private slots:
	void readThreadFinished();
	void writeThreadFinished();
	void setError(QString err);
};

#endif // CONFIGUREDIVECOMPUTER_H
