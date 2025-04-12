// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QVector>

#include "qmlmapwidgethelper.h"
#include "core/divefilter.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "qt-models/maplocationmodel.h"
#include "qt-models/divelocationmodel.h"
#ifndef SUBSURFACE_MOBILE
#include "desktop-widgets/mapwidget.h"
#endif

#define SMALL_CIRCLE_RADIUS_PX            26.0

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
	m_mapLocationModel = new MapLocationModel(this);
	m_smallCircleRadius = SMALL_CIRCLE_RADIUS_PX;
	m_map = nullptr;
	m_editMode = false;
	connect(&diveListNotifier, &DiveListNotifier::diveSiteChanged, this, &MapWidgetHelper::diveSiteChanged);
}

QGeoCoordinate MapWidgetHelper::getCoordinates(struct dive_site *ds)
{
	if (!ds || !ds->has_gps_location())
		return QGeoCoordinate(0.0, 0.0);
	return QGeoCoordinate(ds->location.lat.udeg * 0.000001, ds->location.lon.udeg * 0.000001);
}

void MapWidgetHelper::centerOnDiveSite(struct dive_site *ds)
{
	if (!ds || !ds->has_gps_location()) {
		// dive site with no GPS
		m_mapLocationModel->setSelected(ds);
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
	} else {
		// dive site with GPS
		m_mapLocationModel->setSelected(ds);
		QGeoCoordinate dsCoord (ds->location.lat.udeg * 0.000001, ds->location.lon.udeg * 0.000001);
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(dsCoord)));
	}
}

void MapWidgetHelper::setSelected(const std::vector<dive_site *> divesites)
{
	m_mapLocationModel->setSelected(std::move(divesites));
	m_mapLocationModel->selectionChanged();
	updateEditMode();
}

