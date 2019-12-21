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
	Q_PROPERTY(QString logText READ logText WRITE setLogText NOTIFY logTextChanged)
	Q_PROPERTY(bool locationServiceEnabled MEMBER m_locationServiceEnabled WRITE setLocationServiceEnabled NOTIFY locationServiceEnabledChanged)
	Q_PROPERTY(bool locationServiceAvailable MEMBER m_locationServiceAvailable WRITE setLocationServiceAvailable NOTIFY locationServiceAvailableChanged)
	Q_PROPERTY(bool loadFromCloud MEMBER m_loadFromCloud WRITE setLoadFromCloud NOTIFY loadFromCloudChanged)
	Q_PROPERTY(QString startPageText MEMBER m_startPageText WRITE setStartPageText NOTIFY startPageTextChanged)
	Q_PROPERTY(bool verboseEnabled MEMBER m_verboseEnabled WRITE setVerboseEnabled NOTIFY verboseEnabledChanged)
	Q_PROPERTY(QString notificationText MEMBER m_notificationText WRITE setNotificationText NOTIFY notificationTextChanged)
	Q_PROPERTY(int updateSelectedDive MEMBER m_updateSelectedDive WRITE setUpdateSelectedDive NOTIFY updateSelectedDiveChanged)
	Q_PROPERTY(int selectedDiveTimestamp MEMBER m_selectedDiveTimestamp WRITE setSelectedDiveTimestamp NOTIFY selectedDiveTimestampChanged)
	Q_PROPERTY(QStringList suitList READ suitList NOTIFY suitListChanged)
	Q_PROPERTY(QStringList buddyList READ buddyList NOTIFY buddyListChanged)
	Q_PROPERTY(QStringList divemasterList READ divemasterList NOTIFY divemasterListChanged)
	Q_PROPERTY(QStringList locationList READ locationList NOTIFY locationListChanged)
	Q_PROPERTY(QStringList cylinderInit READ cylinderInit CONSTANT)
	Q_PROPERTY(QString progressMessage MEMBER m_progressMessage WRITE setProgressMessage NOTIFY progressMessageChanged)
	Q_PROPERTY(bool btEnabled MEMBER m_btEnabled WRITE setBtEnabled NOTIFY btEnabledChanged)

	Q_PROPERTY(QString DC_vendor READ DC_vendor WRITE DC_setVendor)
	Q_PROPERTY(QString DC_product READ DC_product WRITE DC_setProduct)
	Q_PROPERTY(QString DC_devName READ DC_devName WRITE DC_setDevName)
	Q_PROPERTY(QString DC_devBluetoothName READ DC_devBluetoothName WRITE DC_setDevBluetoothName)
	Q_PROPERTY(QString descriptor READ DC_descriptor)
	Q_PROPERTY(bool DC_forceDownload READ DC_forceDownload WRITE DC_setForceDownload NOTIFY DC_ForceDownloadChanged)
	Q_PROPERTY(bool DC_bluetoothMode READ DC_bluetoothMode WRITE DC_setBluetoothMode)
	Q_PROPERTY(bool DC_saveDump READ DC_saveDump WRITE DC_setSaveDump)
	Q_PROPERTY(int DC_deviceId READ DC_deviceId WRITE DC_setDeviceId)
	Q_PROPERTY(QString pluggedInDeviceName MEMBER m_pluggedInDeviceName NOTIFY pluggedInDeviceNameChanged)
	Q_PROPERTY(bool showNonDiveComputers MEMBER m_showNonDiveComputers WRITE setShowNonDiveComputers NOTIFY showNonDiveComputersChanged)
