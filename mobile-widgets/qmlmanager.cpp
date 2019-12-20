// SPDX-License-Identifier: GPL-2.0
#include "qmlmanager.h"
#include "qmlprefs.h"
#include <QUrl>
#include <QSettings>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QAuthenticator>
#include <QDesktopServices>
#include <QTextDocument>
#include <QRegularExpression>
#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <QDateTime>
#include <QClipboard>
#include <QFile>
#include <QtConcurrent>
#include <QFuture>

#include <QBluetoothLocalDevice>

#include "qt-models/divelistmodel.h"
#include "qt-models/gpslistmodel.h"
#include "qt-models/completionmodels.h"
#include "qt-models/messagehandlermodel.h"
#include "qt-models/tankinfomodel.h"
#include "core/device.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/qthelper.h"
#include "core/qt-gui.h"
#include "core/git-access.h"
#include "core/cloudstorage.h"
#include "core/membuffer.h"
#include "core/downloadfromdcthread.h"
#include "core/subsurface-string.h"
#include "core/pref.h"
#include "core/selection.h"
#include "core/ssrf.h"
#include "core/save-profiledata.h"
#include "core/settings/qPrefGeneral.h"
#include "core/settings/qPrefLocationService.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include "core/settings/qPrefUnit.h"
#include "core/trip.h"
#include "backend-shared/exportfuncs.h"
#include "core/worldmap-save.h"
#include "core/uploadDiveLogsDE.h"
#include "core/uploadDiveShare.h"


QMLManager *QMLManager::m_instance = NULL;
bool noCloudToCloud = false;

#define RED_FONT QLatin1Literal("<font color=\"red\">")
#define END_FONT QLatin1Literal("</font>")

extern "C" void showErrorFromC(char *buf)
{
	QString error(buf);
	free(buf);
	// By using invokeMethod with Qt:AutoConnection, the error string is safely
	// transported across thread boundaries, if not called from the UI thread.
	QMetaObject::invokeMethod(QMLManager::instance(), "registerError", Qt::AutoConnection, Q_ARG(QString, error));
}

static void progressCallback(const char *text)
{
	QMLManager *self = QMLManager::instance();
	if (self) {
		self->appendTextToLog(QString(text));
		self->setProgressMessage(QString(text));
	}
}

static void appendTextToLogStandalone(const char *text)
{
	QMLManager *self = QMLManager::instance();
	if (self)
		self->appendTextToLog(QString(text));
}

// show the git progress in the passive notification area
extern "C" int gitProgressCB(const char *text)
{
	static QElapsedTimer timer;
	static qint64 lastTime = 0;
	static QMLManager *self;

	if (!self)
		self = QMLManager::instance();

	if (!timer.isValid()) {
		timer.restart();
		lastTime = 0;
	}
	if (self) {
		qint64 elapsed = timer.elapsed();
		self->appendTextToLog(text);
		self->setNotificationText(text);
		if (elapsed - lastTime > 50) { // 20 Hz refresh
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
		lastTime = elapsed;
	}
	// return 0 so that we don't end the download
	return 0;
}

void QMLManager::registerError(QString error)
{
	appendTextToLog(error);
	if (!m_lastError.isEmpty())
		m_lastError += '\n';
	m_lastError += error;
}

QString QMLManager::consumeError()
{
	QString ret;
	ret.swap(m_lastError);
	return ret;
}

void QMLManager::btHostModeChange(QBluetoothLocalDevice::HostMode state)
{
	BTDiscovery *btDiscovery = BTDiscovery::instance();

	qDebug() << "btHostModeChange to " << state;
	if (state != QBluetoothLocalDevice::HostPoweredOff) {
		connectionListModel.removeAllAddresses();
		btDiscovery->BTDiscoveryReDiscover();
		m_btEnabled = btDiscovery->btAvailable();
	} else {
		connectionListModel.removeAllAddresses();
		set_non_bt_addresses();
		m_btEnabled = false;
	}
	emit btEnabledChanged();
}

void QMLManager::btRescan()
{
	BTDiscovery::instance()->BTDiscoveryReDiscover();
}

QMLManager::QMLManager() : m_locationServiceEnabled(false),
	m_verboseEnabled(false),
	deletedDive(0),
	deletedTrip(0),
	m_updateSelectedDive(-1),
	m_selectedDiveTimestamp(0),
	alreadySaving(false),
	m_pluggedInDeviceName(""),
	m_showNonDiveComputers(false)
{
	m_instance = this;
	m_lastDevicePixelRatio = qApp->devicePixelRatio();
	timer.start();
	connect(qobject_cast<QApplication *>(QApplication::instance()), &QApplication::applicationStateChanged, this, &QMLManager::applicationStateChanged);

	// make upload signals available in QML
	connect(uploadDiveLogsDE::instance(), &uploadDiveLogsDE::uploadFinish,
			this, &QMLManager::uploadFinish);
	connect(uploadDiveLogsDE::instance(), &uploadDiveLogsDE::uploadProgress,
			this, &QMLManager::uploadProgress);
	connect(uploadDiveShare::instance(), &uploadDiveShare::uploadProgress,
			this, &QMLManager::uploadProgress);

	// uploadDiveShare::uploadFinish() is defined with 3 parameters,
	// whereas QMLManager::uploadFinish() is defined with 2 paramters,
	// Solution add a slot as landing zone.
	connect(uploadDiveShare::instance(), SIGNAL(uploadDiveShare::uploadFinish(bool, const QString &, const QByteArray &)),
			this, SLOT(uploadFinishSlot(bool, const QString &, const QByteArray &)));

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#if defined(Q_OS_ANDROID)
	// on Android we first try the GenericDataLocation (typically /storage/emulated/0) and if that fails
	// (as happened e.g. on a Sony Xperia phone) we try several other default locations, with the TempLocation as last resort
	QStringList fileLocations =
		QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation) +
		QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation) +
		QStandardPaths::standardLocations(QStandardPaths::DownloadLocation) +
		QStandardPaths::standardLocations(QStandardPaths::TempLocation);
#elif defined(Q_OS_IOS)
	// on iOS we should save the data to the DocumentsLocation so it becomes accessible to the user
	QStringList fileLocations =
		QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
#endif
	appLogFileOpen = false;
	for (const QString &fileLocation : fileLocations) {
		appLogFileName = fileLocation + "/subsurface.log";
		appLogFile.setFileName(appLogFileName);
		if (!appLogFile.open(QIODevice::ReadWrite|QIODevice::Truncate)) {
			appendTextToLog("Failed to open logfile " + appLogFileName
					+ " at " + QDateTime::currentDateTime().toString()
					+ " error: " + appLogFile.errorString());
		} else {
			// found a directory that works
			appLogFileOpen = true;
			break;
		}
	}
	if (appLogFileOpen) {
		appendTextToLog("Successfully opened logfile " + appLogFileName
				+ " at " + QDateTime::currentDateTime().toString());
		// if we were able to write the overall logfile, also write the libdivecomputer logfile
		QString libdcLogFileName = appLogFileName.replace("/subsurface.log", "/libdivecomputer.log");
		// remove the existing libdivecomputer logfile so we don't copy an old one by mistake
		QFile libdcLog(libdcLogFileName);
		libdcLog.remove();
		logfile_name = copy_qstring(libdcLogFileName);
	} else {
		appendTextToLog("No writeable location found, in-memory log only and no libdivecomputer log");
	}
