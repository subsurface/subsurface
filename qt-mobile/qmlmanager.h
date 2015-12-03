#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>

#include "gpslocation.h"

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
	Q_PROPERTY(bool loadFromCloud READ loadFromCloud WRITE setLoadFromCloud NOTIFY loadFromCloudChanged)
public:
	QMLManager();
	~QMLManager();

	static QMLManager *instance();

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

	bool loadFromCloud() const;
	void setLoadFromCloud(bool done);

	QString logText() const;
	void setLogText(const QString &logText);
	void appendTextToLog(const QString &newText);

public slots:
	void savePreferences();
	void saveCloudCredentials();
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
	bool m_loadFromCloud;
	static QMLManager *m_instance;

signals:
	void cloudUserNameChanged();
	void cloudPasswordChanged();
	void saveCloudPasswordChanged();
	void locationServiceEnabledChanged();
	void logTextChanged();
	void timeThresholdChanged();
	void distanceThresholdChanged();
	void loadFromCloudChanged();
};

#endif
