// SPDX-License-Identifier: GPL-2.0
#include "qmlmanager.h"
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
#include <QUndoStack>

#include <QBluetoothLocalDevice>

#include "qt-models/gpslistmodel.h"
#include "qt-models/completionmodels.h"
#include "qt-models/messagehandlermodel.h"
#include "qt-models/tankinfomodel.h"
#include "qt-models/mobilelistmodel.h"
#include "core/device.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/divefilter.h"
#include "core/filterconstraint.h"
#include "core/qthelper.h"
#include "core/qt-gui.h"
#include "core/git-access.h"
#include "core/cloudstorage.h"
#include "core/membuffer.h"
#include "core/downloadfromdcthread.h"
#include "core/subsurfacestartup.h" // for ignore_bt flag
#include "core/subsurface-string.h"
#include "core/string-format.h"
#include "core/pref.h"
#include "core/selection.h"
#include "core/ssrf.h"
#include "core/save-profiledata.h"
#include "core/settings/qPrefLog.h"
#include "core/settings/qPrefLocationService.h"
#include "core/settings/qPrefTechnicalDetails.h"
#include "core/settings/qPrefPartialPressureGas.h"
#include "core/settings/qPrefUnit.h"
#include "core/trip.h"
#include "backend-shared/exportfuncs.h"
#include "core/worldmap-save.h"
#include "core/uploadDiveLogsDE.h"
#include "core/uploadDiveShare.h"
#include "commands/command_base.h"
#include "commands/command.h"

#if defined(Q_OS_ANDROID)
#include "core/serial_usb_android.h"
std::vector<android_usb_serial_device_descriptor> androidSerialDevices;

#endif

QMLManager *QMLManager::m_instance = NULL;
bool noCloudToCloud = false;

#define RED_FONT QLatin1String("<font color=\"red\">")
#define END_FONT QLatin1String("</font>")

extern "C" void showErrorFromC(char *buf)
{
	QString error(buf);
	free(buf);
	// By using invokeMethod with Qt:AutoConnection, the error string is safely
	// transported across thread boundaries, if not called from the UI thread.
	QMetaObject::invokeMethod(QMLManager::instance(), "registerError", Qt::AutoConnection, Q_ARG(QString, error));
}

// this gets called from libdivecomputer
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

// This flag is set to true by operations that are not implemented in the
// undo system. It is therefore only cleared on save and load.
static bool dive_list_changed = false;

void mark_divelist_changed(bool changed)
{
	if (dive_list_changed == changed)
		return;
	dive_list_changed = changed;
	updateWindowTitle();
}

int unsaved_changes()
{
	return dive_list_changed;
}

// this callback is used from the uiNotification() function
// the detour via callback allows us to keep the core code independent from QMLManager
// I'm not sure it makes sense to have three different progress callbacks,
// but the usage models (and the situations in the program flow where they are used)
// are really vastly different...
// this is mainly intended for the early stages of the app so the user sees that
// things are progressing
static void showProgress(QString msg)
{
	QMLManager *self = QMLManager::instance();
	if (self)
		self->setNotificationText(msg);
}

