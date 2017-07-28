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

#include "qt-models/divelistmodel.h"
#include "qt-models/gpslistmodel.h"
#include "core/divelist.h"
#include "core/device.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/qt-gui.h"
#include "core/git-access.h"
#include "core/cloudstorage.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"
#include "core/membuffer.h"
#include "qt-models/tankinfomodel.h"
#include "core/downloadfromdcthread.h"

QMLManager *QMLManager::m_instance = NULL;

#define RED_FONT QLatin1Literal("<font color=\"red\">")
#define END_FONT QLatin1Literal("</font>")

#define NOCLOUD_LOCALSTORAGE format_string("%s/cloudstorage/localrepo[master]", system_default_directory())

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

QMLManager::QMLManager() : m_locationServiceEnabled(false),
	m_verboseEnabled(false),
	reply(0),
	deletedDive(0),
	deletedTrip(0),
	m_updateSelectedDive(-1),
	m_selectedDiveTimestamp(0),
	m_credentialStatus(UNKNOWN),
	alreadySaving(false),
	m_device_data(new DCDeviceData(this)),
	m_libdcLog(false)
{
	m_instance = this;
	m_lastDevicePixelRatio = qApp->devicePixelRatio();
	connect(qobject_cast<QApplication *>(QApplication::instance()), &QApplication::applicationStateChanged, this, &QMLManager::applicationStateChanged);

	QString libdcLogFileName = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).first() + "/libdivecomputer.log";
	logfile_name = strdup(libdcLogFileName.toUtf8().data());
#if defined(Q_OS_ANDROID)
	appLogFileName = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).first() + "/subsurface.log";
	appLogFile.setFileName(appLogFileName);
	if (!appLogFile.open(QIODevice::ReadWrite|QIODevice::Truncate)) {
		appLogFileOpen = false;
		appendTextToLog("Failed to open logfile " + appLogFileName
				+ " at " + QDateTime::currentDateTime().toString());
	} else {
		appLogFileOpen = true;
		appendTextToLog("Successfully opened logfile " + appLogFileName
				+ " at " + QDateTime::currentDateTime().toString());
	}
#endif
	appendTextToLog("Starting " + getUserAgent());
	appendTextToLog(QStringLiteral("build with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion()));
	setStartPageText(tr("Starting..."));

#if defined(BT_SUPPORT)
	// ensure that we start the BTDiscovery - this should be triggered by the export of the class
	// to QML, but that doesn't seem to always work
	BTDiscovery *btDiscovery = BTDiscovery::instance();
	m_btEnabled = btDiscovery->btAvailable();
#else
	m_btEnabled = false;
#endif
	setShowPin(false);
	// create location manager service
	locationProvider = new GpsLocation(&appendTextToLogStandalone, this);
	progress_callback = &progressCallback;
	connect(locationProvider, SIGNAL(haveSourceChanged()), this, SLOT(hasLocationSourceChanged()));
	setLocationServiceAvailable(locationProvider->hasLocationsSource());
	set_git_update_cb(&gitProgressCB);

	// make sure we know if the current cloud repo has been successfully synced
	syncLoadFromCloud();
}

void QMLManager::applicationStateChanged(Qt::ApplicationState state)
{
	if (!timer.isValid())
		timer.start();
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
		appendTextToLog(QString::number(timer.elapsed() / 1000.0,'f', 3) + ": done saving to git local / remote");
	}
}

void QMLManager::openLocalThenRemote(QString url)
{
	clear_dive_file_data();
	setNotificationText(tr("Open local dive data file"));
	QByteArray fileNamePrt = QFile::encodeName(url);
	bool glo = prefs.git_local_only;
	prefs.git_local_only = true;
	int error = parse_file(fileNamePrt.data());
	prefs.git_local_only = glo;
	if (error) {
		appendTextToLog(QStringLiteral("loading dives from cache failed %1").arg(error));
		setNotificationText(tr("Opening local data file failed"));
	} else {
		// if we can load from the cache, we know that we have at least a valid email
		if (credentialStatus() == UNKNOWN)
			setCredentialStatus(VALID_EMAIL);
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
		process_dives(false, false);
		DiveListModel::instance()->clear();
		DiveListModel::instance()->addAllDives();
		appendTextToLog(QStringLiteral("%1 dives loaded from cache").arg(dive_table.nr));
		setNotificationText(tr("%1 dives loaded from local dive data file").arg(dive_table.nr));
	}
	if (oldStatus() == NOCLOUD) {
		// if we switch to credentials from NOCLOUD, we take things online temporarily
		prefs.git_local_only = false;
		appendTextToLog(QStringLiteral("taking things online to be able to switch to cloud account"));
	}
	set_filename(fileNamePrt.data(), true);
	if (prefs.git_local_only) {
		appendTextToLog(QStringLiteral("have cloud credentials, but user asked not to connect to network"));
		alreadySaving = false;
	} else {
		appendTextToLog(QStringLiteral("have cloud credentials, trying to connect"));
		tryRetrieveDataFromBackend();
	}
}

