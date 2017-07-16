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

		property var newCenter: QtPositioning.coordinate(0, 0);

		Component.onCompleted: {
			map.activeMapType = map.supportedMapTypes[esriMapTypeIndexes.SATELLITE];
		}

		ParallelAnimation {
			id: mapAnimation

			CoordinateAnimation {
				target: map
				property: "center"
				to: map.newCenter
				duration: 2000
			}
			NumberAnimation {
				target: map
				property: "zoomLevel"
				to: 17
				duration: 3000
				easing.type: Easing.InCubic
			}
		}

		function centerOnCoordinates(latitude, longitude) {
			map.newCenter = QtPositioning.coordinate(latitude, longitude);
			map.zoomLevel = 2;
			mapAnimation.restart();
		}
	}
}
