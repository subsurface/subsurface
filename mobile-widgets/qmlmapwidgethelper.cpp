// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QClipboard>
#include <QGeoCoordinate>
#include <QDebug>
#include <QVector>

#include "qmlmapwidgethelper.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/helpers.h"
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
	int idx;
	struct dive *dive;
	QVector<struct dive_site *> selDS;
	QVector<QGeoCoordinate> selGC;
	QGeoCoordinate dsCoord;

	for_each_dive (idx, dive) {
		struct dive_site *dss = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(dss) || !dive->selected)
			continue;
		// only store dive sites with GPS
		selDS.append(dss);
		selGC.append(QGeoCoordinate(dss->latitude.udeg * 0.000001,
		                            dss->longitude.udeg * 0.000001));
	}
	if (!dive_site_has_gps_location(ds) && !selDS.size()) {
		// only a single dive site with no GPS selected
		m_mapLocationModel->setSelectedUuid(ds ? ds->uuid : 0, false);
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");

	} else if (selDS.size() == 1) {
		// a single dive site with GPS selected
		ds = selDS.at(0);
		m_mapLocationModel->setSelectedUuid(ds->uuid, false);
		dsCoord.setLatitude(ds->latitude.udeg * 0.000001);
		dsCoord.setLongitude(ds->longitude.udeg * 0.000001);
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(dsCoord)));
	} else if (selDS.size() > 1) {
		/* more than one dive sites with GPS selected.
		 * find the most top-left and bottom-right dive sites on the map coordinate system. */
		ds = selDS.at(0);
		m_mapLocationModel->setSelectedUuid(ds->uuid, false);
		qreal minLat = 0.0, minLon = 0.0, maxLat = 0.0, maxLon = 0.0;
		bool start = true;
		foreach(QGeoCoordinate gc, selGC) {
			qreal lat = gc.latitude();
			qreal lon = gc.longitude();
			if (start) {
				minLat = maxLat = lat;
				minLon = maxLon = lon;
				start = false;
				continue;
			}
			if (lat < minLat)
				minLat = lat;
			else if (lat > maxLat)
				maxLat = lat;
			if (lon < minLon)
				minLon = lon;
			else if (lon > maxLon)
				maxLon = lon;
		}
		// pass rectangle coordinates to QML
		QGeoCoordinate coordTopLeft(minLat, minLon);
		QGeoCoordinate coordBottomRight(maxLat, maxLon);
		QGeoCoordinate coordCenter(minLat + (maxLat - minLat) * 0.5, minLon + (maxLon - minLon) * 0.5);
		QMetaObject::invokeMethod(m_map, "centerOnRectangle",
		                          Q_ARG(QVariant, QVariant::fromValue(coordTopLeft)),
		                          Q_ARG(QVariant, QVariant::fromValue(coordBottomRight)),
		                          Q_ARG(QVariant, QVariant::fromValue(coordCenter)));
	}
}

void MapWidgetHelper::reloadMapLocations()
{
	struct dive_site *ds;
	int idx;
	QMap<QString, MapLocation *> locationNameMap;
	m_mapLocationModel->clear();
	MapLocation *location;
	QVector<MapLocation *> locationList;
	qreal latitude, longitude;

	if (displayed_dive_site.uuid && dive_site_has_gps_location(&displayed_dive_site)) {
		latitude = displayed_dive_site.latitude.udeg * 0.000001;
		longitude = displayed_dive_site.longitude.udeg * 0.000001;
		location = new MapLocation(displayed_dive_site.uuid, QGeoCoordinate(latitude, longitude),
		                           QString(displayed_dive_site.name));
		locationList.append(location);
		locationNameMap[QString(displayed_dive_site.name)] = location;
	}
	for_each_dive_site(idx, ds) {
		if (!dive_site_has_gps_location(ds) || ds->uuid == displayed_dive_site.uuid)
			continue;
		latitude = ds->latitude.udeg * 0.000001;
		longitude = ds->longitude.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QString name(ds->name);
		// don't add dive locations with the same name, unless they are
		// at least MIN_DISTANCE_BETWEEN_DIVE_SITES_M apart
		if (locationNameMap[name]) {
			MapLocation *existingLocation = locationNameMap[name];
			QGeoCoordinate coord = qvariant_cast<QGeoCoordinate>(existingLocation->getRole(MapLocation::Roles::RoleCoordinate));
			if (dsCoord.distanceTo(coord) < MIN_DISTANCE_BETWEEN_DIVE_SITES_M)
				continue;
		}
		location = new MapLocation(ds->uuid, dsCoord, name);
		locationList.append(location);
		locationNameMap[name] = location;
	}
	m_mapLocationModel->addList(locationList);
}