// show the git progress in the passive notification area
extern "C" int gitProgressCB(const char *text)
{
	// regular users, during regular operation, likely really don't
	// care at all about the git progress
	if (verbose) {
		showProgress(QString(text));
		appendTextToLogStandalone(text);
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

void QMLManager::rescanConnections()
{
	connectionListModel.removeAllAddresses();
	usbRescan();
	btRescan();
#if defined(SERIAL_FTDI)
	connectionListModel.addAddress("FTDI");
#endif
}

void QMLManager::usbRescan()
{
#if defined(Q_OS_ANDROID)
	androidUsbPopoulateConnections();
#endif
}

extern void (*uiNotificationCallback)(QString);

// Currently we have two markers for unsaved changes:
// 1) unsaved_changes() returns true for non-undoable changes.
// 2) Command::isClean() returns false for undoable changes.
static bool unsavedChanges()
{
	return unsaved_changes() || !Command::isClean();
}

QMLManager::QMLManager() : m_locationServiceEnabled(false),
	m_verboseEnabled(false),
	m_diveListProcessing(false),
	m_initialized(false),
	m_pluggedInDeviceName(""),
	m_showNonDiveComputers(false),
	undoAction(Command::undoAction(this)),
	m_oldStatus(qPrefCloudStorage::CS_UNKNOWN)
{
	m_instance = this;
	m_lastDevicePixelRatio = qApp->devicePixelRatio();
	timer.start();

	// make upload signals available in QML
	// Remark: signal - signal connect
	connect(uploadDiveLogsDE::instance(), &uploadDiveLogsDE::uploadFinish,
			this, &QMLManager::uploadFinish);
	connect(uploadDiveLogsDE::instance(), &uploadDiveLogsDE::uploadProgress,
			this, &QMLManager::uploadProgress);
	connect(uploadDiveShare::instance(), &uploadDiveShare::uploadProgress,
			this, &QMLManager::uploadProgress);

	// uploadDiveShare::uploadFinish() is defined with 3 parameters,
	// whereas QMLManager::uploadFinish() is defined with 2 parameters,
	// Solution add a slot as landing zone.
	connect(uploadDiveShare::instance(), &uploadDiveShare::uploadFinish,
			this, &QMLManager::uploadFinishSlot);

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
	uiNotificationCallback = showProgress;
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
	if (ignore_bt) {
		m_btEnabled = false;
	} else {
		// ensure that we start the BTDiscovery - this should be triggered by the export of the class
		// to QML, but that doesn't seem to always work
		BTDiscovery *btDiscovery = BTDiscovery::instance();
		m_btEnabled = btDiscovery->btAvailable();
		connect(&btDiscovery->localBtDevice, &QBluetoothLocalDevice::hostModeStateChanged,
			this, &QMLManager::btHostModeChange);
	}
	// add log call back to the location manager service singleton
	GpsLocation::instance()->setLogCallBack(&appendTextToLogStandalone);
	progress_callback = &progressCallback;
	connect(GpsLocation::instance(), &GpsLocation::haveSourceChanged, this, &QMLManager::hasLocationSourceChanged);
	setLocationServiceAvailable(GpsLocation::instance()->hasLocationsSource());
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

	// monitor when dives changed - but only in verbose mode
	// careful - changing verbose at runtime isn't enough (of course that could be added if we want it)
	if (verbose)
		connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &QMLManager::divesChanged);

	// get updates to the undo/redo texts
	connect(Command::getUndoStack(), &QUndoStack::undoTextChanged, this, &QMLManager::undoTextChanged);
	connect(Command::getUndoStack(), &QUndoStack::redoTextChanged, this, &QMLManager::redoTextChanged);

	// now that everything is setup, connect the application changed signal
	connect(qobject_cast<QApplication *>(QApplication::instance()), &QApplication::applicationStateChanged, this, &QMLManager::applicationStateChanged);

	// we start out with clean data
	updateHaveLocalChanges(false);

	// setup Command infrastructure
	Command::init();
}

void QMLManager::applicationStateChanged(Qt::ApplicationState state)
{
	static bool initializeOnce = false;
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
	stateText.append((unsavedChanges() ? QLatin1String("") : QLatin1String("no ")) + QLatin1String("unsaved changes"));
	appendTextToLog(stateText);

	if (state == Qt::ApplicationActive && !m_initialized && !initializeOnce) {
		// once the app UI is displayed, finish our setup and mark the app as initialized
		initializeOnce = true;
		finishSetup();
		appInitialized();
	}
	if (state == Qt::ApplicationInactive && unsavedChanges()) {
		// saveChangesCloud ensures that we don't have two conflicting saves going on
		appendTextToLog("trying to save data as user switched away from app");
		saveChangesCloud(false);
		appendTextToLog("done trying to save to git local / remote");
	}
}

void QMLManager::openLocalThenRemote(QString url)
{
	// clear out the models and the fulltext index
	clear_dive_file_data();
	setDiveListProcessing(true);
	setNotificationText(tr("Open local dive data file"));
	appendTextToLog(QString("Open dive data file %1 - git_local only is %2").arg(url).arg(git_local_only));
	QByteArray encodedFilename = QFile::encodeName(url);

	/* if this is a cloud storage repo and we have no local cache (i.e., it's the first time
	 * we try to open this), parse_file (which is called by openAndMaybeSync) will ALWAYS connect
	 * to the remote and populate the cache.
	 * Otherwise parse_file will respect the git_local_only flag and only update if that isn't set */
	int error = parse_file(encodedFilename.constData(), &dive_table, &trip_table, &dive_site_table, &device_table, &filter_preset_table);
	if (error) {
		/* there can be 2 reasons for this:
		 * 1) we have cloud credentials, but there is no local repo (yet).
		 *    This implies that the PIN verify is still to be done.
		 * 2) we are in a very clean state after installing the app, and
		 *    want to use a NO CLOUD setup. The intial repo has no initial
		 *    commit in it, so its master branch does not yet exist. We do not
		 *    care about this, as the very first commit of dive data to the
		 *    no cloud repo solves this.
		 */
		auto credStatus = qPrefCloudStorage::cloud_verification_status();
		if (credStatus != qPrefCloudStorage::CS_NOCLOUD) {
			appendTextToLog(QStringLiteral("loading dives from cache failed %1").arg(error));
			setNotificationText(tr("Opening local data file failed"));
			if (credStatus != qPrefCloudStorage::CS_INCORRECT_USER_PASSWD)
				qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_NEED_TO_VERIFY);
		}
	} else {
		// if we can load from the cache, we know that we have a valid cloud account
		// and we know that there was at least one successful sync with the cloud when
		// that local cache was created - so there is a common ancestor
		setLoadFromCloud(true);
		if (qPrefCloudStorage::cloud_verification_status() == qPrefCloudStorage::CS_UNKNOWN)
			qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_VERIFIED);
		qPrefUnits::set_unit_system(git_prefs.unit_system);
		qPrefTechnicalDetails::set_tankbar(git_prefs.tankbar);
		qPrefTechnicalDetails::set_dcceiling(git_prefs.dcceiling);
		qPrefTechnicalDetails::set_show_ccr_setpoint(git_prefs.show_ccr_setpoint);
		qPrefTechnicalDetails::set_show_ccr_sensors(git_prefs.show_ccr_sensors);
		qPrefPartialPressureGas::set_po2(git_prefs.pp_graphs.po2);
		// the following steps can take a long time, so provide updates
		setNotificationText(tr("Processing %1 dives").arg(dive_table.nr));
		process_loaded_dives();
		setNotificationText(tr("%1 dives loaded from local dive data file").arg(dive_table.nr));
	}
	if (qPrefCloudStorage::cloud_verification_status() == qPrefCloudStorage::CS_NEED_TO_VERIFY) {
		appendTextToLog(QStringLiteral("have cloud credentials, but still needs PIN"));
	}
	if (qPrefCloudStorage::cloud_verification_status() == qPrefCloudStorage::CS_INCORRECT_USER_PASSWD) {
		appendTextToLog(QStringLiteral("incorrect password for cloud credentials"));
		setNotificationText(tr("Incorrect cloud credentials"));
	}
	if (m_oldStatus == qPrefCloudStorage::CS_NOCLOUD) {
		// if we switch to credentials from CS_NOCLOUD, we take things online temporarily
		git_local_only = false;
		appendTextToLog(QStringLiteral("taking things online to be able to switch to cloud account"));
	}
	set_filename(encodedFilename.constData());
	if (git_local_only && qPrefCloudStorage::cloud_verification_status() != qPrefCloudStorage::CS_NOCLOUD)
		appendTextToLog(QStringLiteral("have cloud credentials, but user asked not to connect to network"));

	// if we were unable to sync with the remote, we assume that there are local changes
	updateHaveLocalChanges(!git_remote_sync_successful);
	updateAllGlobalLists();
	setDiveListProcessing(false);
	// this could have added a new local cache directory
	emit cloudCacheListChanged();
}

// Convenience function to accesss dive directly via its row.
static struct dive *diveInRow(const QAbstractItemModel *model, int row)
{
	QModelIndex index = model->index(row, 0, QModelIndex());
	return index.isValid() ?  model->data(index, DiveTripModelBase::DIVE_ROLE).value<struct dive *>() : nullptr;
}

void QMLManager::selectRow(int row)
{
	dive *d = diveInRow(MobileModels::instance()->listModel(), row);
	select_single_dive(d);
}

void QMLManager::selectSwipeRow(int row)
{
	dive *d = diveInRow(MobileModels::instance()->swipeModel(), row);
	select_single_dive(d);
}

