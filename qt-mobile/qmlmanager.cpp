#include "qmlmanager.h"
#include <QUrl>
#include <QSettings>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QAuthenticator>
#include <QDesktopServices>
#include <QTextDocument>

#include "qt-models/divelistmodel.h"
#include <gpslistmodel.h>
#include "divelist.h"
#include "pref.h"
#include "qthelper.h"
#include "qt-gui.h"
#include "git-access.h"

QMLManager *QMLManager::m_instance = NULL;

static void appendTextToLogStandalone(const char *text)
{
	QMLManager *mgr = QMLManager::instance();
	if (mgr)
		mgr->appendTextToLog(QString(text));
}

extern "C" int gitProgressCB(int percent)
{
	static int lastPercent = -10;

	if (percent - lastPercent >= 10) {
		lastPercent += 10;
		QMLManager *mgr = QMLManager::instance();
		if (mgr)
			mgr->loadDiveProgress(percent);
	}
	// return 0 so that we don't end the download
	return 0;
}

QMLManager::QMLManager() :
	m_locationServiceEnabled(false),
	m_verboseEnabled(false),
	m_loadFromCloud(false),
	reply(0),
	mgr(0)
{
	m_instance = this;
	appendTextToLog(getUserAgent());
	appendTextToLog(QString("build with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion()));
	qDebug() << "Starting" << getUserAgent();
	qDebug() << QString("build with Qt Version %1, runtime from Qt Version %2").arg(QT_VERSION_STR).arg(qVersion());
	m_startPageText = tr("Searching for dive data");
	// create location manager service
	locationProvider = new GpsLocation(&appendTextToLogStandalone, this);
	set_git_update_cb(&gitProgressCB);
}

void QMLManager::finishSetup()
{
	// Initialize cloud credentials.
	setCloudUserName(prefs.cloud_storage_email);
	setCloudPassword(prefs.cloud_storage_password);
	setSaveCloudPassword(prefs.save_password_local);
	// if the cloud credentials are valid, we should get the GPS Webservice ID as well
	QString url;
	if (!same_string(prefs.cloud_storage_email, "") &&
	    !same_string(prefs.cloud_storage_password, "") &&
	    getCloudURL(url) == 0) {
		clear_dive_file_data();
		QByteArray fileNamePrt  = QFile::encodeName(url);
		prefs.git_local_only = true;
		int error = parse_file(fileNamePrt.data());
		prefs.git_local_only = false;
		if (error) {
			appendTextToLog(QString("loading dives from cache failed %1").arg(error));
		} else {
			prefs.unit_system = informational_prefs.unit_system;
			prefs.units = informational_prefs.units;
			int i;
			struct dive *d;
			process_dives(false, false);
			DiveListModel::instance()->clear();
			for_each_dive(i, d) {
				DiveListModel::instance()->addDive(d);
			}
			appendTextToLog(QString("%1 dives loaded from cache").arg(i));
		}
		appendTextToLog("have cloud credentials, trying to connect");
		tryRetrieveDataFromBackend();
	} else {
		appendTextToLog("no cloud credentials, tell user no dives found");
		setStartPageText(tr("No recorded dives found. You can download your dives to this device from the Subsurface cloud storage service, from your dive computer, or add them manually."));
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
	s.setValue("save_password_local", saveCloudPassword());
	if (saveCloudPassword())
		s.setValue("password", cloudPassword());
	s.sync();
	if (!same_string(prefs.cloud_storage_email, qPrintable(cloudUserName()))) {
		free(prefs.cloud_storage_email);
		prefs.cloud_storage_email = strdup(qPrintable(cloudUserName()));
		cloudCredentialsChanged = true;
	}
	if (saveCloudPassword() != prefs.save_password_local)
		prefs.save_password_local = saveCloudPassword();

	cloudCredentialsChanged |= !same_string(prefs.cloud_storage_password, qPrintable(cloudPassword()));

	if (saveCloudPassword()) {
		if (!same_string(prefs.cloud_storage_password, qPrintable(cloudPassword()))) {
			free(prefs.cloud_storage_password);
			prefs.cloud_storage_password = strdup(qPrintable(cloudPassword()));
		}
	}
	if (cloudCredentialsChanged) {
		free(prefs.userid);
		prefs.userid = NULL;
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
		if (!mgr)
			mgr = new QNetworkAccessManager(this);
		connect(mgr, &QNetworkAccessManager::authenticationRequired, this, &QMLManager::provideAuth, Qt::UniqueConnection);
		connect(mgr, &QNetworkAccessManager::finished, this, execute, Qt::UniqueConnection);
		QUrl url(CLOUDREDIRECTURL);
		request = QNetworkRequest(url);
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setRawHeader("Accept", "text/html");
		reply = mgr->get(request);
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleError(QNetworkReply::NetworkError)));
		connect(reply, &QNetworkReply::sslErrors, this, &QMLManager::handleSslErrors);
	}
}

void QMLManager::tryRetrieveDataFromBackend()
{
	checkCredentialsAndExecute(&QMLManager::retrieveUserid);
}

void QMLManager::loadDives()
{
	checkCredentialsAndExecute(&QMLManager::loadDivesWithValidCredentials);
}

void QMLManager::provideAuth(QNetworkReply *reply, QAuthenticator *auth)
{
	if (auth->user() == QString(prefs.cloud_storage_email) &&
	    auth->password() == QString(prefs.cloud_storage_password)) {
		// OK, credentials have been tried and didn't work, so they are invalid
		appendTextToLog("Cloud credentials are invalid");
		setStartPageText(tr("Cloud credentials are invalid"));
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
	setStartPageText(tr("Cannot open cloud storage: Error creating https connection"));
	Q_FOREACH(QSslError e, errors) {
		qDebug() << e.errorString();
	}
	reply->abort();
	reply->deleteLater();
}

void QMLManager::handleError(QNetworkReply::NetworkError nError)
{
	QString errorString = reply->errorString();
	qDebug() << "handleError" << nError << errorString;
	setStartPageText(tr("Cannot open cloud storage: %1").arg(errorString));
	reply->abort();
	reply->deleteLater();
}

void QMLManager::retrieveUserid()
{
	if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 302) {
		appendTextToLog(QString("Cloud storage connection not working correctly: ") + reply->readAll());
		return;
	}
	QString userid(prefs.userid);
	if (userid.isEmpty()) {
		if (same_string(prefs.cloud_storage_email, "") || same_string(prefs.cloud_storage_password, "")) {
			appendTextToLog("cloud user name or password are empty, can't retrieve web user id");
			return;
		}
		appendTextToLog(QString("calling getUserid with user %1").arg(prefs.cloud_storage_email));
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
	if (!loadFromCloud())
		loadDivesWithValidCredentials();
}

void QMLManager::loadDiveProgress(int percent)
{
	QString text(tr("Loading dive list from cloud storage."));
	while(percent > 0) {
		text.append(".");
		percent -= 10;
	}
	setStartPageText(text);
}

void QMLManager::loadDivesWithValidCredentials()
{
	if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 302) {
		appendTextToLog(QString("Cloud storage connection not working correctly: ") + reply->readAll());
		setStartPageText(tr("Cannot connect to cloud storage"));
		return;
	}
	appendTextToLog("Cloud credentials valid, loading dives...");
	loadDiveProgress(0);
	QString url;
	if (getCloudURL(url)) {
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		setStartPageText(tr("Cloud storage error: %1").arg(errorString));
		return;
	}
	QByteArray fileNamePrt  = QFile::encodeName(url);
	if (check_git_sha(fileNamePrt.data()) == 0) {
		qDebug() << "local cache was current, no need to modify dive list";
		appendTextToLog("Cloud sync shows local cache was current");
		setLoadFromCloud(true);
		return;
	}
	clear_dive_file_data();
	DiveListModel::instance()->clear();

	int error = parse_file(fileNamePrt.data());
	if (!error) {
		report_error("filename is now %s", fileNamePrt.data());
		const char *error_string = get_error_string();
		appendTextToLog(error_string);
		set_filename(fileNamePrt.data(), true);
	} else {
		report_error("failed to open file %s", fileNamePrt.data());
		QString errorString(get_error_string());
		appendTextToLog(errorString);
		setStartPageText(tr("Cloud storage error: %1").arg(errorString));
		return;
	}
	prefs.unit_system = informational_prefs.unit_system;
	prefs.units = informational_prefs.units;
	process_dives(false, false);

	int i;
	struct dive *d;

	for_each_dive(i, d) {
		DiveListModel::instance()->addDive(d);
	}
	appendTextToLog(QString("%1 dives loaded").arg(i));
	if (dive_table.nr == 0)
		setStartPageText(tr("Cloud storage open successfully. No dives in dive list."));
	setLoadFromCloud(true);
}

void QMLManager::refreshDiveList()
{
	int i;
	struct dive *d;
	DiveListModel::instance()->clear();
	for_each_dive(i, d) {
		DiveListModel::instance()->addDive(d);
	}

}

// update the dive and return the notes field, stripped of the HTML junk
QString QMLManager::commitChanges(QString diveId, QString date, QString location, QString gps, QString duration, QString depth,
			       QString airtemp, QString watertemp, QString suit, QString buddy, QString diveMaster, QString notes)
{
	struct dive *d = get_dive_by_uniq_id(diveId.toInt());
	// notes comes back as rich text - let's convert this into plain text
	QTextDocument doc;
	doc.setHtml(notes);
	notes = doc.toPlainText();

	if (!d) {
		qDebug() << "don't touch this... no dive";
		return notes;
	}
	bool diveChanged = false;
	bool needResort = false;

	if (date != get_dive_date_string(d->when)) {
		needResort = true;
		QDateTime newDate;
		// what a pain - Qt will not parse dates if the day of the week is incorrect
		// so if the user changed the date but didn't update the day of the week (most likely behavior, actually),
		// we need to make sure we don't try to parse that
		QString format(QString(prefs.date_format) + " " + prefs.time_format);
		if (format.contains("ddd") || format.contains("dddd")) {
			QString dateFormatToDrop = format.contains("ddd") ? "ddd" : "dddd";
			QDateTime ts;
			QLocale loc = getLocale();
			ts.setMSecsSinceEpoch(d->when * 1000L);
			QString drop = loc.toString(ts.toUTC(), dateFormatToDrop);
			format.replace(dateFormatToDrop, "");
			date.replace(drop, "");
		}
		newDate = QDateTime::fromString(date, format);
		d->when = newDate.toMSecsSinceEpoch() / 1000 + gettimezoneoffset(newDate.toMSecsSinceEpoch() / 1000);
	}
	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	char *locationtext = NULL;
	if (ds)
		locationtext = ds->name;
	if (!same_string(locationtext, qPrintable(location))) {
		diveChanged = true;
		// this is not ideal - and it's missing the gps information
		// but for now let's just create a new dive site
		ds = get_dive_site_by_uuid(create_dive_site(qPrintable(location), d->when));
		d->dive_site_uuid = ds->uuid;
	}
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
	if (get_dive_duration_string(d->duration.seconds, tr("h:"), tr("min")) != duration) {
		diveChanged = true;
		int h = 0, m = 0, s = 0;
		QRegExp r1(QString("(\\d*)%1[\\s,:]*(\\d*)%2[\\s,:]*(\\d*)%3").arg(tr("h")).arg(tr("min")).arg(tr("sec")), Qt::CaseInsensitive);
		QRegExp r2(QString("(\\d*)%1[\\s,:]*(\\d*)%2").arg(tr("h")).arg(tr("min")), Qt::CaseInsensitive);
		QRegExp r3(QString("(\\d*)%1").arg(tr("min")), Qt::CaseInsensitive);
		QRegExp r4(QString("(\\d*):(\\d*):(\\d*)"));
		QRegExp r5(QString("(\\d*):(\\d*)"));
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
		}
		d->duration.seconds = h * 3600 + m * 60 + s;
	}
	if (get_depth_string(d->maxdepth.mm, true, true) != depth) {
		diveChanged = true;
		if (depth.contains(tr("ft")))
			prefs.units.length = units::FEET;
		else if (depth.contains(tr("m")))
			prefs.units.length = units::METERS;
		d->maxdepth.mm = parseLengthToMm(depth);
		if (same_string(d->dc.model, "manually added dive"))
			d->dc.maxdepth.mm = d->maxdepth.mm;
	}
	if (get_temperature_string(d->airtemp) != airtemp) {
		diveChanged = true;
		if (airtemp.contains(tr("C")))
			prefs.units.temperature = units::CELSIUS;
		else if (airtemp.contains(tr("F")))
			prefs.units.temperature = units::FAHRENHEIT;
		d->airtemp.mkelvin = parseTemperatureToMkelvin(airtemp);
	}
	if (get_temperature_string(d->watertemp) != watertemp) {
		diveChanged = true;
		if (watertemp.contains(tr("C")))
			prefs.units.temperature = units::CELSIUS;
		else if (watertemp.contains(tr("F")))
			prefs.units.temperature = units::FAHRENHEIT;
		d->watertemp.mkelvin = parseTemperatureToMkelvin(watertemp);
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
	if (needResort)
		sort_table(&dive_table);
	if (diveChanged || needResort) {
		refreshDiveList();
		mark_divelist_changed(true);
	}
	return notes;
}

void QMLManager::saveChanges()
{
	if (!loadFromCloud()) {
		appendTextToLog("Don't save dives without loading from the cloud, first.");
		return;
	}
	appendTextToLog("Saving dives.");
	QString fileName;
	if (getCloudURL(fileName)) {
		appendTextToLog(get_error_string());
		return;
	}

	if (save_dives(fileName.toUtf8().data())) {
		appendTextToLog(get_error_string());
		return;
	}

	appendTextToLog("Dive saved.");
	set_filename(fileName.toUtf8().data(), true);
	mark_divelist_changed(false);
}

QString QMLManager::addDive()
{
	appendTextToLog("Adding new dive.");
	return DiveListModel::instance()->startAddDive();
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

bool QMLManager::saveCloudPassword() const
{
	return m_saveCloudPassword;
}

void QMLManager::setSaveCloudPassword(bool saveCloudPassword)
{
	m_saveCloudPassword = saveCloudPassword;
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
	m_cloudUserName = cloudUserName;
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

void QMLManager::setLoadFromCloud(bool done)
{
	m_loadFromCloud = done;
	emit loadFromCloudChanged();
}

QString QMLManager::startPageText() const
{
	return m_startPageText;
}

void QMLManager::setStartPageText(QString text)
{
	m_startPageText = text;
	emit startPageTextChanged();
}

void QMLManager::showMap(QString location)
{
	if (!location.isEmpty()) {
		QString link = QString("https://www.google.com/maps/place/%1/@%2,5000m/data=!3m1!1e3!4m2!3m1!1s0x0:0x0")
			       .arg(location).arg(location);
		QDesktopServices::openUrl(link);
	}
}

QString QMLManager::getNumber(QString diveId)
{
	int dive_id = diveId.toInt();
	struct dive *d = get_dive_by_uniq_id(dive_id);
	QString number;
	if (d)
		number = QString::number(d->number);
	return number;
}

QString QMLManager::getDate(QString diveId)
{
	int dive_id = diveId.toInt();
	struct dive *d = get_dive_by_uniq_id(dive_id);
	QString datestring;
	if (d)
		datestring = get_dive_date_string(d->when);
	return datestring;
}