#endif
	set_error_cb(&showErrorFromC);
	appendTextToLog("Starting " + getUserAgent());
	appendTextToLog(QStringLiteral("built with libdivecomputer v%1").arg(dc_version(NULL)));
	appendTextToLog(QStringLiteral("built with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion()));
	int git_maj, git_min, git_rev;
	git_libgit2_version(&git_maj, &git_min, &git_rev);
	appendTextToLog(QStringLiteral("built with libgit2 %1.%2.%3").arg(git_maj).arg(git_min).arg(git_rev));
	appendTextToLog(QStringLiteral("Running on %1").arg(QSysInfo::prettyProductName()));
#if defined(Q_OS_ANDROID)
	extern QString getAndroidHWInfo();
	appendTextToLog(getAndroidHWInfo());
#endif
	setStartPageText(tr("Starting..."));

	// ensure that we start the BTDiscovery - this should be triggered by the export of the class
	// to QML, but that doesn't seem to always work
	BTDiscovery *btDiscovery = BTDiscovery::instance();
	m_btEnabled = btDiscovery->btAvailable();
	connect(&btDiscovery->localBtDevice, &QBluetoothLocalDevice::hostModeStateChanged,
		this, &QMLManager::btHostModeChange);
	QMLPrefs::instance()->setShowPin(false);
	// create location manager service
	locationProvider = new GpsLocation(&appendTextToLogStandalone, this);
	progress_callback = &progressCallback;
	connect(locationProvider, SIGNAL(haveSourceChanged()), this, SLOT(hasLocationSourceChanged()));
	setLocationServiceAvailable(locationProvider->hasLocationsSource());
	set_git_update_cb(&gitProgressCB);

	// present dive site lists sorted by name
	locationModel.sort(LocationInformationModel::NAME);

	// make sure we know if the current cloud repo has been successfully synced
	syncLoadFromCloud();

	memset(&m_copyPasteDive, 0, sizeof(m_copyPasteDive));
	memset(&what, 0, sizeof(what));

	// Let's set some defaults to be copied so users don't necessarily need
	// to know how to configure this
	what.divemaster = true;
	what.buddy = true;
	what.suit = true;
	what.tags = true;
	what.cylinders = true;
	what.weights = true;
}

void QMLManager::applicationStateChanged(Qt::ApplicationState state)
{
	QString stateText;
	switch (state) {
	case Qt::ApplicationActive: stateText = "active"; break;
	case Qt::ApplicationHidden: stateText = "hidden"; break;
	case Qt::ApplicationSuspended: stateText = "suspended"; break;
	case Qt::ApplicationInactive: stateText = "inactive"; break;
	default: stateText = QString("none of the four: 0x") + QString::number(state, 16);
	}
	stateText.prepend("AppState changed to ");
	stateText.append(" with ");
	stateText.append((alreadySaving ? QLatin1Literal("") : QLatin1Literal("no ")) + QLatin1Literal("save ongoing"));
	stateText.append(" and ");
	stateText.append((unsaved_changes() ? QLatin1Literal("") : QLatin1Literal("no ")) + QLatin1Literal("unsaved changes"));
	appendTextToLog(stateText);

	if (!alreadySaving && state == Qt::ApplicationInactive && unsaved_changes()) {
		// FIXME
		//       make sure the user sees that we are saving data if they come back
		//       while this is running
		saveChangesCloud(false);
		appendTextToLog("done saving to git local / remote");
	}
}

void QMLManager::openLocalThenRemote(QString url)
{
	CollapsedDiveListSortModel::instance()->setSourceModel(nullptr);
	DiveListModel::instance()->clear();
	setNotificationText(tr("Open local dive data file"));
	QByteArray fileNamePrt = QFile::encodeName(url);
	/* if this is a cloud storage repo and we have no local cache (i.e., it's the first time
	 * we try to open this), parse_file will ALWAYS connect to the remote and populate the cache.
	 * Otherwise parse_file will respect the git_local_only flag and only update if that isn't set */
	int error = parse_file(fileNamePrt.data(), &dive_table, &trip_table, &dive_site_table);
	if (error) {
		appendTextToLog(QStringLiteral("loading dives from cache failed %1").arg(error));
		setNotificationText(tr("Opening local data file failed"));
		/* there can be 2 reasons for this:
		 * 1) we have cloud credentials, but there is no local repo (yet).
		 *    This implies that the PIN verify is still to be done.
		 * 2) we are in a very clean state after installing the app, and
		 *    want to use a NO CLOUD setup. The intial repo has no initial
		 *    commit in it, so its master branch does not yet exist. We do not
		 *    care about this, as the very first commit of dive data to the
		 *    no cloud repo solves this.
		 */
		auto credStatus = QMLPrefs::instance()->credentialStatus();
		if (credStatus != qPrefCloudStorage::CS_NOCLOUD &&
		    credStatus != qPrefCloudStorage::CS_INCORRECT_USER_PASSWD)
			QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_NEED_TO_VERIFY);
	} else {
		// if we can load from the cache, we know that we have a valid cloud account
		// and we know that there was at least one successful sync with the cloud when
		// that local cache was created - so there is a common ancestor
		setLoadFromCloud(true);
		if (QMLPrefs::instance()->credentialStatus() == qPrefCloudStorage::CS_UNKNOWN)
			QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_VERIFIED);
		if (git_prefs.unit_system == IMPERIAL)
			qPrefUnits::set_unit_system("imperial");
		else if (git_prefs.unit_system == METRIC)
			qPrefUnits::set_unit_system("metric");
		qPrefTechnicalDetails::set_tankbar(git_prefs.tankbar);
		qPrefTechnicalDetails::set_dcceiling(git_prefs.dcceiling);
		qPrefTechnicalDetails::set_show_ccr_setpoint(git_prefs.show_ccr_setpoint);
		qPrefTechnicalDetails::set_show_ccr_sensors(git_prefs.show_ccr_sensors);
		qPrefPartialPressureGas::set_po2(git_prefs.pp_graphs.po2);
		process_loaded_dives();
		DiveListModel::instance()->reload();
		CollapsedDiveListSortModel::instance()->setSourceModel(DiveListModel::instance());
		CollapsedDiveListSortModel::instance()->updateFilterState();
		appendTextToLog(QStringLiteral("%1 dives loaded from cache").arg(dive_table.nr));
		setNotificationText(tr("%1 dives loaded from local dive data file").arg(dive_table.nr));
	}
	if (QMLPrefs::instance()->credentialStatus() == qPrefCloudStorage::CS_NEED_TO_VERIFY) {
		appendTextToLog(QStringLiteral("have cloud credentials, but still needs PIN"));
		QMLPrefs::instance()->setShowPin(true);
	}
	if (QMLPrefs::instance()->credentialStatus() == qPrefCloudStorage::CS_INCORRECT_USER_PASSWD) {
		appendTextToLog(QStringLiteral("incorrect password for cloud credentials"));
		setNotificationText(tr("Incorrect cloud credentials"));
	}
	if (QMLPrefs::instance()->oldStatus() == qPrefCloudStorage::CS_NOCLOUD) {
		// if we switch to credentials from CS_NOCLOUD, we take things online temporarily
		git_local_only = false;
		appendTextToLog(QStringLiteral("taking things online to be able to switch to cloud account"));
	}
	set_filename(fileNamePrt.data());
	if (git_local_only) {
		appendTextToLog(QStringLiteral("have cloud credentials, but user asked not to connect to network"));
		alreadySaving = false;
	} else {
		appendTextToLog(QStringLiteral("have cloud credentials, trying to connect"));
		tryRetrieveDataFromBackend();
	}
	updateAllGlobalLists();
}

void QMLManager::updateSiteList()
{
	LocationInformationModel::instance()->update();
	emit locationListChanged();
}

void QMLManager::updateAllGlobalLists()
{
	buddyModel.updateModel(); emit buddyListChanged();
	suitModel.updateModel(); emit suitListChanged();
	divemasterModel.updateModel(); emit divemasterListChanged();
	// TODO: Probably not needed anymore, as the dive site list is generated on the fly!
	updateSiteList();
}

void QMLManager::mergeLocalRepo()
{
	char *filename = NOCLOUD_LOCALSTORAGE;
	struct dive_table table = { 0 };
	struct trip_table trips = { 0 };
	struct dive_site_table sites = { 0 };
	parse_file(filename, &table, &trips, &sites);
	add_imported_dives(&table, &trips, &sites, IMPORT_MERGE_ALL_TRIPS);
}

void QMLManager::copyAppLogToClipboard()
{
	// The About page offers a button to copy logs so they can be pasted elsewhere
	QApplication::clipboard()->setText(getCombinedLogs(), QClipboard::Clipboard);
}

bool QMLManager::createSupportEmail()
{
	QString mailToLink = "mailto:in-app-support@subsurface-divelog.org?subject=Subsurface-mobile support request";
	mailToLink += "&body=Please describe your issue here and keep the logs below:\n\n\n\n";
	mailToLink += getCombinedLogs();
	if (QDesktopServices::openUrl(QUrl(mailToLink))) {
		appendTextToLog("OS accepted support email");
		return true;
	}
	appendTextToLog("failed to create support email");
	return false;
}

// useful for support requests
QString QMLManager::getCombinedLogs()
{
	// Add heading and append subsurface.log
	QString copyString = "\n---------- subsurface.log ----------\n";
	copyString += MessageHandlerModel::self()->logAsString();

	// Add heading and append libdivecomputer.log
	QFile f(logfile_name);
	if (f.open(QFile::ReadOnly | QFile::Text)) {
		copyString += "\n\n\n---------- libdivecomputer.log ----------\n";

		QTextStream in(&f);
		copyString += in.readAll();
	}

	copyString += "---------- finish ----------\n";

#if defined(Q_OS_ANDROID)
	// on Android, the clipboard is effectively limited in size, but there is no
	// predefined hard limit. All remote procedure calls use a shared Binder
	// transaction buffer that is limited to 1MB. To work around this let's truncate
	// the log once it is more than half a million characters. Qt doesn't tell us if
	// the clipboard transaction fails, hopefully this will typically leave enough
	// margin of error.
	if (copyString.size() > 500000) {
		copyString.truncate(500000);
		copyString += "\n\n---------- truncated ----------\n";
	}
#endif
	return copyString;
}

void QMLManager::finishSetup()
{
	// Initialize cloud credentials.
	QMLPrefs::instance()->setCloudPassword(qPrefCloudStorage::cloud_storage_password());
	git_local_only = !prefs.cloud_auto_sync;
	QMLPrefs::instance()->setCredentialStatus((qPrefCloudStorage::cloud_status) prefs.cloud_verification_status);
	// if the cloud credentials are valid, we should get the GPS Webservice ID as well
	QString url;
	if (!qPrefCloudStorage::cloud_storage_email().isEmpty() &&
	    !QMLPrefs::instance()->cloudPassword().isEmpty() &&
	    getCloudURL(url) == 0) {
		// we know that we are the first ones to access git storage, so we don't need to test,
		// but we need to make sure we stay the only ones accessing git storage
		alreadySaving = true;
		openLocalThenRemote(url);
	} else if (!empty_string(existing_filename) &&
				QMLPrefs::instance()->credentialStatus() != qPrefCloudStorage::CS_UNKNOWN) {
		QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_NOCLOUD);
		saveCloudCredentials(qPrefCloudStorage::cloud_storage_email(), qPrefCloudStorage::cloud_storage_password());
		appendTextToLog(tr("working in no-cloud mode"));
		int error = parse_file(existing_filename, &dive_table, &trip_table, &dive_site_table);
		if (error) {
			// we got an error loading the local file
			setNotificationText(tr("Error parsing local storage, giving up"));
			set_filename(NULL);
		} else {
			// successfully opened the local file, now add thigs to the dive list
			consumeFinishedLoad(0);
			appendTextToLog(QString("working in no-cloud mode, finished loading %1 dives from %2").arg(dive_table.nr).arg(existing_filename));
		}
	} else {
		QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_UNKNOWN);
		appendTextToLog(tr("no cloud credentials"));
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	}
}

QMLManager::~QMLManager()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	if (appLogFileOpen)
		appLogFile.close();
#endif
	m_instance = NULL;
}

QMLManager *QMLManager::instance()
{
	return m_instance;
}

#define CLOUDURL QString(prefs.cloud_base_url)
#define CLOUDREDIRECTURL CLOUDURL + "/cgi-bin/redirect.pl"

