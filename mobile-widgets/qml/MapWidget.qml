// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.0
import QtLocation 5.3
import QtPositioning 5.3
import org.subsurfacedivelog.mobile 1.0

Item {
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

		readonly property var mapType: { "STREET": supportedMapTypes[0], "SATELLITE": supportedMapTypes[1] }
		readonly property var defaultCenter: QtPositioning.coordinate(0, 0)
		readonly property var defaultZoomIn: 17.0
		readonly property var defaultZoomOut: 1.0
		property var newCenter: defaultCenter
		property var newZoom: 1.0

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
				sourceItem: Image {
					id: mapItemImage
					source: "qrc:///mapwidget-marker" + (mapHelper.model.selectedUuid === model.uuid ? "-selected" : "")

					SequentialAnimation {
						id: mapItemImageAnimation
						PropertyAnimation {
							target: mapItemImage; property: "scale"; from: 1.0; to: 0.7; duration: 120;
						}
						PropertyAnimation {
							target: mapItemImage; property: "scale"; from: 0.7; to: 1.0; duration: 80;
						}
					}
				}

				MouseArea {
					anchors.fill: parent
					onClicked: {
						mapHelper.model.selectedUuid = model.uuid
						mapItemImageAnimation.restart()
					}
				}
			}
		}

		ParallelAnimation {
			id: mapAnimationZoomIn
			CoordinateAnimation {
				target: map; property: "center"; to: map.newCenter; duration: 2000;
			}
			NumberAnimation {
				target: map; property: "zoomLevel"; to: map.newZoom ; duration: 3000; easing.type: Easing.InCubic;
			}
		}

		ParallelAnimation {
			id: mapAnimationZoomOut
			NumberAnimation {
				target: map; property: "zoomLevel"; from: map.zoomLevel; to: map.newZoom; duration: 3000;
			}
			SequentialAnimation {
				PauseAnimation { duration: 2000 }
				CoordinateAnimation {
					target: map; property: "center"; to: map.newCenter; duration: 2000;
				}
			}
		}

		function animateMapZoomIn(coord) {
			zoomLevel = defaultZoomOut
			newCenter = coord
			newZoom = map.defaultZoomIn
			mapAnimationZoomIn.restart()
			mapAnimationZoomOut.stop()
		}

		function animateMapZoomOut() {
			newCenter = map.defaultCenter
			newZoom = map.defaultZoomOut
			mapAnimationZoomIn.stop()
			mapAnimationZoomOut.restart()
		}

		function centerOnMapLocation(mapLocation) {
			mapHelper.model.selectedUuid = mapLocation.uuid
			animateMapZoomIn(mapLocation.coordinate)
		}

		function deselectMapLocation() {
			mapHelper.model.selectedUuid = 0
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
				target: toggleImage; property: "scale"; from: 1.0; to: 0.8; duration: 120;
			}
			PropertyAnimation {
				target: toggleImage; property: "scale"; from: 0.8; to: 1.0; duration: 80;
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
				mapHelper.copyToClipboardCoordinates(map.center, false);
				break;
			case contextMenu.actions.COPY_LOCATION_SEXAGESIMAL:
				mapHelper.copyToClipboardCoordinates(map.center, true);
				break;
			}
		}
	}
}