void QMLManager::updateAllGlobalLists()
{
	emit buddyListChanged();
	emit suitListChanged();
	emit divemasterListChanged();
	// TODO: It would be nice if we could export the list of locations via model/view instead of a Q_PROPERTY
	emit locationListChanged();
}

static QString nocloud_localstorage()
{
	return QString(system_default_directory()) + "/cloudstorage/localrepo[master]";
}

void QMLManager::mergeLocalRepo()
{
	struct dive_table table = empty_dive_table;
	struct trip_table trips = empty_trip_table;
	struct dive_site_table sites = empty_dive_site_table;
	struct device_table devices;
	struct filter_preset_table filter_presets;
	parse_file(qPrintable(nocloud_localstorage()), &table, &trips, &sites, &devices, &filter_presets);
	add_imported_dives(&table, &trips, &sites, &devices, IMPORT_MERGE_ALL_TRIPS);
	mark_divelist_changed(true);
}

void QMLManager::copyAppLogToClipboard()
{
	// The About page offers a button to copy logs so they can be pasted elsewhere
	QApplication::clipboard()->setText(getCombinedLogs(), QClipboard::Clipboard);
}

void QMLManager::copyGpsFixesToClipboard()
{
	// This of course creates a potential privacy issue, so let's be clear about that
	QString gpsWarning("Sending these GPS data to someone exposes your location history; ");
	gpsWarning += "they can, however, be helpful when debugging problems with the app. ";
	gpsWarning += "Please consider carefully where you are seninding these data.\n\n";
	gpsWarning += GpsLocation::instance()->getFixString();
	QApplication::clipboard()->setText(gpsWarning, QClipboard::Clipboard);
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
	appendTextToLog("finishSetup called");
	// Initialize cloud credentials.
	git_local_only = !prefs.cloud_auto_sync;

	QString url;
	if (!qPrefCloudStorage::cloud_storage_email().isEmpty() &&
	    !qPrefCloudStorage::cloud_storage_password().isEmpty() &&
	    getCloudURL(url) == 0) {
		openLocalThenRemote(url);
	} else if (!empty_string(existing_filename) &&
		   qPrefCloudStorage::cloud_verification_status() != qPrefCloudStorage::CS_UNKNOWN) {
		rememberOldStatus();
		set_filename(qPrintable(nocloud_localstorage()));
		qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_NOCLOUD);
		saveCloudCredentials(qPrefCloudStorage::cloud_storage_email(), qPrefCloudStorage::cloud_storage_password(), qPrefCloudStorage::cloud_storage_pin());
		appendTextToLog(tr("working in no-cloud mode"));
		int error = parse_file(existing_filename, &dive_table, &trip_table, &dive_site_table, &device_table, &filter_preset_table);
		if (error) {
			// we got an error loading the local file
			setNotificationText(tr("Error parsing local storage, giving up"));
			set_filename(NULL);
		} else {
			// successfully opened the local file, now add thigs to the dive list
			consumeFinishedLoad();
			updateHaveLocalChanges(true);
			appendTextToLog(QString("working in no-cloud mode, finished loading %1 dives from %2").arg(dive_table.nr).arg(existing_filename));
		}
	} else {
		qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_UNKNOWN);
		appendTextToLog(tr("no cloud credentials"));
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	}
	m_initialized = true;
	emit initializedChanged();
	// this could have brought in new cache directories, so make sure QML
	// calls our getter function again and doesn't show us outdated information
	emit cloudCacheListChanged();
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

void QMLManager::saveCloudCredentials(const QString &newEmail, const QString &newPassword, const QString &pin)
{
	bool cloudCredentialsChanged = false;
	bool noCloud = qPrefCloudStorage::cloud_verification_status() == qPrefCloudStorage::CS_NOCLOUD;

	// email address MUST be lower case or bad things happen
	QString email = newEmail.toLower();

	// make sure we only have letters, numbers, and +-_. in password and email address
	QRegularExpression regExp("^[a-zA-Z0-9@.+_-]+$");
	if (!noCloud) {
		// in case of NO_CLOUD, the email address + passwd do not care, so do not check it.
		if (newPassword.isEmpty() ||
			!regExp.match(newPassword).hasMatch() ||
			!regExp.match(email).hasMatch()) {
			setStartPageText(RED_FONT + tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.") + END_FONT);
			return;
		}
		// use the same simplistic regex as the backend to check email addresses
		regExp = QRegularExpression("^[a-zA-Z0-9.+_-]+@[a-zA-Z0-9.+_-]+\\.[a-zA-Z0-9]+");
		if (!regExp.match(email).hasMatch()) {
			setStartPageText(RED_FONT + tr("Invalid format for email address") + END_FONT);
			return;
		}
	}
	if (!same_string(prefs.cloud_storage_email, qPrintable(email))) {
		cloudCredentialsChanged = true;
	}

	if (!same_string(prefs.cloud_storage_password, qPrintable(newPassword))) {
		cloudCredentialsChanged = true;
	}

	if (qPrefCloudStorage::cloud_verification_status() != qPrefCloudStorage::CS_NOCLOUD &&
		!cloudCredentialsChanged) {
		// just go back to the dive list
		qPrefCloudStorage::set_cloud_verification_status(m_oldStatus);
	}

	if (!noCloud && !verifyCredentials(email, newPassword, pin)) {
		appendTextToLog("saveCloudCredentials: given cloud credentials didn't verify");
		return;
	}
	qPrefCloudStorage::set_cloud_storage_email(email);
	qPrefCloudStorage::set_cloud_storage_password(newPassword);

	if (m_oldStatus == qPrefCloudStorage::CS_NOCLOUD && cloudCredentialsChanged && dive_table.nr) {
		// we came from NOCLOUD and are connecting to a cloud account;
		// since we already have dives in the table, let's remember that so we can keep them
		noCloudToCloud = true;
		appendTextToLog("transitioning from no-cloud to cloud and have dives");
	}
	if (qPrefCloudStorage::cloud_storage_email().isEmpty() ||
		qPrefCloudStorage::cloud_storage_password().isEmpty()) {
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	} else if (cloudCredentialsChanged) {
		// let's make sure there are no unsaved changes
		saveChangesLocal();
		syncLoadFromCloud();
		manager()->clearAccessCache(); // remove any chached credentials
		clear_git_id(); // invalidate our remembered GIT SHA
		clear_dive_file_data();
		setStartPageText(tr("Attempting to open cloud storage with new credentials"));
		// since we changed credentials, we need to try to connect to the cloud, regardless
		// of whether we're in offline mode or not, to make sure the repository is synced
		currentGitLocalOnly = git_local_only;
		git_local_only = false;
		loadDivesWithValidCredentials();
	}
	rememberOldStatus();
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
		return false;
	} else if (prefs.cloud_verification_status == qPrefCloudStorage::CS_VERIFIED) {
		appendTextToLog(QStringLiteral("PIN accepted"));
		setStartPageText(RED_FONT + tr("PIN accepted, credentials verified") + END_FONT);
	}
	return true;
}