void QMLManager::saveCloudCredentials(const QString &newEmail, const QString &newPassword)
{
	bool cloudCredentialsChanged = false;
	bool noCloud = QMLPrefs::instance()->credentialStatus() == qPrefCloudStorage::CS_NOCLOUD;

	QMLPrefs::instance()->setCloudPassword(newPassword);

	// make sure we only have letters, numbers, and +-_. in password and email address
	QRegularExpression regExp("^[a-zA-Z0-9@.+_-]+$");
	if (!noCloud) {
		// in case of NO_CLOUD, the email address + passwd do not care, so do not check it.
		if (QMLPrefs::instance()->cloudPassword().isEmpty() ||
			!regExp.match(QMLPrefs::instance()->cloudPassword()).hasMatch() ||
			!regExp.match(newEmail).hasMatch()) {
			setStartPageText(RED_FONT + tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.") + END_FONT);
			return;
		}
		// use the same simplistic regex as the backend to check email addresses
		regExp = QRegularExpression("^[a-zA-Z0-9.+_-]+@[a-zA-Z0-9.+_-]+\\.[a-zA-Z0-9]+");
		if (!regExp.match(newEmail).hasMatch()) {
			setStartPageText(RED_FONT + tr("Invalid format for email address") + END_FONT);
			return;
		}
	}
	if (!same_string(prefs.cloud_storage_email, qPrintable(newEmail))) {
		cloudCredentialsChanged = true;
	}

	cloudCredentialsChanged |= !same_string(prefs.cloud_storage_password,
						qPrintable(QMLPrefs::instance()->cloudPassword()));

	if (QMLPrefs::instance()->credentialStatus() != qPrefCloudStorage::CS_NOCLOUD &&
		!cloudCredentialsChanged) {
		// just go back to the dive list
		QMLPrefs::instance()->setCredentialStatus(QMLPrefs::instance()->oldStatus());
	}

	if (!noCloud &&
	    !verifyCredentials(newEmail, QMLPrefs::instance()->cloudPassword(), QMLPrefs::instance()->cloudPin()))
		return;

	qPrefCloudStorage::set_cloud_storage_email(newEmail);
	qPrefCloudStorage::set_cloud_storage_password(QMLPrefs::instance()->cloudPassword());
	qPrefCloudStorage::set_cloud_verification_status(QMLPrefs::instance()->credentialStatus());

	if (!same_string(prefs.cloud_storage_password,
					qPrintable(QMLPrefs::instance()->cloudPassword()))) {
		free((void *)prefs.cloud_storage_password);
		prefs.cloud_storage_password = copy_qstring(QMLPrefs::instance()->cloudPassword());
	}
	if (noCloud && cloudCredentialsChanged && dive_table.nr) {
		// we came from NOCLOUD and are connecting to a cloud account;
		// since we already have dives in the table, let's remember that so we can keep them
		noCloudToCloud = true;
		appendTextToLog("transitioning from no-cloud to cloud and have dives");
	}
	if (qPrefCloudStorage::cloud_storage_email().isEmpty() ||
		QMLPrefs::instance()->cloudPassword().isEmpty()) {
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	} else if (cloudCredentialsChanged) {
		// let's make sure there are no unsaved changes
		saveChangesLocal();
		syncLoadFromCloud();
		QString url;
		getCloudURL(url);
		manager()->clearAccessCache(); // remove any chached credentials
		clear_git_id(); // invalidate our remembered GIT SHA
		DiveListModel::instance()->reload();
		GpsListModel::instance()->clear();
		setStartPageText(tr("Attempting to open cloud storage with new credentials"));
		// we therefore know that no one else is already accessing THIS git repo;
		// let's make sure we stay the only ones doing so
		alreadySaving = true;
		// since we changed credentials, we need to try to connect to the cloud, regardless
		// of whether we're in offline mode or not, to make sure the repository is synced
		currentGitLocalOnly = git_local_only;
		git_local_only = false;
		openLocalThenRemote(url);
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY &&
				!QMLPrefs::instance()->cloudPin().isEmpty()) {
		// the user entered a PIN?
		tryRetrieveDataFromBackend();
	}
}

bool QMLManager::verifyCredentials(QString email, QString password, QString pin)
{
	setStartPageText(tr("Testing cloud credentials"));
	if (pin.isEmpty())
		appendTextToLog(QStringLiteral("verify credentials for email %1 (no PIN)").arg(email));
	else
		appendTextToLog(QStringLiteral("verify credentials for email %1 PIN %2").arg(email, pin));
	CloudStorageAuthenticate *csa = new CloudStorageAuthenticate(this);
	csa->backend(email, password, pin);
	// let's wait here for the signal to avoid too many more nested functions
	QTimer myTimer;
	myTimer.setSingleShot(true);
	QEventLoop loop;
	connect(csa, &CloudStorageAuthenticate::finishedAuthenticate, &loop, &QEventLoop::quit);
	connect(&myTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
	myTimer.start(5000);
	loop.exec();
	if (!myTimer.isActive()) {
		// got no response from the server
		setStartPageText(RED_FONT + tr("No response from cloud server to validate the credentials") + END_FONT);
		return false;
	}
	myTimer.stop();
	if (prefs.cloud_verification_status == qPrefCloudStorage::CS_INCORRECT_USER_PASSWD) {
		appendTextToLog(QStringLiteral("Incorrect email / password combination"));
		setStartPageText(RED_FONT + tr("Incorrect email / password combination") + END_FONT);
		return false;
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_NEED_TO_VERIFY) {
		if (pin.isEmpty()) {
			appendTextToLog(QStringLiteral("Cloud credentials require PIN entry"));
			setStartPageText(RED_FONT + tr("Cloud credentials require verification PIN") + END_FONT);
		} else {
			appendTextToLog(QStringLiteral("PIN provided but not accepted"));
			setStartPageText(RED_FONT + tr("Incorrect PIN, please try again") + END_FONT);
		}
		QMLPrefs::instance()->setShowPin(true);
		return false;
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED) {
		appendTextToLog(QStringLiteral("PIN accepted"));
		setStartPageText(RED_FONT + tr("PIN accepted, credentials verified") + END_FONT);
	}
	return true;
}

void QMLManager::tryRetrieveDataFromBackend()
{
	// if the cloud credentials are present, we should try to get the GPS Webservice ID
	// and (if we haven't done so) load the dive list
	if (!empty_string(prefs.cloud_storage_email) &&
	    !empty_string(prefs.cloud_storage_password)) {
		setStartPageText(tr("Testing cloud credentials"));
		appendTextToLog("Have credentials, let's see if they are valid");
		CloudStorageAuthenticate *csa = new CloudStorageAuthenticate(this);
		csa->backend(prefs.cloud_storage_email, prefs.cloud_storage_password,
						QMLPrefs::instance()->cloudPin());
		// let's wait here for the signal to avoid too many more nested functions
		QTimer myTimer;
		myTimer.setSingleShot(true);
		QEventLoop loop;
		connect(csa, &CloudStorageAuthenticate::finishedAuthenticate, &loop, &QEventLoop::quit);
		connect(&myTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
		myTimer.start(5000);
		loop.exec();
		if (!myTimer.isActive()) {
			// got no response from the server
			setStartPageText(RED_FONT + tr("No response from cloud server to validate the credentials") + END_FONT);
			revertToNoCloudIfNeeded();
			return;
		}
		myTimer.stop();
		QMLPrefs::instance()->setCloudPin("");
		if (prefs.cloud_verification_status == qPrefCloudStorage::CS_INCORRECT_USER_PASSWD) {
			appendTextToLog(QStringLiteral("Incorrect cloud credentials"));
			setStartPageText(RED_FONT + tr("Incorrect cloud credentials") + END_FONT);
			revertToNoCloudIfNeeded();
			return;
		} else if (prefs.cloud_verification_status != qPrefCloudStorage::CS_VERIFIED) {
			// here we need to enter the PIN
			appendTextToLog(QStringLiteral("Need to verify the email address - enter PIN"));
			setStartPageText(RED_FONT + tr("Cannot connect to cloud storage - cloud account not verified") + END_FONT);
			revertToNoCloudIfNeeded();
			QMLPrefs::instance()->setShowPin(true);
			return;
		}
		if (QMLPrefs::instance()->showPin())
			QMLPrefs::instance()->setShowPin(false);

		// now check the redirect URL to make sure everything is set up on the cloud server
		connect(manager(), &QNetworkAccessManager::authenticationRequired, this, &QMLManager::provideAuth, Qt::UniqueConnection);
		QUrl url(CLOUDREDIRECTURL);
		QNetworkRequest request(url);
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setRawHeader("Accept", "text/html");
		QNetworkReply *reply = manager()->get(request);
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleError(QNetworkReply::NetworkError)));
		connect(reply, &QNetworkReply::sslErrors, this, &QMLManager::handleSslErrors);
		connect(reply, &QNetworkReply::finished, this, &QMLManager::retrieveUserid);
	}
}

void QMLManager::provideAuth(QNetworkReply *reply, QAuthenticator *auth)
{
	if (auth->user() == QString(prefs.cloud_storage_email) &&
	    auth->password() == QString(prefs.cloud_storage_password)) {
		// OK, credentials have been tried and didn't work, so they are invalid
		appendTextToLog("Cloud credentials are invalid");
		setStartPageText(RED_FONT + tr("Cloud credentials are invalid") + END_FONT);
		QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_INCORRECT_USER_PASSWD);
		reply->disconnect();
		reply->abort();
		reply->deleteLater();
		return;
	}
	auth->setUser(prefs.cloud_storage_email);
	auth->setPassword(prefs.cloud_storage_password);
}

void QMLManager::handleSslErrors(const QList<QSslError> &errors)
{
	auto *reply = qobject_cast<QNetworkReply *>(sender());
	setStartPageText(RED_FONT + tr("Cannot open cloud storage: Error creating https connection") + END_FONT);
	for (QSslError e: errors) {
		appendTextToLog(e.errorString());
	}
	reply->abort();
	reply->deleteLater();
	setNotificationText(QStringLiteral(""));
}

void QMLManager::handleError(QNetworkReply::NetworkError nError)
{
	auto *reply = qobject_cast<QNetworkReply *>(sender());
	QString errorString = reply->errorString();
	appendTextToLog(QStringLiteral("handleError ") + nError + QStringLiteral(": ") + errorString);
	setStartPageText(RED_FONT + tr("Cannot open cloud storage: %1").arg(errorString) + END_FONT);
	reply->abort();
	reply->deleteLater();
	setNotificationText(QStringLiteral(""));
}

void QMLManager::retrieveUserid()
{
	auto *reply = qobject_cast<QNetworkReply *>(sender());
	if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 302) {
		appendTextToLog(QStringLiteral("Cloud storage connection not working correctly: (%1) %2")
				.arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
				.arg(QString(reply->readAll())));
		setStartPageText(RED_FONT + tr("Cannot connect to cloud storage") + END_FONT);
		revertToNoCloudIfNeeded();
		return;
	}
	QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_VERIFIED);
	setStartPageText(tr("Cloud credentials valid, loading dives..."));
	// this only gets called with "alreadySaving" already locked
	loadDivesWithValidCredentials();
}

