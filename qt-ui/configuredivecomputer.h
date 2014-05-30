#ifndef CONFIGUREDIVECOMPUTER_H
#define CONFIGUREDIVECOMPUTER_H

#include <QObject>
#include <QThread>
#include "libdivecomputer.h"

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
signals:
	void deviceSettings(QString settings);
	void message(QString msg);
	void error(QString err);
	void readFinished();
	void writeFinished();
	void stateChanged(states newState);
private:
	ReadSettingsThread *readThread;
	void setState(states newState);


	void readHWSettings(device_data_t *data);
private slots:
	void readThreadFinished();
	void setError(QString err);
};

#endif // CONFIGUREDIVECOMPUTER_H