void QMLManager::loadDivesWithValidCredentials()
{
	QString url;
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
	} else {
		appendTextToLog("Cloud sync brought newer data, reloading the dive list");
		setDiveListProcessing(true);
		// if we aren't switching from no-cloud mode, let's clear the dive data
		if (!noCloudToCloud) {
			appendTextToLog("Clear out in memory dive data");
			clear_dive_file_data();
		} else {
			appendTextToLog("Switching from no cloud mode; keep in memory dive data");
		}
		if (git != dummy_git_repository) {
			appendTextToLog(QString("have repository and branch %1").arg(branch));
			error = git_load_dives(git, branch, &dive_table, &trip_table, &dive_site_table, &device_table, &filter_preset_table);
		} else {
			appendTextToLog(QString("didn't receive valid git repo, try again"));
			error = parse_file(fileNamePrt.data(), &dive_table, &trip_table, &dive_site_table, &device_table, &filter_preset_table);
		}
		setDiveListProcessing(false);
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
		consumeFinishedLoad();
	}
	setLoadFromCloud(true);

	// if we came from local storage mode, let's merge the local data into the local cache
	// for the remote data - which then later gets merged with the remote data if necessary
	if (noCloudToCloud) {
		git_storage_update_progress(qPrintable(tr("Loading dives from local storage ('no cloud' mode)")));
		mergeLocalRepo();
		appendTextToLog(QStringLiteral("%1 dives loaded after importing nocloud local storage").arg(dive_table.nr));
		noCloudToCloud = false;
		mark_divelist_changed(true);
		emit syncStateChanged();
		saveChangesLocal();
		if (git_local_only == false) {
			appendTextToLog(QStringLiteral("taking things back offline now that storage is synced"));
			git_local_only = true;
		}
	}
	// if we successfully synced with the cloud, update that status
	updateHaveLocalChanges(!git_remote_sync_successful);
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
	if (m_oldStatus == qPrefCloudStorage::CS_NOCLOUD) {
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
		qPrefCloudStorage::set_cloud_storage_password("");
		rememberOldStatus();
		qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_NOCLOUD);
		set_filename(qPrintable(nocloud_localstorage()));
		setStartPageText(RED_FONT + tr("Failed to connect to cloud server, reverting to no cloud status") + END_FONT);
	}
}

void QMLManager::consumeFinishedLoad()
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
	appendTextToLog(QStringLiteral("%1 dives loaded").arg(dive_table.nr));
	if (dive_table.nr == 0)
		setStartPageText(tr("Cloud storage open successfully. No dives in dive list."));
}

void QMLManager::refreshDiveList()
{
	MobileModels::instance()->invalidate();
}

// Ouch. Editing a dive might create a dive site or change an existing dive site.
// The following structure describes such a change caused by a dive edit.
// Hopefully, we can remove this in due course by using finer-grained undo-commands.
struct DiveSiteChange {
	Command::OwningDiveSitePtr createdDs; // not-null if we created a dive site.

	dive_site *editDs = nullptr; // not-null if we are supposed to edit an existing dive site.
	location_t location = zero_location; // new value of the location if we edit an existing dive site.

	bool changed = false; // true if either a dive site or the dive was changed.
};

static void setupDivesite(DiveSiteChange &res, struct dive *d, struct dive_site *ds, double lat, double lon, const char *locationtext)
{
	location_t location = create_location(lat, lon);
	if (ds) {
		res.editDs = ds;
		res.location = location;
	} else {
		res.createdDs.reset(alloc_dive_site_with_name(locationtext));
		d->dive_site = res.createdDs.get();
	}
	res.changed = true;
}

bool QMLManager::checkDate(struct dive *d, QString date)
{
	QString oldDate = formatDiveDateTime(d);
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
			d->dc.when = d->when = dateTimeToTimestamp(newDate);
			return true;
		}
		appendTextToLog("none of our parsing attempts worked for the date string");
	}
	return false;
}

bool QMLManager::checkLocation(DiveSiteChange &res, struct dive *d, QString location, QString gps)
{
	struct dive_site *ds = get_dive_site_for_dive(d);
	bool changed = false;
	QString oldLocation = get_dive_location(d);
	if (oldLocation != location) {
		ds = get_dive_site_by_name(qPrintable(location), &dive_site_table);
		if (!ds && !location.isEmpty()) {
			res.createdDs.reset(alloc_dive_site_with_name(qPrintable(location)));
			res.changed = true;
			ds = res.createdDs.get();
		}
		d->dive_site = ds;
		changed = true;
	}
	// now make sure that the GPS coordinates match - if the user changed the name but not
	// the GPS coordinates, this still does the right thing as the now new dive site will
	// have no coordinates, so the coordinates from the edit screen will get added
	if (formatDiveGPS(d) != gps) {
		double lat, lon;
		if (parseGpsText(gps, &lat, &lon)) {
			qDebug() << "parsed GPS, using it";
			// there are valid GPS coordinates - just use them
			setupDivesite(res, d, ds, lat, lon, qPrintable(location));
		} else if (gps == GPS_CURRENT_POS) {
			qDebug() << "gps was our default text for no GPS";
			// user asked to use current pos
			QString gpsString = getCurrentPosition();
			if (gpsString != GPS_CURRENT_POS) {
				qDebug() << "but now I got a valid location" << gpsString;
				if (parseGpsText(qPrintable(gpsString), &lat, &lon))
					setupDivesite(res, d, ds, lat, lon, qPrintable(location));
			} else {
				appendTextToLog("couldn't get GPS location in time");
			}
		} else {
			// just something we can't parse, so tell the user
			appendTextToLog(QString("wasn't able to parse gps string '%1'").arg(gps));
		}
	}
	return changed | res.changed;
}