void QMLManager::mergeLocalRepo()
{
	char *filename = NOCLOUD_LOCALSTORAGE;
	parse_file(filename);
	process_dives(true, false);
}

void QMLManager::finishSetup()
{
	// Initialize cloud credentials.
	setCloudUserName(prefs.cloud_storage_email);
	setCloudPassword(prefs.cloud_storage_password);
	setSyncToCloud(!prefs.git_local_only);
	// if the cloud credentials are valid, we should get the GPS Webservice ID as well
	QString url;
	if (!cloudUserName().isEmpty() &&
	    !cloudPassword().isEmpty() &&
	    getCloudURL(url) == 0) {
		// we know that we are the first ones to access git storage, so we don't need to test,
		// but we need to make sure we stay the only ones accessing git storage
		alreadySaving = true;
		openLocalThenRemote(url);
	} else if (!same_string(existing_filename, "")) {
		setCredentialStatus(NOCLOUD);
		appendTextToLog(tr("working in no-cloud mode"));
		int error = parse_file(existing_filename);
		if (error) {
			// we got an error loading the local file
			appendTextToLog(QString("got error %2 when parsing file %1").arg(existing_filename, get_error_string()));
			setNotificationText(tr("Error parsing local storage, giving up"));
			set_filename(NULL, "");
		} else {
			// successfully opened the local file, now add thigs to the dive list
			consumeFinishedLoad(0);
			appendTextToLog(QString("working in no-cloud mode, finished loading %1 dives from %2").arg(dive_table.nr).arg(existing_filename));
		}
	} else {
		setCredentialStatus(INCOMPLETE);
		appendTextToLog(tr("no cloud credentials"));
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	}
	setDistanceThreshold(prefs.distance_threshold);
	setTimeThreshold(prefs.time_threshold / 60);
}

QMLManager::~QMLManager()
{
#if defined(Q_OS_ANDROID)
	if (appLogFileOpen)
		appLogFile.close();
#endif
	m_instance = NULL;
}

QMLManager *QMLManager::instance()
{
	return m_instance;
}

void QMLManager::savePreferences()
{
	auto location = SettingsObjectWrapper::instance()->location_settings;
	location->setTimeThreshold(timeThreshold() * 60);
	location->setDistanceThreshold(distanceThreshold());
}

#define CLOUDURL QString(prefs.cloud_base_url)
#define CLOUDREDIRECTURL CLOUDURL + "/cgi-bin/redirect.pl"