void MapWidgetHelper::selectedLocationChanged(MapLocation *location)
{
	int idx;
	struct dive *dive;
	m_selectedDiveIds.clear();
	QGeoCoordinate locationCoord = location->coordinate();
	for_each_dive (idx, dive) {
		struct dive_site *ds = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(ds))
			continue;
		const qreal latitude = ds->latitude.udeg * 0.000001;
		const qreal longitude = ds->longitude.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		if (locationCoord.distanceTo(dsCoord) < m_smallCircleRadius)
			m_selectedDiveIds.append(idx);
	}
	emit selectedDivesChanged(m_selectedDiveIds);
}

void MapWidgetHelper::selectVisibleLocations()
{
	int idx;
	struct dive *dive;
	bool selectedFirst = false;
	m_selectedDiveIds.clear();
	for_each_dive (idx, dive) {
		struct dive_site *ds = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(ds))
			continue;
		const qreal latitude = ds->latitude.udeg * 0.000001;
		const qreal longitude = ds->longitude.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QPointF point;
		QMetaObject::invokeMethod(m_map, "fromCoordinate", Q_RETURN_ARG(QPointF, point),
		                          Q_ARG(QGeoCoordinate, dsCoord));
		if (!qIsNaN(point.x())) {
			if (!selectedFirst) {
				m_mapLocationModel->setSelectedUuid(ds->uuid, false);
				selectedFirst = true;
			}
			m_selectedDiveIds.append(idx);
		}
	}
	emit selectedDivesChanged(m_selectedDiveIds);
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

void MapWidgetHelper::updateCurrentDiveSiteCoordinates(quint32 uuid, QGeoCoordinate coord)
{
	MapLocation *loc = m_mapLocationModel->getMapLocationForUuid(uuid);
	if (loc)
		loc->setCoordinate(coord);
	displayed_dive_site.latitude.udeg = llrint(coord.latitude() * 1000000.0);
	displayed_dive_site.longitude.udeg = llrint(coord.longitude() * 1000000.0);
	emit coordinatesChanged();
}

bool MapWidgetHelper::editMode()
{
	return m_editMode;
}

void MapWidgetHelper::setEditMode(bool editMode)
{
	m_editMode = editMode;
	MapLocation *exists = m_mapLocationModel->getMapLocationForUuid(displayed_dive_site.uuid);
	// if divesite uuid doesn't exist in the model, add a new MapLocation.
	if (editMode && !exists) {
		QGeoCoordinate coord(0.0, 0.0);
		m_mapLocationModel->add(new MapLocation(displayed_dive_site.uuid, coord,
		                                        QString(displayed_dive_site.name)));
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate",
		                          Q_ARG(QVariant, QVariant::fromValue(coord)));
	}
	emit editModeChanged();
}

QString MapWidgetHelper::pluginObject()
{
	QString str;
	str += "import QtQuick 2.0;";
	str += "import QtLocation 5.3;";
	str += "Plugin {";
	str += "    id: mapPlugin;";
	str += "    name: 'googlemaps';";
	str += "    PluginParameter { name: 'googlemaps.maps.language'; value: '%lang%' }";
	str += "    Component.onCompleted: {";
	str += "        if (availableServiceProviders.indexOf(name) === -1) {";
	str += "            console.warn('MapWidget.qml: cannot find a plugin named: ' + name);";
	str += "        }";
	str += "    }";
	str += "}";
	QString lang = uiLanguage(NULL).replace('_', '-');
	str.replace("%lang%", lang);
	return str;
}
