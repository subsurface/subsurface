#include "qmlmanager.h"
#include <QUrl>
#include <QSettings>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QAuthenticator>

#include "qt-models/divelistmodel.h"
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
	reply(0),
	mgr(0)
{
	m_instance = this;
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
	if (!same_string(prefs.cloud_storage_email, "") &&
	    !same_string(prefs.cloud_storage_password, ""))
		tryRetrieveDataFromBackend();
	else
		m_startPageText = "No recorded dives found. You can download your dives to this device from the Subsurface cloud storage service, from your dive computer, or add them manually.";

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
	if (userid.isEmpty())
		userid = locationProvider->getUserid(prefs.cloud_storage_email, prefs.cloud_storage_password);
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
	clear_dive_file_data();

	QByteArray fileNamePrt  = QFile::encodeName(url);
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
	process_dives(false, false);

	int i;
	struct dive *d;

	DiveListModel::instance()->clear();
	for_each_dive(i, d) {
		DiveListModel::instance()->addDive(d);
	}
	appendTextToLog(QString("%1 dives loaded").arg(i));
	if (dive_table.nr == 0)
		setStartPageText(tr("Cloud storage open successfully. No dives in dive list."));
	setLoadFromCloud(true);
}

void QMLManager::commitChanges(QString diveId, QString suit, QString buddy, QString diveMaster, QString notes)
{
	struct dive *d = get_dive_by_uniq_id(diveId.toInt());
	bool diveChanged = false;

	if (!same_string(d->suit, suit.toUtf8().data())) {
		diveChanged = true;
		free(d->suit);
		d->suit = strdup(suit.toUtf8().data());
	}
	if (!same_string(d->buddy, buddy.toUtf8().data())) {
		diveChanged = true;
		free(d->buddy);
		d->buddy = strdup(buddy.toUtf8().data());
	}
	if (!same_string(d->divemaster, diveMaster.toUtf8().data())) {
		diveChanged = true;
		free(d->divemaster);
		d->divemaster = strdup(diveMaster.toUtf8().data());
	}
	if (!same_string(d->notes, notes.toUtf8().data())) {
		diveChanged = true;
		free(d->notes);
		d->notes = strdup(notes.toUtf8().data());
	}
	if (diveChanged) {
		DiveListModel::instance()->updateDive(d);
		mark_divelist_changed(true);
	}
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

void QMLManager::addDive()
{
	appendTextToLog("Adding new dive.");
	DiveListModel::instance()->startAddDive();
}

void QMLManager::applyGpsData()
{
	locationProvider->applyLocations();
}

void QMLManager::sendGpsData()
{
	locationProvider->uploadToServer();
}

void QMLManager::clearGpsData()
{
	locationProvider->clearGpsData();
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
