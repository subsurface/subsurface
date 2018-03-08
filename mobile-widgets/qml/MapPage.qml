// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami

Kirigami.Page {
	id: mapPage
	objectName: "MapPage"
	title: qsTr("Map")
	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0

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
	}

	function reloadMap() {
		mapWidget.mapHelper.reloadMapLocations()
	}

	function centerOnDiveSiteUUID(uuid) {
		if (!uuid) {
			console.warn("main.qml: centerOnDiveSiteUUI(): uuid is undefined!")
			return
		}
		mapWidget.mapHelper.centerOnDiveSiteUUID(uuid)
	}

	function centerOnLocation(lat, lon) {
		mapWidget.map.centerOnCoordinate(QtPositioning.coordinate(lat, lon))
	}
}
