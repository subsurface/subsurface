#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QScreen>
#include <QElapsedTimer>

#include "core/gpslocation.h"

class QMLManager : public QObject {
	Q_OBJECT
	Q_ENUMS(credentialStatus_t)
	Q_PROPERTY(QString cloudUserName READ cloudUserName WRITE setCloudUserName NOTIFY cloudUserNameChanged)
	Q_PROPERTY(QString cloudPassword READ cloudPassword WRITE setCloudPassword NOTIFY cloudPasswordChanged)
	Q_PROPERTY(QString logText READ logText WRITE setLogText NOTIFY logTextChanged)
	Q_PROPERTY(bool locationServiceEnabled READ locationServiceEnabled WRITE setLocationServiceEnabled NOTIFY locationServiceEnabledChanged)
	Q_PROPERTY(int distanceThreshold READ distanceThreshold WRITE setDistanceThreshold NOTIFY distanceThresholdChanged)
	Q_PROPERTY(int timeThreshold READ timeThreshold WRITE setTimeThreshold NOTIFY timeThresholdChanged)
	Q_PROPERTY(bool loadFromCloud READ loadFromCloud WRITE setLoadFromCloud NOTIFY loadFromCloudChanged)
	Q_PROPERTY(QString startPageText READ startPageText WRITE setStartPageText NOTIFY startPageTextChanged)
	Q_PROPERTY(bool verboseEnabled READ verboseEnabled WRITE setVerboseEnabled NOTIFY verboseEnabledChanged)
	Q_PROPERTY(credentialStatus_t credentialStatus READ credentialStatus WRITE setCredentialStatus NOTIFY credentialStatusChanged)
	Q_PROPERTY(credentialStatus_t oldStatus READ oldStatus WRITE setOldStatus NOTIFY oldStatusChanged)
	Q_PROPERTY(int accessingCloud READ accessingCloud WRITE setAccessingCloud NOTIFY accessingCloudChanged)
	Q_PROPERTY(bool syncToCloud READ syncToCloud WRITE setSyncToCloud NOTIFY syncToCloudChanged)

public:
	QMLManager();
	~QMLManager();

	enum credentialStatus_t {
		INCOMPLETE,
		UNKNOWN,
		INVALID,
		VALID_EMAIL,
		VALID
	};

	static QMLManager *instance();

	QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

	bool locationServiceEnabled() const;
	void setLocationServiceEnabled(bool locationServiceEnable);

	bool verboseEnabled() const;
	void setVerboseEnabled(bool verboseMode);

	int distanceThreshold() const;
	void setDistanceThreshold(int distance);

	int timeThreshold() const;
	void setTimeThreshold(int time);

	bool loadFromCloud() const;
	void setLoadFromCloud(bool done);
	void syncLoadFromCloud();

	QString startPageText() const;
	void setStartPageText(const QString& text);

	credentialStatus_t credentialStatus() const;
	void setCredentialStatus(const credentialStatus_t value);

	credentialStatus_t oldStatus() const;
	void setOldStatus(const credentialStatus_t value);

	QString logText() const;
	void setLogText(const QString &logText);

	int accessingCloud() const;
	void setAccessingCloud(int status);

	bool syncToCloud() const;
	void setSyncToCloud(bool status);

	typedef void (QMLManager::*execute_function_type)();

public slots:
	void applicationStateChanged(Qt::ApplicationState state);
	void savePreferences();
	void saveCloudCredentials();
	void checkCredentialsAndExecute(execute_function_type execute);
	void tryRetrieveDataFromBackend();
	void handleError(QNetworkReply::NetworkError nError);
	void handleSslErrors(const QList<QSslError> &errors);
	void retrieveUserid();
	void loadDivesWithValidCredentials();
	void loadDiveProgress(int percent);
	void provideAuth(QNetworkReply *reply, QAuthenticator *auth);
	void commitChanges(QString diveId, QString date, QString location,
			   QString gps, QString duration, QString depth,
			   QString airtemp, QString watertemp, QString suit,
			   QString buddy, QString diveMaster, QString weight, QString notes,
			   QString startpressure, QString endpressure, QString gasmix);
	void saveChangesLocal();
	void saveChangesCloud(bool forceRemoteSync);
	void deleteDive(int id);
	void undoDelete(int id);
	QString addDive();
	void addDiveAborted(int id);
	void applyGpsData();
	void sendGpsData();
	void downloadGpsData();
	void populateGpsData();
	void clearGpsData();
	void finishSetup();
	void openLocalThenRemote(QString url);
	int getIndex(const QString& diveId);
	QString getNumber(const QString& diveId);
	QString getDate(const QString& diveId);
	QString getCurrentPosition();
	QString getVersion() const;
	void deleteGpsFix(quint64 when);
	void refreshDiveList();
	void screenChanged(QScreen *screen);
	qreal lastDevicePixelRatio();
	void appendTextToLog(const QString &newText);

private:
	QString m_cloudUserName;
	QString m_cloudPassword;
	QString m_ssrfGpsWebUserid;
	QString m_startPageText;
	QString m_logText;
	bool m_locationServiceEnabled;
	bool m_verboseEnabled;
	int m_distanceThreshold;
	int m_timeThreshold;
	GpsLocation *locationProvider;
	bool m_loadFromCloud;
	static QMLManager *m_instance;
	QNetworkReply *reply;
	QNetworkRequest request;
	struct dive *deletedDive;
	struct dive_trip *deletedTrip;
	int m_accessingCloud;
	bool m_syncToCloud;
	credentialStatus_t m_credentialStatus;
	credentialStatus_t m_oldStatus;
	qreal m_lastDevicePixelRatio;
	QElapsedTimer timer;
	bool alreadySaving;

signals:
	void cloudUserNameChanged();
	void cloudPasswordChanged();
	void locationServiceEnabledChanged();
	void verboseEnabledChanged();
	void logTextChanged();
	void timeThresholdChanged();
	void distanceThresholdChanged();
	void loadFromCloudChanged();
	void startPageTextChanged();
	void credentialStatusChanged();
	void oldStatusChanged();
	void accessingCloudChanged();
	void syncToCloudChanged();
	void sendScreenChanged(QScreen *screen);
};

#endif