void QMLManager::loadDivesWithValidCredentials()
{
	QString url;
	timestamp_t currentDiveTimestamp = m_selectedDiveTimestamp;
	if (getCloudURL(url)) {
		setStartPageText(RED_FONT + tr("Cloud storage error: %1").arg(consumeError()) + END_FONT);
		revertToNoCloudIfNeeded();
		return;
	}
	QByteArray fileNamePrt = QFile::encodeName(url);
	git_repository *git;
	const char *branch;
	int error;
	if (check_git_sha(fileNamePrt.data(), &git, &branch) == 0) {
		appendTextToLog("Cloud sync shows local cache was current");
		goto successful_exit;
	}
	appendTextToLog("Cloud sync brought newer data, reloading the dive list");

	// if we aren't switching from no-cloud mode, let's clear the dive data
	if (!noCloudToCloud) {
		appendTextToLog("Clear out in memory dive data");
		DiveListModel::instance()->clear();
	} else {
		appendTextToLog("Switching from no cloud mode; keep in memory dive data");
	}
	if (git != dummy_git_repository) {
		appendTextToLog(QString("have repository and branch %1").arg(branch));
		error = git_load_dives(git, branch);
	} else {
		appendTextToLog(QString("didn't receive valid git repo, try again"));
		error = parse_file(fileNamePrt.data(), &dive_table, &trip_table, &dive_site_table);
	}
	if (!error) {
		report_error("filename is now %s", fileNamePrt.data());
		set_filename(fileNamePrt.data());
	} else {
		report_error("failed to open file %s", fileNamePrt.data());
		setNotificationText(consumeError());
		revertToNoCloudIfNeeded();
		set_filename(NULL);
		return;
	}
	consumeFinishedLoad(currentDiveTimestamp);

successful_exit:
	alreadySaving = false;
	setLoadFromCloud(true);
	// if we came from local storage mode, let's merge the local data into the local cache
	// for the remote data - which then later gets merged with the remote data if necessary
	if (noCloudToCloud) {
		git_storage_update_progress(qPrintable(tr("Loading dives from local storage ('no cloud' mode)")));
		mergeLocalRepo();
		DiveListModel::instance()->reload();
		appendTextToLog(QStringLiteral("%1 dives loaded after importing nocloud local storage").arg(dive_table.nr));
		noCloudToCloud = false;
		mark_divelist_changed(true);
		saveChangesLocal();
		if (git_local_only == false) {
			appendTextToLog(QStringLiteral("taking things back offline now that storage is synced"));
			git_local_only = true;
		}
	}
	// if we got here just for an initial connection to the cloud, reset to offline
	if (currentGitLocalOnly) {
		currentGitLocalOnly = false;
		git_local_only = true;
	}
	return;
}

void QMLManager::revertToNoCloudIfNeeded()
{
	if (currentGitLocalOnly) {
		// we tried to connect to the cloud for the first time and that failed
		currentGitLocalOnly = false;
		git_local_only = true;
	}
	if (QMLPrefs::instance()->oldStatus() == qPrefCloudStorage::CS_NOCLOUD) {
		// we tried to switch to a cloud account and had previously used local data,
		// but connecting to the cloud account (and subsequently merging the local
		// and cloud data) failed - so let's delete the cloud credentials and go
		// back to CS_NOCLOUD mode in order to prevent us from losing the locally stored
		// dives
		if (git_local_only == true) {
			appendTextToLog(QStringLiteral("taking things back offline since sync with cloud failed"));
			git_local_only = false;
		}
		free((void *)prefs.cloud_storage_email);
		prefs.cloud_storage_email = NULL;
		free((void *)prefs.cloud_storage_password);
		prefs.cloud_storage_password = NULL;
		qPrefCloudStorage::set_cloud_storage_email("");
		QMLPrefs::instance()->setCloudPassword("");
		QMLPrefs::instance()->setCredentialStatus(qPrefCloudStorage::CS_NOCLOUD);
		set_filename(NOCLOUD_LOCALSTORAGE);
		setStartPageText(RED_FONT + tr("Failed to connect to cloud server, reverting to no cloud status") + END_FONT);
	}
	alreadySaving = false;
}

void QMLManager::consumeFinishedLoad(timestamp_t currentDiveTimestamp)
{
	prefs.unit_system = git_prefs.unit_system;
	if (git_prefs.unit_system == IMPERIAL)
		git_prefs.units = IMPERIAL_units;
	else if (git_prefs.unit_system == METRIC)
		git_prefs.units = SI_units;
	prefs.units = git_prefs.units;
	prefs.tankbar = git_prefs.tankbar;
	prefs.dcceiling = git_prefs.dcceiling;
	prefs.show_ccr_setpoint = git_prefs.show_ccr_setpoint;
	prefs.show_ccr_sensors = git_prefs.show_ccr_sensors;
	prefs.pp_graphs.po2 = git_prefs.pp_graphs.po2;
	process_loaded_dives();
	DiveListModel::instance()->reload();
	if (currentDiveTimestamp)
		setUpdateSelectedDive(DiveListSortModel::instance()->getIdxForId(get_dive_id_closest_to(currentDiveTimestamp)));
	appendTextToLog(QStringLiteral("%1 dives loaded").arg(dive_table.nr));
	if (dive_table.nr == 0)
		setStartPageText(tr("Cloud storage open successfully. No dives in dive list."));
	alreadySaving = false;
}

void QMLManager::refreshDiveList()
{
	DiveListModel::instance()->reload();
}

void QMLManager::setupDivesite(struct dive *d, struct dive_site *ds, double lat, double lon, const char *locationtext)
{
	location_t location = create_location(lat, lon);
	if (ds) {
		ds->location = location;
	} else {
		unregister_dive_from_dive_site(d);
		add_dive_to_dive_site(d, create_dive_site_with_gps(locationtext, &location, &dive_site_table));
		// We created a new dive site - let the dive site model know.
		updateSiteList();
	}
}

bool QMLManager::checkDate(const DiveObjectHelper &myDive, struct dive * d, QString date)
{
	QString oldDate = myDive.date() + " " + myDive.time();
	if (date != oldDate) {
		QDateTime newDate;
		// what a pain - Qt will not parse dates if the day of the week is incorrect
		// so if the user changed the date but didn't update the day of the week (most likely behavior, actually),
		// we need to make sure we don't try to parse that
		QString format(QString(prefs.date_format_short) + QChar(' ') + prefs.time_format);
		if (format.contains(QLatin1String("ddd")) || format.contains(QLatin1String("dddd"))) {
			QString dateFormatToDrop = format.contains(QLatin1String("ddd")) ? QStringLiteral("ddd") : QStringLiteral("dddd");
			QDateTime ts;
			QLocale loc = getLocale();
			ts.setMSecsSinceEpoch(d->when * 1000L);
			QString drop = loc.toString(ts.toUTC(), dateFormatToDrop);
			format.replace(dateFormatToDrop, "");
			date.replace(drop, "");
		}
		// set date from string and make sure it's treated as UTC (like all our time stamps)
		newDate = QDateTime::fromString(date, format);
		newDate.setTimeSpec(Qt::UTC);
		if (!newDate.isValid()) {
			appendTextToLog("unable to parse date " + date + " with the given format " + format);
			QRegularExpression isoDate("\\d+-\\d+-\\d+[^\\d]+\\d+:\\d+");
			if (date.contains(isoDate)) {
				newDate = QDateTime::fromString(date, "yyyy-M-d h:m:s");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date, "yy-M-d h:m:s");
				if (newDate.isValid())
					goto parsed;
			}
			QRegularExpression isoDateNoSecs("\\d+-\\d+-\\d+[^\\d]+\\d+");
			if (date.contains(isoDateNoSecs)) {
				newDate = QDateTime::fromString(date, "yyyy-M-d h:m");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date, "yy-M-d h:m");
				if (newDate.isValid())
					goto parsed;
			}
			QRegularExpression usDate("\\d+/\\d+/\\d+[^\\d]+\\d+:\\d+:\\d+");
			if (date.contains(usDate)) {
				newDate = QDateTime::fromString(date, "M/d/yyyy h:m:s");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date, "M/d/yy h:m:s");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date.toLower(), "M/d/yyyy h:m:sap");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date.toLower(), "M/d/yy h:m:sap");
				if (newDate.isValid())
					goto parsed;
			}
			QRegularExpression usDateNoSecs("\\d+/\\d+/\\d+[^\\d]+\\d+:\\d+");
			if (date.contains(usDateNoSecs)) {
				newDate = QDateTime::fromString(date, "M/d/yyyy h:m");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date, "M/d/yy h:m");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date.toLower(), "M/d/yyyy h:map");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date.toLower(), "M/d/yy h:map");
				if (newDate.isValid())
					goto parsed;
			}
			QRegularExpression leDate("\\d+\\.\\d+\\.\\d+[^\\d]+\\d+:\\d+:\\d+");
			if (date.contains(leDate)) {
				newDate = QDateTime::fromString(date, "d.M.yyyy h:m:s");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date, "d.M.yy h:m:s");
				if (newDate.isValid())
					goto parsed;
			}
			QRegularExpression leDateNoSecs("\\d+\\.\\d+\\.\\d+[^\\d]+\\d+:\\d+");
			if (date.contains(leDateNoSecs)) {
				newDate = QDateTime::fromString(date, "d.M.yyyy h:m");
				if (newDate.isValid())
					goto parsed;
				newDate = QDateTime::fromString(date, "d.M.yy h:m");
				if (newDate.isValid())
					goto parsed;
			}
		}
parsed:
		if (newDate.isValid()) {
			// stupid Qt... two digit years are always 19xx - WTF???
			// so if adding a hundred years gets you into something before a year from now...
			// add a hundred years.
			if (newDate.addYears(100) < QDateTime::currentDateTime().addYears(1))
				newDate = newDate.addYears(100);
			d->dc.when = d->when = newDate.toMSecsSinceEpoch() / 1000;
			return true;
		}
		appendTextToLog("none of our parsing attempts worked for the date string");
	}
	return false;
}

