#include "gpslocation.h"
#include "gpslistmodel.h"
#include "pref.h"
#include "dive.h"
#include "helpers.h"
#include <time.h>
#include <unistd.h>
#include <QDebug>
#include <QVariant>
#include <QUrlQuery>
#include <QApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#define GPS_FIX_ADD_URL "http://api.subsurface-divelog.org/api/dive/add/"
#define GPS_FIX_DELETE_URL "http://api.subsurface-divelog.org/api/dive/delete/"
#define GPS_FIX_DOWNLOAD_URL "http://api.subsurface-divelog.org/api/dive/get/"
#define GET_WEBSERVICE_UID_URL "https://cloud.subsurface-divelog.org/webuserid/"

GpsLocation *GpsLocation::m_Instance = NULL;

GpsLocation::GpsLocation(void (*showMsgCB)(const char *), QObject *parent)
{
	Q_ASSERT_X(m_Instance == NULL, "GpsLocation", "GpsLocation recreated");
	m_Instance = this;
	m_GpsSource = 0;
	waitingForPosition = false;
	showMessageCB = showMsgCB;
	// create a QSettings object that's separate from the main application settings
	geoSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope,
				    QString("org.subsurfacedivelog"), QString("subsurfacelocation"), this);
#ifdef SUBSURFACE_MOBILE
	if (hasLocationsSource())
		status("Found GPS");
#endif
	userAgent = getUserAgent();
	loadFromStorage();
}

GpsLocation *GpsLocation::instance()
{
	Q_ASSERT(m_Instance != NULL);

	return m_Instance;
}

GpsLocation::~GpsLocation()
{
	m_Instance = NULL;
}

QGeoPositionInfoSource *GpsLocation::getGpsSource()
{
	if (!m_GpsSource) {
		m_GpsSource = QGeoPositionInfoSource::createDefaultSource(this);
		if (m_GpsSource != 0) {
#ifndef SUBSURFACE_MOBILE
			if (verbose)
#endif
				status("created GPS source");
			QString msg = QString("have position source %1").arg(m_GpsSource->sourceName());
			connect(m_GpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(newPosition(QGeoPositionInfo)));
			connect(m_GpsSource, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
			m_GpsSource->setUpdateInterval(5 * 60 * 1000); // 5 minutes so the device doesn't drain the battery
		} else {
#ifdef SUBSURFACE_MOBILE
			status("don't have GPS source");
#endif
		}
	}
	return m_GpsSource;
}

bool GpsLocation::hasLocationsSource()
{
	QGeoPositionInfoSource *gpsSource = getGpsSource();
	return gpsSource != 0 && (gpsSource->supportedPositioningMethods() & QGeoPositionInfoSource::SatellitePositioningMethods);
}

void GpsLocation::serviceEnable(bool toggle)
{
	QGeoPositionInfoSource *gpsSource = getGpsSource();
	if (!gpsSource) {
		if (toggle)
			status("Can't start location service, no location source available");
		return;
	}
	if (toggle) {
		gpsSource->startUpdates();
		status("Starting Subsurface GPS service");
	} else {
		gpsSource->stopUpdates();
		status("Stopping Subsurface GPS service");
	}
}

QString GpsLocation::currentPosition()
{
	if (!hasLocationsSource())
		return tr("Unknown GPS location");
	if (m_trackers.count() && m_trackers.lastKey() + 300 >= QDateTime::currentMSecsSinceEpoch() / 1000) {
		// we can simply use the last position that we tracked
		gpsTracker gt = m_trackers.last();
		QString gpsString = printGPSCoords(gt.latitude.udeg, gt.longitude.udeg);
		qDebug() << "returning last position" << gpsString;
		return gpsString;
	}
	qDebug() << "requesting new GPS position";
	m_GpsSource->requestUpdate();
	// ok, we need to get the current position and somehow in the callback update the location in the QML UI
	// punting right now
	waitingForPosition = true;
	return QString("waiting for the next gps location");
}

void GpsLocation::newPosition(QGeoPositionInfo pos)
{
	int64_t lastTime;
	QGeoCoordinate lastCoord;
	QString msg("received new position %1");
	status(qPrintable(msg.arg(pos.coordinate().toString())));
	int nr = m_trackers.count();
	if (nr) {
		gpsTracker gt = m_trackers.last();
		lastCoord.setLatitude(gt.latitude.udeg / 1000000.0);
		lastCoord.setLongitude(gt.longitude.udeg / 1000000.0);
		lastTime = gt.when;
	}
	// if we are waiting for a position update or
	// if we have no record stored or if at least the configured minimum
	// time has passed or we moved at least the configured minimum distance
	if (!nr || waitingForPosition ||
	    (int64_t)pos.timestamp().toTime_t() > lastTime + prefs.time_threshold ||
	    lastCoord.distanceTo(pos.coordinate()) > prefs.distance_threshold) {
		waitingForPosition = false;
		gpsTracker gt;
		gt.when = pos.timestamp().toTime_t();
		gt.when += gettimezoneoffset(gt.when);
		gt.latitude.udeg = rint(pos.coordinate().latitude() * 1000000);
		gt.longitude.udeg = rint(pos.coordinate().longitude() * 1000000);
		addFixToStorage(gt);
	}
}

void GpsLocation::updateTimeout()
{
	status("request to get new position timed out");
}

void GpsLocation::status(QString msg)
{
	qDebug() << msg;
	if (showMessageCB)
		(*showMessageCB)(qPrintable(msg));
}

QString GpsLocation::getUserid(QString user, QString passwd)
{
	qDebug() << "called getUserid";
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);

	QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	QUrl url(GET_WEBSERVICE_UID_URL);
	QString data;
	data = user + " " + passwd;
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	request.setRawHeader("Accept", "text/html");
	request.setRawHeader("Content-type", "application/x-www-form-urlencoded");
	reply = manager->post(request, data.toUtf8());
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(getUseridError(QNetworkReply::NetworkError)));
	timer.start(10000);
	loop.exec();
	if (timer.isActive()) {
		timer.stop();
		if (reply->error() == QNetworkReply::NoError) {
			QString result = reply->readAll();
			status(QString("received ") + result);
			result.remove("WebserviceID:");
			reply->deleteLater();
			return result;
		}
	} else {
		status("Getting Web service ID timed out");
	}
	reply->deleteLater();
	return QString();
}

