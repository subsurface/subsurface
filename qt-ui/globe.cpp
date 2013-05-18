#include "globe.h"
#include "kmessagewidget.h"
#include "../dive.h"

#include <QDebug>

#include <marble/AbstractFloatItem.h>
#include <marble/GeoDataPlacemark.h>
#include <marble/GeoDataDocument.h>
#include <marble/MarbleModel.h>
#if INCOMPLETE_MARBLE
#include "marble/GeoDataTreeModel.h"
#else
#include <marble/GeoDataTreeModel.h>
#endif
#include <QMouseEvent>
#include <QMessageBox>

GlobeGPS::GlobeGPS(QWidget* parent) : MarbleWidget(parent), loadedDives(0)
{

	setMapThemeId("earth/bluemarble/bluemarble.dgml");
	//setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
	setProjection( Marble::Spherical );

	setAnimationsEnabled(true);
	setShowClouds( false );
	setShowBorders( false );
	setShowPlaces( true );
	setShowCrosshairs( false );
	setShowGrid( false );
	setShowOverviewMap(false);
	setShowScaleBar(true);

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

	messageWidget->animatedHide();

	loadedDives = new GeoDataDocument;

	diveLocations.clear();
	int idx = 0;
	struct dive *dive;
	for_each_dive(idx, dive) {
		if (dive_has_gps_location(dive)) {
			// don't add dive locations twice.
			if( diveLocations.contains( QString(dive->location)))
				continue;

			diveLocations.append( QString(dive->location) );
			GeoDataPlacemark *place = new GeoDataPlacemark( dive->location );
			place->setCoordinate(dive->longitude.udeg / 1000000.0,dive->latitude.udeg / 1000000.0 , 0, GeoDataCoordinates::Degree );
			loadedDives->append( place );
		}
	}
	model()->treeModel()->addDocument( loadedDives );
}

void GlobeGPS::centerOn(dive* dive)
{
	// dive has changed, if we had the 'editingDive', hide it.
	if(messageWidget->isVisible()){
		messageWidget->animatedHide();
	}

	editingDiveCoords = 0;

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
	messageWidget->setMessageType(KMessageWidget::Warning);
	messageWidget->setText(QObject::tr("This dive has no location! Please move the planet to the desired position, then double-click to set the new location for this dive."));
	messageWidget->animatedShow();

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
	messageWidget->animatedHide();
}

void GlobeGPS::mousePressEvent(QMouseEvent* event)
{
	qreal lat, lon;
	if (editingDiveCoords &&  geoCoordinates(event->pos().x(), event->pos().y(), lon,lat, GeoDataCoordinates::Radian)){
		changeDiveGeoPosition(lon, lat, GeoDataCoordinates::Radian);
	}
}

void GlobeGPS::setMessageWidget(KMessageWidget* globeMessage)
{
	messageWidget = globeMessage;
}