bool QMLManager::checkDuration(struct dive *d, QString duration)
{
	if (formatDiveDuration(d) != duration) {
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

bool QMLManager::checkDepth(dive *d, QString depth)
{
	if (get_depth_string(d->dc.maxdepth.mm, true, true) != depth) {
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
	struct dive *orig = get_dive_by_uniq_id(diveId.toInt());

	if (!orig) {
		appendTextToLog("cannot commit changes: no dive");
		return;
	}
	if (verbose) {
		qDebug().noquote() << QStringLiteral("diveId  :'%1'\n").arg(diveId) <<
				      QStringLiteral("number  :'%1'\n").arg(number) <<
				      QStringLiteral("date    :'%1'\n").arg(date) <<
				      QStringLiteral("location:'%1'\n").arg(location) <<
				      QStringLiteral("gps     :'%1'\n").arg(gps) <<
				      QStringLiteral("duration:'%1'\n").arg(duration) <<
				      QStringLiteral("depth   :'%1'\n").arg(depth) <<
				      QStringLiteral("airtemp :'%1'\n").arg(airtemp) <<
				      QStringLiteral("watertmp:'%1'\n").arg(watertemp) <<
				      QStringLiteral("suit    :'%1'\n").arg(suit) <<
				      QStringLiteral("buddy   :'%1'\n").arg(buddy) <<
				      QStringLiteral("diveMstr:'%1'\n").arg(diveMaster) <<
				      QStringLiteral("weight  :'%1'\n").arg(weight) <<
				      QStringLiteral("state   :'%1'\n").arg(state);
	}

	Command::OwningDivePtr d_ptr(alloc_dive()); // Automatically delete dive if we exit early!
	dive *d = d_ptr.get();
	copy_dive(orig, d);

	// notes comes back as rich text - let's convert this into plain text
	QTextDocument doc;
	doc.setHtml(notes);
	notes = doc.toPlainText();

	bool diveChanged = false;

	diveChanged = checkDate(d, date);

	DiveSiteChange dsChange;
	diveChanged |= checkLocation(dsChange, d, location, gps);

	diveChanged |= checkDuration(d, duration);

	diveChanged |= checkDepth(d, depth);

	if (QString::number(d->number) != number) {
		diveChanged = true;
		d->number = number.toInt();
	}
	if (get_temperature_string(d->airtemp, true) != airtemp) {
		diveChanged = true;
		d->airtemp.mkelvin = parseTemperatureToMkelvin(airtemp);
	}
	if (get_temperature_string(d->watertemp, true) != watertemp) {
		diveChanged = true;
		d->watertemp.mkelvin = parseTemperatureToMkelvin(watertemp);
	}
	if (formatSumWeight(d) != weight) {
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
	// start and end pressures
	// first, normalize the lists - QML gives us a list with just one empty string if nothing was entered
	if (startpressure == QStringList(QString()))
		startpressure = QStringList();
	if (endpressure == QStringList(QString()))
		endpressure = QStringList();
	if (formatStartPressure(d) != startpressure || formatEndPressure(d) != endpressure) {
		diveChanged = true;
		for ( int i = 0, j = 0 ; j < startpressure.length() && j < endpressure.length() ; i++ ) {
			if (state != "add" && !is_cylinder_used(d, i))
				continue;

			cylinder_t *cyl = get_or_create_cylinder(d, i);
			cyl->start.mbar = parsePressureToMbar(startpressure[j]);
			cyl->end.mbar = parsePressureToMbar(endpressure[j]);
			if (cyl->end.mbar > cyl->start.mbar)
				cyl->end.mbar = cyl->start.mbar;

			j++;
		}
	}
	// gasmix for first cylinder
	if (formatFirstGas(d) != gasmix) {
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
				get_or_create_cylinder(d, i)->gasmix.o2.permille = o2;
				get_cylinder(d, i)->gasmix.he.permille = he;
			}
			j++;
		}
	}
	// info for first cylinder
	if (formatGetCylinder(d) != usedCylinder) {
		diveChanged = true;
		int size = 0, wp = 0, j = 0, k = 0;
		for (j = 0; k < usedCylinder.length(); j++) {
			if (state != "add" && !is_cylinder_used(d, j))
				continue;

			for (int i = 0; i < tank_info_table.nr; i++) {
				const tank_info &ti = tank_info_table.infos[i];
				if (ti.name == usedCylinder[k] ) {
					if (ti.ml > 0){
						size = ti.ml;
						wp = ti.bar * 1000;
					} else {
						size = (int) (cuft_to_l(ti.cuft) * 1000 / bar_to_atm(psi_to_bar(ti.psi)));
						wp = psi_to_mbar(ti.psi);
					}
					break;
				}
			}
			get_or_create_cylinder(d, j)->type.description = copy_qstring(usedCylinder[k]);
			get_cylinder(d, j)->type.size.mliter = size;
			get_cylinder(d, j)->type.workingpressure.mbar = wp;
			k++;
		}
	}
	if (d->suit != suit) {
		diveChanged = true;
		free(d->suit);
		d->suit = copy_qstring(suit);
	}
	if (d->buddy != buddy) {
		if (buddy.contains(",")){
			buddy = buddy.replace(QRegExp("\\s*,\\s*"), ", ");
		}
		diveChanged = true;
		free(d->buddy);
		d->buddy = copy_qstring(buddy);
	}
	if (d->divemaster != diveMaster) {
		if (diveMaster.contains(",")){
			diveMaster = diveMaster.replace(QRegExp("\\s*,\\s*"), ", ");
		}
		diveChanged = true;
		free(d->divemaster);
		d->divemaster = copy_qstring(diveMaster);
	}
	if (d->rating != rating) {
		diveChanged = true;
		d->rating = rating;
	}
	if (d->visibility != visibility) {
		diveChanged = true;
		d->visibility = visibility;
	}
	if (formatNotes(d) != notes) {
		diveChanged = true;
		free(d->notes);
		d->notes = copy_qstring(notes);
	}
	// now that we have it all figured out, let's see what we need
	// to update
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
		Command::editDive(orig, d_ptr.release(), dsChange.createdDs.release(), dsChange.editDs, dsChange.location); // With release() we're giving up ownership
		changesNeedSaving();
	}
}

void QMLManager::updateTripDetails(QString tripIdString, QString tripLocation, QString tripNotes)
{
	int tripId = tripIdString.toInt();
	dive_trip_t *trip = get_trip_by_uniq_id(tripId);
	if (!trip) {
		qDebug() << "updateTripData: cannot find trip for tripId" << tripIdString;
		return;
	}
	bool changed = false;
	if (tripLocation != trip->location) {
		changed = true;
		Command::editTripLocation(trip, tripLocation);
	}
	if (tripNotes != trip->notes) {
		changed = true;
		Command::editTripNotes(trip, tripNotes);
	}
	if (changed)
		changesNeedSaving();
}

void QMLManager::removeDiveFromTrip(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog(QString("Asked to remove non-existing dive with id %1 from its trip.").arg(id));
		return;
	}
	if (!d->divetrip) {
		appendTextToLog(QString("Asked to remove dive with id %1 from its trip (but it's not part of a trip).").arg(id));
		return;
	}
	QVector <dive *> dives;
	dives.append(d);
	Command::removeDivesFromTrip(dives);
	changesNeedSaving();
}