int GpsLocation::getGpsNum() const
{
	return m_trackers.count();
}

static void copy_gps_location(struct gpsTracker &gps, struct dive *d)
{
	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	ds->latitude = gps.latitude;
	ds->longitude = gps.longitude;
}

#define SAME_GROUP 6 * 3600 /* six hours */
bool GpsLocation::applyLocations()
{
	int i;
	bool changed = false;
	int last = 0;
	int cnt = m_trackers.count();
	if (cnt == 0)
		return false;

	// create a table with the GPS information
	QList<struct gpsTracker> gpsTable = m_trackers.values();

	// now walk the dive table and see if we can fill in missing gps data
	struct dive *d;
	for_each_dive(i, d) {
		if (dive_has_gps_location(d))
			continue;
		for (int j = last; j < cnt; j++) {
			if (time_during_dive_with_offset(d, gpsTable[j].when, SAME_GROUP)) {
				if (verbose)
					qDebug() << "processing gpsFix @" << get_dive_date_string(gpsTable[j].when) <<
						    "which is withing six hours of dive from" <<
						    get_dive_date_string(d->when) << "until" <<
						    get_dive_date_string(d->when + d->duration.seconds);
				/*
				 * If position is fixed during dive. This is the good one.
				 * Asign and mark position, and end gps_location loop
				 */
				if (time_during_dive_with_offset(d, gpsTable[j].when, 0)) {
					if (verbose)
						qDebug() << "gpsFix is during the dive, pick that one";
					copy_gps_location(gpsTable[j], d);
					changed = true;
					last = j;
					break;
				} else {
					/*
					 * If it is not, check if there are more position fixes in SAME_GROUP range
					 */
					if (j + 1 < cnt && time_during_dive_with_offset(d, gpsTable[j+1].when, SAME_GROUP)) {
						if (verbose)
							qDebug() << "look at the next gps fix @" << get_dive_date_string(gpsTable[j+1].when);
						/* first let's test if this one is during the dive */
						if (time_during_dive_with_offset(d, gpsTable[j+1].when, 0)) {
							if (verbose)
								qDebug() << "which is during the dive, pick that one";
							copy_gps_location(gpsTable[j+1], d);
							changed = true;
							last = j + 1;
							break;
						}
						/* we know the gps fixes are sorted; if they are both before the dive, ignore the first,
						 * if theay are both after the dive, take the first,
						 * if the first is before and the second is after, take the closer one */
						if (gpsTable[j+1].when < d->when) {
							if (verbose)
								qDebug() << "which is closer to the start of the dive, do continue with that";
							continue;
						} else if (gpsTable[j].when > d->when + d->duration.seconds) {
							if (verbose)
								qDebug() << "which is even later after the end of the dive, so pick the previous one";
							copy_gps_location(gpsTable[j], d);
							changed = true;
							last = j;
							break;
						} else {
							/* ok, gpsFix is before, nextgpsFix is after */
							if (d->when - gpsTable[j].when <= gpsTable[j+1].when - (d->when + d->duration.seconds)) {
								if (verbose)
									qDebug() << "pick the one before as it's closer to the start";
								copy_gps_location(gpsTable[j], d);
								changed = true;
								last = j;
								break;
							} else {
								if (verbose)
									qDebug() << "pick the one after as it's closer to the start";
								copy_gps_location(gpsTable[j+1], d);
								changed = true;
								last = j + 1;
								break;
							}
						}
					/*
					 * If no more positions in range, the actual is the one. Asign, mark and end loop.
					 */
					} else {
						if (verbose)
							qDebug() << "which seems to be the best one for this dive, so pick it";
						copy_gps_location(gpsTable[j], d);
						changed = true;
						last = j;
						break;
					}
				}
			} else {
				/* If position is out of SAME_GROUP range and in the future, mark position for
				 * next dive iteration and end the gps_location loop
				 */
				if (gpsTable[j].when >= d->when + d->duration.seconds + SAME_GROUP) {
					last = j;
					break;
				}
			}

		}
	}
	if (changed)
		mark_divelist_changed(true);
	return changed;
}

