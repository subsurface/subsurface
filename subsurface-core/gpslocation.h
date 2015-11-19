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
	GpsLocation(void (*showMsgCB)(const char *msg), QObject *parent);
	void applyLocations();
	int getGpsNum() const;
	QString getUserid(QString user, QString passwd);
	bool hasLocationsSource();

private:
	QGeoPositionInfo lastPos;
	QGeoPositionInfoSource *gpsSource;
	void status(QString msg);
	QSettings *geoSettings;
	QNetworkReply *reply;
	QString userAgent;
	void (*showMessageCB)(const char *msg);

signals:

public slots:
	void serviceEnable(bool toggle);
	void newPosition(QGeoPositionInfo pos);
	void updateTimeout();
	void uploadToServer();
	void postError(QNetworkReply::NetworkError error);
	void getUseridError(QNetworkReply::NetworkError error);
	void clearGpsData();

};

#endif // GPSLOCATION_H