bool QMLManager::checkLocation(const DiveObjectHelper &myDive, struct dive *d, QString location, QString gps)
{
	bool diveChanged = false;
	struct dive_site *ds = get_dive_site_for_dive(d);
	qDebug() << "checkLocation" << location << "gps" << gps << "dive had" << myDive.location << "gps" << myDive.gas;
	if (myDive.location != location) {
		diveChanged = true;
		ds = get_dive_site_by_name(qPrintable(location), &dive_site_table);
		if (!ds && !location.isEmpty())
			ds = create_dive_site(qPrintable(location), &dive_site_table);
		unregister_dive_from_dive_site(d);
		add_dive_to_dive_site(d, ds);
		// We created a new dive site - let the dive site model know.
		updateSiteList();
	}
	// now make sure that the GPS coordinates match - if the user changed the name but not
	// the GPS coordinates, this still does the right thing as the now new dive site will
	// have no coordinates, so the coordinates from the edit screen will get added
	if (myDive.gps != gps) {
		double lat, lon;
		if (parseGpsText(gps, &lat, &lon)) {
			qDebug() << "parsed GPS, using it";
			// there are valid GPS coordinates - just use them
			setupDivesite(d, ds, lat, lon, qPrintable(myDive.location));
			diveChanged = true;
		} else if (gps == GPS_CURRENT_POS) {
			qDebug() << "gps was our default text for no GPS";
			// user asked to use current pos
			QString gpsString = getCurrentPosition();
			if (gpsString != GPS_CURRENT_POS) {
				qDebug() << "but now I got a valid location" << gpsString;
				if (parseGpsText(qPrintable(gpsString), &lat, &lon)) {
					setupDivesite(d, ds, lat, lon, qPrintable(myDive.location));
					diveChanged = true;
				}
			} else {
				appendTextToLog("couldn't get GPS location in time");
			}
		} else {
			// just something we can't parse, so tell the user
			appendTextToLog(QString("wasn't able to parse gps string '%1'").arg(gps));
		}
	}
	return diveChanged;
}

bool QMLManager::checkDuration(const DiveObjectHelper &myDive, struct dive *d, QString duration)
{
	if (myDive.duration != duration) {
		int h = 0, m = 0, s = 0;
		QRegExp r1(QStringLiteral("(\\d*)\\s*%1[\\s,:]*(\\d*)\\s*%2[\\s,:]*(\\d*)\\s*%3").arg(tr("h")).arg(tr("min")).arg(tr("sec")), Qt::CaseInsensitive);
		QRegExp r2(QStringLiteral("(\\d*)\\s*%1[\\s,:]*(\\d*)\\s*%2").arg(tr("h")).arg(tr("min")), Qt::CaseInsensitive);
		QRegExp r3(QStringLiteral("(\\d*)\\s*%1").arg(tr("min")), Qt::CaseInsensitive);
		QRegExp r4(QStringLiteral("(\\d*):(\\d*):(\\d*)"));
		QRegExp r5(QStringLiteral("(\\d*):(\\d*)"));
		QRegExp r6(QStringLiteral("(\\d*)"));
		if (r1.indexIn(duration) >= 0) {
			h = r1.cap(1).toInt();
			m = r1.cap(2).toInt();
			s = r1.cap(3).toInt();
		} else if (r2.indexIn(duration) >= 0) {
			h = r2.cap(1).toInt();
			m = r2.cap(2).toInt();
		} else if (r3.indexIn(duration) >= 0) {
			m = r3.cap(1).toInt();
		} else if (r4.indexIn(duration) >= 0) {
			h = r4.cap(1).toInt();
			m = r4.cap(2).toInt();
			s = r4.cap(3).toInt();
		} else if (r5.indexIn(duration) >= 0) {
			h = r5.cap(1).toInt();
			m = r5.cap(2).toInt();
		} else if (r6.indexIn(duration) >= 0) {
			m = r6.cap(1).toInt();
		}
		d->dc.duration.seconds = d->duration.seconds = h * 3600 + m * 60 + s;
		if (same_string(d->dc.model, "manually added dive"))
			free_samples(&d->dc);
		else
			appendTextToLog("Cannot change the duration on a dive that wasn't manually added");
		return true;
	}
	return false;
}

bool QMLManager::checkDepth(const DiveObjectHelper &myDive, dive *d, QString depth)
{
	if (myDive.depth != depth) {
		int depthValue = parseLengthToMm(depth);
		// the QML code should stop negative depth, but massively huge depth can make
		// the profile extremely slow or even run out of memory and crash, so keep
		// the depth <= 500m
		if (0 <= depthValue && depthValue <= 500000) {
			d->maxdepth.mm = depthValue;
			if (same_string(d->dc.model, "manually added dive")) {
				d->dc.maxdepth.mm = d->maxdepth.mm;
				free_samples(&d->dc);
			}
			return true;
		}
	}
	return false;
}

// update the dive and return the notes field, stripped of the HTML junk
void QMLManager::commitChanges(QString diveId, QString number, QString date, QString location, QString gps, QString duration, QString depth,
			       QString airtemp, QString watertemp, QString suit, QString buddy, QString diveMaster, QString weight, QString notes,
			       QStringList startpressure, QStringList endpressure, QStringList gasmix, QStringList usedCylinder, int rating, int visibility, QString state)
{
	struct dive *d = get_dive_by_uniq_id(diveId.toInt());

	if (!d) {
		appendTextToLog("cannot commit changes: no dive");
		return;
	}

	DiveObjectHelper myDive(d);

	// notes comes back as rich text - let's convert this into plain text
	QTextDocument doc;
	doc.setHtml(notes);
	notes = doc.toPlainText();

	bool diveChanged = false;
	bool needResort = false;

	diveChanged = needResort = checkDate(myDive, d, date);

	diveChanged |= checkLocation(myDive, d, location, gps);

	diveChanged |= checkDuration(myDive, d, duration);

	diveChanged |= checkDepth(myDive, d, depth);

	if (QString::number(myDive.number) != number) {
		diveChanged = true;
		d->number = number.toInt();
	}
	if (myDive.airTemp != airtemp) {
		diveChanged = true;
		d->airtemp.mkelvin = parseTemperatureToMkelvin(airtemp);
	}
	if (myDive.waterTemp != watertemp) {
		diveChanged = true;
		d->watertemp.mkelvin = parseTemperatureToMkelvin(watertemp);
	}
	if (myDive.sumWeight != weight) {
		diveChanged = true;
		// not sure what we'd do if there was more than one weight system
		// defined - for now just ignore that case
		if (d->weightsystems.nr == 0) {
			weightsystem_t ws = { { parseWeightToGrams(weight) } , strdup(qPrintable(tr("weight"))) };
			add_to_weightsystem_table(&d->weightsystems, 0, ws); // takes ownership of the string
		} else if (d->weightsystems.nr == 1) {
			d->weightsystems.weightsystems[0].weight.grams = parseWeightToGrams(weight);
		}
	}
	// start and end pressures for first cylinder only
	if (myDive.startPressure != startpressure || myDive.endPressure != endpressure) {
		diveChanged = true;
		for ( int i = 0, j = 0 ; j < startpressure.length() && j < endpressure.length() ; i++ ) {
			if (state != "add" && !is_cylinder_used(d, i))
				continue;

			get_cylinder(d, i)->start.mbar = parsePressureToMbar(startpressure[j]);
			get_cylinder(d, i)->end.mbar = parsePressureToMbar(endpressure[j]);
			if (get_cylinder(d, i)->end.mbar > get_cylinder(d, i)->start.mbar)
				get_cylinder(d, i)->end.mbar = get_cylinder(d, i)->start.mbar;

			j++;
		}
	}
	// gasmix for first cylinder
	if (myDive.firstGas != gasmix) {
		for ( int i = 0, j = 0 ; j < gasmix.length() ; i++ ) {
			if (state != "add" && !is_cylinder_used(d, i))
				continue;

			int o2 = parseGasMixO2(gasmix[j]);
			int he = parseGasMixHE(gasmix[j]);
			// the QML code SHOULD only accept valid gas mixes, but just to make sure
			if (o2 >= 0 && o2 <= 1000 &&
				he >= 0 && he <= 1000 &&
				o2 + he <= 1000) {
				diveChanged = true;
				get_cylinder(d, i)->gasmix.o2.permille = o2;
				get_cylinder(d, i)->gasmix.he.permille = he;
			}
			j++;
		}
	}
	// info for first cylinder
	if (myDive.getCylinder != usedCylinder) {
		diveChanged = true;
		unsigned long i;
		int size = 0, wp = 0, j = 0, k = 0;
		for (j = 0; k < usedCylinder.length(); j++) {
			if (state != "add" && !is_cylinder_used(d, j))
				continue;

			for (i = 0; i < MAX_TANK_INFO && tank_info[i].name != NULL; i++) {
				if (tank_info[i].name == usedCylinder[k] ) {
					if (tank_info[i].ml > 0){
						size = tank_info[i].ml;
						wp = tank_info[i].bar * 1000;
					} else {
						size = (int) (cuft_to_l(tank_info[i].cuft) * 1000 / bar_to_atm(psi_to_bar(tank_info[i].psi)));
						wp = psi_to_mbar(tank_info[i].psi);
					}
					break;
				}
			}
			get_cylinder(d, j)->type.description = copy_qstring(usedCylinder[k]);
			get_cylinder(d, j)->type.size.mliter = size;
			get_cylinder(d, j)->type.workingpressure.mbar = wp;
			k++;
		}
	}
	if (myDive.suit != suit) {
		diveChanged = true;
		free(d->suit);
		d->suit = copy_qstring(suit);
	}
	if (myDive.buddy != buddy) {
		if (buddy.contains(",")){
			buddy = buddy.replace(QRegExp("\\s*,\\s*"), ", ");
		}
		diveChanged = true;
		free(d->buddy);
		d->buddy = copy_qstring(buddy);
	}
	if (myDive.divemaster != diveMaster) {
		if (diveMaster.contains(",")){
			diveMaster = diveMaster.replace(QRegExp("\\s*,\\s*"), ", ");
		}
		diveChanged = true;
		free(d->divemaster);
		d->divemaster = copy_qstring(diveMaster);
	}
	if (myDive.rating != rating) {
		diveChanged = true;
		d->rating = rating;
	}
	if (myDive.visibility != visibility) {
		diveChanged = true;
		d->visibility = visibility;
	}
	if (myDive.notes != notes) {
		diveChanged = true;
		free(d->notes);
		d->notes = copy_qstring(notes);
	}
	// now that we have it all figured out, let's see what we need
	// to update
	DiveListModel *dm = DiveListModel::instance();
	int modelIdx = dm->getDiveIdx(d->id);
	int oldIdx = get_idx_by_uniq_id(d->id);
	if (needResort) {
		// we know that the only thing that might happen in a resort is that
		// this one dive moves to a different spot in the dive list
		sort_dive_table(&dive_table);
		sort_trip_table(&trip_table);
		int newIdx = get_idx_by_uniq_id(d->id);
		if (newIdx != oldIdx) {
			DiveListModel::instance()->removeDive(modelIdx);
			modelIdx += (newIdx - oldIdx);
			DiveListModel::instance()->insertDive(modelIdx);
			diveChanged = true; // because we already modified things
		}
	}
	if (diveChanged) {
		if (d->maxdepth.mm == d->dc.maxdepth.mm &&
		    d->maxdepth.mm > 0 &&
		    same_string(d->dc.model, "manually added dive") &&
		    d->dc.samples == 0) {
			// so we have depth > 0, a manually added dive and no samples
			// let's create an actual profile so the desktop version can work it
			// first clear out the mean depth (or the fake_dc() function tries
			// to be too clever)
			d->meandepth.mm = d->dc.meandepth.mm = 0;
			fake_dc(&d->dc);
		}
		fixup_dive(d);
		DiveListModel::instance()->updateDive(modelIdx, d);
		invalidate_dive_cache(d);
		mark_divelist_changed(true);
	}
	if (diveChanged || needResort)
		changesNeedSaving();
}