void QMLManager::saveCloudCredentials()
{
	QSettings s;
	bool cloudCredentialsChanged = false;
	// make sure we only have letters, numbers, and +-_. in password and email address
	QRegularExpression regExp("^[a-zA-Z0-9@.+_-]+$");
	QString cloudPwd = cloudPassword();
	QString cloudUser = cloudUserName();
	if (cloudPwd.isEmpty() || !regExp.match(cloudPwd).hasMatch() || !regExp.match(cloudUser).hasMatch()) {
		setStartPageText(RED_FONT + tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.") + END_FONT);
		return;
	}
	// use the same simplistic regex as the backend to check email addresses
	regExp = QRegularExpression("^[a-zA-Z0-9.+_-]+@[a-zA-Z0-9.+_-]+\\.[a-zA-Z0-9]+");
	if (!regExp.match(cloudUser).hasMatch()) {
		setStartPageText(RED_FONT + tr("Invalid format for email address") + END_FONT);
		return;
	}
	setOldStatus(credentialStatus());
	s.beginGroup("CloudStorage");
	s.setValue("email", cloudUser);
	s.setValue("password", cloudPwd);
	s.sync();
	if (!same_string(prefs.cloud_storage_email, qPrintable(cloudUser))) {
		free(prefs.cloud_storage_email);
		prefs.cloud_storage_email = strdup(qPrintable(cloudUser));
		cloudCredentialsChanged = true;
	}

	cloudCredentialsChanged |= !same_string(prefs.cloud_storage_password, qPrintable(cloudPwd));

	if (!cloudCredentialsChanged) {
		// just go back to the dive list
		setCredentialStatus(oldStatus());
	}

	if (!same_string(prefs.cloud_storage_password, qPrintable(cloudPwd))) {
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = strdup(qPrintable(cloudPwd));
	}
	if (cloudUser.isEmpty() || cloudPwd.isEmpty()) {
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	} else if (cloudCredentialsChanged) {
		// let's make sure there are no unsaved changes
		saveChangesLocal();
		free(prefs.userid);
		prefs.userid = NULL;
		syncLoadFromCloud();
		QString url;
		getCloudURL(url);
		manager()->clearAccessCache(); // remove any chached credentials
		clear_git_id(); // invalidate our remembered GIT SHA
		clear_dive_file_data();
		DiveListModel::instance()->clear();
		GpsListModel::instance()->clear();
		setStartPageText(tr("Attempting to open cloud storage with new credentials"));
		// we therefore know that no one else is already accessing THIS git repo;
		// let's make sure we stay the only ones doing so
		alreadySaving = true;
		// since we changed credentials, we need to try to connect to the cloud, regardless
		// of whether we're in offline mode or not, to make sure the repository is synced
		currentGitLocalOnly = prefs.git_local_only;
		prefs.git_local_only = false;
		openLocalThenRemote(url);
	} else if (prefs.cloud_verification_status == CS_NEED_TO_VERIFY && !cloudPin().isEmpty()) {
		// the user entered a PIN?
		tryRetrieveDataFromBackend();
	}
}

void QMLManager::checkCredentialsAndExecute(execute_function_type execute)
{
	// if the cloud credentials are present, we should try to get the GPS Webservice ID
	// and (if we haven't done so) load the dive list
	if (!same_string(prefs.cloud_storage_email, "") &&
	    !same_string(prefs.cloud_storage_password, "")) {
		setStartPageText(tr("Testing cloud credentials"));
		appendTextToLog("Have credentials, let's see if they are valid");
		CloudStorageAuthenticate *csa = new CloudStorageAuthenticate(this);
		csa->backend(prefs.cloud_storage_email, prefs.cloud_storage_password, cloudPin());
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
		setCloudPin("");
		if (prefs.cloud_verification_status == CS_INCORRECT_USER_PASSWD) {
			appendTextToLog(QStringLiteral("Incorrect cloud credentials"));
			setStartPageText(RED_FONT + tr("Incorrect cloud credentials") + END_FONT);
			revertToNoCloudIfNeeded();
			return;
		} else if (prefs.cloud_verification_status != CS_VERIFIED) {
			// here we need to enter the PIN
			appendTextToLog(QStringLiteral("Need to verify the email address - enter PIN"));
			setStartPageText(RED_FONT + tr("Cannot connect to cloud storage - cloud account not verified") + END_FONT);
			revertToNoCloudIfNeeded();
			setShowPin(true);
			return;
		}
		if (showPin())
			setShowPin(false);

		// now check the redirect URL to make sure everything is set up on the cloud server
		connect(manager(), &QNetworkAccessManager::authenticationRequired, this, &QMLManager::provideAuth, Qt::UniqueConnection);
		QUrl url(CLOUDREDIRECTURL);
		request = QNetworkRequest(url);
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setRawHeader("Accept", "text/html");
		reply = manager()->get(request);
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleError(QNetworkReply::NetworkError)));
		connect(reply, &QNetworkReply::sslErrors, this, &QMLManager::handleSslErrors);
		connect(reply, &QNetworkReply::finished, this, execute, Qt::UniqueConnection);
	}
}

void QMLManager::tryRetrieveDataFromBackend()
{
	checkCredentialsAndExecute(&QMLManager::retrieveUserid);
}

void QMLManager::provideAuth(QNetworkReply *reply, QAuthenticator *auth)
{
	if (auth->user() == QString(prefs.cloud_storage_email) &&
	    auth->password() == QString(prefs.cloud_storage_password)) {
		// OK, credentials have been tried and didn't work, so they are invalid
		appendTextToLog("Cloud credentials are invalid");
		setStartPageText(RED_FONT + tr("Cloud credentials are invalid") + END_FONT);
		setCredentialStatus(INVALID);
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
	setStartPageText(RED_FONT + tr("Cannot open cloud storage: Error creating https connection") + END_FONT);
	Q_FOREACH (QSslError e, errors) {
		appendTextToLog(e.errorString());
	}
	reply->abort();
	reply->deleteLater();
	setNotificationText(QStringLiteral(""));
}

void QMLManager::handleError(QNetworkReply::NetworkError nError)
{
	QString errorString = reply->errorString();
	appendTextToLog(QStringLiteral("handleError ") + nError + QStringLiteral(": ") + errorString);
	setStartPageText(RED_FONT + tr("Cannot open cloud storage: %1").arg(errorString) + END_FONT);
	reply->abort();
	reply->deleteLater();
	setNotificationText(QStringLiteral(""));
}

void QMLManager::retrieveUserid()
{
	if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 302) {
		appendTextToLog(QStringLiteral("Cloud storage connection not working correctly: (%1) %2")
				.arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
				.arg(QString(reply->readAll())));
		setStartPageText(RED_FONT + tr("Cannot connect to cloud storage") + END_FONT);
		revertToNoCloudIfNeeded();
		return;
	}
	setCredentialStatus(VALID);
	QString userid(prefs.userid);
	if (userid.isEmpty()) {
		if (same_string(prefs.cloud_storage_email, "") || same_string(prefs.cloud_storage_password, "")) {
			appendTextToLog("cloud user name or password are empty, can't retrieve web user id");
			revertToNoCloudIfNeeded();
			return;
		}
		appendTextToLog(QStringLiteral("calling getUserid with user %1").arg(prefs.cloud_storage_email));
		userid = locationProvider->getUserid(prefs.cloud_storage_email, prefs.cloud_storage_password);
	}
	if (!userid.isEmpty()) {
		// overwrite the existing userid
		free(prefs.userid);
		prefs.userid = strdup(qPrintable(userid));
		QSettings s;
		s.setValue("subsurface_webservice_uid", prefs.userid);
		s.sync();
	}
	setCredentialStatus(VALID);
	setStartPageText(tr("Cloud credentials valid, loading dives..."));
	// this only gets called with "alreadySaving" already locked
	loadDivesWithValidCredentials();
}

