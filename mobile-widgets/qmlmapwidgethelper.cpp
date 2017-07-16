#include <QDebug>
#include "qmlmapwidgethelper.h"

#include "core/dive.h"
#include "core/divesite.h"

MapWidgetHelper::MapWidgetHelper(QObject *parent) : QObject(parent)
{
}

void MapWidgetHelper::centerOnDiveSite(struct dive_site *ds)
{
	if (!dive_site_has_gps_location(ds))
		return;

	qreal longitude = ds->longitude.udeg / 1000000.0;
	qreal latitude = ds->latitude.udeg / 1000000.0;
}
