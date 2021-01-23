// SPDX-License-Identifier: GPL-2.0
#include "core/gpslocation.h"
#include "core/divesite.h"
#include "core/dive.h"
#include "qt-models/gpslistmodel.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/errorhelper.h"
#include "core/settings/qPrefLocationService.h"
#include <time.h>
#include <unistd.h>
#include <QDebug>
#include <QVariant>
#include <QUrlQuery>
#include <QApplication>
#include <QTimer>

GpsLocation::GpsLocation() :
	m_GpsSource(0),
	showMessageCB(0),
	waitingForPosition(false),
	haveSource(UNKNOWN)
{
	// create a QSettings object that's separate from the main application settings
	geoSettings = new QSettings(QSettings::NativeFormat, QSettings::UserScope,
				    QStringLiteral("org.subsurfacedivelog"), QStringLiteral("subsurfacelocation"), this);
	userAgent = getUserAgent();
	(void)getGpsSource();
	loadFromStorage();

	// register changes in time threshold
	connect(qPrefLocationService::instance(), SIGNAL(time_thresholdChanged(int)), this, SLOT(setGpsTimeThreshold(int)));
}

GpsLocation *GpsLocation::instance()
{
	static GpsLocation self;
	return &self;
}

GpsLocation::~GpsLocation()
{
}

void GpsLocation::setLogCallBack(void (*showMsgCB)(const char *))
{
	showMessageCB = showMsgCB;
}

void GpsLocation::setGpsTimeThreshold(int seconds)
{
	if (m_GpsSource) {
		m_GpsSource->setUpdateInterval(seconds * 1000);
		status(QStringLiteral("Set GPS service update interval to %1 s").arg(m_GpsSource->updateInterval() / 1000));
	}
}

