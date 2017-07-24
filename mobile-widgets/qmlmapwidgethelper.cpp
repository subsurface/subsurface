// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QClipboard>
#include <QGeoCoordinate>
#include <QDebug>

#include "qmlmapwidgethelper.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "qt-models/maplocationmodel.h"

#define MIN_DISTANCE_BETWEEN_DIVE_SITES_M 50.0
#define SMALL_CIRCLE_RADIUS_PX            26.0

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
	m_mapLocationModel = new MapLocationModel(this);
	connect(m_mapLocationModel, SIGNAL(selectedLocationChanged(MapLocation *)),
	        this, SLOT(selectedLocationChanged(MapLocation *)));
}

void MapWidgetHelper::centerOnDiveSite(struct dive_site *ds)
{
	if (!dive_site_has_gps_location(ds)) {
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
		return;
	}
	MapLocation *location = m_mapLocationModel->getMapLocationForUuid(ds->uuid);
	QMetaObject::invokeMethod(m_map, "centerOnMapLocation", Q_ARG(QVariant, QVariant::fromValue(location)));
}

void MapWidgetHelper::reloadMapLocations()
{
	struct dive_site *ds;
	int idx;
	m_mapLocationModel->clear();
	QVector<MapLocation *> locationList;

	for_each_dive_site(idx, ds) {
		if (!dive_site_has_gps_location(ds))
			continue;
		const qreal latitude = ds->latitude.udeg * 0.000001;
		const qreal longitude = ds->longitude.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		// check if there are no locations too close to the current dive site
		bool diveSiteTooClose = false;
		foreach(MapLocation *location, locationList) {
			QGeoCoordinate coord = qvariant_cast<QGeoCoordinate>(location->getRole(MapLocation::Roles::RoleCoordinate));
			if (dsCoord.distanceTo(coord) < MIN_DISTANCE_BETWEEN_DIVE_SITES_M) {
				diveSiteTooClose = true;
				break;
			}
		}
		if (!diveSiteTooClose)
			locationList.append(new MapLocation(ds->uuid, QGeoCoordinate(latitude, longitude)));
	}
	m_mapLocationModel->addList(locationList);
}

void MapWidgetHelper::selectedLocationChanged(MapLocation *location)
{
	qDebug() << location;
}

/*
 * Based on a 2D Map widget circle with center "coord" and radius SMALL_CIRCLE_RADIUS_PX,
 * obtain a "small circle" with radius m_smallCircleRadius in meters:
 *     https://en.wikipedia.org/wiki/Circle_of_a_sphere
 *
 * The idea behind this circle is to be able to select multiple nearby dives, when clicking on
 * the map. This code can be in QML, but it is in C++ instead for performance reasons.
 *
 * This can be made faster with an exponential regression [a * exp(b * x)], with a pretty
 * decent R-squared, but it becomes bound to map provider zoom level mappings and the
 * SMALL_CIRCLE_RADIUS_PX value, which makes the code hard to maintain.
 */
void MapWidgetHelper::calculateSmallCircleRadius(QGeoCoordinate coord)
{
	QPointF point;
	QMetaObject::invokeMethod(m_map, "fromCoordinate", Q_RETURN_ARG(QPointF, point),
	                          Q_ARG(QGeoCoordinate, coord), Q_ARG(bool, false));
	QPointF point2(point.x() + SMALL_CIRCLE_RADIUS_PX, point.y());
	QGeoCoordinate coord2;
	QMetaObject::invokeMethod(m_map, "toCoordinate", Q_RETURN_ARG(QGeoCoordinate, coord2),
	                          Q_ARG(QPointF, point2), Q_ARG(bool, false));
	m_smallCircleRadius = coord2.distanceTo(coord);
}

void MapWidgetHelper::copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional)
{
	bool savep = prefs.coordinates_traditional;
	prefs.coordinates_traditional = formatTraditional;

	const int lat = llrint(1000000.0 * coord.latitude());
	const int lon = llrint(1000000.0 * coord.longitude());
	const char *coordinates = printGPSCoords(lat, lon);
	QApplication::clipboard()->setText(QString(coordinates), QClipboard::Clipboard);

	free((void *)coordinates);
	prefs.coordinates_traditional = savep;
}