void QMLManager::changesNeedSaving()
{
	// we no longer save right away on iOS because file access is so slow; on the other hand,
	// on Android the save as the user switches away doesn't seem to work... drat.
	// as a compromise for now we save just to local storage on Android right away (that appears
	// to be reasonably fast), but don't save at all (and only remember that we need to save things
	// on iOS
	// on all other platforms we just save the changes and be done with it
	mark_divelist_changed(true);
#if defined(Q_OS_ANDROID)
	saveChangesLocal();
#elif !defined(Q_OS_IOS)
	saveChangesCloud(false);
#endif
	updateAllGlobalLists();
}

void QMLManager::openNoCloudRepo()
/*
 * Open the No Cloud repo. In case this repo does not (yet)
 * exists, create one first. When done, open the repo, which
 * is obviously empty when just created.
 */
{
	char *filename = NOCLOUD_LOCALSTORAGE;
	const char *branch;
	struct git_repository *git;

	git = is_git_repository(filename, &branch, NULL, false);

	if (git == dummy_git_repository) {
		git_create_local_repo(filename);
		set_filename(filename);
		auto s = qPrefGeneral::instance();
		s->set_default_filename(filename);
		s->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	}

	openLocalThenRemote(filename);
}

void QMLManager::saveChangesLocal()
{
	if (unsaved_changes()) {
		if (QMLPrefs::instance()->credentialStatus() == qPrefCloudStorage::CS_NOCLOUD) {
			if (empty_string(existing_filename)) {
				char *filename = NOCLOUD_LOCALSTORAGE;
				git_create_local_repo(filename);
				set_filename(filename);
				auto s = qPrefGeneral::instance();
				s->set_default_filename(filename);
				s->set_default_file_behavior(LOCAL_DEFAULT_FILE);
			}
		} else if (!m_loadFromCloud) {
			// this seems silly, but you need a common ancestor in the repository in
			// order to be able to merge che changes later
			appendTextToLog("Don't save dives without loading from the cloud, first.");
			return;
		}
		if (alreadySaving) {
			appendTextToLog("save operation already in progress, can't save locally");
			return;
		}
		alreadySaving = true;
		bool glo = git_local_only;
		git_local_only = true;
		if (save_dives(existing_filename)) {
			setNotificationText(consumeError());
			set_filename(NULL);
			git_local_only = glo;
			alreadySaving = false;
			return;
		}
		git_local_only = glo;
		mark_divelist_changed(false);
		alreadySaving = false;
	} else {
		appendTextToLog("local save requested with no unsaved changes");
	}
}

void QMLManager::saveChangesCloud(bool forceRemoteSync)
{
	if (!unsaved_changes() && !forceRemoteSync) {
		appendTextToLog("asked to save changes but no unsaved changes");
		return;
	}
	if (alreadySaving) {
		appendTextToLog("save operation in progress already");
		return;
	}
	// first we need to store any unsaved changes to the local repo
	gitProgressCB("Save changes to local cache");
	saveChangesLocal();

	// if the user asked not to push to the cloud we are done
	if (git_local_only && !forceRemoteSync)
		return;

	if (!m_loadFromCloud) {
		setNotificationText(tr("Fatal error: cannot save data file. Please copy log file and report."));
		appendTextToLog("Don't save dives without loading from the cloud, first.");
		return;
	}

	bool glo = git_local_only;
	git_local_only = false;
	alreadySaving = true;
	loadDivesWithValidCredentials();
	alreadySaving = false;
	git_local_only = glo;
}

bool QMLManager::undoDelete(int id)
{
	if (!deletedDive || deletedDive->id != id) {
		appendTextToLog("Trying to undo delete but can't find the deleted dive");
		return false;
	}
	if (deletedTrip)
		insert_trip(deletedTrip, &trip_table);
	if (deletedDive->divetrip) {
		struct dive_trip *trip = deletedDive->divetrip;
		deletedDive->divetrip = NULL;
		add_dive_to_trip(deletedDive, trip);
	}
	record_dive(deletedDive);
	DiveListModel::instance()->insertDive(get_idx_by_uniq_id(deletedDive->id));
	changesNeedSaving();
	deletedDive = NULL;
	deletedTrip = NULL;
	return true;
}

void QMLManager::selectDive(int id)
{
	int i;
	extern int amount_selected;
	struct dive *dive = NULL;

	amount_selected = 0;
	for_each_dive (i, dive) {
		dive->selected = (dive->id == id);
		if (dive->selected)
			amount_selected++;
	}
	if (amount_selected == 0)
		qWarning("QManager::selectDive() called with unknown id");
	else
		CollapsedDiveListSortModel::instance()->updateSelectionState();
}

void QMLManager::deleteDive(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog("trying to delete non-existing dive");
		return;
	}
	// create the storage for the deleted dive and trip (if applicable)
	if (!deletedDive)
		deletedDive = alloc_dive();
	copy_dive(d, deletedDive);
	if (!deletedTrip) {
		deletedTrip = alloc_trip();
	} else {
		free(deletedTrip->location);
		free(deletedTrip->notes);
		memset(deletedTrip, 0, sizeof(struct dive_trip));
	}
	// if this is the last dive in that trip, remember the trip as well
	if (d->divetrip && d->divetrip->dives.nr == 1) {
		*deletedTrip = *d->divetrip;
		deletedTrip->location = copy_string(d->divetrip->location);
		deletedTrip->notes = copy_string(d->divetrip->notes);
		deletedTrip->dives.nr = 0;
		deletedDive->divetrip = deletedTrip;
	}
	DiveListModel::instance()->removeDiveById(id);
	delete_single_dive(get_idx_by_uniq_id(id));
	DiveListModel::instance()->resetInternalData();
	changesNeedSaving();
}

bool QMLManager::toggleDiveSite(bool toggle)
{
	if (toggle)
		what.divesite = what.divesite ? false : true;

	return what.divesite;
}

bool QMLManager::toggleNotes(bool toggle)
{
	if (toggle)
		what.notes = what.notes ? false : true;

	return what.notes;
}

bool QMLManager::toggleDiveMaster(bool toggle)
{
	if (toggle)
		what.divemaster = what.divemaster ? false : true;

	return what.divemaster;
}

bool QMLManager::toggleBuddy(bool toggle)
{
	if (toggle)
		what.buddy = what.buddy ? false : true;

	return what.buddy;
}

bool QMLManager::toggleSuit(bool toggle)
{
	if (toggle)
		what.suit = what.suit ? false : true;

	return what.suit;
}

bool QMLManager::toggleRating(bool toggle)
{
	if (toggle)
		what.rating = what.rating ? false : true;

	return what.rating;
}

bool QMLManager::toggleVisibility(bool toggle)
{
	if (toggle)
		what.visibility = what.visibility ? false : true;

	return what.visibility;
}

bool QMLManager::toggleTags(bool toggle)
{
	if (toggle)
		what.tags = what.tags ? false : true;

	return what.tags;
}

bool QMLManager::toggleCylinders(bool toggle)
{
	if (toggle)
		what.cylinders = what.cylinders ? false : true;

	return what.cylinders;
}

bool QMLManager::toggleWeights(bool toggle)
{
	if (toggle)
		what.weights = what.weights ? false : true;

	return what.weights;
}

void QMLManager::copyDiveData(int id)
{
	m_copyPasteDive = get_dive_by_uniq_id(id);
	if (!m_copyPasteDive) {
		appendTextToLog("trying to copy non-existing dive");
		return;
	}

	setNotificationText("Copy");
}

void QMLManager::pasteDiveData(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog("trying to paste to non-existing dive");
		return;
	}
	if (!m_copyPasteDive) {
		appendTextToLog("dive to paste is not selected");
		return;
	}
	selective_copy_dive(m_copyPasteDive, d, what, false);

	invalidate_dive_cache(d);
	mark_divelist_changed(true);
	changesNeedSaving();
	setNotificationText("Paste");

	int modelIdx = DiveListModel::instance()->getDiveIdx(id);
	DiveListModel::instance()->updateDive(modelIdx, d);
}

void QMLManager::cancelDownloadDC()
{
	import_thread_cancelled = true;
}

QString QMLManager::addDive()
{
	appendTextToLog("Adding new dive.");
	return DiveListModel::instance()->startAddDive();
}

void QMLManager::addDiveAborted(int id)
{
	DiveListModel::instance()->removeDiveById(id);
	delete_single_dive(get_idx_by_uniq_id(id));
}