QGeoPositionInfoSource *GpsLocation::getGpsSource()
{
	if (haveSource == NOGPS)
		return 0;

	if (!m_GpsSource) {
		m_GpsSource = QGeoPositionInfoSource::createDefaultSource(this);
		if (m_GpsSource != 0) {
#if defined(Q_OS_IOS)
			// at least Qt 5.6.0 isn't doing things right on iOS - it MUST check for permission before
			// accessing the position source
			// I have a hacked version of Qt 5.6.0 that I will build the iOS binaries with for now;
			// this test below righ after creating the source checks if the location service is disabled
			// and set's an error right when the position source is created to indicate this
			if (m_GpsSource->error() == QGeoPositionInfoSource::AccessError) {
				haveSource = NOGPS;
				emit haveSourceChanged();
				m_GpsSource = NULL;
				return NULL;
			}
#endif
			haveSource = (m_GpsSource->supportedPositioningMethods() & QGeoPositionInfoSource::SatellitePositioningMethods) ? HAVEGPS : NOGPS;
			emit haveSourceChanged();
#ifndef SUBSURFACE_MOBILE
			if (verbose)
#endif
				status(QStringLiteral("Created position source %1").arg(m_GpsSource->sourceName()));
			connect(m_GpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(newPosition(QGeoPositionInfo)));
			connect(m_GpsSource, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
			connect(m_GpsSource, SIGNAL(error(QGeoPositionInfoSource::Error)), this, SLOT(positionSourceError(QGeoPositionInfoSource::Error)));
			setGpsTimeThreshold(prefs.time_threshold);
		} else {
#ifdef SUBSURFACE_MOBILE
			status("don't have GPS source");
#endif
			haveSource = NOGPS;
			emit haveSourceChanged();
		}
	}
	return m_GpsSource;
}

bool GpsLocation::hasLocationsSource()
{
	(void)getGpsSource();
	return haveSource == HAVEGPS;
}

void GpsLocation::serviceEnable(bool toggle)
{
	QGeoPositionInfoSource *gpsSource = getGpsSource();
	if (haveSource != HAVEGPS) {
		if (toggle)
			status("Can't start location service, no location source available");
		return;
	}
	if (toggle) {
		gpsSource->startUpdates();
		status(QStringLiteral("Starting Subsurface GPS service with update interval %1").arg(gpsSource->updateInterval()));
	} else {
		gpsSource->stopUpdates();
		status("Stopping Subsurface GPS service");
	}
}

QString GpsLocation::currentPosition()
{
	qDebug() << "current position requested";
	if (!hasLocationsSource())
		return tr("Unknown GPS location (no GPS source)");
	if (m_trackers.count()) {
		QDateTime lastFixTime =	timestampToDateTime(m_trackers.lastKey() - gettimezoneoffset());
		QDateTime now = QDateTime::currentDateTime();
		int delta = lastFixTime.secsTo(now);
		qDebug() << "lastFixTime" << lastFixTime.toString() << "now" << now.toString() << "delta" << delta;
		if (delta < 300) {
			// we can simply use the last position that we tracked
			gpsTracker gt = m_trackers.last();
			QString gpsString = printGPSCoords(&gt.location);
			qDebug() << "returning last position" << gpsString;
			return gpsString;
		} else {
			qDebug() << "last position saved was too old" << lastFixTime.toString();
		}
	}
	qDebug() << "requesting new GPS position";
	m_GpsSource->requestUpdate();
	// ok, we need to get the current position and somehow in the callback update the location in the QML UI
	// punting right now
	waitingForPosition = true;
	return GPS_CURRENT_POS;
}

void GpsLocation::newPosition(QGeoPositionInfo pos)
{
	int64_t lastTime = 0;
	int64_t thisTime = dateTimeToTimestamp(pos.timestamp()) + gettimezoneoffset();
	QGeoCoordinate lastCoord;
	int nr = m_trackers.count();
	if (nr) {
		gpsTracker gt = m_trackers.last();
		lastCoord.setLatitude(gt.location.lat.udeg / 1000000.0);
		lastCoord.setLongitude(gt.location.lon.udeg / 1000000.0);
		lastTime = gt.when;
	}
	// if we are waiting for a position update or
	// if we have no record stored or if at least the configured minimum
	// time has passed or we moved at least the configured minimum distance
	int64_t delta = thisTime - lastTime;
	if (!nr || waitingForPosition || delta > prefs.time_threshold ||
	    lastCoord.distanceTo(pos.coordinate()) > prefs.distance_threshold) {
		QString msg = QStringLiteral("received new position %1 after delta %2 threshold %3 (now %4 last %5)");
		status(qPrintable(msg.arg(pos.coordinate().toString()).arg(delta).arg(prefs.time_threshold).arg(pos.timestamp().toString()).arg(timestampToDateTime(lastTime).toString())));
		gpsTracker gt;
		gt.when = thisTime;
		gt.location = create_location(pos.coordinate().latitude(), pos.coordinate().longitude());
		addFixToStorage(gt);
		gpsTracker gtNew = m_trackers.last();
		waitingForPosition = false;
		acquiredPosition();
	}
}

void GpsLocation::updateTimeout()
{
	status("request to get new position timed out");
}

void GpsLocation::positionSourceError(QGeoPositionInfoSource::Error)
{
	status("error receiving a GPS location");
	haveSource = NOGPS;
	emit haveSourceChanged();
}

void GpsLocation::status(QString msg)
{
	if (showMessageCB)
		(*showMessageCB)(qPrintable(msg));
	else
		qDebug() << msg;
}

int GpsLocation::getGpsNum() const
{
	return m_trackers.count();
}

#define SAME_GROUP 6 * 3600 /* six hours */
#define ADD_LOCATION(_dive, _gpsfix, _mark)				\
{									\
	fixes.push_back( { _dive, _gpsfix.location, _gpsfix.name } );	\
	last = _mark;							\
}

std::vector<DiveAndLocation> GpsLocation::getLocations()
{
	int i;
	int last = 0;
	int cnt = m_trackers.count();
	std::vector<DiveAndLocation> fixes;
	if (cnt == 0)
		return fixes;

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
						    get_dive_date_string(dive_endtime(d));
				/*
				 * If position is fixed during dive. This is the good one.
				 * Asign and mark position, and end gps_location loop
				 */
				if (time_during_dive_with_offset(d, gpsTable[j].when, 0)) {
					if (verbose)
						qDebug() << "gpsFix is during the dive, pick that one";
					ADD_LOCATION(d, gpsTable[j], j);
					break;
				} else {
					/*
					 * If it is not, check if there are more position fixes in SAME_GROUP range
					 */
					if (j + 1 < cnt && time_during_dive_with_offset(d, gpsTable[j+1].when, SAME_GROUP)) {
						if (verbose)
							qDebug() << "look at the next gps fix @" << get_dive_date_string(gpsTable[j+1].when);
						/* we know the gps fixes are sorted; if they are both before the dive, ignore the first,
						 * if theay are both after the dive, take the first,
						 * if the first is before and the second is after, take the closer one */
						if (gpsTable[j+1].when < d->when) {
							if (verbose)
								qDebug() << "which is closer to the start of the dive, do continue with that";
							continue;
						} else if (gpsTable[j].when > dive_endtime(d)) {
							if (verbose)
								qDebug() << "which is even later after the end of the dive, so pick the previous one";
							ADD_LOCATION(d, gpsTable[j], j);
							break;
						} else {
							/* ok, gpsFix is before, nextgpsFix is after */
							if (d->when - gpsTable[j].when <= gpsTable[j+1].when - dive_endtime(d)) {
								if (verbose)
									qDebug() << "pick the one before as it's closer to the start";
								ADD_LOCATION(d, gpsTable[j], j);
								break;
							} else {
								if (verbose)
									qDebug() << "pick the one after as it's closer to the start";
								ADD_LOCATION(d, gpsTable[j + 1], j + 1);
								break;
							}
						}
					/*
					 * If no more positions in range, the actual is the one. Asign, mark and end loop.
					 */
					} else {
						if (verbose)
							qDebug() << "which seems to be the best one for this dive, so pick it";
						ADD_LOCATION(d, gpsTable[j], j);
						break;
					}
				}
			} else {
				/* If position is out of SAME_GROUP range and in the future, mark position for
				 * next dive iteration and end the gps_location loop
				 */
				if (gpsTable[j].when >= dive_endtime(d) + SAME_GROUP) {
					last = j;
					break;
				}
			}

		}
	}

	return fixes;
}

