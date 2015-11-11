#include "qt-mobile/gpslocation.h"
#include "qt-mobile/qmlmanager.h"
#include "pref.h"
#include "dive.h"
#include <time.h>
#include <QDebug>
#include <QVariant>

GpsLocation::GpsLocation(QObject *parent)
{
	QSettings *geoSettings = new QSettings();
	gpsSource = QGeoPositionInfoSource::createDefaultSource(parent);
	if (gpsSource != 0) {
		QString msg = QString("have position source %1").arg(gpsSource->sourceName());
		connect(gpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(newPosition(QGeoPositionInfo)));
		connect(gpsSource, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
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

#define MINTIME 600
#define MINDIST 200

void GpsLocation::newPosition(QGeoPositionInfo pos)
{
	time_t lastTime;
	QGeoCoordinate lastCoord;
	QString msg("received new position %1");
	status(qPrintable(msg.arg(pos.coordinate().toString())));
	geoSettings.beginGroup("locations");
	int nr = geoSettings.value("count", 0).toInt();
	if (nr) {
		lastCoord.setLatitude(geoSettings.value(QString("gpsFix%1_lat").arg(nr)).toInt() / 1000000.0);
		lastCoord.setLongitude(geoSettings.value(QString("gpsFix%1_lon").arg(nr)).toInt() / 1000000.0);
		time_t lastTime = geoSettings.value(QString("gpsFix%1_time").arg(nr)).toULongLong();
	}
	// if we have no record stored or if at least 10 minutes have passed or we moved at least 200m
	if (!nr || pos.timestamp().toTime_t() > lastTime + MINTIME || lastCoord.distanceTo(pos.coordinate()) > MINDIST) {
		geoSettings.setValue("count", nr + 1);
		geoSettings.setValue(QString("gpsFix%1_time").arg(nr), pos.timestamp().toTime_t());
		geoSettings.setValue(QString("gpsFix%1_lat").arg(nr), rint(pos.coordinate().latitude() * 1000000));
		geoSettings.setValue(QString("gpsFix%1_lon").arg(nr), rint(pos.coordinate().longitude() * 1000000));
		geoSettings.sync();
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