QMap<qint64, gpsTracker> GpsLocation::currentGPSInfo() const
{
	return m_trackers;
}

void GpsLocation::loadFromStorage()
{
	qDebug() << "loadFromStorage with # trackers" << m_trackers.count();
	int nr = geoSettings->value(QString("count")).toInt();
	qDebug() << "loading from settings:" << nr;
	for (int i = 0; i < nr; i++) {
		struct gpsTracker gt;
		gt.when = geoSettings->value(QString("gpsFix%1_time").arg(i)).toLongLong();
		gt.latitude.udeg = geoSettings->value(QString("gpsFix%1_lat").arg(i)).toInt();
		gt.longitude.udeg = geoSettings->value(QString("gpsFix%1_lon").arg(i)).toInt();
		gt.name = geoSettings->value(QString("gpsFix%1_name").arg(i)).toString();
		gt.idx = i;
		m_trackers.insert(gt.when, gt);
		qDebug() << "inserted" << i << "timestamps are" << m_trackers.keys();
	}
}

void GpsLocation::replaceFixToStorage(gpsTracker &gt)
{
	if (!m_trackers.keys().contains(gt.when)) {
		qDebug() << "shouldn't have called replace, call add instead";
		addFixToStorage(gt);
		return;
	}
	gpsTracker replacedTracker = m_trackers.value(gt.when);
	geoSettings->setValue(QString("gpsFix%1_time").arg(replacedTracker.idx), gt.when);
	geoSettings->setValue(QString("gpsFix%1_lat").arg(replacedTracker.idx), gt.latitude.udeg);
	geoSettings->setValue(QString("gpsFix%1_lon").arg(replacedTracker.idx), gt.longitude.udeg);
	geoSettings->setValue(QString("gpsFix%1_name").arg(replacedTracker.idx), gt.name);
	replacedTracker.latitude = gt.latitude;
	replacedTracker.longitude = gt.longitude;
	replacedTracker.name = gt.name;
}

void GpsLocation::addFixToStorage(gpsTracker &gt)
{
	int nr = m_trackers.count();
	qDebug() << "addFixToStorage before there are" << nr << "fixes at" << m_trackers.keys();
	geoSettings->setValue("count", nr + 1);
	geoSettings->setValue(QString("gpsFix%1_time").arg(nr), gt.when);
	geoSettings->setValue(QString("gpsFix%1_lat").arg(nr), gt.latitude.udeg);
	geoSettings->setValue(QString("gpsFix%1_lon").arg(nr), gt.longitude.udeg);
	geoSettings->setValue(QString("gpsFix%1_name").arg(nr), gt.name);
	gt.idx = nr;
	geoSettings->sync();
	m_trackers.insert(gt.when, gt);
}

