#include "globe.h"
#include "../dive.h"

#include <QDebug>

#include <marble/AbstractFloatItem.h>
#include <marble/GeoDataPlacemark.h>
#include <marble/GeoDataDocument.h>
#include <marble/MarbleModel.h>
#include <marble/GeoDataTreeModel.h>

#include <QMouseEvent>
#include <QMessageBox>

GlobeGPS::GlobeGPS(QWidget* parent) : MarbleWidget(parent), loadedDives(0)
{
	setMapThemeId("earth/bluemarble/bluemarble.dgml");
	setProjection( Marble::Spherical );

	setAnimationsEnabled(true);
	setShowClouds( false );
	setShowBorders( false );
	setShowPlaces( false );
	setShowCrosshairs( false );
	setShowGrid( false );
	setShowOverviewMap(false);
	setShowScaleBar(false);

	Q_FOREACH( AbstractFloatItem * floatItem, floatItems() ){
		if ( floatItem && floatItem->nameId() == "compass" ) {
			floatItem->setPosition( QPoint( 10, 10 ) );
			floatItem->setContentSize( QSize( 50, 50 ) );
		}
	}
}

void GlobeGPS::reload()
{
	if (loadedDives){
		model()->treeModel()->removeDocument(loadedDives);
		delete loadedDives;
	}
	if (editingDiveCoords){
		editingDiveCoords = 0;
	}

	loadedDives = new GeoDataDocument;

	int idx = 0;
	struct dive *dive;
	for_each_dive(idx, dive) {
		if (dive_has_gps_location(dive)) {
			// don't add dive locations twice.
			if( diveLocations.contains( QString(dive->location)))
				continue;

			GeoDataPlacemark *place = new GeoDataPlacemark( dive->location );
			place->setCoordinate(dive->longitude.udeg / 1000000.0,dive->latitude.udeg / 1000000.0 , 0, GeoDataCoordinates::Degree );
			loadedDives->append( place );
		}
	}
	model()->treeModel()->addDocument( loadedDives );
}

void GlobeGPS::centerOn(dive* dive)
{
	qreal longitude = dive->longitude.udeg / 1000000.0;
	qreal latitude = dive->latitude.udeg / 1000000.0;

	if (!longitude || !latitude){
		prepareForGetDiveCoordinates(dive);
		return;
	}

	centerOn(longitude,latitude, true);
}

void GlobeGPS::prepareForGetDiveCoordinates(dive* dive)
{
	QMessageBox::warning(parentWidget(),
						 tr("This dive has no location!"),
						 tr("Move the planet to the desired position, then \n double-click to set the new location of this dive."));

	editingDiveCoords = dive;
}

void GlobeGPS::changeDiveGeoPosition(qreal lon, qreal lat, GeoDataCoordinates::Unit unit)
{
	// convert to degrees if in radian.
	if (unit == GeoDataCoordinates::Radian){
		lon = lon * 180 / M_PI;
		lat = lat * 180 / M_PI;
	}

	if (!editingDiveCoords){
		return;
	}

	editingDiveCoords->latitude.udeg = (int) lat * 1000000.0;
	editingDiveCoords->longitude.udeg = (int) lon * 1000000.0;
	centerOn(lon, lat, true);
	reload();
	editingDiveCoords = 0;
}

void GlobeGPS::mousePressEvent(QMouseEvent* event)
{
	qreal lat, lon;
	if (editingDiveCoords &&  geoCoordinates(event->pos().x(), event->pos().y(), lon,lat, GeoDataCoordinates::Radian)){
		changeDiveGeoPosition(lon, lat, GeoDataCoordinates::Radian);
	}
}

