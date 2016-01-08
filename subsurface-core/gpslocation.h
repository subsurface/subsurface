#ifndef GPSLOCATION_H
#define GPSLOCATION_H

#include "units.h"
#include <QObject>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QSettings>
#include <QNetworkReply>

struct gpsTracker {
	degrees_t latitude;
	degrees_t longitude;
	time_t when;
	QString name;
};

class GpsLocation : QObject
{
	Q_OBJECT
public:
	GpsLocation(void (*showMsgCB)(const char *msg), QObject *parent);
	~GpsLocation();
	static GpsLocation *instance();
	void applyLocations();
	int getGpsNum() const;
	QString getUserid(QString user, QString passwd);
	bool hasLocationsSource();
	QString currentPosition();

private:
	QGeoPositionInfo lastPos;
	QGeoPositionInfoSource *getGpsSource();
	QGeoPositionInfoSource *m_GpsSource;
	void status(QString msg);
	QSettings *geoSettings;
	QNetworkReply *reply;
	QString userAgent;
	void (*showMessageCB)(const char *msg);
	static GpsLocation *m_Instance;
	bool waitingForPosition;

public slots:
	void serviceEnable(bool toggle);
	void newPosition(QGeoPositionInfo pos);
	void updateTimeout();
	void uploadToServer();
	void downloadFromServer();
	void postError(QNetworkReply::NetworkError error);
	void getUseridError(QNetworkReply::NetworkError error);
	void updateModel();
	void clearGpsData();

};

#endif // GPSLOCATION_H