public:
	QMLManager();
	~QMLManager();

	enum export_types {
		EX_DIVES_XML,
		EX_DIVE_SITES_XML,
		EX_UDDF,
		EX_DIVELOGS_DE,
		EX_DIVESHARE,
		EX_CSV_DIVE_PROFILE,
		EX_CSV_DETAILS,
		EX_CSV_PROFILE,
		EX_PROFILE_PNG,
		EX_WORLD_MAP,
		EX_TEX,
		EX_LATEX,
		EX_IMAGE_DEPTHS
	};
	Q_ENUM(export_types)
	Q_INVOKABLE void exportToFile(export_types type, QString directory, bool anonymize);
	Q_INVOKABLE void exportToWEB(export_types type, QString userId, QString password, bool anonymize);


	QString DC_vendor() const;
	void DC_setVendor(const QString& vendor);

	QString DC_product() const;
	void DC_setProduct(const QString& product);

	QString DC_devName() const;
	void DC_setDevName(const QString& devName);

	Q_INVOKABLE void retrieveBluetoothName();

	QString DC_devBluetoothName() const;
	void DC_setDevBluetoothName(const QString& devBluetoothName);

	QString DC_descriptor() const;

	bool DC_forceDownload() const;
	void DC_setForceDownload(bool force);

	bool DC_bluetoothMode() const;
	void DC_setBluetoothMode(bool mode);

	bool DC_saveDump() const;
	void DC_setSaveDump(bool dumpMode);

	int DC_deviceId() const;
	void DC_setDeviceId(int deviceId);

	Q_INVOKABLE QStringList getProductListFromVendor(const QString& vendor);
	Q_INVOKABLE int getMatchingAddress(const QString &vendor, const QString &product);
	Q_INVOKABLE int getDetectedVendorIndex();
	Q_INVOKABLE int getDetectedProductIndex(const QString &currentVendorText);
	Q_INVOKABLE int getConnectionIndex(const QString &deviceSubstr);
	Q_INVOKABLE void setGitLocalOnly(const bool &value);
	Q_INVOKABLE void setFilter(const QString filterText);

	static QMLManager *instance();
	Q_INVOKABLE void registerError(QString error);
	QString consumeError();

	bool locationServiceEnabled() const;
	void setLocationServiceEnabled(bool locationServiceEnable);

	bool locationServiceAvailable() const;
	void setLocationServiceAvailable(bool locationServiceAvailable);

	bool verboseEnabled() const;
	void setVerboseEnabled(bool verboseMode);

	bool loadFromCloud() const;
	void setLoadFromCloud(bool done);
	void syncLoadFromCloud();

	QString startPageText() const;
	void setStartPageText(const QString& text);

	QString logText() const;
	void setLogText(const QString &logText);

	QString notificationText() const;
	void setNotificationText(QString text);

	int updateSelectedDive() const;
	void setUpdateSelectedDive(int idx);

	int selectedDiveTimestamp() const;
	void setSelectedDiveTimestamp(int when);

	QString progressMessage() const;
	void setProgressMessage(QString text);

	bool btEnabled() const;
	void setBtEnabled(bool value);

	void setShowNonDiveComputers(bool show);

	DiveListSortModel *dlSortModel;

	QStringList suitList() const;
	QStringList buddyList() const;
	QStringList divemasterList() const;
	QStringList locationList() const;
	QStringList cylinderInit() const;
	Q_INVOKABLE void setStatusbarColor(QColor color);
	void btHostModeChange(QBluetoothLocalDevice::HostMode state);
	QObject *qmlWindow;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	void writeToAppLogFile(QString logText);
#endif

