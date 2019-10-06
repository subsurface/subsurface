// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: mapPage
	objectName: "MapPage"
	title: qsTr("Map")
	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0
	property bool firstRun: true
	width: rootItem.colWidth
	MapWidget {
		id: mapWidget
		anchors.fill: parent
		onSelectedDivesChanged: {
			if (list.length === 0) {
				console.warn("main.qml: onSelectedDivesChanged(): received empty list!")
				return
			}
			var id = list[0] // only single dive selection is supported
			var idx = diveModel.getIdxForId(id)
			if (idx === -1) {
				console.warn("main.qml: onSelectedDivesChanged(): cannot find list index for dive id:", id)
				return
			}
			diveList.setCurrentDiveListIndex(idx, true)
		}
		Component.onCompleted: {
			mapWidget.map.zoomLevel = mapWidget.map.defaultZoomOut
			mapWidget.map.center = mapWidget.map.defaultCenter
		}
	}

	function reloadMap() {
		mapWidget.mapHelper.reloadMapLocations()
	}

	function centerOnDiveSite(ds) {
		if (!ds) {
			console.warn("main.qml: centerOnDiveSite(): dive site is undefined!")
			return
		}
		// on firstRun, hard pan/center the map to the desired location so that
		// we don't start at an arbitrary location such as [0,0] or London.
		if (firstRun) {
			var coord = mapWidget.mapHelper.getCoordinates(ds)
			centerOnLocationHard(coord.latitude, coord.longitude)
			firstRun = false
		} // continue here as centerOnDiveSite() also does marker selection.
		mapWidget.mapHelper.centerOnDiveSite(ds)
	}

	function centerOnLocation(lat, lon) {
		if (firstRun) {
			centerOnLocationHard(lat, lon)
			firstRun = false
			return // no need to animate via centerOnCoordinate().
		}
		mapWidget.map.centerOnCoordinate(QtPositioning.coordinate(lat, lon))
	}

	function centerOnLocationHard(lat, lon) {
		mapWidget.map.zoomLevel = mapWidget.map.defaultZoomIn
		mapWidget.map.center = QtPositioning.coordinate(lat, lon)
	}
}
