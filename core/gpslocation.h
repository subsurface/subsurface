// SPDX-License-Identifier: GPL-2.0
#ifndef GPSLOCATION_H
#define GPSLOCATION_H

#include "units.h"
#include <QObject>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QSettings>
#include <QNetworkReply>
#include <QMap>

#define GPS_CURRENT_POS QObject::tr("Waiting to aquire GPS location")

struct gpsTracker {
	degrees_t latitude;
	degrees_t longitude;
	qint64 when;
	QString name;
	int idx;
};

class GpsLocation : public QObject {
	Q_OBJECT
public:
	GpsLocation(void (*showMsgCB)(const char *msg), QObject *parent);
	~GpsLocation();
	static GpsLocation *instance();
	static bool hasInstance();
	bool applyLocations();
	int getGpsNum() const;
	QString getUserid(QString user, QString passwd);
	bool hasLocationsSource();
	QString currentPosition();
	void setGpsTimeThreshold(int seconds);

	QMap<qint64, gpsTracker> currentGPSInfo() const;

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
	QMap<qint64, gpsTracker> m_trackers;
	QList<gpsTracker> m_deletedTrackers;
	void loadFromStorage();
	void addFixToStorage(gpsTracker &gt);
	void replaceFixToStorage(gpsTracker &gt);
	void deleteFixFromStorage(gpsTracker &gt);
	void deleteFixesFromServer();
	enum { UNKNOWN, NOGPS, HAVEGPS } haveSource;

signals:
	void haveSourceChanged();

public slots:
	void serviceEnable(bool toggle);
	void newPosition(QGeoPositionInfo pos);
	void updateTimeout();
	void positionSourceError(QGeoPositionInfoSource::Error error);
	void uploadToServer();
	void downloadFromServer();
	void postError(QNetworkReply::NetworkError error);
	void getUseridError(QNetworkReply::NetworkError error);
	void clearGpsData();
	void deleteGpsFix(qint64 when);
};

#endif // GPSLOCATION_H