QString QMLManager::getCurrentPosition()
{
	static bool hasLocationSource = false;
	if (locationProvider->hasLocationsSource() != hasLocationSource) {
		hasLocationSource = !hasLocationSource;
		setLocationServiceAvailable(hasLocationSource);
	}
	if (!hasLocationSource)
		return tr("Unknown GPS location");

	QString positionResponse = locationProvider->currentPosition();
	if (positionResponse == GPS_CURRENT_POS)
		connect(locationProvider, &GpsLocation::acquiredPosition, this, &QMLManager::waitingForPositionChanged, Qt::UniqueConnection);
	else
		disconnect(locationProvider, &GpsLocation::acquiredPosition, this, &QMLManager::waitingForPositionChanged);
	return positionResponse;
}

void QMLManager::applyGpsData()
{
	appendTextToLog("Applying GPS fiexs");
	int cnt = locationProvider->applyLocations();
	if (cnt == 0)
		return;
	appendTextToLog(QString("Attached %1 GPS fixes").arg(cnt));
	refreshDiveList();
}

void QMLManager::populateGpsData()
{
	if (GpsListModel::instance())
		GpsListModel::instance()->update();
}

void QMLManager::clearGpsData()
{
	locationProvider->clearGpsData();
	populateGpsData();
}

void QMLManager::deleteGpsFix(quint64 when)
{
	locationProvider->deleteGpsFix(when);
	populateGpsData();
}

QString QMLManager::logText() const
{
	QString logText = m_logText + QString("\nNumer of GPS fixes: %1").arg(locationProvider->getGpsNum());
	return logText;
}

void QMLManager::setLogText(const QString &logText)
{
	m_logText = logText;
	emit logTextChanged();
}

void QMLManager::appendTextToLog(const QString &newText)
{
	qDebug() << QString::number(timer.elapsed() / 1000.0,'f', 3) + ": " + newText;
}

void QMLManager::setLocationServiceEnabled(bool locationServiceEnabled)
{
	m_locationServiceEnabled = locationServiceEnabled;
	locationProvider->serviceEnable(m_locationServiceEnabled);
	emit locationServiceEnabledChanged();
}

void QMLManager::setLocationServiceAvailable(bool locationServiceAvailable)
{
	appendTextToLog(QStringLiteral("location service is ") + (locationServiceAvailable ? QStringLiteral("available") : QStringLiteral("not available")));
	m_locationServiceAvailable = locationServiceAvailable;
	emit locationServiceAvailableChanged();
}

void QMLManager::hasLocationSourceChanged()
{
	setLocationServiceAvailable(locationProvider->hasLocationsSource());
}

void QMLManager::setVerboseEnabled(bool verboseMode)
{
	m_verboseEnabled = verboseMode;
	verbose = verboseMode;
	appendTextToLog(QStringLiteral("verbose is ") + (verbose ? QStringLiteral("on") : QStringLiteral("off")));
	emit verboseEnabledChanged();
}

void QMLManager::syncLoadFromCloud()
{
	QSettings s;
	QString cloudMarker = QLatin1Literal("loadFromCloud") + QString(prefs.cloud_storage_email);
	m_loadFromCloud = s.contains(cloudMarker) && s.value(cloudMarker).toBool();
}

void QMLManager::setLoadFromCloud(bool done)
{
	QSettings s;
	QString cloudMarker = QLatin1Literal("loadFromCloud") + QString(prefs.cloud_storage_email);
	s.setValue(cloudMarker, done);
	m_loadFromCloud = done;
	emit loadFromCloudChanged();
}

void QMLManager::setStartPageText(const QString& text)
{
	m_startPageText = text;
	emit startPageTextChanged();
}

QString QMLManager::getNumber(const QString& diveId)
{
	int dive_id = diveId.toInt();
	struct dive *d = get_dive_by_uniq_id(dive_id);
	QString number;
	if (d)
		number = QString::number(d->number);
	return number;
}

QString QMLManager::getDate(const QString& diveId)
{
	int dive_id = diveId.toInt();
	struct dive *d = get_dive_by_uniq_id(dive_id);
	QString datestring;
	if (d)
		datestring = get_short_dive_date_string(d->when);
	return datestring;
}

QString QMLManager::getVersion() const
{
	QRegExp versionRe(".*:([()\\.,\\d]+).*");
	if (!versionRe.exactMatch(getUserAgent()))
		return QString();

	return versionRe.cap(1);
}

QString QMLManager::getGpsFromSiteName(const QString &siteName)
{
	struct dive_site *ds;

	ds = get_dive_site_by_name(qPrintable(siteName), &dive_site_table);
	if (!ds)
		return QString();
	return printGPSCoords(&ds->location);
}

void QMLManager::setNotificationText(QString text)
{
	m_notificationText = text;
	emit notificationTextChanged();
}

void QMLManager::setUpdateSelectedDive(int idx)
{
	m_updateSelectedDive = idx;
	emit updateSelectedDiveChanged();
}

void QMLManager::setSelectedDiveTimestamp(int when)
{
	m_selectedDiveTimestamp = when;
	emit selectedDiveTimestampChanged();
}

qreal QMLManager::lastDevicePixelRatio()
{
	return m_lastDevicePixelRatio;
}

void QMLManager::setDevicePixelRatio(qreal dpr, QScreen *screen)
{
	if (m_lastDevicePixelRatio != dpr) {
		m_lastDevicePixelRatio = dpr;
		emit sendScreenChanged(screen);
	}
}

void QMLManager::screenChanged(QScreen *screen)
{
	qDebug("QMLManager received screen changed notification (%d,%d)", screen->size().width(), screen->size().height());
	m_lastDevicePixelRatio = screen->devicePixelRatio();
	emit sendScreenChanged(screen);
}

void QMLManager::quit()
{
	if (unsaved_changes())
		saveChangesCloud(false);
	QApplication::quit();
}

QStringList QMLManager::suitList() const
{
	return suitModel.stringList();
}

QStringList QMLManager::buddyList() const
{
	return buddyModel.stringList();
}

QStringList QMLManager::divemasterList() const
{
	return divemasterModel.stringList();
}

QStringList QMLManager::locationList() const
{
	return locationModel.allSiteNames();
}

QStringList QMLManager::cylinderInit() const
{
	QStringList cylinders;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		for (int j = 0; j < d->cylinders.nr; j++) {
			if (!empty_string(get_cylinder(d, j)->type.description))
				cylinders << get_cylinder(d, j)->type.description;
		}
	}

	for (unsigned long ti = 0; ti < MAX_TANK_INFO && tank_info[ti].name != NULL; ti++) {
		QString cyl = tank_info[ti].name;
		if (cyl == "")
			continue;
		cylinders << cyl;
	}

	cylinders.removeDuplicates();
	cylinders.sort();
	return cylinders;
}

void QMLManager::setProgressMessage(QString text)
{
	m_progressMessage = text;
	emit progressMessageChanged();
}

void QMLManager::setBtEnabled(bool value)
{
	m_btEnabled = value;
}

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)

void writeToAppLogFile(QString logText)
{
	// write to storage and flush so that the data doesn't get lost
	logText.append("\n");
	QMLManager *self = QMLManager::instance();
	if (self) {
		self->writeToAppLogFile(logText);
	}
}

void QMLManager::writeToAppLogFile(QString logText)
{
	if (appLogFileOpen) {
		appLogFile.write(qPrintable(logText));
		appLogFile.flush();
	}
}
#endif

#if defined(Q_OS_ANDROID)
//HACK to color the system bar on Android, use qtandroidextras and call the appropriate Java methods
//this code is based on code in the Kirigami example app for Android (under LGPL-2) Copyright 2017 Marco Martin

#include <QtAndroid>

// there doesn't appear to be an include that defines these in an easily accessible way
// WindowManager.LayoutParams
#define FLAG_TRANSLUCENT_STATUS 0x04000000
#define FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS 0x80000000
// View
#define SYSTEM_UI_FLAG_LIGHT_STATUS_BAR 0x00002000