QMap<qint64, gpsTracker> GpsLocation::currentGPSInfo() const
{
	return m_trackers;
}

void GpsLocation::loadFromStorage()
{
	int nr = geoSettings->value(QStringLiteral("count")).toInt();
	for (int i = 0; i < nr; i++) {
		struct gpsTracker gt;
		gt.when = geoSettings->value(QStringLiteral("gpsFix%1_time").arg(i)).toLongLong();
		gt.location.lat.udeg = geoSettings->value(QStringLiteral("gpsFix%1_lat").arg(i)).toInt();
		gt.location.lon.udeg = geoSettings->value(QStringLiteral("gpsFix%1_lon").arg(i)).toInt();
		gt.name = geoSettings->value(QStringLiteral("gpsFix%1_name").arg(i)).toString();
		gt.idx = i;
		m_trackers.insert(gt.when, gt);
	}
}

QString GpsLocation::getFixString()
{
	// only used for debugging
	QString res;
	struct gpsTracker gpsEntry;
	foreach (gpsEntry, m_trackers.values())
		res += QString("%1: %2; %3 ; \"%4\"\n").arg(gpsEntry.when).arg(gpsEntry.location.lat.udeg).arg(gpsEntry.location.lon.udeg).arg(gpsEntry.name);
	return res;
}

