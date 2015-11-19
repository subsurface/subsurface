#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>

#include "gpslocation.h"

void qmlUiShowMessage(const char *errorString);

class QMLManager : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString cloudUserName READ cloudUserName WRITE setCloudUserName NOTIFY cloudUserNameChanged)
	Q_PROPERTY(QString cloudPassword READ cloudPassword WRITE setCloudPassword NOTIFY cloudPasswordChanged)
	Q_PROPERTY(bool saveCloudPassword READ saveCloudPassword WRITE setSaveCloudPassword NOTIFY saveCloudPasswordChanged)
	Q_PROPERTY(QString logText READ logText WRITE setLogText NOTIFY logTextChanged)
	Q_PROPERTY(bool locationServiceEnabled READ locationServiceEnabled WRITE setLocationServiceEnabled NOTIFY locationServiceEnabledChanged)
	Q_PROPERTY(int distanceThreshold READ distanceThreshold WRITE setDistanceThreshold NOTIFY distanceThresholdChanged)
	Q_PROPERTY(int timeThreshold READ timeThreshold WRITE setTimeThreshold NOTIFY timeThresholdChanged)
public:
	QMLManager();
	~QMLManager();

	QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

	bool saveCloudPassword() const;
	void setSaveCloudPassword(bool saveCloudPassword);

	bool locationServiceEnabled() const;
	void setLocationServiceEnabled(bool locationServiceEnable);

	int distanceThreshold() const;
	void setDistanceThreshold(int distance);

	int timeThreshold() const;
	void setTimeThreshold(int time);

	QString logText() const;
	void setLogText(const QString &logText);
	void appendTextToLog(const QString &newText);

public slots:
	void savePreferences();
	void loadDives();
	void commitChanges(QString diveId, QString suit, QString buddy, QString diveMaster, QString notes);
	void saveChanges();
	void addDive();
	void applyGpsData();
	void sendGpsData();
	void clearGpsData();

private:
	QString m_cloudUserName;
	QString m_cloudPassword;
	QString m_ssrfGpsWebUserid;
	bool m_saveCloudPassword;
	QString m_logText;
	bool m_locationServiceEnabled;
	int m_distanceThreshold;
	int m_timeThreshold;
	GpsLocation *locationProvider;

signals:
	void cloudUserNameChanged();
	void cloudPasswordChanged();
	void saveCloudPasswordChanged();
	void locationServiceEnabledChanged();
	void logTextChanged();
	void timeThresholdChanged();
	void distanceThresholdChanged();
};

#endif
