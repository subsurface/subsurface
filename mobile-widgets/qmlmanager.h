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
#include "qt-models/completionmodels.h"
#include "qt-models/divelocationmodel.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/subsurface-qt/divelistnotifier.h"

#if defined(Q_OS_ANDROID)
#include "core/serial_usb_android.h"
#endif

class QAction;
struct DiveSiteChange; // An obscure implementation artifact - remove in due course.

class QMLManager : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString logText READ logText WRITE setLogText NOTIFY logTextChanged)
	Q_PROPERTY(bool locationServiceEnabled MEMBER m_locationServiceEnabled WRITE setLocationServiceEnabled NOTIFY locationServiceEnabledChanged)
	Q_PROPERTY(bool locationServiceAvailable MEMBER m_locationServiceAvailable WRITE setLocationServiceAvailable NOTIFY locationServiceAvailableChanged)
	Q_PROPERTY(bool loadFromCloud MEMBER m_loadFromCloud WRITE setLoadFromCloud NOTIFY loadFromCloudChanged)
	Q_PROPERTY(QString startPageText MEMBER m_startPageText WRITE setStartPageText NOTIFY startPageTextChanged)
	Q_PROPERTY(bool verboseEnabled MEMBER m_verboseEnabled WRITE setVerboseEnabled NOTIFY verboseEnabledChanged)
	Q_PROPERTY(QString notificationText MEMBER m_notificationText WRITE setNotificationText NOTIFY notificationTextChanged)
	Q_PROPERTY(QStringList suitList READ suitList NOTIFY suitListChanged)
	Q_PROPERTY(QStringList buddyList READ buddyList NOTIFY buddyListChanged)
	Q_PROPERTY(QStringList divemasterList READ divemasterList NOTIFY divemasterListChanged)
	Q_PROPERTY(QStringList locationList READ locationList NOTIFY locationListChanged)
	Q_PROPERTY(QStringList cylinderInit READ cylinderInit CONSTANT)
	Q_PROPERTY(QStringList cloudCacheList READ cloudCacheList NOTIFY cloudCacheListChanged)
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
	Q_PROPERTY(qPrefCloudStorage::cloud_status oldStatus MEMBER m_oldStatus WRITE setOldStatus NOTIFY oldStatusChanged)
	Q_PROPERTY(QString undoText READ getUndoText NOTIFY undoTextChanged) // this is a read-only property
	Q_PROPERTY(QString redoText READ getRedoText NOTIFY redoTextChanged) // this is a read-only property
	Q_PROPERTY(bool diveListProcessing MEMBER m_diveListProcessing  WRITE setDiveListProcessing NOTIFY diveListProcessingChanged)
	Q_PROPERTY(bool initialized MEMBER m_initialized NOTIFY initializedChanged)
	Q_PROPERTY(QString syncState READ getSyncState NOTIFY syncStateChanged)

public:
	QMLManager();
	~QMLManager();

	enum export_types {
		EX_DIVES_XML,
		EX_DIVE_SITES_XML,
		EX_UDDF,
		EX_DIVELOGS_DE,
		EX_DIVESHARE
	};
	Q_ENUM(export_types)
#if !defined(Q_OS_ANDROID)
	Q_INVOKABLE void exportToFile(export_types type, QString directory, bool anonymize);
#endif
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

	QString getUndoText() const;
	QString getRedoText() const;

	Q_INVOKABLE QStringList getProductListFromVendor(const QString& vendor);
	Q_INVOKABLE int getMatchingAddress(const QString &vendor, const QString &product);
	Q_INVOKABLE int getDetectedVendorIndex();
	Q_INVOKABLE int getDetectedProductIndex(const QString &currentVendorText);
	Q_INVOKABLE int getConnectionIndex(const QString &deviceSubstr);
	Q_INVOKABLE void setGitLocalOnly(const bool &value);
	Q_INVOKABLE void setFilter(const QString filterText, int mode);
	Q_INVOKABLE void selectRow(int row);
	Q_INVOKABLE void selectSwipeRow(int row);
	Q_INVOKABLE void importCacheRepo(QString repo);

	static QMLManager *instance();
	Q_INVOKABLE void registerError(QString error);
	QString consumeError();

	bool locationServiceEnabled() const;
	void setLocationServiceEnabled(bool locationServiceEnable);

	bool locationServiceAvailable() const;
	void setLocationServiceAvailable(bool locationServiceAvailable);

	bool verboseEnabled() const;
	void setVerboseEnabled(bool verboseMode);

	void setLoadFromCloud(bool done);
	void syncLoadFromCloud();

	QString startPageText() const;
	void setStartPageText(const QString& text);

	QString logText() const;
	void setLogText(const QString &logText);

	QString notificationText() const;
	void setNotificationText(QString text);

	QString progressMessage() const;
	void setProgressMessage(QString text);

	bool btEnabled() const;
	void setBtEnabled(bool value);

	void setShowNonDiveComputers(bool show);
	qreal lastDevicePixelRatio();
	void setDevicePixelRatio(qreal dpr, QScreen *screen);

	void setDiveListProcessing(bool value);

	QStringList suitList() const;
	QStringList buddyList() const;
	QStringList divemasterList() const;
	QStringList locationList() const;
	QStringList cylinderInit() const;
	QStringList cloudCacheList() const;
	Q_INVOKABLE void setStatusbarColor(QColor color);
	void btHostModeChange(QBluetoothLocalDevice::HostMode state);
	QObject *qmlWindow;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	void writeToAppLogFile(QString logText);