void GpsLocation::deleteFixFromStorage(gpsTracker &gt)
{
	bool found = false;
	int i;
	qint64 when = gt.when;
	int cnt = m_trackers.count();
	if (cnt == 0 || !m_trackers.keys().contains(when)) {
		qDebug() << "no gps fix with timestamp" << when;
		return;
	}
	if (geoSettings->value(QString("gpsFix%1_time").arg(gt.idx)).toULongLong() != when) {
		qDebug() << "uh oh - index for tracker has gotten out of sync...";
		return;
	}
	if (cnt > 1) {
		// now we move the last tracker into that spot (so our settings stay consecutive)
		// and delete the last settings entry
		when = geoSettings->value(QString("gpsFix%1_time").arg(cnt - 1)).toLongLong();
		struct gpsTracker movedTracker = m_trackers.value(when);
		movedTracker.idx = gt.idx;
		m_trackers.remove(movedTracker.when);
		m_trackers.insert(movedTracker.when, movedTracker);
		geoSettings->setValue(QString("gpsFix%1_time").arg(movedTracker.idx), when);
		geoSettings->setValue(QString("gpsFix%1_lat").arg(movedTracker.idx), movedTracker.latitude.udeg);
		geoSettings->setValue(QString("gpsFix%1_lon").arg(movedTracker.idx), movedTracker.longitude.udeg);
		geoSettings->setValue(QString("gpsFix%1_name").arg(movedTracker.idx), movedTracker.name);
		geoSettings->remove(QString("gpsFix%1_lat").arg(cnt - 1));
		geoSettings->remove(QString("gpsFix%1_lon").arg(cnt - 1));
		geoSettings->remove(QString("gpsFix%1_time").arg(cnt - 1));
		geoSettings->remove(QString("gpsFix%1_name").arg(cnt - 1));
	}
	geoSettings->setValue("count", cnt - 1);
	geoSettings->sync();
	m_trackers.remove(gt.when);
}

void GpsLocation::deleteGpsFix(qint64 when)
{
	struct gpsTracker defaultTracker = { .when = 0 };
	struct gpsTracker deletedTracker = m_trackers.value(when, defaultTracker);
	if (deletedTracker.when != when) {
		qDebug() << "can't find tracker for timestamp" << when;
		return;
	}
	deleteFixFromStorage(deletedTracker);
	m_deletedTrackers.append(deletedTracker);
}

void GpsLocation::clearGpsData()
{
	m_trackers.clear();
	geoSettings->clear();
	geoSettings->sync();
}

void GpsLocation::postError(QNetworkReply::NetworkError error)
{
	status(QString("error when sending a GPS fix: %1").arg(reply->errorString()));
}

void GpsLocation::getUseridError(QNetworkReply::NetworkError error)
{
	status(QString("error when retrieving Subsurface webservice user id: %1").arg(reply->errorString()));
}

void GpsLocation::deleteFixesFromServer()
{
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);

	QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	QUrl url(GPS_FIX_DELETE_URL);
	QList<qint64> keys = m_trackers.keys();
	qint64 key;
	while (!m_deletedTrackers.isEmpty()) {
		gpsTracker gt = m_deletedTrackers.takeFirst();
		int idx = gt.idx;
		QDateTime dt;
		QUrlQuery data;
		dt.setTime_t(gt.when - gettimezoneoffset(gt.when));
		qDebug() << dt.toString() << get_dive_date_string(gt.when);
		data.addQueryItem("login", prefs.userid);
		data.addQueryItem("dive_date", dt.toString("yyyy-MM-dd"));
		data.addQueryItem("dive_time", dt.toString("hh:mm"));
		status(data.toString(QUrl::FullyEncoded).toUtf8());
		QNetworkRequest request;
		request.setUrl(url);
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setRawHeader("Accept", "text/json");
		request.setRawHeader("Content-type", "application/x-www-form-urlencoded");
		reply = manager->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
		connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
			this, SLOT(postError(QNetworkReply::NetworkError)));
		timer.start(10000);
		loop.exec();
		if (timer.isActive()) {
			timer.stop();
			if (reply->error() != QNetworkReply::NoError) {
				QString response = reply->readAll();
				status(QString("Server response:") + reply->readAll());
			}
		} else {
			status("Deleting on the server timed out - try again later");
			m_deletedTrackers.prepend(gt);
			break;
		}
		reply->deleteLater();
		status(QString("completed deleting gps fix %1 - response: ").arg(gt.idx) + reply->readAll());
	}
}