void QMLManager::addTripForDive(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog(QString("Asked to create trip for non-existing dive with id %1").arg(id));
		return;
	}
	if (d->divetrip) {
		appendTextToLog(QString("Asked to create trip for dive %1 with id %2 but it's already part of a trip with location %3.").arg(d->number).arg(id).arg(d->divetrip->location));
		return;
	}
	QVector <dive *> dives;
	dives.append(d);
	Command::createTrip(dives);
	changesNeedSaving();
}

void QMLManager::addDiveToTrip(int id, int tripId)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog(QString("Asked to add non-existing dive with id %1 to trip %2.").arg(id).arg(tripId));
		return;
	}
	struct dive_trip *dt = get_trip_by_uniq_id(tripId);
	if (!dt) {
		appendTextToLog(QString("Asked to add dive with id %1 to trip with id %2 which cannot be found.").arg(id).arg(tripId));
		return;
	}
	QVector <dive *> dives;
	dives.append(d);
	Command::addDivesToTrip(dives, dt);
	changesNeedSaving();
}

void QMLManager::changesNeedSaving(bool fromUndo)
{
	// we no longer save right away on iOS because file access is so slow; on the other hand,
	// on Android the save as the user switches away doesn't seem to work... drat.
	// as a compromise for now we save just to local storage on Android right away (that appears
	// to be reasonably fast), but don't save at all (and only remember that we need to save things
	// on iOS
	// on all other platforms we just save the changes and be done with it
	mark_divelist_changed(true);
	emit syncStateChanged();
#if defined(Q_OS_IOS)
	saveChangesLocal(fromUndo);
#else
	saveChangesCloud(false, fromUndo);
#endif
	updateAllGlobalLists();
}

void QMLManager::openNoCloudRepo()
{
	/*
	 * Open the No Cloud repo. In case this repo does not (yet)
	 * exists, create one first. When done, open the repo, which
	 * is obviously empty when just created.
	 */
	QString filename = nocloud_localstorage();
	const char *branch;
	struct git_repository *git;

	appendTextToLog(QString("User asked not to connect to cloud, using %1 as repo.").arg(filename));
	git = is_git_repository(qPrintable(filename), &branch, NULL, false);

	if (git == dummy_git_repository) {
		// repo doesn't exist, create it and write the empty dive list to it
		git_create_local_repo(qPrintable(filename));
		save_dives(qPrintable(filename));
		set_filename(qPrintable(filename));
		auto s = qPrefLog::instance();
		s->set_default_filename(qPrintable(filename));
		s->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	}
	openLocalThenRemote(filename);
}

void QMLManager::saveChangesLocal(bool fromUndo)
{
	if (unsavedChanges()) {
		if (qPrefCloudStorage::cloud_verification_status() == qPrefCloudStorage::CS_NOCLOUD) {
			if (empty_string(existing_filename)) {
				QString filename = nocloud_localstorage();
				git_create_local_repo(qPrintable(filename));
				set_filename(qPrintable(filename));
				auto s = qPrefLog::instance();
				s->set_default_filename(qPrintable(filename));
				s->set_default_file_behavior(LOCAL_DEFAULT_FILE);
			}
		} else if (!m_loadFromCloud) {
			// this seems silly, but you need a common ancestor in the repository in
			// order to be able to merge che changes later
			appendTextToLog("Don't save dives without loading from the cloud, first.");
			return;
		}
		bool glo = git_local_only;
		git_local_only = true;
		int error = save_dives(existing_filename);
		git_local_only = glo;
		if (error) {
			setNotificationText(consumeError());
			set_filename(NULL);
			return;
		}
		mark_divelist_changed(false);
		Command::setClean();
		updateHaveLocalChanges(true);
		// provide a useful undo/redo notification
		// NOTE: the QML UI interprets a leading '[action]' (where only the two brackets are checked for)
		//       as an indication to use the text between those two brackets as the label of a button that
		//       can be used to open the context menu
		QString msgFormat = tr("[%1]Changes saved:'%2'.\n%1 possible via context menu");
		if (fromUndo)
			setNotificationText(msgFormat.arg(tr("Redo")).arg(tr("Undo: %1").arg(getRedoText())));
		else
			setNotificationText(msgFormat.arg(tr("Undo")).arg(getUndoText()));
	} else {
		appendTextToLog("local save requested with no unsaved changes");
	}
}

void QMLManager::saveChangesCloud(bool forceRemoteSync, bool fromUndo)
{
	if (!unsavedChanges() && !forceRemoteSync) {
		appendTextToLog("asked to save changes but no unsaved changes");
		return;
	}
	// first we need to store any unsaved changes to the local repo
	gitProgressCB("Save changes to local cache");
	saveChangesLocal(fromUndo);
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
	loadDivesWithValidCredentials();
	git_local_only = glo;
}

void QMLManager::undo()
{
	Command::getUndoStack()->undo();
	changesNeedSaving(true);
}

void QMLManager::redo()
{
	Command::getUndoStack()->redo();
	changesNeedSaving();
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
		qWarning("QManager::selectDive() called with unknown id %d",id);
}

void QMLManager::deleteDive(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog("trying to delete non-existing dive");
		return;
	}
	Command::deleteDive(QVector<dive *>{ d });
	changesNeedSaving();
}

void QMLManager::toggleDiveInvalid(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog("trying to toggle invalid flag of non-existing dive");
		return;
	}
	Command::editInvalid(!d->invalid, true);
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
	if (!m_copyPasteDive) {
		appendTextToLog("dive to paste is not selected");
		return;
	}
	Command::pasteDives(m_copyPasteDive, what);
	changesNeedSaving();
}

