// SPDX-License-Identifier: GPL-2.0
#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QScreen>
#include <QElapsedTimer>
#include <QColor>
#include <QFile>

#include "core/btdiscovery.h"
#include "core/gpslocation.h"
#include "core/downloadfromdcthread.h"
#include "qt-models/divelistmodel.h"
#include "qt-models/completionmodels.h"
#include "qt-models/divelocationmodel.h"

#define NOCLOUD_LOCALSTORAGE format_string("%s/cloudstorage/localrepo[master]", system_default_directory())

class QMLManager : public QObject {
	Q_OBJECT
	Q_ENUMS(cloud_status_qml)
	Q_PROPERTY(QString cloudUserName MEMBER m_cloudUserName WRITE setCloudUserName NOTIFY cloudUserNameChanged)
	Q_PROPERTY(QString cloudPassword MEMBER m_cloudPassword WRITE setCloudPassword NOTIFY cloudPasswordChanged)
	Q_PROPERTY(QString cloudPin MEMBER m_cloudPin WRITE setCloudPin NOTIFY cloudPinChanged)
	Q_PROPERTY(QString logText READ logText WRITE setLogText NOTIFY logTextChanged)
	Q_PROPERTY(bool locationServiceEnabled MEMBER m_locationServiceEnabled WRITE setLocationServiceEnabled NOTIFY locationServiceEnabledChanged)
	Q_PROPERTY(bool locationServiceAvailable MEMBER m_locationServiceAvailable WRITE setLocationServiceAvailable NOTIFY locationServiceAvailableChanged)
	Q_PROPERTY(int distanceThreshold MEMBER m_distanceThreshold WRITE setDistanceThreshold NOTIFY distanceThresholdChanged)
	Q_PROPERTY(int timeThreshold MEMBER m_timeThreshold WRITE setTimeThreshold NOTIFY timeThresholdChanged)
	Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
	Q_PROPERTY(bool loadFromCloud MEMBER m_loadFromCloud WRITE setLoadFromCloud NOTIFY loadFromCloudChanged)
	Q_PROPERTY(QString startPageText MEMBER m_startPageText WRITE setStartPageText NOTIFY startPageTextChanged)
	Q_PROPERTY(bool verboseEnabled MEMBER m_verboseEnabled WRITE setVerboseEnabled NOTIFY verboseEnabledChanged)
	Q_PROPERTY(cloud_status_qml credentialStatus MEMBER m_credentialStatus WRITE setCredentialStatus NOTIFY credentialStatusChanged)
	Q_PROPERTY(cloud_status_qml oldStatus MEMBER m_oldStatus WRITE setOldStatus NOTIFY oldStatusChanged)
	Q_PROPERTY(QString notificationText MEMBER m_notificationText WRITE setNotificationText NOTIFY notificationTextChanged)
	Q_PROPERTY(bool syncToCloud MEMBER m_syncToCloud WRITE setSyncToCloud NOTIFY syncToCloudChanged)
	Q_PROPERTY(int updateSelectedDive MEMBER m_updateSelectedDive WRITE setUpdateSelectedDive NOTIFY updateSelectedDiveChanged)
	Q_PROPERTY(int selectedDiveTimestamp MEMBER m_selectedDiveTimestamp WRITE setSelectedDiveTimestamp NOTIFY selectedDiveTimestampChanged)
	Q_PROPERTY(QStringList suitList READ suitList NOTIFY suitListChanged)
	Q_PROPERTY(QStringList buddyList READ buddyList NOTIFY buddyListChanged)
	Q_PROPERTY(QStringList divemasterList READ divemasterList NOTIFY divemasterListChanged)
	Q_PROPERTY(QStringList locationList READ locationList NOTIFY locationListChanged)
	Q_PROPERTY(QStringList cylinderInit READ cylinderInit CONSTANT)
	Q_PROPERTY(bool showPin MEMBER m_showPin WRITE setShowPin NOTIFY showPinChanged)
	Q_PROPERTY(QString progressMessage MEMBER m_progressMessage WRITE setProgressMessage NOTIFY progressMessageChanged)
	Q_PROPERTY(bool developer MEMBER m_developer WRITE setDeveloper NOTIFY developerChanged)
	Q_PROPERTY(bool btEnabled MEMBER m_btEnabled WRITE setBtEnabled NOTIFY btEnabledChanged)

