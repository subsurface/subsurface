// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.0
import QtLocation 5.3
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0

Item {
	property int nSelectedDives: 0

	Plugin {
		id: mapPlugin
		name: "esri"
	}

	MapWidgetHelper {
		id: mapHelper
		map: map
		editMode: false
		onSelectedDivesChanged: nSelectedDives = list.length
		onEditModeChanged: editMessage.isVisible = editMode === true ? 1 : 0
		onCoordinatesChanged: {}
	}

	Map {
		id: map
		anchors.fill: parent
		plugin: mapPlugin
		zoomLevel: 1

		readonly property var mapType: { "STREET": supportedMapTypes[0], "SATELLITE": supportedMapTypes[1] }
		readonly property var defaultCenter: QtPositioning.coordinate(0, 0)
		readonly property real defaultZoomIn: 12.0
		readonly property real defaultZoomOut: 1.0
		readonly property real textVisibleZoom: 8.0
		readonly property real zoomStep: 2.0
		property var newCenter: defaultCenter
		property real newZoom: 1.0
		property real newZoomOut: 1.0
		property var clickCoord: QtPositioning.coordinate(0, 0)

		Component.onCompleted: activeMapType = mapType.SATELLITE
		onZoomLevelChanged: mapHelper.calculateSmallCircleRadius(map.center)

		MapItemView {
			id: mapItemView
			model: mapHelper.model
			delegate: MapQuickItem {
				id: mapItem
				anchorPoint.x: 0
				anchorPoint.y: mapItemImage.height
				coordinate:  model.coordinate
				z: mapHelper.model.selectedUuid === model.uuid ? mapHelper.model.count - 1 : 0
				sourceItem: Item {
					Image {
						id: mapItemImage
						source: "qrc:///mapwidget-marker" + (mapHelper.model.selectedUuid === model.uuid ? "-selected" : "")
						SequentialAnimation {
							id: mapItemImageAnimation
							PropertyAnimation { target: mapItemImage; property: "scale"; from: 1.0; to: 0.7; duration: 120 }
							PropertyAnimation { target: mapItemImage; property: "scale"; from: 0.7; to: 1.0; duration: 80 }
						}
						MouseArea {
							drag.target: (mapHelper.editMode && mapHelper.model.selectedUuid === model.uuid) ? mapItem : undefined
							anchors.fill: parent
							onClicked: {
								if (!mapHelper.editMode)
									mapHelper.model.setSelectedUuid(model.uuid, true)
							}
							onDoubleClicked: map.doubleClickHandler(mapItem.coordinate)
							onReleased: {
								if (mapHelper.editMode && mapHelper.model.selectedUuid === model.uuid) {
									mapHelper.updateCurrentDiveSiteCoordinates(mapHelper.model.selectedUuid, mapItem.coordinate)
								}
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
							color: mapHelper.model.selectedUuid === model.uuid ? "white" : "lightgrey"
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
				CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 1000 }
				NumberAnimation {
					target: map; property: "zoomLevel"; to: map.newZoom ; duration: 2000; easing.type: Easing.InCubic
				}
			}
		}

		ParallelAnimation {
			id: mapAnimationZoomOut
			NumberAnimation { target: map; property: "zoomLevel"; from: map.zoomLevel; to: map.newZoom; duration: 3000 }
			SequentialAnimation {
				PauseAnimation { duration: 2000 }
				CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 2000 }
			}
		}

		ParallelAnimation {
			id: mapAnimationClick
			CoordinateAnimation { target: map; property: "center"; to: map.newCenter; duration: 500	}
			NumberAnimation { target: map; property: "zoomLevel"; to: map.newZoom; duration: 500 }
		}

		MouseArea {
			anchors.fill: parent
			onDoubleClicked: map.doubleClickHandler(map.toCoordinate(Qt.point(mouseX, mouseY)))
		}

		function doubleClickHandler(coord) {
			newCenter = coord
			newZoom = zoomLevel + zoomStep
			if (newZoom > maximumZoomLevel)
				newZoom = maximumZoomLevel
			mapAnimationClick.restart()
		}

		function animateMapZoomOut() {
			newCenter = defaultCenter
			newZoom = defaultZoomOut
			mapAnimationZoomIn.stop()
			mapAnimationZoomOut.restart()
		}

		function centerOnCoordinate(coord) {
			newCenter = coord
			if (coord.latitude === 0.0 && coord.longitude === 0.0) {
				newZoom = 2.6
				newZoomOut = newZoom
			} else {
				var zoomStored = zoomLevel
				newZoomOut = zoomLevel
				while (zoomLevel > minimumZoomLevel) {
					var pt = fromCoordinate(coord, false)
					if (pt.x > 0 && pt.y > 0 && pt.x < width && pt.y < height) {
						newZoomOut = zoomLevel
						break
					}
					zoomLevel--
				}
				zoomLevel = zoomStored
				newZoom = defaultZoomIn
			}
			mapAnimationZoomIn.restart()
			mapAnimationZoomOut.stop()
		}

		function deselectMapLocation() {
			animateMapZoomOut()
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
		source: "qrc:///mapwidget-toggle-" + (map.activeMapType === map.mapType.SATELLITE ? "street" : "satellite")
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
		source: "qrc:///mapwidget-zoom-in"
		SequentialAnimation {
			id: imageZoomInAnimation
			PropertyAnimation { target: imageZoomIn; property: "scale"; from: 1.0; to: 0.8; duration: 120 }
			PropertyAnimation { target: imageZoomIn; property: "scale"; from: 0.8; to: 1.0; duration: 80 }
		}
		MouseArea {
			anchors.fill: parent
			onClicked: {
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
		source: "qrc:///mapwidget-zoom-out"
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
				map.newCenter = map.center
				map.newZoom = map.zoomLevel - map.zoomStep
				mapAnimationClick.restart()
				imageZoomOutAnimation.restart()
			}
		}
	}

	function openLocationInGoogleMaps(latitude, longitude) {
		var loc = latitude + " " + longitude
		var url = "https://www.google.com/maps/place/" + loc + "/@" + loc + ",5000m/data=!3m1!1e3!4m2!3m1!1s0x0:0x0"
		Qt.openUrlExternally(url)
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
			}
		}
	}
}