public slots:
	void appInitialized();
	void applicationStateChanged(Qt::ApplicationState state);
	void saveCloudCredentials(const QString &email, const QString &password);
	bool verifyCredentials(QString email, QString password, QString pin);
	void tryRetrieveDataFromBackend();
	void handleError(QNetworkReply::NetworkError nError);
	void handleSslErrors(const QList<QSslError> &errors);
	void retrieveUserid();
	void loadDivesWithValidCredentials();
	void provideAuth(QNetworkReply *reply, QAuthenticator *auth);
	void commitChanges(QString diveId, QString number, QString date, QString location, QString gps,
			   QString duration, QString depth, QString airtemp,
			   QString watertemp, QString suit, QString buddy,
			   QString diveMaster, QString weight, QString notes, QStringList startpressure,
			   QStringList endpressure, QStringList gasmix, QStringList usedCylinder, int rating, int visibility, QString state);
	void changesNeedSaving();
	void openNoCloudRepo();
	void saveChangesLocal();
	void saveChangesCloud(bool forceRemoteSync);
	void selectDive(int id);
	void deleteDive(int id);
	void copyDiveData(int id);
	void pasteDiveData(int id);
	bool toggleDiveSite(bool toggle);
	bool toggleNotes(bool toggle);
	bool toggleDiveMaster(bool toggle);
	bool toggleBuddy(bool toggle);
	bool toggleSuit(bool toggle);
	bool toggleRating(bool toggle);
	bool toggleVisibility(bool toggle);
	bool toggleTags(bool toggle);
	bool toggleCylinders(bool toggle);
	bool toggleWeights(bool toggle);
	bool undoDelete(int id);
	QString addDive();
	void addDiveAborted(int id);
	void applyGpsData();
	void populateGpsData();
	void cancelDownloadDC();
	void clearGpsData();
	QString getCombinedLogs();
	void copyAppLogToClipboard();
	bool createSupportEmail();
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
	void showDownloadPage(QString deviceString);

private:
	BuddyCompletionModel buddyModel;
	SuitCompletionModel suitModel;
	DiveMasterCompletionModel divemasterModel;
	DiveSiteSortedModel locationModel;
	QString m_startPageText;
	QString m_logText;
	QString m_lastError;
	bool m_locationServiceEnabled;
	bool m_locationServiceAvailable;
	bool m_verboseEnabled;
	GpsLocation *locationProvider;
	bool m_loadFromCloud;
	static QMLManager *m_instance;
	struct dive *deletedDive;
	struct dive_trip *deletedTrip;
	QString m_notificationText;
	int m_updateSelectedDive;
	int m_selectedDiveTimestamp;
	qreal m_lastDevicePixelRatio;
	QElapsedTimer timer;
	bool alreadySaving;
	bool checkDate(const DiveObjectHelper &myDive, struct dive *d, QString date);
	bool checkLocation(const DiveObjectHelper &myDive, struct dive *d, QString location, QString gps);
	bool checkDuration(const DiveObjectHelper &myDive, struct dive *d, QString duration);
	bool checkDepth(const DiveObjectHelper &myDive, struct dive *d, QString depth);
	bool currentGitLocalOnly;
	Q_INVOKABLE DCDeviceData *m_device_data;
	QString m_progressMessage;
	bool m_btEnabled;
	void updateAllGlobalLists();
	void updateSiteList();
	void setupDivesite(struct dive *d, struct dive_site *ds, double lat, double lon, const char *locationtext);
	QString m_pluggedInDeviceName;
	bool m_showNonDiveComputers;
	struct dive *m_copyPasteDive = NULL;
	struct dive_components what;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	QString appLogFileName;
	QFile appLogFile;
	bool appLogFileOpen;
#endif

signals:
	void locationServiceEnabledChanged();
	void locationServiceAvailableChanged();
	void verboseEnabledChanged();
	void logTextChanged();
	void loadFromCloudChanged();
	void startPageTextChanged();
	void notificationTextChanged();
	void updateSelectedDiveChanged();
	void selectedDiveTimestampChanged();
	void sendScreenChanged(QScreen *screen);
	void progressMessageChanged();
	void btEnabledChanged();
	void suitListChanged();
	void buddyListChanged();
	void divemasterListChanged();
	void locationListChanged();
	void waitingForPositionChanged();
	void pluggedInDeviceNameChanged();
	void showNonDiveComputersChanged();
	void DC_ForceDownloadChanged();

	// From upload process
	void uploadFinish(bool success, const QString &text);
	void uploadProgress(qreal percentage);

private slots:
	void uploadFinishSlot(bool success, const QString &text, const QByteArray &html);
};

#endif
