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
