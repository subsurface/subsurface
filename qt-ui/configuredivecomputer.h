#ifndef CONFIGUREDIVECOMPUTER_H
#define CONFIGUREDIVECOMPUTER_H

#include <QObject>
#include <QThread>
#include <QVariant>
#include "libdivecomputer.h"
#include <QDateTime>
class ReadSettingsThread : public QThread {
	Q_OBJECT
public:
	ReadSettingsThread(QObject *parent, device_data_t *data);
	virtual void run();
	QString result;
	QString lastError;
signals:
	void error(QString err);
private:
	device_data_t *data;
};

class WriteSettingsThread : public QThread {
	Q_OBJECT
public:
	WriteSettingsThread(QObject *parent, device_data_t *data, QString settingName, QVariant settingValue);
	virtual void run();
	QString result;
	QString lastError;
signals:
	void error(QString err);
private:
	device_data_t *data;
	QString m_settingName;
	QVariant m_settingValue;
};

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

	void setDeviceName(device_data_t *data, QString newName);
	void setDeviceDateAndTime(device_data_t *data, QDateTime dateAndTime);
signals:
	void deviceSettings(QString settings);
	void message(QString msg);
	void error(QString err);
	void readFinished();
	void writeFinished();
	void stateChanged(states newState);
private:
	ReadSettingsThread *readThread;
	WriteSettingsThread *writeThread;
	void setState(states newState);

	void writeSettingToDevice(device_data_t *data, QString settingName, QVariant settingValue);
private slots:
	void readThreadFinished();
	void writeThreadFinished();
	void setError(QString err);
};

#endif // CONFIGUREDIVECOMPUTER_H
