#include "globe.h"
#include "../dive.h"

#include <QDebug>

#include <marble/AbstractFloatItem.h>
#include <marble/GeoDataPlacemark.h>
#include <marble/GeoDataDocument.h>
#include <marble/MarbleModel.h>
#include <marble/GeoDataTreeModel.h>

using namespace Marble;

GlobeGPS::GlobeGPS(QWidget* parent) : MarbleWidget(parent), loadedDives(0)
{
	setMapThemeId("earth/bluemarble/bluemarble.dgml");
	setProjection( Marble::Spherical );

	// Enable the cloud cover and enable the country borders
	setShowClouds( false );
	setShowBorders( false );
	setShowPlaces( false );
	setShowCrosshairs( false );
	setShowGrid( false );

	// Hide the FloatItems: Compass and StatusBar
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


	loadedDives = new GeoDataDocument;

	int idx = 0;
	struct dive *dive;
	for_each_dive(idx, dive) {
		if (dive_has_gps_location(dive)) {
			GeoDataPlacemark *place = new GeoDataPlacemark( dive->location );
			place->setDescription(dive->notes);
			place->setCoordinate(dive->longitude.udeg / 1000000.0,dive->latitude.udeg / 1000000.0 , 0, GeoDataCoordinates::Degree );
			loadedDives->append( place );
		}
	}
	model()->treeModel()->addDocument( loadedDives );
}

void GlobeGPS::centerOn(dive* dive)
{
	centerOn(dive->longitude.udeg / 1000000.0,dive->latitude.udeg / 1000000.0);
}