void MapWidgetHelper::centerOnSelectedDiveSite()
{
	std::vector<struct dive_site *> selDS = m_mapLocationModel->selectedDs();

	if (selDS.empty()) {
		// no selected dives with GPS coordinates
		QMetaObject::invokeMethod(m_map, "deselectMapLocation");
		return;
	}

	// find the most top-left and bottom-right dive sites on the map coordinate system.
	qreal minLat = 0.0, minLon = 0.0, maxLat = 0.0, maxLon = 0.0;
	int count = 0;
	for(struct dive_site *dss: selDS) {
		if (!has_location(&dss->location))
			continue;
		qreal lat = dss->location.lat.udeg * 0.000001;
		qreal lon = dss->location.lon.udeg * 0.000001;
		if (++count == 1) {
			minLat = maxLat = lat;
			minLon = maxLon = lon;
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

	// Pass coordinates to QML, either as a point or as a rectangle.
	// If we didn't find any coordinates, do nothing.
	if (count == 1) {
		QGeoCoordinate dsCoord (selDS[0]->location.lat.udeg * 0.000001, selDS[0]->location.lon.udeg * 0.000001);
		QMetaObject::invokeMethod(m_map, "centerOnCoordinate", Q_ARG(QVariant, QVariant::fromValue(dsCoord)));
	} else if (count > 1) {
		QGeoCoordinate coordTopLeft(minLat, minLon);
		QGeoCoordinate coordBottomRight(maxLat, maxLon);
		QGeoCoordinate coordCenter(minLat + (maxLat - minLat) * 0.5, minLon + (maxLon - minLon) * 0.5);
		QMetaObject::invokeMethod(m_map, "centerOnRectangle",
					  Q_ARG(QVariant, QVariant::fromValue(coordTopLeft)),
					  Q_ARG(QVariant, QVariant::fromValue(coordBottomRight)),
					  Q_ARG(QVariant, QVariant::fromValue(coordCenter)));
	}
}

void MapWidgetHelper::updateEditMode()
{
#ifndef SUBSURFACE_MOBILE
	// The filter being set to dive site is the signal that we are in dive site edit mode.
	// This is the case when either the dive site edit tab or the dive site list tab are active.
	bool old = m_editMode;
	m_editMode = DiveFilter::instance()->diveSiteMode();
	if (old != m_editMode)
		emit editModeChanged();
#endif
}

void MapWidgetHelper::reloadMapLocations()
{
	updateEditMode();
	m_mapLocationModel->reload(m_map);
}

void MapWidgetHelper::selectedLocationChanged(struct dive_site *ds_in)
{
	QList<int> selectedDiveIds;

	if (!ds_in)
		return;
	const MapLocation *location = m_mapLocationModel->getMapLocation(ds_in);
	if (!location)
		return;
	QGeoCoordinate locationCoord = location->coordinate;

	for (auto [idx, dive]: enumerated_range(divelog.dives)) {
		struct dive_site *ds = dive->dive_site;
		if (!ds || !ds->has_gps_location())
			continue;
#ifndef SUBSURFACE_MOBILE
		const qreal latitude = ds->location.lat.udeg * 0.000001;
		const qreal longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		if (locationCoord.distanceTo(dsCoord) < m_smallCircleRadius)
			selectedDiveIds.append(idx);
	}
#else // the mobile version doesn't support multi-dive selection
		if (ds == location->divesite)
			selectedDiveIds.append(dive->id); // use id here instead of index
	}
	int last; // get latest dive chronologically
	if (!selectedDiveIds.isEmpty()) {
		 last = selectedDiveIds.last();
		 selectedDiveIds.clear();
		 selectedDiveIds.append(last);
	}
#endif
	emit selectedDivesChanged(selectedDiveIds);
}

void MapWidgetHelper::selectVisibleLocations()
{
	QList<int> selectedDiveIds;
	for (auto [idx, dive]: enumerated_range(divelog.dives)) {
		struct dive_site *ds = dive->dive_site;
		if (!ds || ds->has_gps_location())
			continue;
		const qreal latitude = ds->location.lat.udeg * 0.000001;
		const qreal longitude = ds->location.lon.udeg * 0.000001;
		QGeoCoordinate dsCoord(latitude, longitude);
		QPointF point;
		QMetaObject::invokeMethod(m_map, "fromCoordinate", Q_RETURN_ARG(QPointF, point),
		                          Q_ARG(QGeoCoordinate, dsCoord));
		if (!qIsNaN(point.x()))
#ifndef SUBSURFACE_MOBILE // indices on desktop
			selectedDiveIds.append(idx);
	}
#else // use id on mobile instead of index
			selectedDiveIds.append(dive->id);
	}
	int last; // get latest dive chronologically
	if (!selectedDiveIds.isEmpty()) {
		 last = selectedDiveIds.last();
		 selectedDiveIds.clear();
		 selectedDiveIds.append(last);
	}
#endif
	emit selectedDivesChanged(selectedDiveIds);
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
	QApplication::clipboard()->setText(printGPSCoords(&location), QClipboard::Clipboard);

	prefs.coordinates_traditional = savep;
}

void MapWidgetHelper::updateCurrentDiveSiteCoordinatesFromMap(struct dive_site *ds, QGeoCoordinate coord)
{
	MapLocation *loc = m_mapLocationModel->getMapLocation(ds);
	if (loc)
		loc->coordinate = coord;
	location_t location = mk_location(coord);
	emit coordinatesChanged(ds, location);
}

void MapWidgetHelper::diveSiteChanged(struct dive_site *ds, int field)
{
	centerOnDiveSite(ds);
}

bool MapWidgetHelper::editMode() const
{
	return m_editMode;
}

QString MapWidgetHelper::pluginObject()
{
	QString lang = getUiLanguage().replace('_', '-');
	QString cacheFolder = QString::fromStdString(system_default_directory() + "/googlemaps").replace("\\", "/");
	return QStringLiteral("import QtQuick 2.0;"
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
			      "import QtLocation 6.0;"
#else
			      "import QtLocation 5.3;"
#endif
			      "Plugin {"
			      "    id: mapPlugin;"
			      "    name: 'googlemaps';"
			      "    PluginParameter { name: 'googlemaps.maps.language'; value: '%1' }"
			      "    PluginParameter { name: 'googlemaps.cachefolder'; value: '%2' }"
			      "    Component.onCompleted: {"
			      "        if (availableServiceProviders.indexOf(name) === -1) {"
			      "            console.warn('MapWidget.qml: cannot find a plugin named: ' + name);"
			      "        }"
			      "    }"
			      "}").arg(lang, cacheFolder);
}
