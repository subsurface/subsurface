#include "qt-mobile/gpslocation.h"
#include <QDebug>


GpsLocation::GpsLocation(QObject *parent)
{
	QGeoPositionInfoSource *gpsSource = QGeoPositionInfoSource::createDefaultSource(parent);
	if (gpsSource != 0) {
		qDebug() << "have position source" << gpsSource->sourceName();
		connect(gpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(newPosition(QGeoPositionInfo)));
		connect(gpsSource, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
		lastPos = gpsSource->lastKnownPosition();
		gpsSource->requestUpdate(1000);
		QGeoCoordinate lastCoord = lastPos.coordinate();
		if (lastCoord.isValid()) {
			qDebug() << lastCoord.toString();
		} else {
			qDebug() << "invalid last position";
		}
	} else {
		qDebug() << "don't have GPS source";
	}
}
void GpsLocation::newPosition(QGeoPositionInfo pos)
{
	qDebug() << "received new position" << pos.coordinate().toString();
}

void GpsLocation::updateTimeout()
{
	qDebug() << "request to get new position timed out";
}