	Q_PROPERTY(QString DC_vendor READ DC_vendor WRITE DC_setVendor)
	Q_PROPERTY(QString DC_product READ DC_product WRITE DC_setProduct)
	Q_PROPERTY(QString DC_devName READ DC_devName WRITE DC_setDevName)
	Q_PROPERTY(QString DC_devBluetoothName READ DC_devBluetoothName WRITE DC_setDevBluetoothName)
	Q_PROPERTY(QString descriptor READ DC_descriptor)
	Q_PROPERTY(bool DC_forceDownload READ DC_forceDownload WRITE DC_setForceDownload)
	Q_PROPERTY(bool DC_bluetoothMode READ DC_bluetoothMode WRITE DC_setBluetoothMode)
	Q_PROPERTY(bool DC_createNewTrip READ DC_createNewTrip WRITE DC_setCreateNewTrip)
	Q_PROPERTY(bool DC_saveDump READ DC_saveDump WRITE DC_setSaveDump)
	Q_PROPERTY(bool DC_saveLog READ DC_saveLog WRITE DC_setSaveLog)
	Q_PROPERTY(int DC_deviceId READ DC_deviceId WRITE DC_setDeviceId)
	Q_PROPERTY(int DC_diveId READ DC_diveId WRITE DC_setDiveId)
public:
	QMLManager();
	~QMLManager();

	QString DC_vendor() const;
	void DC_setVendor(const QString& vendor);

	QString DC_product() const;
	void DC_setProduct(const QString& product);

	QString DC_devName() const;
	void DC_setDevName(const QString& devName);

	QString DC_devBluetoothName() const;
	void DC_setDevBluetoothName(const QString& devBluetoothName);

	QString DC_descriptor() const;

	bool DC_forceDownload() const;
	void DC_setForceDownload(bool force);

	bool DC_bluetoothMode() const;
	void DC_setBluetoothMode(bool mode);

	bool DC_createNewTrip() const;
	void DC_setCreateNewTrip(bool create);

	bool DC_saveDump() const;
	void DC_setSaveDump(bool dumpMode);

	bool DC_saveLog() const;
	void DC_setSaveLog(bool saveLog);

	int DC_deviceId() const;
	void DC_setDeviceId(int deviceId);

	int DC_diveId() const;
	void DC_setDiveId(int diveId);

	Q_INVOKABLE QStringList getProductListFromVendor(const QString& vendor);
	Q_INVOKABLE int getMatchingAddress(const QString &vendor, const QString &product);
	Q_INVOKABLE int getDetectedVendorIndex();
	Q_INVOKABLE int getDetectedProductIndex(const QString &currentVendorText);
public:
	enum cloud_status_qml {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};

	static QMLManager *instance();
	Q_INVOKABLE void registerError(QString error);
	QString consumeError();

	QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

	QString cloudPin() const;
	void setCloudPin(const QString &cloudPin);

	bool locationServiceEnabled() const;
	void setLocationServiceEnabled(bool locationServiceEnable);

	bool locationServiceAvailable() const;
	void setLocationServiceAvailable(bool locationServiceAvailable);

	bool verboseEnabled() const;
	void setVerboseEnabled(bool verboseMode);

	int distanceThreshold() const;
	void setDistanceThreshold(int distance);

	int timeThreshold() const;
	void setTimeThreshold(int time);

	QString theme() const;
	void setTheme(QString theme);

	bool loadFromCloud() const;
	void setLoadFromCloud(bool done);
	void syncLoadFromCloud();

	QString startPageText() const;
	void setStartPageText(const QString& text);

	cloud_status_qml credentialStatus() const;
	void setCredentialStatus(const cloud_status_qml value);

	cloud_status_qml oldStatus() const;
	void setOldStatus(const cloud_status_qml value);

	QString logText() const;
	void setLogText(const QString &logText);

	QString notificationText() const;
	void setNotificationText(QString text);

	bool syncToCloud() const;
	void setSyncToCloud(bool status);

	int updateSelectedDive() const;
	void setUpdateSelectedDive(int idx);

	int selectedDiveTimestamp() const;
	void setSelectedDiveTimestamp(int when);

	QString progressMessage() const;
	void setProgressMessage(QString text);

	bool developer() const;
	void setDeveloper(bool value);

	bool btEnabled() const;
	void setBtEnabled(bool value);

	DiveListSortModel *dlSortModel;

