#ifndef GPSLOCATION_H
#define GPSLOCATION_H

#include "units.h"
#include <QObject>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QSettings>
#include <QNetworkReply>

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
	QNetworkReply *reply;

signals:

public slots:
	void serviceEnable(bool toggle);
	void newPosition(QGeoPositionInfo pos);
	void updateTimeout();
	void uploadToServer();
	void postError(QNetworkReply::NetworkError error);
	void clearGpsData();

};

#endif // GPSLOCATION_H