#endif
	qPrefCloudStorage::cloud_status oldStatus() const;
	void setOldStatus(const qPrefCloudStorage::cloud_status value);
	void rememberOldStatus();

	QString getSyncState() const;

public slots:
	void appInitialized();
	void applicationStateChanged(Qt::ApplicationState state);
	void saveCloudCredentials(const QString &email, const QString &password, const QString &pin);
	void commitChanges(QString diveId, QString number, QString date, QString location, QString gps,
			   QString duration, QString depth, QString airtemp,
			   QString watertemp, QString suit, QString buddy,
			   QString diveMaster, QString weight, QString notes, QStringList startpressure,
			   QStringList endpressure, QStringList gasmix, QStringList usedCylinder, int rating, int visibility, QString state);
	void updateTripDetails(QString tripIdString, QString tripLocation, QString tripNotes);
	void removeDiveFromTrip(int id);
	void addTripForDive(int id);
	void addDiveToTrip(int id, int tripId);
	void changesNeedSaving(bool fromUndo = false);
	void openNoCloudRepo();
	void saveChangesCloud(bool forceRemoteSync, bool fromUndo = false);
	void selectDive(int id);
	void deleteDive(int id);
	void toggleDiveInvalid(int id);
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
	void undo();
	void redo();
	int addDive();
	void applyGpsData();
	void populateGpsData();
	void cancelDownloadDC();
	void clearGpsData();
	QString getCombinedLogs();
	void copyAppLogToClipboard();
	void copyGpsFixesToClipboard();
	bool createSupportEmail();
	void finishSetup();
	QString getNumber(const QString& diveId);
	QString getDate(const QString& diveId);
	QString getCurrentPosition();
	QString getGpsFromSiteName(const QString& siteName);
	QString getVersion() const;
	void deleteGpsFix(quint64 when);
	void refreshDiveList();
	void screenChanged(QScreen *screen);
	void appendTextToLog(const QString &newText);
	void quit();
	void hasLocationSourceChanged();
	void btRescan();
	void usbRescan();
	void rescanConnections();
#if defined(Q_OS_ANDROID)
	void showDownloadPage(QAndroidJniObject usbDevice);
	void restartDownload(QAndroidJniObject usbDevice);
	void androidUsbPopoulateConnections();
	QString getProductVendorConnectionIdx(android_usb_serial_device_descriptor descriptor);
#endif
	void divesChanged(const QVector<dive *> &dives, DiveField field);

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
	bool m_diveListProcessing;
	bool m_initialized;
	bool m_loadFromCloud;
	static QMLManager *m_instance;
	QString m_notificationText;
	qreal m_lastDevicePixelRatio;
	QElapsedTimer timer;
	bool checkDate(struct dive *d, QString date);
	bool checkLocation(DiveSiteChange &change, struct dive *d, QString location, QString gps);
	bool checkDuration(struct dive *d, QString duration);
	bool checkDepth(struct dive *d, QString depth);
	bool currentGitLocalOnly;
	bool localChanges;
	QString m_progressMessage;
	bool m_btEnabled;
	void updateAllGlobalLists();
	void updateHaveLocalChanges(bool status);

	location_t getGps(QString &gps);
	QString m_pluggedInDeviceName;
	bool m_showNonDiveComputers;
	struct dive *m_copyPasteDive = NULL;
	struct dive_components what;
	QAction *undoAction;

	bool verifyCredentials(QString email, QString password, QString pin);
	void loadDivesWithValidCredentials();
	void revertToNoCloudIfNeeded();
	void consumeFinishedLoad();
	void mergeLocalRepo();
	void openLocalThenRemote(QString url);
	void saveChangesLocal(bool fromUndo = false);

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	QString appLogFileName;
	QFile appLogFile;
	bool appLogFileOpen;
#endif
	qPrefCloudStorage::cloud_status m_oldStatus;

signals:
	void locationServiceEnabledChanged();
	void locationServiceAvailableChanged();
	void verboseEnabledChanged();
	void diveListProcessingChanged();
	void initializedChanged();
	void logTextChanged();
	void loadFromCloudChanged();
	void startPageTextChanged();
	void notificationTextChanged();
	void sendScreenChanged(QScreen *screen);
	void progressMessageChanged();
	void btEnabledChanged();
	void suitListChanged();
	void buddyListChanged();
	void divemasterListChanged();
	void locationListChanged();
	void cloudCacheListChanged();
	void waitingForPositionChanged();
	void pluggedInDeviceNameChanged();
	void showNonDiveComputersChanged();
	void DC_ForceDownloadChanged();
	void oldStatusChanged();
	void undoTextChanged();
	void redoTextChanged();
	void restartDownloadSignal();
	void syncStateChanged();

	// From upload process
	void uploadFinish(bool success, const QString &text);
	void uploadProgress(qreal percentage);

private slots:
	void uploadFinishSlot(bool success, const QString &text, const QByteArray &html);
};

#endif
