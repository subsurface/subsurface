#include "qt-mobile/gpslocation.h"
#include "qt-mobile/qmlmanager.h"
#include <QDebug>


GpsLocation::GpsLocation(QObject *parent)
{
	QGeoPositionInfoSource *gpsSource = QGeoPositionInfoSource::createDefaultSource(parent);
	if (gpsSource != 0) {
		QString msg = QString("have position source %1").arg(gpsSource->sourceName());
		connect(gpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(newPosition(QGeoPositionInfo)));
		connect(gpsSource, SIGNAL(updateTimeout()), this, SLOT(updateTimeout()));
		lastPos = gpsSource->lastKnownPosition();
		gpsSource->startUpdates();
		QGeoCoordinate lastCoord = lastPos.coordinate();
		if (lastCoord.isValid()) {
			status(msg + lastCoord.toString());
		} else {
			status(msg + "invalid last position");
		}
	} else {
		status("don't have GPS source");
	}
}
void GpsLocation::newPosition(QGeoPositionInfo pos)
{
	QString msg("received new position %1");
	status(qPrintable(msg.arg(pos.coordinate().toString())));
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