void GpsLocation::uploadToServer()
{
	// we want to do this one at a time (the server prefers that)
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);

	QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	QUrl url(GPS_FIX_ADD_URL);
	QList<qint64> keys = m_trackers.keys();
	qint64 key;
	qDebug() << "uploading:" << keys;
	Q_FOREACH(key, keys) {
		struct gpsTracker gt = m_trackers.value(key);
		int idx = gt.idx;
		QDateTime dt;
		QUrlQuery data;
		dt.setTime_t(gt.when - gettimezoneoffset(gt.when));
		qDebug() << dt.toString() << get_dive_date_string(gt.when);
		data.addQueryItem("login", prefs.userid);
		data.addQueryItem("dive_date", dt.toString("yyyy-MM-dd"));
		data.addQueryItem("dive_time", dt.toString("hh:mm"));
		data.addQueryItem("dive_latitude", QString::number(gt.latitude.udeg / 1000000.0, 'f', 6));
		data.addQueryItem("dive_longitude", QString::number(gt.longitude.udeg / 1000000.0, 'f', 6));
		if (gt.name.isEmpty())
			gt.name = "Auto-created dive";
		data.addQueryItem("dive_name", gt.name);
		status(data.toString(QUrl::FullyEncoded).toUtf8());
		QNetworkRequest request;
		request.setUrl(url);
		request.setRawHeader("User-Agent", getUserAgent().toUtf8());
		request.setRawHeader("Accept", "text/json");
		request.setRawHeader("Content-type", "application/x-www-form-urlencoded");
		reply = manager->post(request, data.toString(QUrl::FullyEncoded).toUtf8());
		connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
		connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
		// somehoe I cannot get this to work with the new connect syntax:
		// connect(reply, &QNetworkReply::error, this, &GpsLocation::postError);
		connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
			this, SLOT(postError(QNetworkReply::NetworkError)));
		timer.start(10000);
		loop.exec();
		if (timer.isActive()) {
			timer.stop();
			if (reply->error() != QNetworkReply::NoError) {
				QString response = reply->readAll();
				if (!response.contains("Duplicate entry")) {
					status(QString("Server response:") + reply->readAll());
					break;
				}
			}
		} else {
			status("Uploading to server timed out");
			break;
		}
		reply->deleteLater();
		status(QString("completed sending gps fix %1 - response: ").arg(gt.idx) + reply->readAll());
	}
	// and now remove the ones that were locally deleted
	deleteFixesFromServer();
}

void GpsLocation::downloadFromServer()
{
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	QUrl url(QString(GPS_FIX_DOWNLOAD_URL "?login=%1").arg(prefs.userid));
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());
	request.setRawHeader("Accept", "text/json");
	request.setRawHeader("Content-type", "text/html");
	qDebug() << "downloadFromServer accessing" << url;
	reply = manager->get(request);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(getUseridError(QNetworkReply::NetworkError)));
	timer.start(10000);
	loop.exec();
	if (timer.isActive()) {
		timer.stop();
		if (!reply->error()) {
			QString response = reply->readAll();
			QJsonDocument json = QJsonDocument::fromJson(response.toLocal8Bit());
			QJsonObject object = json.object();
			if (object.value("download").toString() != "ok") {
				qDebug() << "problems downloading GPS fixes";
				return;
			}
			qDebug() << "already have" << m_trackers.count() << "GPS fixes";
			QJsonArray downloadedFixes = object.value("dives").toArray();
			qDebug() << downloadedFixes.count() << "GPS fixes downloaded";
			for (int i = 0; i < downloadedFixes.count(); i++) {
				QJsonObject fix = downloadedFixes[i].toObject();
				QString date = fix.value("date").toString();
				QString time = fix.value("time").toString();
				QString name = fix.value("name").toString();
				QString latitude = fix.value("latitude").toString();
				QString longitude = fix.value("longitude").toString();
				QDateTime timestamp = QDateTime::fromString(date + " " + time, "yyyy-M-d hh:m:s");

				struct gpsTracker gt;
				gt.when = timestamp.toMSecsSinceEpoch() / 1000 + gettimezoneoffset(timestamp.toMSecsSinceEpoch() / 1000);
				gt.latitude.udeg = latitude.toDouble() * 1000000;
				gt.longitude.udeg = longitude.toDouble() * 1000000;
				gt.name = name;
				qDebug() << "download new fix at" << gt.when;
				// add this GPS fix to the QMap and the settings (remove existing fix at the same timestamp first)
				if (m_trackers.keys().contains(gt.when)) {
					qDebug() << "already have a fix at time stamp" << gt.when;
					replaceFixToStorage(gt);
				} else {
					addFixToStorage(gt);
				}
			}
		} else {
			qDebug() << "network error" << reply->error() << reply->errorString() << reply->readAll();
		}
	} else {
		qDebug() << "download timed out";
		status("Download from server timed out");
	}
	reply->deleteLater();
}
