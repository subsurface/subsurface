#include "qt-mobile/gpslocation.h"
#include "qt-mobile/qmlmanager.h"
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

GpsLocation::GpsLocation(QObject *parent)
{
	// create a QSettings object that's separate from the main application settings
	geoSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope,
				    QString("org.subsurfacedivelog"), QString("subsurfacelocation"), this);
	gpsSource = QGeoPositionInfoSource::createDefaultSource(parent);
	if (gpsSource != 0) {
		QString msg = QString("have position source %1").arg(gpsSource->sourceName());
		connect(gpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(newPosition(QGeoPositionInfo)));
		connect(gpsSource, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
		gpsSource->setUpdateInterval(5 * 60 * 1000); // 5 minutes so the device doesn't drain the battery
	} else {
		status("don't have GPS source");
	}
}

void GpsLocation::serviceEnable(bool toggle)
{
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

void GpsLocation::newPosition(QGeoPositionInfo pos)
{
	time_t lastTime;
	QGeoCoordinate lastCoord;
	QString msg("received new position %1");
	status(qPrintable(msg.arg(pos.coordinate().toString())));
	int nr = geoSettings->value("count", 0).toInt();
	if (nr) {
		lastCoord.setLatitude(geoSettings->value(QString("gpsFix%1_lat").arg(nr - 1)).toInt() / 1000000.0);
		lastCoord.setLongitude(geoSettings->value(QString("gpsFix%1_lon").arg(nr - 1)).toInt() / 1000000.0);
		lastTime = geoSettings->value(QString("gpsFix%1_time").arg(nr - 1)).toULongLong();
	}
	// if we have no record stored or if at least the configured minimum
	// time has passed or we moved at least the configured minimum distance
	if (!nr ||
	    pos.timestamp().toTime_t() > lastTime + prefs.time_threshold ||
	    lastCoord.distanceTo(pos.coordinate()) > prefs.distance_threshold) {
		geoSettings->setValue("count", nr + 1);
		geoSettings->setValue(QString("gpsFix%1_time").arg(nr), pos.timestamp().toTime_t());
		geoSettings->setValue(QString("gpsFix%1_lat").arg(nr), rint(pos.coordinate().latitude() * 1000000));
		geoSettings->setValue(QString("gpsFix%1_lon").arg(nr), rint(pos.coordinate().longitude() * 1000000));
		geoSettings->sync();
	}
}

void GpsLocation::updateTimeout()
{
	status("request to get new position timed out");
}

void GpsLocation::status(QString msg)
{
	qDebug() << msg;
	qmlUiShowMessage(qPrintable(msg));
}

int GpsLocation::getGpsNum() const
{
	return geoSettings->value("count", 0).toInt();
}

struct gpsTracker {
	degrees_t latitude;
	degrees_t longitude;
	time_t when;
};

static void copy_gps_location(struct gpsTracker *gps, struct dive *d)
{
	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	ds->latitude = gps->latitude;
	ds->longitude = gps->longitude;
}

#define SAME_GROUP 6 * 3600 /* six hours */
bool GpsLocation::applyLocations()
{
	int i;
	bool changed = false;
	int last = 0;
	int cnt = geoSettings->value("count", 0).toInt();
	if (cnt == 0)
		return false;

	// create a table with the GPS information
	struct gpsTracker *gpsTable = (struct gpsTracker *)calloc(cnt, sizeof(struct gpsTracker));
	for (int i = 0; i < cnt; i++) {
		gpsTable[i].latitude.udeg = geoSettings->value(QString("gpsFix%1_lat").arg(i)).toInt();
		gpsTable[i].longitude.udeg = geoSettings->value(QString("gpsFix%1_lon").arg(i)).toInt();
		gpsTable[i].when = geoSettings->value(QString("gpsFix%1_time").arg(i)).toULongLong();
	}

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
					copy_gps_location(gpsTable + j, d);
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
							copy_gps_location(gpsTable + j + 1, d);
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
							copy_gps_location(gpsTable + j, d);
							changed = true;
							last = j;
							break;
						} else {
							/* ok, gpsFix is before, nextgpsFix is after */
							if (d->when - gpsTable[j].when <= gpsTable[j+1].when - (d->when + d->duration.seconds)) {
								if (verbose)
									qDebug() << "pick the one before as it's closer to the start";
								copy_gps_location(gpsTable + j, d);
								changed = true;
								last = j;
								break;
							} else {
								if (verbose)
									qDebug() << "pick the one after as it's closer to the start";
								copy_gps_location(gpsTable + j + 1, d);
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
						copy_gps_location(gpsTable + j, d);
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
}

void GpsLocation::clearGpsData()
{
	geoSettings->clear();
	geoSettings->sync();
}

void GpsLocation::postError(QNetworkReply::NetworkError error)
{
	status(QString("error when sending a GPS fix: %1").arg(reply->errorString()));
}

void GpsLocation::uploadToServer()
{
	// we want to do this one at a time (the server prefers that)
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);

	QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	QUrl url("http://api.subsurface-divelog.org/api/dive/add/");
	int count = geoSettings->value("count", 0).toInt();
	for (int i = 0; i < count; i++) {
		QDateTime dt;
		QUrlQuery data;
		if (geoSettings->contains(QString("gpsFix%1_uploaded").arg(i)))
			continue;
		time_t when = geoSettings->value(QString("gpsFix%1_time").arg(i), 0).toULongLong();
		dt.setTime_t(when);
		qDebug() << dt.toString() << get_dive_date_string(when);
		data.addQueryItem("login", prefs.userid);
		data.addQueryItem("dive_date", dt.toString("yyyy-MM-dd"));
		data.addQueryItem("dive_time", dt.toString("hh:mm"));
		data.addQueryItem("dive_latitude", QString::number(geoSettings->value(QString("gpsFix%1_lat").arg(i)).toInt() / 1000000.0));
		data.addQueryItem("dive_longitude", QString::number(geoSettings->value(QString("gpsFix%1_lon").arg(i)).toInt() / 1000000.0));
		QString name(geoSettings->value(QString("gpsFix%1_name").arg(i)).toString());
		if (name.isEmpty())
			name = "Auto-created dive";
		data.addQueryItem("dive_name", name);
		status(data.toString(QUrl::FullyEncoded).toUtf8());
		QNetworkRequest request;
		request.setUrl(url);
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
		status(QString("completed sending gps fix %1 - response: ").arg(i) + reply->readAll());
		geoSettings->setValue(QString("gpsFix%1_uploaded").arg(i), 1);
	}
}
