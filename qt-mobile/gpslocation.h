#ifndef GPSLOCATION_H
#define GPSLOCATION_H

#include "units.h"
#include <QObject>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QSettings>

class GpsLocation : QObject
{
	Q_OBJECT
public:
	GpsLocation(QObject *parent);
	bool applyLocations();
	int getGpsNum() const;

private:
	QGeoPositionInfo lastPos;
	QGeoPositionInfoSource *gpsSource;
	void status(QString msg);
	QSettings *geoSettings;

signals:

public slots:
	void serviceEnable(bool toggle);
	void newPosition(QGeoPositionInfo pos);
	void updateTimeout();
	void clearGpsData();
};

#endif // GPSLOCATION_H