void QMLManager::setStatusbarColor(QColor color)
{
	QtAndroid::runOnAndroidThread([=]() {
		QAndroidJniObject window = QtAndroid::androidActivity().callObjectMethod("getWindow", "()Landroid/view/Window;");
		window.callMethod<void>("addFlags", "(I)V", FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
		window.callMethod<void>("clearFlags", "(I)V", FLAG_TRANSLUCENT_STATUS);
		window.callMethod<void>("setStatusBarColor", "(I)V", color.rgba());
		window.callMethod<void>("setNavigationBarColor", "(I)V", color.rgba());
	});
}
#else
void QMLManager::setStatusbarColor(QColor)
{
	// noop
}

#endif

void QMLManager::retrieveBluetoothName()
{
	QString name = DC_devName();
	const QList<BTDiscovery::btVendorProduct> btDCs = BTDiscovery::instance()->getBtDcs();
	for (BTDiscovery::btVendorProduct btDC: btDCs) {
		qDebug() << "compare" <<name << btDC.btpdi.address;
		if (name.contains(btDC.btpdi.address))
			DC_setDevBluetoothName(btDC.btpdi.name);
	}
}

QString QMLManager::DC_vendor() const
{
	return DCDeviceData::instance()->vendor();
}

QString QMLManager::DC_product() const
{
	return DCDeviceData::instance()->product();
}

QString QMLManager::DC_devName() const
{
	return DCDeviceData::instance()->devName();
}

QString QMLManager::DC_devBluetoothName() const
{
	return DCDeviceData::instance()->devBluetoothName();
}

QString QMLManager::DC_descriptor() const
{
	return DCDeviceData::instance()->descriptor();
}

bool QMLManager::DC_forceDownload() const
{
	return DCDeviceData::instance()->forceDownload();
}

bool QMLManager::DC_bluetoothMode() const
{
	return DCDeviceData::instance()->bluetoothMode();
}

bool QMLManager::DC_saveDump() const
{
	return DCDeviceData::instance()->saveDump();
}

int QMLManager::DC_deviceId() const
{
	return DCDeviceData::instance()->deviceId();
}

void QMLManager::DC_setDeviceId(int deviceId)
{
	DCDeviceData::instance()->setDeviceId(deviceId);
}

void QMLManager::DC_setVendor(const QString& vendor)
{
	DCDeviceData::instance()->setVendor(vendor);
}

void QMLManager::DC_setProduct(const QString& product)
{
	DCDeviceData::instance()->setProduct(product);
}

void QMLManager::DC_setDevName(const QString& devName)
{
	DCDeviceData::instance()->setDevName(devName);
}

void QMLManager::DC_setDevBluetoothName(const QString& devBluetoothName)
{
	DCDeviceData::instance()->setDevBluetoothName(devBluetoothName);
}

void QMLManager::DC_setBluetoothMode(bool mode)
{
	DCDeviceData::instance()->setBluetoothMode(mode);
}

void QMLManager::DC_setForceDownload(bool force)
{
	DCDeviceData::instance()->setForceDownload(force);
	DC_ForceDownloadChanged();
}

void QMLManager::DC_setSaveDump(bool dumpMode)
{
	DCDeviceData::instance()->setSaveDump(dumpMode);
}

QStringList QMLManager::getProductListFromVendor(const QString &vendor)
{
	return DCDeviceData::instance()->getProductListFromVendor(vendor);
}

int QMLManager::getMatchingAddress(const QString &vendor, const QString &product)
{
	return DCDeviceData::instance()->getMatchingAddress(vendor, product);
}

int QMLManager::getDetectedVendorIndex()
{
	return DCDeviceData::instance()->getDetectedVendorIndex();
}

int QMLManager::getDetectedProductIndex(const QString &currentVendorText)
{
	return DCDeviceData::instance()->getDetectedProductIndex(currentVendorText);
}

int QMLManager::getConnectionIndex(const QString &deviceSubstr)
{
	return connectionListModel.indexOf(deviceSubstr);
}

void QMLManager::setGitLocalOnly(const bool &value)
{
	git_local_only = value;
}

void QMLManager::showDownloadPage(QString deviceString)
{
	// we pass the indices for the three combo boxes for vendor, product, and connection
	// to the QML UI
	// for each of these values '-1' means that no entry should be pre-selected
	QString name("-1;-1;-1");

	// try to guess the dive computer (or at least vendor) from the string that
	// we get from the Intent
	// the first couple we do text based because we know exactly what to look for,
	// the rest is based on the vendor and product IDs
	if (deviceString.contains("HeinrichsWeikamp OSTC3")) {
		name = QString("%1;%2;%3")
				.arg(vendorList.indexOf("Heinrichs Weikamp"))
				.arg(productList["Heinrichs Weikamp"].indexOf("OSTC 3"))
				.arg(connectionListModel.indexOf("FTDI"));
	} else if (deviceString.contains("HeinrichsWeikamp OSTC 2N")) {
			name = QString("%1;%2;%3")
					.arg(vendorList.indexOf("Heinrichs Weikamp"))
					.arg(productList["Heinrichs Weikamp"].indexOf("OSTC 2N"))
					.arg(connectionListModel.indexOf("FTDI"));
	} else if (deviceString.contains("mManufacturerName=ATOMIC AQUATICS") &&
		   deviceString.contains("mProductName=COBALT")) {
		if (deviceString.contains("mVersion=2")) {
			name = QString("%1;%2;%3")
					.arg(vendorList.indexOf("Atomic Aquatics"))
					.arg(productList["Atomic Aquatics"].indexOf("Cobalt 2"))
					.arg(connectionListModel.indexOf("USB device"));
		} else {
			name = QString("%1;%2;%3")
					.arg(vendorList.indexOf("Atomic Aquatics"))
					.arg(productList["Atomic Aquatics"].indexOf("Cobalt"))
					.arg(connectionListModel.indexOf("USB device"));
		}
	} else if (deviceString.contains("mVendorId=5267") && // 0x1493 / 0x0030
		   deviceString.contains("mProductId=48")) {
		name = QString("%1;%2;%3")
				.arg(connectionListModel.indexOf("Suunto"))
				.arg(productList["Suunto"].indexOf("EON Steel"))
				.arg(connectionListModel.indexOf("USB device"));
	} else if (deviceString.contains("mVendorId=5267") && // 0x1493 / 0x0033
		   deviceString.contains("mProductId=51")) {
		name = QString("%1;%2;%3")
				.arg(connectionListModel.indexOf("Suunto"))
				.arg(productList["Suunto"].indexOf("EON Core"))
				.arg(connectionListModel.indexOf("USB device"));
	} else if (deviceString.contains("mVendorId=11884") && // 0x2e6c / 0x3201,0x3211,0x4201
		   (deviceString.contains("mProductId=12801") ||
		    deviceString.contains("mProductId=12817") ||
		    deviceString.contains("mProductId=16897"))) {
		name = QString("%1;%2;%3")
				.arg(connectionListModel.indexOf("Scubapro"))
				.arg(productList["Scubapro"].indexOf("G2"))
				.arg(connectionListModel.indexOf("USB device"));
	} else if (deviceString.contains("mVendorId=49745") && // 0xc251 / 0x2006
		   deviceString.contains("mProductId=8198")) {
		name = QString("%1;%2;%3")
				.arg(connectionListModel.indexOf("Scubapro"))
				.arg(productList["Scubapro"].indexOf("Aladin Square"))
				.arg(connectionListModel.indexOf("USB device"));
	} else if (deviceString.contains("mVendorId=1027") && // 0x0403 / 0x6001,0x6010,0x6011,0x6015
		   (deviceString.contains("mProductId=24577") ||
		    deviceString.contains("mProductId=24592") ||
		    deviceString.contains("mProductId=24593") ||
		    deviceString.contains("mProductId=24597"))) {
		name = QString("-1;-1;%1").arg(connectionListModel.indexOf("FTDI"));
	} else if (deviceString.contains("mVendorId=1027") && // 0x0403 / 0xf460
		   deviceString.contains("mProductId=62560")) {
		name = QString("%1;-1;%2")
				.arg(vendorList.indexOf("Oceanic"))
				.arg(connectionListModel.indexOf("FTDI"));
	} else if (deviceString.contains("mVendorId=1027") && // 0x0403 / 0xf680
		   deviceString.contains("mProductId=63104")) {
		name = QString("%1;-1;%2")
				.arg(vendorList.indexOf("Suunto"))
				.arg(connectionListModel.indexOf("FTDI"));
	} else if (deviceString.contains("mVendorId=1027") && // 0x0403 / 0x87d0
		   deviceString.contains("mProductId=34768")) {
		name = QString("%1;-1;%2")
				.arg(vendorList.indexOf("Cressi"))
				.arg(connectionListModel.indexOf("FTDI"));
	}
	// inform the QML UI that it should show the download page
	m_pluggedInDeviceName = strdup(qPrintable(name));
	emit pluggedInDeviceNameChanged();
}

void QMLManager::setFilter(const QString filterText)
{
	// show that we are doing something, then do something in another thread in order not to block the UI
	QMetaObject::invokeMethod(qmlWindow, "showBusyAndDisconnectModel");
	QtConcurrent::run(QThreadPool::globalInstance(),
			  [=]{
				DiveListSortModel::instance()->setFilter(filterText);
				CollapsedDiveListSortModel::instance()->updateFilterState();
				QMetaObject::invokeMethod(qmlWindow, "hideBusyAndConnectModel");
			  });
}

void QMLManager::setShowNonDiveComputers(bool show)
{
	m_showNonDiveComputers = show;
	BTDiscovery::instance()->showNonDiveComputers(show);
}

#if defined(Q_OS_ANDROID)
// implemented in core/android.cpp
void checkPendingIntents();
#endif

void QMLManager::appInitialized()
{
#if defined(Q_OS_ANDROID)
	checkPendingIntents();
#endif
}

void QMLManager::exportToFile(export_types type, QString dir, bool anonymize)
{
	// dir starts with "file://" e.g. "file:///tmp"
	// remove prefix and add standard filenamel
	QString fileName = dir.right(dir.size() - 7) + "/Subsurface_export";

	switch (type)
	{
		case EX_DIVES_XML:
			save_dives_logic(qPrintable(fileName + ".ssrf"), false, anonymize);
			break;
		case EX_DIVE_SITES_XML:
			{
				std::vector<const dive_site *> sites = exportFuncs::instance()->getDiveSitesToExport(false);
				save_dive_sites_logic(qPrintable(fileName + ".xml"), &sites[0], (int)sites.size(), anonymize);
				break;
			}
		case EX_UDDF:
			exportFuncs::instance()->exportUsingStyleSheet(fileName + ".uddf", true, 0, "uddf-export.xslt", anonymize);
			break;
		case EX_CSV_DIVE_PROFILE:
			exportFuncs::instance()->exportUsingStyleSheet(fileName + ".uddf", true, 0, "xml2csv.xslt", anonymize);
			break;
		case EX_CSV_DETAILS:
			exportFuncs::instance()->exportUsingStyleSheet(fileName + ".uddf", true, 0, "xml2manualcsv.xslt", anonymize);
			break;
		case EX_CSV_PROFILE:
			save_profiledata(qPrintable(fileName + ".csv"), true);
			break;
		case EX_PROFILE_PNG:
			exportFuncs::instance()->exportProfile(qPrintable(fileName + ".png"), false);
			break;
		case EX_WORLD_MAP:
			export_worldmap_HTML(qPrintable(fileName + ".html"), true);
			break;
		case EX_TEX:
			exportFuncs::instance()->export_TeX(qPrintable(fileName + ".tex"), true, true);
			break;
		case EX_LATEX:
			exportFuncs::instance()->export_TeX(qPrintable(fileName + ".tex"), true, false);
			break;
		case EX_IMAGE_DEPTHS:
			exportFuncs::instance()->export_depths(qPrintable(fileName), false);
			break;
		default:
			qDebug() << "export to unknown type " << type << " using " << dir << " remove names " << anonymize;
			break;
	}
}

void exportFuncs::saveProfile(const struct dive *dive, const QString filename)
{
	// TBD
}

void QMLManager::exportToWEB(export_types type, QString userId, QString password, bool anonymize)
{
	switch (type)
	{
		case EX_DIVELOGS_DE:
			uploadDiveLogsDE::instance()->doUpload(false, userId, password);
			break;
		case EX_DIVESHARE:
			uploadDiveShare::instance()->doUpload(false, userId, anonymize);
			break;
		default:
			qDebug() << "upload to unknown type " << type << " using " << userId << "/" <<  password << " remove names " << anonymize;
			break;
	}
}

void QMLManager::uploadFinishSlot(bool success, const QString &text, const QByteArray &html)
{
	emit uploadFinish(success, text);
}