void QMLManager::cancelDownloadDC()
{
	import_thread_cancelled = true;
}

int QMLManager::addDive()
{
	// TODO: Duplicate code with desktop-widgets/mainwindow.cpp
	// create a dive an hour from now with a default depth (15m/45ft) and duration (40 minutes)
	// as a starting point for the user to edit
	struct dive d = { 0 };
	int diveId = d.id = dive_getUniqID();
	d.when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset() + 3600;
	d.dc.duration.seconds = 40 * 60;
	d.dc.maxdepth.mm = M_OR_FT(15, 45);
	d.dc.meandepth.mm = M_OR_FT(13, 39); // this creates a resonable looking safety stop
	d.dc.model = strdup("manually added dive"); // don't translate! this is stored in the XML file
	fake_dc(&d.dc);
	fixup_dive(&d);

	// addDive takes over the dive and clears out the structure passed in
	// we do NOT save the modified data at this stage because of the UI flow here... this will
	// be saved once the user finishes editing the newly added dive
	Command::addDive(&d, autogroup, true);

	if (verbose)
		appendTextToLog(QString("Adding new dive with id '%1'").arg(diveId));
	// the QML UI uses the return value to set up the edit screen
	return diveId;
}

QString QMLManager::getCurrentPosition()
{
	static bool hasLocationSource = false;
	if (GpsLocation::instance()->hasLocationsSource() != hasLocationSource) {
		hasLocationSource = !hasLocationSource;
		setLocationServiceAvailable(hasLocationSource);
	}
	if (!hasLocationSource)
		return tr("Unknown GPS location");

	QString positionResponse = GpsLocation::instance()->currentPosition();
	if (positionResponse == GPS_CURRENT_POS)
		connect(GpsLocation::instance(), &GpsLocation::acquiredPosition, this, &QMLManager::waitingForPositionChanged, Qt::UniqueConnection);
	else
		disconnect(GpsLocation::instance(), &GpsLocation::acquiredPosition, this, &QMLManager::waitingForPositionChanged);
	return positionResponse;
}

void QMLManager::applyGpsData()
{
	appendTextToLog("Applying GPS fiexs");
	std::vector<DiveAndLocation> fixes = GpsLocation::instance()->getLocations();
	Command::applyGPSFixes(fixes);
	appendTextToLog(QString("Attached %1 GPS fixes").arg(fixes.size()));
	if (fixes.size())
		changesNeedSaving();
}

void QMLManager::populateGpsData()
{
	if (GpsListModel::instance())
		GpsListModel::instance()->update();
}

void QMLManager::clearGpsData()
{
	GpsLocation::instance()->clearGpsData();
	populateGpsData();
}

void QMLManager::deleteGpsFix(quint64 when)
{
	GpsLocation::instance()->deleteGpsFix(when);
	populateGpsData();
}

QString QMLManager::logText() const
{
	QString logText = m_logText + QString("\nNumer of GPS fixes: %1").arg(GpsLocation::instance()->getGpsNum());
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
	GpsLocation::instance()->serviceEnable(m_locationServiceEnabled);
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
	setLocationServiceAvailable(GpsLocation::instance()->hasLocationsSource());
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
	QString cloudMarker = QLatin1String("loadFromCloud") + QString(prefs.cloud_storage_email);
	m_loadFromCloud = s.contains(cloudMarker) && s.value(cloudMarker).toBool();
}

