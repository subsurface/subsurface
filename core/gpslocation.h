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

#define GPS_CURRENT_POS gettextFromC::tr("Waiting to aquire GPS location")

struct gpsTracker {
	location_t location;
	qint64 when;
	QString name;
	int idx;
};

struct DiveAndLocation {
	struct dive *d;
	location_t location;
	QString name;
};

class GpsLocation : public QObject {
	Q_OBJECT
public:
	GpsLocation(void (*showMsgCB)(const char *msg), QObject *parent);
	~GpsLocation();
	static GpsLocation *instance();
	static bool hasInstance();
	int applyLocations();
	int getGpsNum() const;
	bool hasLocationsSource();
	QString currentPosition();

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
	void acquiredPosition();

public slots:
	void serviceEnable(bool toggle);
	void newPosition(QGeoPositionInfo pos);
	void updateTimeout();
	void positionSourceError(QGeoPositionInfoSource::Error error);
	void postError(QNetworkReply::NetworkError error);
	void setGpsTimeThreshold(int seconds);
#ifdef SUBSURFACE_MOBILE
	void clearGpsData();
#endif
	void deleteGpsFix(qint64 when);
};

#endif // GPSLOCATION_H
