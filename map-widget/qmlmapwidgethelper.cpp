// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QVector>

#include "qmlmapwidgethelper.h"
#include "core/divesite.h"
#include "core/qthelper.h"
#include "qt-models/maplocationmodel.h"

#define MIN_DISTANCE_BETWEEN_DIVE_SITES_M 50.0
#define SMALL_CIRCLE_RADIUS_PX            26.0

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
	m_mapLocationModel = new MapLocationModel(this);
	m_smallCircleRadius = SMALL_CIRCLE_RADIUS_PX;
	m_map = nullptr;
	m_editMode = false;
	connect(m_mapLocationModel, SIGNAL(selectedLocationChanged(MapLocation *)),
	        this, SLOT(selectedLocationChanged(MapLocation *)));
}

QGeoCoordinate MapWidgetHelper::getCoordinates(struct dive_site *ds)
{
	if (!ds || !dive_site_has_gps_location(ds))
		return QGeoCoordinate(0.0, 0.0);
	return QGeoCoordinate(ds->location.lat.udeg * 0.000001, ds->location.lon.udeg * 0.000001);
}

void MapWidgetHelper::centerOnDiveSite(struct dive_site *ds)
{
	if (!ds || !dive_site_has_gps_location(ds)) {
		// dive site with no GPS
		m_mapLocationModel->setSelected(ds, false);
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
	} else {
		// dive site with GPS
		m_mapLocationModel->setSelected(ds, false);
		QGeoCoordinate dsCoord (ds->location.lat.udeg * 0.000001, ds->location.lon.udeg * 0.000001);
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(dsCoord)));
	}
}

void MapWidgetHelper::centerOnSelectedDiveSite()
{
	QVector<struct dive_site *> selDS;
	QVector<QGeoCoordinate> selGC;

	int idx;
	struct dive *dive;
	for_each_dive (idx, dive) {
		if (!dive->selected)
			continue;
		struct dive_site *dss = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(dss))
			continue;
		// only store dive sites with GPS
		selDS.append(dss);
		selGC.append(QGeoCoordinate(dss->location.lat.udeg * 0.000001,
		                            dss->location.lon.udeg * 0.000001));
	}

	if (selDS.isEmpty()) {
		// no selected dives with GPS coordinates
		m_mapLocationModel->setSelected(nullptr, false);
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
	} else if (selDS.size() == 1) {
		centerOnDiveSite(selDS[0]);
	} else if (selDS.size() > 1) {
		/* more than one dive sites with GPS selected.
		 * find the most top-left and bottom-right dive sites on the map coordinate system. */
		m_mapLocationModel->setSelected(selDS[0], false);
		qreal minLat = 0.0, minLon = 0.0, maxLat = 0.0, maxLon = 0.0;
		bool start = true;
		for(struct dive_site *dss: selDS) {
			qreal lat = dss->location.lat.udeg * 0.000001;
			qreal lon = dss->location.lon.udeg * 0.000001;
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
	int idx;
	struct dive *dive;
	QMap<QString, MapLocation *> locationNameMap;
	m_mapLocationModel->clear();
	MapLocation *location;
	QVector<MapLocation *> locationList;
	QVector<struct dive_site *> locations;
	qreal latitude, longitude;

	for_each_dive(idx, dive) {
		// Don't show dive sites of hidden dives, unless this is the currently
		// displayed (edited) dive.
		if (dive->hidden_by_filter && dive != current_dive)
			continue;
		struct dive_site *ds = get_dive_site_for_dive(dive);
		if (!dive_site_has_gps_location(ds) || locations.contains(ds))
			continue;
		latitude = ds->location.lat.udeg * 0.000001;
		longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QString name(ds->name);
		// don't add dive locations with the same name, unless they are
		// at least MIN_DISTANCE_BETWEEN_DIVE_SITES_M apart
		if (locationNameMap.contains(name)) {
			MapLocation *existingLocation = locationNameMap[name];
			QGeoCoordinate coord = existingLocation->coordinate();
			if (dsCoord.distanceTo(coord) < MIN_DISTANCE_BETWEEN_DIVE_SITES_M)
				continue;
		}
		location = new MapLocation(ds, dsCoord, name);
		locationList.append(location);
		locations.append(ds);
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
#ifndef SUBSURFACE_MOBILE
		const qreal latitude = ds->location.lat.udeg * 0.000001;
		const qreal longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		if (locationCoord.distanceTo(dsCoord) < m_smallCircleRadius)
			m_selectedDiveIds.append(idx);
	}
#else // the mobile version doesn't support multi-dive selection
		if (ds == location->divesite())
			m_selectedDiveIds.append(dive->id); // use id here instead of index
	}
	int last; // get latest dive chronologically
	if (!m_selectedDiveIds.isEmpty()) {
		 last = m_selectedDiveIds.last();
		 m_selectedDiveIds.clear();
		 m_selectedDiveIds.append(last);
	}
#endif
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
		const qreal latitude = ds->location.lat.udeg * 0.000001;
		const qreal longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QPointF point;
		QMetaObject::invokeMethod(m_map, "fromCoordinate", Q_RETURN_ARG(QPointF, point),
		                          Q_ARG(QGeoCoordinate, dsCoord));
		if (!qIsNaN(point.x())) {
			if (!selectedFirst) {
				m_mapLocationModel->setSelected(ds, false);
				selectedFirst = true;
			}
#ifndef SUBSURFACE_MOBILE // indexes on desktop
			m_selectedDiveIds.append(idx);
		}
	}
