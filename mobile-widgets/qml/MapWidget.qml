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
		onSelectedDivesChanged: {
			// 'list' contains a list of dive list indexes
			nSelectedDives = list.length
		}
	}

	Map {
		id: map
		anchors.fill: parent
		plugin: mapPlugin
		zoomLevel: 1

		readonly property var mapType: { "STREET": supportedMapTypes[0], "SATELLITE": supportedMapTypes[1] }
		readonly property var defaultCenter: QtPositioning.coordinate(0, 0)
		readonly property real defaultZoomIn: 17.0
		readonly property real defaultZoomOut: 1.0
		readonly property real textVisibleZoom: 8.0
		readonly property real zoomStep: 2.0
		property var newCenter: defaultCenter
		property real newZoom: 1.0
		property var clickCoord: QtPositioning.coordinate(0, 0)

		Component.onCompleted: {
			activeMapType = mapType.SATELLITE
		}

		onZoomLevelChanged: mapHelper.calculateSmallCircleRadius(map.center)

		MapItemView {
			id: mapItemView
			model: mapHelper.model
			delegate: MapQuickItem {
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
							PropertyAnimation {
								target: mapItemImage; property: "scale"; from: 1.0; to: 0.7; duration: 120
							}
							PropertyAnimation {
								target: mapItemImage; property: "scale"; from: 0.7; to: 1.0; duration: 80
							}
						}
						MouseArea {
							anchors.fill: parent
							onClicked: mapHelper.model.setSelectedUuid(model.uuid, true)
							onDoubleClicked: map.doubleClickHandler(model.coordinate)
						}
					}
					Item {
						// Text with a duplicate for shadow. DropShadow as layer effect is kind of slow here.
						y: mapItemImage.y + mapItemImage.height
						visible: map.zoomLevel > map.textVisibleZoom
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
							color: "white"
						}
					}
				}
			}
		}

		ParallelAnimation {
			id: mapAnimationZoomIn
			CoordinateAnimation {
				target: map; property: "center"; to: map.newCenter; duration: 2000
			}
			NumberAnimation {
				target: map; property: "zoomLevel"; to: map.newZoom ; duration: 3000; easing.type: Easing.InCubic
			}
		}

		ParallelAnimation {
			id: mapAnimationZoomOut
			NumberAnimation {
				target: map; property: "zoomLevel"; from: map.zoomLevel; to: map.newZoom; duration: 3000
			}
			SequentialAnimation {
				PauseAnimation { duration: 2000 }
				CoordinateAnimation {
					target: map; property: "center"; to: map.newCenter; duration: 2000
				}
			}
		}

		ParallelAnimation {
			id: mapAnimationClick
			CoordinateAnimation {
				target: map; property: "center"; to: map.newCenter; duration: 500
			}
			NumberAnimation {
				target: map; property: "zoomLevel"; to: map.newZoom; duration: 500
			}
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

		function animateMapZoomIn(coord) {
			zoomLevel = defaultZoomOut
			newCenter = coord
			newZoom = defaultZoomIn
			mapAnimationZoomIn.restart()
			mapAnimationZoomOut.stop()
		}

		function animateMapZoomOut() {
			newCenter = defaultCenter
			newZoom = defaultZoomOut
			mapAnimationZoomIn.stop()
			mapAnimationZoomOut.restart()
		}

		function centerOnMapLocation(mapLocation) {
			mapHelper.model.setSelectedUuid(mapLocation.uuid, false)
			animateMapZoomIn(mapLocation.coordinate)
		}

		function deselectMapLocation() {
			mapHelper.model.setSelectedUuid(0, false)
			animateMapZoomOut()
		}
	}

	Image {
		id: toggleImage
		x: 10; y: x
		source: "qrc:///mapwidget-toggle-" + (map.activeMapType === map.mapType.SATELLITE ? "street" : "satellite")

		SequentialAnimation {
			id: toggleImageAnimation
			PropertyAnimation {
				target: toggleImage; property: "scale"; from: 1.0; to: 0.8; duration: 120
			}
			PropertyAnimation {
				target: toggleImage; property: "scale"; from: 0.8; to: 1.0; duration: 80
			}
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
		source: "qrc:///mapwidget-zoom-in"
		SequentialAnimation {
			id: imageZoomInAnimation
			PropertyAnimation {
				target: imageZoomIn; property: "scale"; from: 1.0; to: 0.8; duration: 120
			}
			PropertyAnimation {
				target: imageZoomIn; property: "scale"; from: 0.8; to: 1.0; duration: 80
			}
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
		SequentialAnimation {
			id: imageZoomOutAnimation
			PropertyAnimation {
				target: imageZoomOut; property: "scale"; from: 1.0; to: 0.8; duration: 120
			}
			PropertyAnimation {
				target: imageZoomOut; property: "scale"; from: 0.8; to: 1.0; duration: 80
			}
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
				break;
			case contextMenu.actions.COPY_LOCATION_DECIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, false)
				break;
			case contextMenu.actions.COPY_LOCATION_SEXAGESIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, true)
				break;
			}
		}
	}
}