void QMLManager::loadDivesWithValidCredentials()
{
	QString url;
	timestamp_t currentDiveTimestamp = selectedDiveTimestamp();
	if (getCloudURL(url)) {
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		setStartPageText(RED_FONT + tr("Cloud storage error: %1").arg(errorString) + END_FONT);
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

	clear_dive_file_data();
	if (git != dummy_git_repository) {
		appendTextToLog(QString("have repository and branch %1").arg(branch));
		error = git_load_dives(git, branch);
	} else {
		appendTextToLog(QString("didn't receive valid git repo, try again"));
		error = parse_file(fileNamePrt.data());
	}
	if (!error) {
		report_error("filename is now %s", fileNamePrt.data());
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		set_filename(fileNamePrt.data(), true);
	} else {
		report_error("failed to open file %s", fileNamePrt.data());
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		setNotificationText(errorString);
		revertToNoCloudIfNeeded();
		return;
	}
	consumeFinishedLoad(currentDiveTimestamp);

successful_exit:
	alreadySaving = false;
	setLoadFromCloud(true);
	// if we came from local storage mode, let's merge the local data into the local cache
	// for the remote data - which then later gets merged with the remote data if necessary
	if (oldStatus() == NOCLOUD) {
		git_storage_update_progress(qPrintable(tr("Loading dives from local storage ('no cloud' mode)")));
		dive_table.preexisting = dive_table.nr;
		mergeLocalRepo();
		DiveListModel::instance()->clear();
		DiveListModel::instance()->addAllDives();
		appendTextToLog(QStringLiteral("%1 dives loaded after importing nocloud local storage").arg(dive_table.nr));
		saveChangesLocal();
		if (syncToCloud() == false) {
			appendTextToLog(QStringLiteral("taking things back offline now that storage is synced"));
			prefs.git_local_only = syncToCloud();
		}
	}
	// if we got here just for an initial connection to the cloud, reset to offline
	if (currentGitLocalOnly) {
		currentGitLocalOnly = false;
		prefs.git_local_only = true;
	}
	return;
}

void QMLManager::revertToNoCloudIfNeeded()
{
	if (currentGitLocalOnly) {
		// we tried to connect to the cloud for the first time and that failed
		currentGitLocalOnly = false;
		prefs.git_local_only = true;
	}
	if (oldStatus() == NOCLOUD) {
		// we tried to switch to a cloud account and had previously used local data,
		// but connecting to the cloud account (and subsequently merging the local
		// and cloud data) failed - so let's delete the cloud credentials and go
		// back to NOCLOUD mode in order to prevent us from losing the locally stored
		// dives
		if (syncToCloud() == false) {
			appendTextToLog(QStringLiteral("taking things back offline since sync with cloud failed"));
			prefs.git_local_only = syncToCloud();
		}
		free(prefs.cloud_storage_email);
		prefs.cloud_storage_email = NULL;
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = NULL;
		setCloudUserName("");
		setCloudPassword("");
		setCredentialStatus(INCOMPLETE);
		set_filename(NOCLOUD_LOCALSTORAGE, true);
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
	DiveListModel::instance()->clear();
	process_dives(false, false);
	DiveListModel::instance()->addAllDives();
	if (currentDiveTimestamp)
		setUpdateSelectedDive(dlSortModel->getIdxForId(get_dive_id_closest_to(currentDiveTimestamp)));
	appendTextToLog(QStringLiteral("%1 dives loaded").arg(dive_table.nr));
	if (dive_table.nr == 0)
		setStartPageText(tr("Cloud storage open successfully. No dives in dive list."));
	alreadySaving = false;
}

void QMLManager::refreshDiveList()
{
	DiveListModel::instance()->clear();
	DiveListModel::instance()->addAllDives();
}

static void setupDivesite(struct dive *d, struct dive_site *ds, double lat, double lon, const char *locationtext)
{
	if (ds) {
		ds->latitude.udeg = (int) (lat * 1000000);
		ds->longitude.udeg = (int) (lon * 1000000);
	} else {
		degrees_t latData, lonData;
		latData.udeg = (int) lat;
		lonData.udeg = (int) lon;
		d->dive_site_uuid = create_dive_site_with_gps(locationtext, latData, lonData, d->when);
	}
}

bool QMLManager::checkDate(DiveObjectHelper *myDive, struct dive * d, QString date)
{
	QString oldDate = myDive->date() + " " + myDive->time();
	if (date != oldDate) {
		QDateTime newDate;
		// what a pain - Qt will not parse dates if the day of the week is incorrect
		// so if the user changed the date but didn't update the day of the week (most likely behavior, actually),
		// we need to make sure we don't try to parse that
		QString format(QString(prefs.date_format) + QChar(' ') + prefs.time_format);
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

bool QMLManager::checkLocation(DiveObjectHelper *myDive, struct dive *d, QString location, QString gps)
{
	bool diveChanged = false;

	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	if (myDive->location() != location) {
		diveChanged = true;
		ds = get_dive_site_by_uuid(create_dive_site(qPrintable(location), d->when));
		d->dive_site_uuid = ds->uuid;
	}
	// now make sure that the GPS coordinates match - if the user changed the name but not
	// the GPS coordinates, this still does the right thing as the now new dive site will
	// have no coordinates, so the coordinates from the edit screen will get added
	if (myDive->gps() != gps) {
		double lat, lon;
		if (parseGpsText(gps, &lat, &lon)) {
			// there are valid GPS coordinates - just use them
			setupDivesite(d, ds, lat, lon, qPrintable(myDive->location()));
			diveChanged = true;
		} else if (gps == GPS_CURRENT_POS) {
			// user asked to use current pos
			QString gpsString = getCurrentPosition();
			if (gpsString != GPS_CURRENT_POS) {
				if (parseGpsText(qPrintable(gpsString), &lat, &lon)) {
					setupDivesite(d, ds, lat, lon, qPrintable(myDive->location()));
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

bool QMLManager::checkDuration(DiveObjectHelper *myDive, struct dive *d, QString duration)
{
	if (myDive->duration() != duration) {
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
		if (same_string(d->dc.model, "manually added dive")) {
			free(d->dc.sample);
			d->dc.sample = 0;
			d->dc.samples = 0;
		} else {
			appendTextToLog("Cannot change the duration on a dive that wasn't manually added");
		}
		return true;
	}
	return false;
}

bool QMLManager::checkDepth(DiveObjectHelper *myDive, dive *d, QString depth)
{
	if (myDive->depth() != depth) {
		int depthValue = parseLengthToMm(depth);
		// the QML code should stop negative depth, but massively huge depth can make
		// the profile extremely slow or even run out of memory and crash, so keep
		// the depth <= 500m
		if (0 <= depthValue && depthValue <= 500000) {
			d->maxdepth.mm = depthValue;
			if (same_string(d->dc.model, "manually added dive")) {
				d->dc.maxdepth.mm = d->maxdepth.mm;
				free(d->dc.sample);
				d->dc.sample = 0;
				d->dc.samples = 0;
			}
			return true;
		}
	}
	return false;
}

// update the dive and return the notes field, stripped of the HTML junk
void QMLManager::commitChanges(QString diveId, QString date, QString location, QString gps, QString duration, QString depth,
			       QString airtemp, QString watertemp, QString suit, QString buddy, QString diveMaster, QString weight, QString notes,
			       QString startpressure, QString endpressure, QString gasmix, QString cylinder, int rating, int visibility)
{
	struct dive *d = get_dive_by_uniq_id(diveId.toInt());
	DiveObjectHelper *myDive = new DiveObjectHelper(d);

	// notes comes back as rich text - let's convert this into plain text
	QTextDocument doc;
	doc.setHtml(notes);
	notes = doc.toPlainText();

	if (!d) {
		appendTextToLog("cannot commit changes: no dive");
		return;
	}
	bool diveChanged = false;
	bool needResort = false;

	diveChanged = needResort = checkDate(myDive, d, date);

	diveChanged |= checkLocation(myDive, d, location, gps);

	diveChanged |= checkDuration(myDive, d, duration);

	diveChanged |= checkDepth(myDive, d, depth);

	if (myDive->airTemp() != airtemp) {
		diveChanged = true;
		d->airtemp.mkelvin = parseTemperatureToMkelvin(airtemp);
	}
	if (myDive->waterTemp() != watertemp) {
		diveChanged = true;
		d->watertemp.mkelvin = parseTemperatureToMkelvin(watertemp);
	}
	// not sure what we'd do if there was more than one weight system
	// defined - for now just ignore that case
	if (weightsystem_none((void *)&d->weightsystem[1])) {
		if (myDive->sumWeight() != weight) {
			diveChanged = true;
			d->weightsystem[0].weight.grams = parseWeightToGrams(weight);
		}
	}
	// start and end pressures for first cylinder only
	if (myDive->startPressure() != startpressure || myDive->endPressure() != endpressure) {
		diveChanged = true;
		d->cylinder[0].start.mbar = parsePressureToMbar(startpressure);
		d->cylinder[0].end.mbar = parsePressureToMbar(endpressure);
		if (d->cylinder[0].end.mbar > d->cylinder[0].start.mbar)
			d->cylinder[0].end.mbar = d->cylinder[0].start.mbar;
	}
	// gasmix for first cylinder
	if (myDive->firstGas() != gasmix) {
		int o2 = parseGasMixO2(gasmix);
		int he = parseGasMixHE(gasmix);
		// the QML code SHOULD only accept valid gas mixes, but just to make sure
		if (o2 >= 0 && o2 <= 1000 &&
		    he >= 0 && he <= 1000 &&
		    o2 + he <= 1000) {
			diveChanged = true;
			d->cylinder[0].gasmix.o2.permille = o2;
			d->cylinder[0].gasmix.he.permille = he;
		}
	}
	// info for first cylinder
	if (myDive->getCylinder() != cylinder) {
		diveChanged = true;
		unsigned long i;
		int size = 0, wp = 0;
		for (i = 0; i < sizeof(tank_info) && tank_info[i].name != NULL; i++) {
			if (tank_info[i].name == cylinder ) {
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
		d->cylinder[0].type.description = strdup(qPrintable(cylinder));
		d->cylinder[0].type.size.mliter = size;
		d->cylinder[0].type.workingpressure.mbar = wp;
	}
	if (myDive->suit() != suit) {
		diveChanged = true;
		free(d->suit);
		d->suit = strdup(qPrintable(suit));
	}
	if (myDive->buddy() != buddy) {
		if (buddy.contains(",")){
			buddy = buddy.replace(QRegExp("\\s*,\\s*"), ", ");
		}
		if (!buddy.contains("Multiple Buddies")) {
			diveChanged = true;
			free(d->buddy);
			d->buddy = strdup(qPrintable(buddy));
		}
	}
	if (myDive->divemaster() != diveMaster) {
		diveChanged = true;
		free(d->divemaster);
		d->divemaster = strdup(qPrintable(diveMaster));
	}
	if (myDive->rating() != rating) {
		diveChanged = true;
		d->rating = rating;
	}
	if (myDive->visibility() != visibility) {
		diveChanged = true;
		d->visibility = visibility;
	}
	if (myDive->notes() != notes) {
		diveChanged = true;
		free(d->notes);
		d->notes = strdup(qPrintable(notes));
	}
	// now that we have it all figured out, let's see what we need
	// to update
	DiveListModel *dm = DiveListModel::instance();
	int modelIdx = dm->getDiveIdx(d->id);
	int oldIdx = get_idx_by_uniq_id(d->id);
	if (needResort) {
		// we know that the only thing that might happen in a resort is that
		// this one dive moves to a different spot in the dive list
		sort_table(&dive_table);
		int newIdx = get_idx_by_uniq_id(d->id);
		if (newIdx != oldIdx) {
			DiveListModel::instance()->removeDive(modelIdx);
			modelIdx += (newIdx - oldIdx);
			DiveListModel::instance()->insertDive(modelIdx, myDive);
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
			// to be too clever
			d->meandepth.mm = d->dc.meandepth.mm = 0;
			d->dc = *fake_dc(&d->dc, true);
		}
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
}
void QMLManager::saveChangesLocal()
{
	if (unsaved_changes()) {
		if (credentialStatus() == NOCLOUD) {
			if (same_string(existing_filename, "")) {
				char *filename = NOCLOUD_LOCALSTORAGE;
				if (git_create_local_repo(filename))
					appendTextToLog(get_error_string());
				set_filename(filename, true);
				GeneralSettingsObjectWrapper s(this);
				s.setDefaultFilename(filename);
				s.setDefaultFileBehavior(LOCAL_DEFAULT_FILE);
			}
		} else if (!loadFromCloud()) {
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
		bool glo = prefs.git_local_only;
		prefs.git_local_only = true;
		if (save_dives(existing_filename)) {
			QString errorString(get_error_string());
			appendTextToLog(errorString);
			setNotificationText(errorString);
			set_filename(NULL, true);
			prefs.git_local_only = glo;
			alreadySaving = false;
			return;
		}
		prefs.git_local_only = glo;
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
	if (prefs.git_local_only && !forceRemoteSync)
		return;

	if (!loadFromCloud()) {
		appendTextToLog("Don't save dives without loading from the cloud, first.");
		return;
	}

	bool glo = prefs.git_local_only;
	prefs.git_local_only = false;
	alreadySaving = true;
	loadDivesWithValidCredentials();
	alreadySaving = false;
	prefs.git_local_only = glo;
}

bool QMLManager::undoDelete(int id)
{
	if (!deletedDive || deletedDive->id != id) {
		appendTextToLog("Trying to undo delete but can't find the deleted dive");
		return false;
	}
	if (deletedTrip)
		insert_trip(&deletedTrip);
	if (deletedDive->divetrip) {
		struct dive_trip *trip = deletedDive->divetrip;
		tripflag_t tripflag = deletedDive->tripflag; // this gets overwritten in add_dive_to_trip()
		deletedDive->divetrip = NULL;
		deletedDive->next = NULL;
		deletedDive->pprev = NULL;
		add_dive_to_trip(deletedDive, trip);
		deletedDive->tripflag = tripflag;
	}
	record_dive(deletedDive);
	QList<dive *>diveAsList;
	diveAsList << deletedDive;
	DiveListModel::instance()->addDive(diveAsList);
	changesNeedSaving();
	deletedDive = NULL;
	deletedTrip = NULL;
	return true;
}

void QMLManager::deleteDive(int id)
{
	struct dive *d = get_dive_by_uniq_id(id);
	if (!d) {
		appendTextToLog("trying to delete non-existing dive");
		return;
	}
	// clean up (or create) the storage for the deleted dive and trip (if applicable)
	if (!deletedDive)
		deletedDive = alloc_dive();
	else
		clear_dive(deletedDive);
	copy_dive(d, deletedDive);
	if (!deletedTrip) {
		deletedTrip = (struct dive_trip *)calloc(1, sizeof(struct dive_trip));
	} else {
		free(deletedTrip->location);
		free(deletedTrip->notes);
		memset(deletedTrip, 0, sizeof(struct dive_trip));
	}
	// if this is the last dive in that trip, remember the trip as well
	if (d->divetrip && d->divetrip->nrdives == 1) {
		*deletedTrip = *d->divetrip;
		deletedTrip->location = copy_string(d->divetrip->location);
		deletedTrip->notes = copy_string(d->divetrip->notes);
		deletedTrip->nrdives = 0;
		deletedDive->divetrip = deletedTrip;
	}
	DiveListModel::instance()->removeDiveById(id);
	delete_single_dive(get_idx_by_uniq_id(id));
	changesNeedSaving();
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

	return locationProvider->currentPosition();
}

void QMLManager::applyGpsData()
{
	if (locationProvider->applyLocations())
		refreshDiveList();
}

void QMLManager::sendGpsData()
{
	locationProvider->uploadToServer();
}

void QMLManager::downloadGpsData()
{
	locationProvider->downloadFromServer();
	populateGpsData();
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
	if (!timer.isValid())
		timer.start();
	qDebug() << QString::number(timer.elapsed() / 1000.0,'f', 3) + ": " + newText;
}

bool QMLManager::locationServiceEnabled() const
{
	return m_locationServiceEnabled;
}

void QMLManager::setLocationServiceEnabled(bool locationServiceEnabled)
{
	m_locationServiceEnabled = locationServiceEnabled;
	locationProvider->serviceEnable(m_locationServiceEnabled);
	emit locationServiceEnabledChanged();
}

bool QMLManager::locationServiceAvailable() const
{
#if defined(Q_OS_IOS)
	return false;
#else
	return m_locationServiceAvailable;
#endif
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

bool QMLManager::verboseEnabled() const
{
	return m_verboseEnabled;
}

void QMLManager::setVerboseEnabled(bool verboseMode)
{
	m_verboseEnabled = verboseMode;
	verbose = verboseMode;
	appendTextToLog(QStringLiteral("verbose is ") + (verbose ? QStringLiteral("on") : QStringLiteral("off")));
	emit verboseEnabledChanged();
}

QString QMLManager::cloudPassword() const
{
	return m_cloudPassword;
}

void QMLManager::setCloudPassword(const QString &cloudPassword)
{
	m_cloudPassword = cloudPassword;
	emit cloudPasswordChanged();
}

QString QMLManager::cloudPin() const
{
	return m_cloudPin;
}

void QMLManager::setCloudPin(const QString &cloudPin)
{
	m_cloudPin = cloudPin;
	emit cloudPinChanged();
}

QString QMLManager::cloudUserName() const
{
	return m_cloudUserName;
}

void QMLManager::setCloudUserName(const QString &cloudUserName)
{
	m_cloudUserName = cloudUserName.toLower();
	emit cloudUserNameChanged();
}

int QMLManager::distanceThreshold() const
{
	return m_distanceThreshold;
}

void QMLManager::setDistanceThreshold(int distance)
{
	m_distanceThreshold = distance;
	emit distanceThresholdChanged();
}

int QMLManager::timeThreshold() const
{
	return m_timeThreshold;
}

void QMLManager::setTimeThreshold(int time)
{
	m_timeThreshold = time;
	locationProvider->setGpsTimeThreshold(m_timeThreshold * 60);
	emit timeThresholdChanged();
}

void QMLManager::setTheme(QString theme)
{
	QSettings s;
	s.beginGroup("Theme");
	s.setValue("currentTheme", theme);
	emit themeChanged();
}

QString QMLManager::theme() const
{
	QSettings s;
	s.beginGroup("Theme");
	return s.value("currentTheme", "Blue").toString();
}

bool QMLManager::loadFromCloud() const
{
	return m_loadFromCloud;
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

QString QMLManager::startPageText() const
{
	return m_startPageText;
}

void QMLManager::setStartPageText(const QString& text)
{
	m_startPageText = text;
	emit startPageTextChanged();
}

QMLManager::credentialStatus_t QMLManager::credentialStatus() const
{
	return m_credentialStatus;
}

void QMLManager::setCredentialStatus(const credentialStatus_t value)
{
	if (m_credentialStatus != value) {
		if (value == NOCLOUD)
			appendTextToLog("Switching to no cloud mode");
		m_credentialStatus = value;
		emit credentialStatusChanged();
	}
}

QMLManager::credentialStatus_t QMLManager::oldStatus() const
{
	return m_oldStatus;
}

void QMLManager::setOldStatus(const credentialStatus_t value)
{
	if (m_oldStatus != value) {
		m_oldStatus = value;
		emit oldStatusChanged();
	}
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
		datestring = get_dive_date_string(d->when);
	return datestring;
}

QString QMLManager::getVersion() const
{
	QRegExp versionRe(".*:([()\\.,\\d]+).*");
	if (!versionRe.exactMatch(getUserAgent()))
		return QString();

	return versionRe.cap(1);
}

QString QMLManager::notificationText() const
{
	return m_notificationText;
}

void QMLManager::setNotificationText(QString text)
{
	m_notificationText = text;
	emit notificationTextChanged();
}

bool QMLManager::syncToCloud() const
{
	return m_syncToCloud;
}

void QMLManager::setSyncToCloud(bool status)
{
	m_syncToCloud = status;
	prefs.git_local_only = !status;
	QSettings s;
	s.beginGroup("CloudStorage");
	s.setValue("git_local_only", prefs.git_local_only);
	emit syncToCloudChanged();
}

int QMLManager::updateSelectedDive() const
{
	return m_updateSelectedDive;
}

void QMLManager::setUpdateSelectedDive(int idx)
{
	m_updateSelectedDive = idx;
	emit updateSelectedDiveChanged();
}

int QMLManager::selectedDiveTimestamp() const
{
	return m_selectedDiveTimestamp;
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
	m_lastDevicePixelRatio = screen->devicePixelRatio();
	emit sendScreenChanged(screen);
}

void QMLManager::quit()
{
	if (unsaved_changes())
		saveChangesCloud(false);
	QApplication::quit();
}

QStringList QMLManager::suitInit() const
{
	QStringList suits;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		QString temp = d->suit;
		if (!temp.isEmpty())
			suits << d->suit;
	}
	suits.removeDuplicates();
	suits.sort();
	return suits;
}

QStringList QMLManager::buddyInit() const
{
	QStringList buddies;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		QString temp = d->buddy;
		if (!temp.isEmpty() && !temp.contains(",")){
			buddies << d->buddy;
		}
		else if (!temp.isEmpty()){
			QRegExp sep("(,\\s)");
			QStringList tempList = temp.split(sep);
			buddies << tempList;
		}
	}
	buddies.removeDuplicates();
	buddies.sort();
	return buddies;
}

QStringList QMLManager::divemasterInit() const
{
	QStringList divemasters;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		QString temp = d->divemaster;
		if (!temp.isEmpty())
			divemasters << d->divemaster;
	}
	divemasters.removeDuplicates();
	divemasters.sort();
	return divemasters;
}

QStringList QMLManager::cylinderInit() const
{
	QStringList cylinders;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		for (int j = 0; j < MAX_CYLINDERS; j++) {
			if (! same_string(d->cylinder[j].type.description, ""))
				cylinders << d->cylinder[j].type.description;
		}
	}
	cylinders.removeDuplicates();
	cylinders.sort();
	return cylinders;
}

bool QMLManager::showPin() const
{
	return m_showPin;
}

void QMLManager::setShowPin(bool enable)
{
	m_showPin = enable;
	emit showPinChanged();
}

QString QMLManager::progressMessage() const
{
	return m_progressMessage;
}

void QMLManager::setProgressMessage(QString text)
{
	m_progressMessage = text;
	emit progressMessageChanged();
}

bool QMLManager::libdcLog() const
{
	return m_libdcLog;
}

void QMLManager::setLibdcLog(bool value)
{
	m_libdcLog = value;
	DCDeviceData::instance()->setSaveLog(value);
	emit libdcLogChanged();
}

bool QMLManager::developer() const
{
	return m_developer;
}

void QMLManager::setDeveloper(bool value)
{
	m_developer = value;
	emit developerChanged();
}

bool QMLManager::btEnabled() const
{
	return m_btEnabled;
}

#if defined (Q_OS_ANDROID)

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
		appLogFile.write(logText.toUtf8().data());
		appLogFile.flush();
	}
}

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
void QMLManager::setStatusbarColor(QColor color)
{
	// noop
}

#endif
