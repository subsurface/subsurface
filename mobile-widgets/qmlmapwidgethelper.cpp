// SPDX-License-Identifier: GPL-2.0
#include <QDebug>

#include "qmlmapwidgethelper.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "qt-models/maplocationmodel.h"

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
	m_mapLocationModel = new MapLocationModel(this);
}

void MapWidgetHelper::centerOnDiveSite(struct dive_site *ds)
{
	if (!dive_site_has_gps_location(ds))
		return;

	qreal longitude = ds->longitude.udeg / 1000000.0;
	qreal latitude = ds->latitude.udeg / 1000000.0;

	QMetaObject::invokeMethod(m_map, "centerOnCoordinates",
	                          Q_ARG(QVariant, latitude),
	                          Q_ARG(QVariant, longitude));
}

void MapWidgetHelper::reloadMapLocations()
{
	struct dive_site *ds;
	int idx;
	m_mapLocationModel->clear();

	for_each_dive_site(idx, ds) {
		if (!dive_site_has_gps_location(ds))
			continue;
		const qreal longitude = ds->longitude.udeg / 1000000.0;
		const qreal latitude = ds->latitude.udeg / 1000000.0;
		m_mapLocationModel->add(new MapLocation(QGeoCoordinate(latitude, longitude)));
	}
}
