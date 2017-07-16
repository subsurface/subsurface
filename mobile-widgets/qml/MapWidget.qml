import QtQuick 2.0
import QtLocation 5.3
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0

Item {

	readonly property var esriMapTypeIndexes: { "STREET": 0, "SATELLITE": 1 };

	Plugin {
		id: mapPlugin
		name: "esri"
	}

	MapWidgetHelper {
		id: mapHelper
		map: map
	}

	Map {
		id: map
		anchors.fill: parent
		plugin: mapPlugin
		zoomLevel: 1

		Component.onCompleted: {
			map.activeMapType = map.supportedMapTypes[esriMapTypeIndexes.SATELLITE];
		}

		function centerOnCoordinates(latitude, longitude) {
			map.center = QtPositioning.coordinate(latitude, longitude);
			map.zoomLevel = map.maximumZoomLevel * 0.9;
		}
	}
}
