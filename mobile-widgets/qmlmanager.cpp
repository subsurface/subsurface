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

#include "qt-models/divelistmodel.h"
#include "qt-models/gpslistmodel.h"
#include "core/divelist.h"
#include "core/device.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/qt-gui.h"
#include "core/git-access.h"
#include "core/cloudstorage.h"

QMLManager *QMLManager::m_instance = NULL;

#define RED_FONT QLatin1Literal("<font color=\"red\">")
#define END_FONT QLatin1Literal("</font>")

static void appendTextToLogStandalone(const char *text)
{
	QMLManager *self = QMLManager::instance();
	if (self)
		self->appendTextToLog(QString(text));
}

extern "C" int gitProgressCB(bool reset, const char *text)
{
	static QElapsedTimer timer;
	static qint64 lastTime;
	static QString lastText;
	static QMLManager *self;
	static int lastPercent;

	if (!self)
		self = QMLManager::instance();

	if (!timer.isValid() || reset) {
		timer.restart();
		lastTime = 0;
		lastPercent = 0;
		lastText.clear();
	}
	if (self) {
		qint64 elapsed = timer.elapsed();
		// don't show the same status twice in 200ms
		if (lastText == text && elapsed - lastTime < 200)
			return 0;
		self->loadDiveProgress(++lastPercent);
		QString logText = QString::number(elapsed / 1000.0, 'f', 1) + " / " + QString::number((elapsed - lastTime) / 1000.0, 'f', 3) +
				  QString(" : git %1 (%2)").arg(lastPercent).arg(text);
		self->appendTextToLog(logText);
		qDebug() << logText;
		if (elapsed - lastTime > 500)
			qApp->processEvents();
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
	m_credentialStatus(UNKNOWN),
	m_lastDevicePixelRatio(1.0),
	alreadySaving(false),
	m_selectedDiveTimestamp(0),
	m_updateSelectedDive(-1)
{
	m_instance = this;
	connect(qobject_cast<QApplication *>(QApplication::instance()), &QApplication::applicationStateChanged, this, &QMLManager::applicationStateChanged);
	appendTextToLog(getUserAgent());
	appendTextToLog(QStringLiteral("build with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion()));
	qDebug() << "Starting" << getUserAgent();
	qDebug() << QStringLiteral("build with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion());
	setStartPageText(tr("Starting..."));
	setAccessingCloud(-1);
	// create location manager service
	locationProvider = new GpsLocation(&appendTextToLogStandalone, this);
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
	stateText.prepend(QString::number(timer.elapsed() / 1000.0,'f', 3) + ": AppState changed to ");
	stateText.append(" with ");
	stateText.append((alreadySaving ? QLatin1Literal("") : QLatin1Literal("no ")) + QLatin1Literal("save ongoing"));
	stateText.append(" and ");
	stateText.append((unsaved_changes() ? QLatin1Literal("") : QLatin1Literal("no ")) + QLatin1Literal("unsaved changes"));
	appendTextToLog(stateText);
	qDebug() << stateText;

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
	QByteArray fileNamePrt = QFile::encodeName(url);
	bool glo = prefs.git_local_only;
	prefs.git_local_only = true;
	int error = parse_file(fileNamePrt.data());
	setAccessingCloud(-1);
	prefs.git_local_only = glo;
	if (error) {
		appendTextToLog(QStringLiteral("loading dives from cache failed %1").arg(error));
	} else {
		// if we can load from the cache, we know that we have at least a valid email
		if (credentialStatus() == UNKNOWN)
			setCredentialStatus(VALID_EMAIL);
		prefs.unit_system = informational_prefs.unit_system;
		if (informational_prefs.unit_system == IMPERIAL)
			informational_prefs.units = IMPERIAL_units;
		prefs.units = informational_prefs.units;
		process_dives(false, false);
		DiveListModel::instance()->clear();
		DiveListModel::instance()->addAllDives();
		appendTextToLog(QStringLiteral("%1 dives loaded from cache").arg(dive_table.nr));
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
	} else {
		setCredentialStatus(INCOMPLETE);
		appendTextToLog(QStringLiteral("no cloud credentials"));
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	}
	setDistanceThreshold(prefs.distance_threshold);
	setTimeThreshold(prefs.time_threshold / 60);
}

QMLManager::~QMLManager()
{
	m_instance = NULL;
}

QMLManager *QMLManager::instance()
{
	return m_instance;
}

void QMLManager::savePreferences()
{
	QSettings s;
	s.beginGroup("LocationService");
	s.setValue("time_threshold", timeThreshold() * 60);
	prefs.time_threshold = timeThreshold() * 60;
	s.setValue("distance_threshold", distanceThreshold());
	prefs.distance_threshold = distanceThreshold();
	s.sync();
}

#define CLOUDURL QString(prefs.cloud_base_url)
#define CLOUDREDIRECTURL CLOUDURL + "/cgi-bin/redirect.pl"

void QMLManager::saveCloudCredentials()
{
	QSettings s;
	bool cloudCredentialsChanged = false;
	s.beginGroup("CloudStorage");
	s.setValue("email", cloudUserName());
	s.setValue("password", cloudPassword());
	s.sync();
	if (!same_string(prefs.cloud_storage_email, qPrintable(cloudUserName()))) {
		free(prefs.cloud_storage_email);
		prefs.cloud_storage_email = strdup(qPrintable(cloudUserName()));
		cloudCredentialsChanged = true;
	}

	cloudCredentialsChanged |= !same_string(prefs.cloud_storage_password, qPrintable(cloudPassword()));

	if (!cloudCredentialsChanged) {
		// just go back to the dive list
		setCredentialStatus(oldStatus());
	}
	if (!same_string(prefs.cloud_storage_password, qPrintable(cloudPassword()))) {
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = strdup(qPrintable(cloudPassword()));
	}
	if (cloudUserName().isEmpty() || cloudPassword().isEmpty()) {
		setStartPageText(RED_FONT + tr("Please enter valid cloud credentials.") + END_FONT);
	} else if (cloudCredentialsChanged) {
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
		openLocalThenRemote(url);
	}
}

void QMLManager::checkCredentialsAndExecute(execute_function_type execute)
{
	// if the cloud credentials are present, we should try to get the GPS Webservice ID
	// and (if we haven't done so) load the dive list
	if (!same_string(prefs.cloud_storage_email, "") &&
	    !same_string(prefs.cloud_storage_password, "")) {
		setAccessingCloud(0);
		setStartPageText(tr("Testing cloud credentials"));
		appendTextToLog("Have credentials, let's see if they are valid");
		CloudStorageAuthenticate *csa = new CloudStorageAuthenticate(this);
		csa->backend(prefs.cloud_storage_email, prefs.cloud_storage_password);
		connect(manager(), &QNetworkAccessManager::authenticationRequired, this, &QMLManager::provideAuth, Qt::UniqueConnection);
		connect(manager(), &QNetworkAccessManager::finished, this, execute, Qt::UniqueConnection);
		QUrl url(CLOUDREDIRECTURL);
		request = QNetworkRequest(url);
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setRawHeader("Accept", "text/html");
		reply = manager()->get(request);
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleError(QNetworkReply::NetworkError)));
		connect(reply, &QNetworkReply::sslErrors, this, &QMLManager::handleSslErrors);
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
		qDebug() << e.errorString();
	}
	reply->abort();
	reply->deleteLater();
	setAccessingCloud(-1);
}

void QMLManager::handleError(QNetworkReply::NetworkError nError)
{
	QString errorString = reply->errorString();
	qDebug() << "handleError" << nError << errorString;
	setStartPageText(RED_FONT + tr("Cannot open cloud storage: %1").arg(errorString) + END_FONT);
	reply->abort();
	reply->deleteLater();
	setAccessingCloud(-1);
}

void QMLManager::retrieveUserid()
{
	if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 302) {
		appendTextToLog(QStringLiteral("Cloud storage connection not working correctly: %1").arg(QString(reply->readAll())));
		setStartPageText(RED_FONT + tr("Cannot connect to cloud storage") + END_FONT);
		setAccessingCloud(-1);
		alreadySaving = false;
		return;
	}
	setCredentialStatus(VALID);
	QString userid(prefs.userid);
	if (userid.isEmpty()) {
		if (same_string(prefs.cloud_storage_email, "") || same_string(prefs.cloud_storage_password, "")) {
			appendTextToLog("cloud user name or password are empty, can't retrieve web user id");
			setAccessingCloud(-1);
			alreadySaving = false;
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
	setStartPageText("Cloud credentials valid, loading dives...");
	git_storage_update_progress(true, "load dives with valid credentials");
	// this only gets called with "alreadySaving" already locked
	loadDivesWithValidCredentials();
}

void QMLManager::loadDiveProgress(int percent)
{
	QString text(tr("Loading dive list from cloud storage."));
	setAccessingCloud(percent);
	while (percent > 0) {
		text.append(".");
		percent -= 10;
	}
	setStartPageText(text);
}

void QMLManager::loadDivesWithValidCredentials()
{
	QString url;
	if (getCloudURL(url)) {
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		setStartPageText(RED_FONT + tr("Cloud storage error: %1").arg(errorString) + END_FONT);
		setAccessingCloud(-1);
		alreadySaving = false;
		return;
	}
	QByteArray fileNamePrt = QFile::encodeName(url);
	git_repository *git;
	const char *branch;
	int error;
	if (check_git_sha(fileNamePrt.data(), &git, &branch) == 0) {
		qDebug() << "local cache was current, no need to modify dive list";
		appendTextToLog("Cloud sync shows local cache was current");
		setLoadFromCloud(true);
		setAccessingCloud(-1);
		alreadySaving = false;
		return;
	}
	appendTextToLog("Cloud sync brought newer data, reloading the dive list");
	timestamp_t currentDiveTimestamp = selectedDiveTimestamp();

	clear_dive_file_data();
	if (git != dummy_git_repository) {
		appendTextToLog(QString("have repository and branch %1").arg(branch));
		error = git_load_dives(git, branch);
	} else {
		appendTextToLog(QString("didn't receive valid git repo, try again"));
		error = parse_file(fileNamePrt.data());
	}
	setAccessingCloud(-1);
	if (!error) {
		report_error("filename is now %s", fileNamePrt.data());
		const char *error_string = get_error_string();
		appendTextToLog(error_string);
		set_filename(fileNamePrt.data(), true);
	} else {
		report_error("failed to open file %s", fileNamePrt.data());
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		setStartPageText(RED_FONT + tr("Cloud storage error: %1").arg(errorString) + END_FONT);
		alreadySaving = false;
		return;
	}
	prefs.unit_system = informational_prefs.unit_system;
	if (informational_prefs.unit_system == IMPERIAL)
		informational_prefs.units = IMPERIAL_units;
	prefs.units = informational_prefs.units;
	DiveListModel::instance()->clear();
	process_dives(false, false);
	DiveListModel::instance()->addAllDives();
	if (currentDiveTimestamp)
		setUpdateSelectedDive(dlSortModel->getIdxForId(get_dive_id_closest_to(currentDiveTimestamp)));
	else
		setUpdateSelectedDive(0);
	appendTextToLog(QStringLiteral("%1 dives loaded").arg(dive_table.nr));
	if (dive_table.nr == 0)
		setStartPageText(tr("Cloud storage open successfully. No dives in dive list."));
	setLoadFromCloud(true);
	alreadySaving = false;
}

void QMLManager::refreshDiveList()
{
	DiveListModel::instance()->clear();
	DiveListModel::instance()->addAllDives();
}

// update the dive and return the notes field, stripped of the HTML junk
void QMLManager::commitChanges(QString diveId, QString date, QString location, QString gps, QString duration, QString depth,
			       QString airtemp, QString watertemp, QString suit, QString buddy, QString diveMaster, QString weight, QString notes,
			       QString startpressure, QString endpressure, QString gasmix)
{
#define DROP_EMPTY_PLACEHOLDER(_s) if ((_s) == QLatin1Literal("--")) (_s).clear()

	DROP_EMPTY_PLACEHOLDER(location);
	DROP_EMPTY_PLACEHOLDER(duration);
	DROP_EMPTY_PLACEHOLDER(depth);
	DROP_EMPTY_PLACEHOLDER(airtemp);
	DROP_EMPTY_PLACEHOLDER(watertemp);
	DROP_EMPTY_PLACEHOLDER(suit);
	DROP_EMPTY_PLACEHOLDER(buddy);
	DROP_EMPTY_PLACEHOLDER(diveMaster);
	DROP_EMPTY_PLACEHOLDER(weight);
	DROP_EMPTY_PLACEHOLDER(gasmix);
	DROP_EMPTY_PLACEHOLDER(startpressure);
	DROP_EMPTY_PLACEHOLDER(endpressure);
	DROP_EMPTY_PLACEHOLDER(notes);

#undef DROP_EMPTY_PLACEHOLDER

	struct dive *d = get_dive_by_uniq_id(diveId.toInt());
	// notes comes back as rich text - let's convert this into plain text
	QTextDocument doc;
	doc.setHtml(notes);
	notes = doc.toPlainText();

	if (!d) {
		qDebug() << "don't touch this... no dive";
		return;
	}
	bool diveChanged = false;
	bool needResort = false;

	invalidate_dive_cache(d);
	if (date != get_dive_date_string(d->when)) {
		diveChanged = needResort = true;
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
		newDate = QDateTime::fromString(date, format);
		if (!newDate.isValid()) {
			qDebug() << "unable to parse date" << date << "with the given format" << format;
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
			d->dc.when = d->when = newDate.toMSecsSinceEpoch() / 1000 + gettimezoneoffset(newDate.toMSecsSinceEpoch() / 1000);
		} else {
			qDebug() << "none of our parsing attempts worked for the date string";
		}
	}
	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	char *locationtext = NULL;
	if (ds)
		locationtext = ds->name;
	if (!same_string(locationtext, qPrintable(location))) {
		double lat = 0, lon = 0;
		diveChanged = true;

		// As we create a new dive site, we need to grab the
		// coordinates if we have them

		if (ds && ds->latitude.udeg && ds->longitude.udeg) {
			lat = ds->latitude.udeg;
			lon = ds->longitude.udeg;
		}
		ds = get_dive_site_by_uuid(create_dive_site(qPrintable(location), d->when));

		if (lat && lon) {
			ds->latitude.udeg = lat;
			ds->longitude.udeg = lon;
		}
		d->dive_site_uuid = ds->uuid;
	}
	if (!gps.isEmpty()) {
		QString gpsString = getCurrentPosition();
		if (gpsString != QString("waiting for the next gps location")) {
			qDebug() << "from commitChanges call to getCurrentPosition returns" << gpsString;
			double lat, lon;
			if (parseGpsText(qPrintable(gpsString), &lat, &lon)) {
				struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
				if (ds) {
					ds->latitude.udeg = lat * 1000000;
					ds->longitude.udeg = lon * 1000000;
				} else {
					degrees_t latData, lonData;
					latData.udeg = lat;
					lonData.udeg = lon;
					d->dive_site_uuid = create_dive_site_with_gps("new site", latData, lonData, d->when);
				}
				qDebug() << "set up dive site with new GPS data";
			}
		} else {
			qDebug() << "still don't have a position - will need to implement some sort of callback";
		}
	}
	if (get_dive_duration_string(d->duration.seconds, tr("h:"), tr("min")) != duration) {
		diveChanged = true;
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
			qDebug() << "changing the duration on a dive that wasn't manually added - Uh-oh";
		}

	}
	if (get_depth_string(d->maxdepth.mm, true, true) != depth) {
		int depthValue = parseLengthToMm(depth);
		// the QML code should stop negative depth, but massively huge depth can make
		// the profile extremely slow or even run out of memory and crash, so keep
		// the depth <= 500m
		if (0 <= depthValue && depthValue <= 500000) {
			diveChanged = true;
			d->maxdepth.mm = depthValue;
			if (same_string(d->dc.model, "manually added dive")) {
				d->dc.maxdepth.mm = d->maxdepth.mm;
				free(d->dc.sample);
				d->dc.sample = 0;
				d->dc.samples = 0;
			}
		}
	}
	if (get_temperature_string(d->airtemp, true) != airtemp) {
		diveChanged = true;
		d->airtemp.mkelvin = parseTemperatureToMkelvin(airtemp);
	}
	if (get_temperature_string(d->watertemp, true) != watertemp) {
		diveChanged = true;
		d->watertemp.mkelvin = parseTemperatureToMkelvin(watertemp);
	}
	// not sure what we'd do if there was more than one weight system
	// defined - for now just ignore that case
	if (weightsystem_none((void *)&d->weightsystem[1])) {
		if (get_weight_string(d->weightsystem[0].weight, true) != weight) {
			diveChanged = true;
			d->weightsystem[0].weight.grams = parseWeightToGrams(weight);
		}
	}
	// start and end pressures for first cylinder only
	if (get_pressure_string(d->cylinder[0].start, true) != startpressure || get_pressure_string(d->cylinder[0].end, true) != endpressure) {
		diveChanged = true;
		d->cylinder[0].start.mbar = parsePressureToMbar(startpressure);
		d->cylinder[0].end.mbar = parsePressureToMbar(endpressure);
		if (d->cylinder[0].end.mbar > d->cylinder[0].start.mbar)
			d->cylinder[0].end.mbar = d->cylinder[0].start.mbar;
	}
	// gasmix for first cylinder
	if (get_gas_string(d->cylinder[0].gasmix) != gasmix) {
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
	if (!same_string(d->suit, qPrintable(suit))) {
		diveChanged = true;
		free(d->suit);
		d->suit = strdup(qPrintable(suit));
	}
	if (!same_string(d->buddy, qPrintable(buddy))) {
		diveChanged = true;
		free(d->buddy);
		d->buddy = strdup(qPrintable(buddy));
	}
	if (!same_string(d->divemaster, qPrintable(diveMaster))) {
		diveChanged = true;
		free(d->divemaster);
		d->divemaster = strdup(qPrintable(diveMaster));
	}
	if (!same_string(d->notes, qPrintable(notes))) {
		diveChanged = true;
		free(d->notes);
		d->notes = strdup(qPrintable(notes));
	}
	// now that we have it all figured out, let's see what we need
	// to update
	DiveListModel *dm = DiveListModel::instance();
	int oldModelIdx = dm->getDiveIdx(d->id);
	int oldIdx = get_idx_by_uniq_id(d->id);
	if (needResort) {
		// we know that the only thing that might happen in a resort is that
		// this one dive moves to a different spot in the dive list
		sort_table(&dive_table);
		int newIdx = get_idx_by_uniq_id(d->id);
		if (newIdx != oldIdx) {
			DiveObjectHelper *newDive = new DiveObjectHelper(d);
			DiveListModel::instance()->removeDive(oldModelIdx);
			DiveListModel::instance()->insertDive(oldModelIdx - (newIdx - oldIdx), newDive);
			diveChanged = false; // because we already modified things
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
		DiveListModel::instance()->updateDive(oldModelIdx, d);
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
#if defined(Q_OS_IOS)
	mark_divelist_changed(true);
#elif defined(Q_OS_ANDROID)
	saveChangesLocal();
#else
	saveChangesCloud(false);
#endif
}
void QMLManager::saveChangesLocal()
{
	if (unsaved_changes()) {
		git_storage_update_progress(true, "saving dives locally"); // reset the timers
		if (!loadFromCloud()) {
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
			appendTextToLog(get_error_string());
			setAccessingCloud(-1);
			prefs.git_local_only = glo;
			alreadySaving = false;
			return;
		}
		prefs.git_local_only = glo;
		mark_divelist_changed(false);
		git_storage_update_progress(false, "done with local save");
		alreadySaving = false;
	} else {
		appendTextToLog("local save requested with no unsaved changes");
	}
}

void QMLManager::saveChangesCloud(bool forceRemoteSync)
{
	if (prefs.git_local_only && !forceRemoteSync)
		return;

	git_storage_update_progress(true, "start save change to cloud");
	if (!loadFromCloud()) {
		appendTextToLog("Don't save dives without loading from the cloud, first.");
		return;
	}
	bool glo = prefs.git_local_only;
	// first we need to store any unsaved changes to the local repo
	saveChangesLocal();
	if (alreadySaving) {
		appendTextToLog("save operation in progress already, can't sync with server");
		return;
	}
	prefs.git_local_only = false;
	alreadySaving = true;
	loadDivesWithValidCredentials();
	alreadySaving = false;
	git_storage_update_progress(false, "finished syncing dive list to cloud server");
	setAccessingCloud(-1);
	prefs.git_local_only = glo;
	alreadySaving = false;
}

bool QMLManager::undoDelete(int id)
{
	if (!deletedDive || deletedDive->id != id) {
		qDebug() << "can't find the deleted dive";
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
		qDebug() << "oops, trying to delete non-existing dive";
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
	m_logText += "\n" + newText;
	emit logTextChanged();
}

bool QMLManager::locationServiceEnabled() const
{
	return m_locationServiceEnabled;
}

void QMLManager::setLocationServiceEnabled(bool locationServiceEnabled)
{
	m_locationServiceEnabled = locationServiceEnabled;
	locationProvider->serviceEnable(m_locationServiceEnabled);
}

bool QMLManager::verboseEnabled() const
{
	return m_verboseEnabled;
}

void QMLManager::setVerboseEnabled(bool verboseMode)
{
	m_verboseEnabled = verboseMode;
	verbose = verboseMode;
	qDebug() << "verbose is" << verbose;
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
	emit timeThresholdChanged();
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

int QMLManager::accessingCloud() const
{
	return m_accessingCloud;
}

void QMLManager::setAccessingCloud(int status)
{
	m_accessingCloud = status;
	emit accessingCloudChanged();
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

void QMLManager::screenChanged(QScreen *screen)
{
	m_lastDevicePixelRatio = screen->devicePixelRatio();
	emit sendScreenChanged(screen);
}