#else // use id on mobile instead of index
			m_selectedDiveIds.append(dive->id);
		}
	}
	int last; // get latest dive chronologically
	if (!m_selectedDiveIds.isEmpty()) {
		 last = m_selectedDiveIds.last();
		 m_selectedDiveIds.clear();
		 m_selectedDiveIds.append(last);
	}
#endif
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
	                          Q_ARG(QGeoCoordinate, coord));
	QPointF point2(point.x() + SMALL_CIRCLE_RADIUS_PX, point.y());
	QGeoCoordinate coord2;
	QMetaObject::invokeMethod(m_map, "toCoordinate", Q_RETURN_ARG(QGeoCoordinate, coord2),
	                          Q_ARG(QPointF, point2));
	m_smallCircleRadius = coord2.distanceTo(coord);
}

static location_t mk_location(QGeoCoordinate coord)
{
	return create_location(coord.latitude(), coord.longitude());
}

void MapWidgetHelper::copyToClipboardCoordinates(QGeoCoordinate coord, bool formatTraditional)
{
	bool savep = prefs.coordinates_traditional;
	prefs.coordinates_traditional = formatTraditional;
	location_t location = mk_location(coord);
	char *coordinates = printGPSCoords(&location);
	QApplication::clipboard()->setText(QString(coordinates), QClipboard::Clipboard);

	free(coordinates);
	prefs.coordinates_traditional = savep;
}

void MapWidgetHelper::updateCurrentDiveSiteCoordinatesFromMap(struct dive_site *ds, QGeoCoordinate coord)
{
	MapLocation *loc = m_mapLocationModel->getMapLocation(ds);
	if (loc)
		loc->setCoordinate(coord);
	location_t location = mk_location(coord);
	emit coordinatesChanged(location);
}

void MapWidgetHelper::updateDiveSiteCoordinates(struct dive_site *ds, const location_t &location)
{
	if (!ds)
		return;
	const qreal latitude_r = location.lat.udeg * 0.000001;
	const qreal longitude_r = location.lon.udeg * 0.000001;
	QGeoCoordinate coord(latitude_r, longitude_r);
	m_mapLocationModel->updateMapLocationCoordinates(ds, coord);
	QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(coord)));
}

void MapWidgetHelper::exitEditMode()
{
	m_editMode = false;
	emit editModeChanged();
}

void MapWidgetHelper::enterEditMode(struct dive_site *ds)
{
	// We don't support editing of a dive site that doesn't exist
	if (!ds)
		return;

	m_editMode = true;
	MapLocation *exists = m_mapLocationModel->getMapLocation(ds);
	QGeoCoordinate coord;
	// if divesite doesn't exist in the model, add a new MapLocation.
	if (!exists) {
		coord = m_map->property("center").value<QGeoCoordinate>();
		m_mapLocationModel->add(new MapLocation(ds, coord, QString(ds->name)));
	} else {
		coord = exists->coordinate();
	}
	centerOnDiveSite(ds);
	location_t location = mk_location(coord);
	emit coordinatesChanged(location);
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
	str += "    PluginParameter { name: 'googlemaps.cachefolder'; value: '%cacheFolder%' }";
	str += "    Component.onCompleted: {";
	str += "        if (availableServiceProviders.indexOf(name) === -1) {";
	str += "            console.warn('MapWidget.qml: cannot find a plugin named: ' + name);";
	str += "        }";
	str += "    }";
	str += "}";
	QString lang = uiLanguage(NULL).replace('_', '-');
	str.replace("%lang%", lang);
	QString cacheFolder = QString(system_default_directory()).append("/googlemaps");
	str.replace("%cacheFolder%", cacheFolder.replace("\\", "/"));
	return str;
}