void GpsLocation::replaceFixToStorage(gpsTracker &gt)
{
	if (!m_trackers.keys().contains(gt.when)) {
		addFixToStorage(gt);
		return;
	}
	gpsTracker replacedTracker = m_trackers.value(gt.when);
	geoSettings->setValue(QStringLiteral("gpsFix%1_time").arg(replacedTracker.idx), gt.when);
	geoSettings->setValue(QStringLiteral("gpsFix%1_lat").arg(replacedTracker.idx), gt.location.lat.udeg);
	geoSettings->setValue(QStringLiteral("gpsFix%1_lon").arg(replacedTracker.idx), gt.location.lon.udeg);
	geoSettings->setValue(QStringLiteral("gpsFix%1_name").arg(replacedTracker.idx), gt.name);
	replacedTracker.location = gt.location;
	replacedTracker.name = gt.name;
}

void GpsLocation::addFixToStorage(gpsTracker &gt)
{
	int nr = m_trackers.count();
	geoSettings->setValue("count", nr + 1);
	geoSettings->setValue(QStringLiteral("gpsFix%1_time").arg(nr), gt.when);
	geoSettings->setValue(QStringLiteral("gpsFix%1_lat").arg(nr), gt.location.lat.udeg);
	geoSettings->setValue(QStringLiteral("gpsFix%1_lon").arg(nr), gt.location.lon.udeg);
	geoSettings->setValue(QStringLiteral("gpsFix%1_name").arg(nr), gt.name);
	gt.idx = nr;
	geoSettings->sync();
	m_trackers.insert(gt.when, gt);
}

void GpsLocation::deleteFixFromStorage(gpsTracker &gt)
{
	qint64 when = gt.when;
	int cnt = m_trackers.count();
	if (cnt == 0 || !m_trackers.keys().contains(when)) {
		qDebug() << "no gps fix with timestamp" << when;
		return;
	}
	if (geoSettings->value(QStringLiteral("gpsFix%1_time").arg(gt.idx)).toLongLong() != when) {
		qDebug() << "uh oh - index for tracker has gotten out of sync...";
		return;
	}
	if (cnt > 1) {
		// now we move the last tracker into that spot (so our settings stay consecutive)
		// and delete the last settings entry
		when = geoSettings->value(QStringLiteral("gpsFix%1_time").arg(cnt - 1)).toLongLong();
		struct gpsTracker movedTracker = m_trackers.value(when);
		movedTracker.idx = gt.idx;
		m_trackers.remove(movedTracker.when);
		m_trackers.insert(movedTracker.when, movedTracker);
		geoSettings->setValue(QStringLiteral("gpsFix%1_time").arg(movedTracker.idx), when);
		geoSettings->setValue(QStringLiteral("gpsFix%1_lat").arg(movedTracker.idx), movedTracker.location.lat.udeg);
		geoSettings->setValue(QStringLiteral("gpsFix%1_lon").arg(movedTracker.idx), movedTracker.location.lon.udeg);
		geoSettings->setValue(QStringLiteral("gpsFix%1_name").arg(movedTracker.idx), movedTracker.name);
		geoSettings->remove(QStringLiteral("gpsFix%1_lat").arg(cnt - 1));
		geoSettings->remove(QStringLiteral("gpsFix%1_lon").arg(cnt - 1));
		geoSettings->remove(QStringLiteral("gpsFix%1_time").arg(cnt - 1));
		geoSettings->remove(QStringLiteral("gpsFix%1_name").arg(cnt - 1));
	}
	geoSettings->setValue("count", cnt - 1);
	geoSettings->sync();
	m_trackers.remove(gt.when);
}

void GpsLocation::deleteGpsFix(qint64 when)
{
	auto it = m_trackers.find(when);
	if (it == m_trackers.end()) {
		qWarning() << "GpsLocation::deleteGpsFix(): can't find tracker for timestamp " << when;
		return;
	}
	struct gpsTracker deletedTracker = *it;
	deleteFixFromStorage(deletedTracker);
	m_deletedTrackers.append(deletedTracker);
}

#ifdef SUBSURFACE_MOBILE
void GpsLocation::clearGpsData()
{
	m_trackers.clear();
	geoSettings->clear();
	geoSettings->sync();
}
#endif