void QMLManager::setLoadFromCloud(bool done)
{
	QSettings s;
	QString cloudMarker = QLatin1String("loadFromCloud") + QString(prefs.cloud_storage_email);
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
	appendTextToLog(QStringLiteral("showProgress: ") + text);
	m_notificationText = text;
	emit notificationTextChanged();
	// Once we're initialized, this may be called from signal context, notably when selecting an action from a context menu.
	// Processing events may now cause the menu to be deleted. Deleting a QML object which sent a signal causes QML to quit the application.
	// Therefore, don't process events once the application is started.
	// During startup this is needed so that the notifications are shown.
	if (!m_initialized)
		qApp->processEvents();
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
	if (unsavedChanges())
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

	for (int ti = 0; ti < tank_info_table.nr; ti++) {
		QString cyl = tank_info_table.infos[ti].name;
		if (cyl == "")
			continue;
		cylinders << cyl;
	}

	cylinders.removeDuplicates();
	cylinders.sort();
	// now add fist one that indicates that the user wants no default cylinder
	cylinders.prepend(tr("no default cylinder"));
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
	QtAndroid::runOnAndroidThread([color]() {
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
#if defined(Q_OS_ANDROID)
	// get the currently valid list of devices and set up the USB device descriptor
	// if the connection string matches a connection in that list
	androidSerialDevices = serial_usb_android_get_devices();
	std::string connection = devName.toStdString();
	for (unsigned int i = 0; i < androidSerialDevices.size(); i++) {
		if (androidSerialDevices[i].uiRepresentation == connection) {
			appendTextToLog(QString("setDevName matches USB device %1").arg(i));
			DCDeviceData::instance()->setUsbDevice(androidSerialDevices[i]);
		}
	}
#endif
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

#if defined(Q_OS_ANDROID)
// try to guess which dive computer was plugged into the USB port
QString QMLManager::getProductVendorConnectionIdx(android_usb_serial_device_descriptor descriptor)
{
	// convert the information we get from the Android USB layer into indices for the three
	// combo boxes for vendor, product, and connection in the QML UI
	// for each of these values '-1' means that no entry should be pre-selected
	QString uiString = QString::fromStdString(descriptor.uiRepresentation);
	QString product = QString::fromStdString(descriptor.usbProduct);
	int connIdx = connectionListModel.indexOf(uiString);
	int vendIdx = -1;
	int prodIdx = -1;

	if (!descriptor.manufacturer.empty())
		vendIdx = vendorList.indexOf(QString::fromStdString(descriptor.manufacturer));
	if (!descriptor.product.empty())
		prodIdx = productList[vendorList[vendIdx]].indexOf(QString::fromStdString(descriptor.product));

	// the rest of them simply will get the default -1:-1:index of the uiString
	return QString("%1;%2;%3").arg(vendIdx).arg(prodIdx).arg(connIdx);
}

void QMLManager::androidUsbPopoulateConnections()
{
	// let's understand what devices are available
	androidSerialDevices = serial_usb_android_get_devices();
	appendTextToLog(QString("rescanning USB: %1 devices reported").arg(androidSerialDevices.size()));

	// list all USB devices, this will include the one that triggered the intent
	for (unsigned int i = 0; i < androidSerialDevices.size(); i++) {
		QString uiString = QString::fromStdString(androidSerialDevices[i].uiRepresentation);
		appendTextToLog(QString("found USB device with ui string %1").arg(uiString));
		connectionListModel.addAddress(uiString);
	}
}

void QMLManager::showDownloadPage(QAndroidJniObject usbDevice)
{
	if (!usbDevice.isValid()) {
		// this really shouldn't happen anymore, but just in case...
		m_pluggedInDeviceName = "";
	} else {
		// repopulate the connection list
		rescanConnections();

		// parse the usbDevice
		android_usb_serial_device_descriptor usbDeviceDescriptor = getDescriptor(usbDevice);

		// inform the QML UI that it should show the download page
		m_pluggedInDeviceName = getProductVendorConnectionIdx(usbDeviceDescriptor);
	}
	emit pluggedInDeviceNameChanged();
}

void QMLManager::restartDownload(QAndroidJniObject usbDevice)
{
	// this gets called if we received a permission intent after
	// already trying to download from USB
	if (!usbDevice.isValid()) {
		appendTextToLog("permission intent with invalid UsbDevice - ignoring");
	} else {
		// inform that QML code that it should retry downloading
		// we get the usbDevice again and could verify that this is
		// still the same - but I don't see how this could change while we are waiting
		// for permission
		android_usb_serial_device_descriptor usbDeviceDescriptor = getDescriptor(usbDevice);
		appendTextToLog(QString("got permission from Android, restarting download for %1")
				.arg(QString::fromStdString(usbDeviceDescriptor.uiRepresentation)));
		emit restartDownloadSignal();
	}
}

#endif

static filter_constraint make_filter_constraint(filter_constraint_type type, const QString &s)
{
	filter_constraint res(type);
	filter_constraint_set_stringlist(res, s);
	return res;
}

void QMLManager::setFilter(const QString filterText, int index)
{
	QString f = filterText.trimmed();
	FilterData data;
	if (!f.isEmpty()) {
		// This is ugly - the indices of the mode are hardcoded!
		switch(index) {
		default:
		case 0:
			data.fullText = f;
			break;
		case 1:
			data.constraints.push_back(make_filter_constraint(FILTER_CONSTRAINT_PEOPLE, f));
			break;
		case 2:
			data.constraints.push_back(make_filter_constraint(FILTER_CONSTRAINT_TAGS, f));
			break;
		}
	}
	DiveFilter::instance()->setFilter(data);
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

#if !defined(Q_OS_ANDROID)
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
				std::vector<const dive_site *> sites = getDiveSitesToExport(false);
				save_dive_sites_logic(qPrintable(fileName + ".xml"), sites.data(), (int)sites.size(), anonymize);
				break;
			}
		case EX_UDDF:
			exportUsingStyleSheet(fileName + ".uddf", true, 0, "uddf-export.xslt", anonymize);
			break;
		default:
			qDebug() << "export to unknown type " << type << " using " << dir << " remove names " << anonymize;
			break;
	}
}
#endif

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

qPrefCloudStorage::cloud_status QMLManager::oldStatus() const
{
	return m_oldStatus;
}

void QMLManager::setOldStatus(const qPrefCloudStorage::cloud_status value)
{
	if (m_oldStatus != value) {
		m_oldStatus = value;
		emit oldStatusChanged();
	}
}

void QMLManager::rememberOldStatus()
{
	setOldStatus((qPrefCloudStorage::cloud_status)qPrefCloudStorage::cloud_verification_status());
}

void QMLManager::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	Q_UNUSED(field)
	for (struct dive *d: dives) {
		qDebug() << "dive #" << d->number << "changed, cache is" << (dive_cache_is_valid(d) ? "valid" : "invalidated");
		// a brute force way to deal with that would of course be to call
		// invalidate_dive_cache(d);
	}
}

QString QMLManager::getUndoText() const
{
	QString undoText = Command::getUndoStack()->undoText();
	return undoText;
}

QString QMLManager::getRedoText() const
{
	QString redoText = Command::getUndoStack()->redoText();
	return redoText;
}

void QMLManager::setDiveListProcessing(bool value)
{
	if (m_diveListProcessing != value) {
		m_diveListProcessing = value;
		emit diveListProcessingChanged();
	}

}

void QMLManager::importCacheRepo(QString repo)
{
	struct dive_table table = empty_dive_table;
	struct trip_table trips = empty_trip_table;
	struct dive_site_table sites = empty_dive_site_table;
	struct device_table devices;
	struct filter_preset_table filter_presets;
	QString repoPath = QString("%1/cloudstorage/%2").arg(system_default_directory()).arg(repo);
	appendTextToLog(QString("importing %1").arg(repoPath));
	parse_file(qPrintable(repoPath), &table, &trips, &sites, &devices, &filter_presets);
	add_imported_dives(&table, &trips, &sites, &devices, IMPORT_MERGE_ALL_TRIPS);
	changesNeedSaving();
}

QStringList QMLManager::cloudCacheList() const
{
	QDir localCacheDir(QString("%1/cloudstorage/").arg(system_default_directory()));
	QStringList dirs = localCacheDir.entryList().filter(QRegExp("...+"));
	QStringList result;
	foreach(QString dir, dirs) {
		QString originsDir = QString("%1/cloudstorage/%2/.git/refs/remotes/origin/").arg(system_default_directory()).arg(dir);
		QDir remote(originsDir);
		if (dir == "localrepo") {
			result << QString("localrepo[master]");
		} else {
			foreach(QString branch, remote.entryList().filter(QRegExp("...+"))) {
				result << QString("%1[%2]").arg(dir).arg(branch);
			}
		}
	}
	return result;
}

void QMLManager::updateHaveLocalChanges(bool status)
{
	localChanges = status;
	emit syncStateChanged();
}

// show the state of the dive list
QString QMLManager::getSyncState() const
{
	if (unsavedChanges())
		return tr("(unsaved changes in memory)");
	if (localChanges)
		return tr("(changes synced locally)");
	return tr("(synced with cloud)");
}
