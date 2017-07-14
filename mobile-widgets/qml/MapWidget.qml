import QtQuick 2.0
import QtLocation 5.3
import QtPositioning 5.3

Rectangle {
	id: mapParent
	anchors.fill: parent
	width: 512
	height: width

	Plugin {
		id: mapPlugin
		name: "esri"
	}

	Map {
		anchors.fill: parent
		plugin: mapPlugin
		zoomLevel: 1
	}
}
