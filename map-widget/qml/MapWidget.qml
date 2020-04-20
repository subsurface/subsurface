// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.5
import QtLocation 5.3
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0

Item {
	id: rootItem
	property alias mapHelper: mapHelper
	property alias map: map

	signal selectedDivesChanged(var list)

	MapWidgetHelper {
		id: mapHelper
		map: map
		editMode: false
		onSelectedDivesChanged: rootItem.selectedDivesChanged(list)
		onEditModeChanged: editMessage.isVisible = editMode === true ? 1 : 0
		onCoordinatesChanged: {}
		Component.onCompleted: {
			map.plugin = Qt.createQmlObject(pluginObject, rootItem)
			map.mapType = { "STREET": map.supportedMapTypes[0], "SATELLITE": map.supportedMapTypes[1] }
			map.activeMapType = map.mapType.SATELLITE
		}
	}

	Map {
		id: map
		anchors.fill: parent
		zoomLevel: defaultZoomIn

		property var mapType
		readonly property var defaultCenter: QtPositioning.coordinate(0, 0)
		readonly property real defaultZoomIn: 12.0
		readonly property real defaultZoomOut: 1.0
		readonly property real textVisibleZoom: 11.0
		readonly property real zoomStep: 2.0
		property var newCenter: defaultCenter
		property real newZoom: 1.0
		property real newZoomOut: 1.0
		property var clickCoord: QtPositioning.coordinate(0, 0)
		property bool isReady: false

		Component.onCompleted: isReady = true
		onZoomLevelChanged: {
			if (isReady)
				mapHelper.calculateSmallCircleRadius(map.center)
		}

		MapItemView {
			id: mapItemView
			model: mapHelper.model
			delegate: MapQuickItem {
				id: mapItem
				anchorPoint.x: 0
				anchorPoint.y: mapItemImage.height
				coordinate:  model.coordinate
				z: model.z
				sourceItem: Image {
					id: mapItemImage
					source: model.pixmap
					SequentialAnimation {
						id: mapItemImageAnimation
						PropertyAnimation { target: mapItemImage; property: "scale"; from: 1.0; to: 0.7; duration: 120 }
						PropertyAnimation { target: mapItemImage; property: "scale"; from: 0.7; to: 1.0; duration: 80 }
					}
					MouseArea {
						drag.target: (mapHelper.editMode && model.isSelected) ? mapItem : undefined
						anchors.fill: parent
						onClicked: {
							if (!mapHelper.editMode && model.divesite)
								mapHelper.selectedLocationChanged(model.divesite)
						}
						onDoubleClicked: map.doubleClickHandler(mapItem.coordinate)
						onReleased: {
							if (mapHelper.editMode && model.isSelected) {
								mapHelper.updateCurrentDiveSiteCoordinatesFromMap(model.divesite, mapItem.coordinate)
							}
						}
					}
					Item {
						// Text with a duplicate for shadow. DropShadow as layer effect is kind of slow here.
						y: mapItemImage.y + mapItemImage.height
						visible: map.zoomLevel >= map.textVisibleZoom
						Text {
							id: mapItemTextShadow
							x: mapItemText.x + 2; y: mapItemText.y + 2
							text: mapItemText.text
							font.pointSize: mapItemText.font.pointSize
							color: "black"
						}
						Text {
							id: mapItemText
							text: model.name
							font.pointSize: 11.0
							color: model.isSelected ? "white" : "lightgrey"
						}
					}
				}
			}
		}

		SequentialAnimation {
			id: mapAnimationZoomIn
			NumberAnimation {
				target: map; property: "zoomLevel"; to: map.newZoomOut; duration: Math.abs(map.newZoomOut - map.zoomLevel) * 200
			}
			ParallelAnimation {
				CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 2000; easing.type: Easing.OutCubic }
				NumberAnimation {
					target: map; property: "zoomLevel"; to: map.newZoom; duration: 2000
				}
			}
		}

		ParallelAnimation {
			id: mapAnimationClick
			CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 500	}
			NumberAnimation { target: map; property: "zoomLevel"; to: map.newZoom; duration: 500 }
		}

		MouseArea {
			anchors.fill: parent
			onPressed: { map.stopZoomAnimations(); mouse.accepted = false }
			onWheel: { map.stopZoomAnimations(); wheel.accepted = false }
			onDoubleClicked: map.doubleClickHandler(map.toCoordinate(Qt.point(mouseX, mouseY)))
		}

		function doubleClickHandler(coord) {
			newCenter = coord
			newZoom = zoomLevel + zoomStep
			if (newZoom > maximumZoomLevel)
				newZoom = maximumZoomLevel
			mapAnimationClick.restart()
		}

		function pointIsVisible(pt) {
			return !isNaN(pt.x)
		}

		function coordIsValid(coord) {
			if (coord == null || isNaN(coord.latitude) || isNaN(coord.longitude) ||
			    (coord.latitude === 0.0 && coord.longitude === 0.0))
				return false;
			return true;
		}

		function stopZoomAnimations() {
			mapAnimationZoomIn.stop()
		}

		function centerOnCoordinate(coord) {
			stopZoomAnimations()
			if (!coordIsValid(coord)) {
				console.warn("MapWidget.qml: centerOnCoordinate(): !coordIsValid()")
				return
			}
			var newZoomOutFound = false
			var zoomStored = zoomLevel
			var centerStored = QtPositioning.coordinate(center.latitude, center.longitude)
			newZoomOut = zoomLevel
			newCenter = coord
			zoomLevel = Math.floor(zoomLevel)
			while (zoomLevel > minimumZoomLevel) {
				var pt = fromCoordinate(coord)
				if (pointIsVisible(pt)) {
					newZoomOut = zoomLevel
					newZoomOutFound = true
					break
				}
				zoomLevel -= 1.0
			}
			if (!newZoomOutFound)
				newZoomOut = defaultZoomOut
			zoomLevel = zoomStored
			center = centerStored
			newZoom = zoomStored
			mapAnimationZoomIn.restart()
		}

		function centerOnRectangle(topLeft, bottomRight, centerRect) {
			stopZoomAnimations()
			if (newCenter.latitude === 0.0 && newCenter.longitude === 0.0) {
				// Do nothing
				return
			}
			var centerStored = QtPositioning.coordinate(center.latitude, center.longitude)
			var zoomStored = zoomLevel
			var newZoomOutFound = false
			newCenter = centerRect
			// calculate zoom out
			newZoomOut = zoomLevel
			while (zoomLevel > minimumZoomLevel) {
				var ptCenter = fromCoordinate(centerStored)
				var ptCenterRect = fromCoordinate(centerRect)
				if (pointIsVisible(ptCenter) && pointIsVisible(ptCenterRect)) {
					newZoomOut = zoomLevel
					newZoomOutFound = true
					break
				}
				zoomLevel -= 1.0
			}
			if (!newZoomOutFound)
				newZoomOut = defaultZoomOut
			// calculate zoom in
			center = newCenter
			zoomLevel = Math.floor(maximumZoomLevel)
			var diagonalRect = topLeft.distanceTo(bottomRight)
			while (zoomLevel > minimumZoomLevel) {
				var c0 = toCoordinate(Qt.point(0.0, 0.0))
				var c1 = toCoordinate(Qt.point(width, height))
				if (c0.distanceTo(c1) > diagonalRect) {
					newZoom = zoomLevel - 2.0
					break
				}
				zoomLevel -= 1.0
			}
			if (newZoom > defaultZoomIn)
				newZoom = defaultZoomIn
			zoomLevel = zoomStored
			center = centerStored
			mapAnimationZoomIn.restart()
		}

		function deselectMapLocation() {
			stopZoomAnimations()
		}
	}

	Rectangle {
		id: editMessage
		radius: padding
		color: "#b08000"
		border.color: "white"
		x: (map.width - width) * 0.5; y: padding
		width: editMessageText.width + padding * 2.0
		height: editMessageText.height + padding * 2.0
		visible: false
		opacity: 0.0
		property int isVisible: -1
		property real padding: 10.0
		onOpacityChanged: visible = opacity != 0.0
		states: [
			State { when: editMessage.isVisible === 1; PropertyChanges { target: editMessage; opacity: 1.0 }},
			State { when: editMessage.isVisible === 0; PropertyChanges { target: editMessage; opacity: 0.0 }}
		]
		transitions: Transition { NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad }}
		Text {
			id: editMessageText
			y: editMessage.padding; x: editMessage.padding
			verticalAlignment: Text.AlignVCenter
			color: "white"
			font.pointSize: 11.0
			text: qsTr("Drag the selected dive location")
		}
	}

	Image {
		id: toggleImage
		x: 10; y: x
		width: 40
		height: 40
		source: "qrc:///map-style-" + (map.activeMapType === map.mapType.SATELLITE ? "map" : "photo") + "-icon"
		SequentialAnimation {
			id: toggleImageAnimation
			PropertyAnimation { target: toggleImage; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: toggleImage; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				map.activeMapType = map.activeMapType === map.mapType.SATELLITE ? map.mapType.STREET : map.mapType.SATELLITE
				toggleImageAnimation.restart()
			}
		}
	}

	Image {
		id: imageZoomIn
		x: 10 + (toggleImage.width - imageZoomIn.width) * 0.5; y: toggleImage.y + toggleImage.height + 10
		width: 20
		height: 20
		source: "qrc:///zoom-in-icon"
		SequentialAnimation {
			id: imageZoomInAnimation
			PropertyAnimation { target: imageZoomIn; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: imageZoomIn; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				map.stopZoomAnimations()
				map.newCenter = map.center
				map.newZoom = map.zoomLevel + map.zoomStep
				if (map.newZoom > map.maximumZoomLevel)
					map.newZoom = map.maximumZoomLevel
				mapAnimationClick.restart()
				imageZoomInAnimation.restart()
			}
		}
	}

	Image {
		id: imageZoomOut
		x: imageZoomIn.x; y: imageZoomIn.y + imageZoomIn.height + 10
		source: "qrc:///zoom-out-icon"
		width: 20
		height: 20
		SequentialAnimation {
			id: imageZoomOutAnimation
			PropertyAnimation { target: imageZoomOut; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: imageZoomOut; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
				map.stopZoomAnimations()
				map.newCenter = map.center
				map.newZoom = map.zoomLevel - map.zoomStep
				mapAnimationClick.restart()
				imageZoomOutAnimation.restart()
			}
		}
	}

	/*
	 * open coordinates in google maps while attempting to roughly preserve
	 * the zoom level. the mapping between the QML map zoom level and the
	 * Google Maps zoom level is done via exponential regression:
	 *     y = a * exp(b * x)
	 *
	 * data set:
	 *     qml (x)            gmaps (y in meters)
	 *     21                 257
	 *     15.313216749178913 3260
	 *     12.553216749178931 20436
	 *     11.11321674917894  52883
	 *     9.313216749178952  202114
	 *     7.51321674917896   737136
	 *     5.593216749178958  2495529
	 *     4.153216749178957  3895765
	 *     1.753216749178955  18999949
	 */
	function openLocationInGoogleMaps(latitude, longitude) {
		var loc = latitude + "," + longitude
		var poi = latitude + "+" + longitude
		var x = map.zoomLevel
		var a = 53864950.831693
		var b = -0.60455861606547030630
		var zoom = Math.floor(a * Math.exp(b * x))
		var url = "https://www.google.com/maps/place/" + poi + "/@" + loc + "," + zoom + "m/data=!3m1!1e3!4m2!3m1!1s0x0:0x0"
		Qt.openUrlExternally(url)
		console.log("openLocationInGoogleMaps() map.zoomLevel: " + x + ", url: " + url)
	}

	MapWidgetContextMenu {
		id: contextMenu
		y: 10; x: map.width - y
		onActionSelected: {
			switch (action) {
			case contextMenu.actions.OPEN_LOCATION_IN_GOOGLE_MAPS:
				openLocationInGoogleMaps(map.center.latitude, map.center.longitude)
				break
			case contextMenu.actions.COPY_LOCATION_DECIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, false)
				break
			case contextMenu.actions.COPY_LOCATION_SEXAGESIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, true)
				break
			case contextMenu.actions.SELECT_VISIBLE_LOCATIONS:
				mapHelper.selectVisibleLocations()
				break
			}
		}
	}
}