	QStringList suitList() const;
	QStringList buddyList() const;
	QStringList divemasterList() const;
	QStringList locationList() const;
	QStringList cylinderInit() const;
	bool showPin() const;
	void setShowPin(bool enable);
	Q_INVOKABLE void setStatusbarColor(QColor color);
	void btHostModeChange(QBluetoothLocalDevice::HostMode state);

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	void writeToAppLogFile(QString logText);
#endif

public slots:
	void applicationStateChanged(Qt::ApplicationState state);
	void savePreferences();
	void saveCloudCredentials();
	void tryRetrieveDataFromBackend();
	void handleError(QNetworkReply::NetworkError nError);
	void handleSslErrors(const QList<QSslError> &errors);
	void retrieveUserid();
	void loadDivesWithValidCredentials();
	void provideAuth(QNetworkReply *reply, QAuthenticator *auth);
	void commitChanges(QString diveId, QString date, QString location, QString gps,
			   QString duration, QString depth, QString airtemp,
			   QString watertemp, QString suit, QString buddy,
			   QString diveMaster, QString weight, QString notes, QString startpressure,
			   QString endpressure, QString gasmix, QString cylinder, int rating, int visibility);
	void changesNeedSaving();
	void openNoCloudRepo();
	void saveChangesLocal();
	void saveChangesCloud(bool forceRemoteSync);
	void deleteDive(int id);
	bool undoDelete(int id);
	QString addDive();
	void addDiveAborted(int id);
	void applyGpsData();
	void sendGpsData();
	void downloadGpsData();
	void populateGpsData();
	void cancelDownloadDC();
	void clearGpsData();
	void clearCredentials();
	void cancelCredentialsPinSetup();
	void copyAppLogToClipboard();
	void finishSetup();
	void openLocalThenRemote(QString url);
	void mergeLocalRepo();
	QString getNumber(const QString& diveId);
	QString getDate(const QString& diveId);
	QString getCurrentPosition();
	QString getGpsFromSiteName(const QString& siteName);
	QString getVersion() const;
	void deleteGpsFix(quint64 when);
	void revertToNoCloudIfNeeded();
	void consumeFinishedLoad(timestamp_t currentDiveTimestamp);
	void refreshDiveList();
	void screenChanged(QScreen *screen);
	qreal lastDevicePixelRatio();
	void setDevicePixelRatio(qreal dpr, QScreen *screen);
	void appendTextToLog(const QString &newText);
	void quit();
	void hasLocationSourceChanged();
	void btRescan();


private:
	BuddyCompletionModel buddyModel;
	SuitCompletionModel suitModel;
	DiveMasterCompletionModel divemasterModel;
	LocationInformationModel locationModel;
	QString m_cloudUserName;
	QString m_cloudPassword;
	QString m_cloudPin;
	QString m_ssrfGpsWebUserid;
	QString m_startPageText;
	QString m_logText;
	QString m_lastError;
	bool m_locationServiceEnabled;
	bool m_locationServiceAvailable;
	bool m_verboseEnabled;
	int m_distanceThreshold;
	int m_timeThreshold;
	GpsLocation *locationProvider;
	bool m_loadFromCloud;
	static QMLManager *m_instance;
	struct dive *deletedDive;
	struct dive_trip *deletedTrip;
	QString m_notificationText;
	bool m_syncToCloud;
	int m_updateSelectedDive;
	int m_selectedDiveTimestamp;
	cloud_status_qml m_credentialStatus;
	cloud_status_qml m_oldStatus;
	qreal m_lastDevicePixelRatio;
	QElapsedTimer timer;
	bool alreadySaving;
	bool checkDate(DiveObjectHelper *myDive, struct dive * d, QString date);
	bool checkLocation(DiveObjectHelper *myDive, struct dive *d, QString location, QString gps);
	bool checkDuration(DiveObjectHelper *myDive, struct dive *d, QString duration);
	bool checkDepth(DiveObjectHelper *myDive, struct dive *d, QString depth);
	bool currentGitLocalOnly;
	bool m_showPin;
	Q_INVOKABLE DCDeviceData *m_device_data;
	QString m_progressMessage;
	bool m_developer;
	bool m_btEnabled;
	void updateAllGlobalLists();

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	QString appLogFileName;
	QFile appLogFile;
	bool appLogFileOpen;
#endif

signals:
	void cloudUserNameChanged();
	void cloudPasswordChanged();
	void cloudPinChanged();
	void locationServiceEnabledChanged();
	void locationServiceAvailableChanged();
	void verboseEnabledChanged();
	void logTextChanged();
	void timeThresholdChanged();
	void themeChanged();
	void distanceThresholdChanged();
	void loadFromCloudChanged();
	void startPageTextChanged();
	void credentialStatusChanged();
	void oldStatusChanged();
	void notificationTextChanged();
	void syncToCloudChanged();
	void updateSelectedDiveChanged();
	void selectedDiveTimestampChanged();
	void showPinChanged();
	void sendScreenChanged(QScreen *screen);
	void progressMessageChanged();
	void developerChanged();
	void btEnabledChanged();
	void suitListChanged();
	void buddyListChanged();
	void divemasterListChanged();
	void locationListChanged();
	void waitingForPositionChanged();
};

#endif
