#include "globe.h"
#include <marble/AbstractFloatItem.h>

using namespace Marble;

GlobeGPS::GlobeGPS(QWidget* parent) : MarbleWidget(parent)
{
	setMapThemeId("earth/bluemarble/bluemarble.dgml");
	setProjection( Marble::Spherical );

	// Enable the cloud cover and enable the country borders
	setShowClouds( true );
	setShowBorders( true );

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
