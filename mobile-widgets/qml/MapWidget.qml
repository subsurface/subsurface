// SPDX-License-Identifier: GPL-2.0
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

		readonly property var defaultCenter: QtPositioning.coordinate(0, 0);
		readonly property var defaultZoomIn: 17.0;
		property var newCenter: defaultCenter;
		property var newZoom: 1.0;

		Component.onCompleted: {
			map.activeMapType = map.supportedMapTypes[esriMapTypeIndexes.SATELLITE];
		}

		MapItemView {
			id: mapItemView

			model: mapHelper.model
			delegate: MapQuickItem {
				anchorPoint.x: 0
				anchorPoint.y: mapItemImage.height
				coordinate:  model.coordinate
				z: mapHelper.model.selectedUuid === model.uuid ? mapHelper.model.count - 1 : 0
				sourceItem: Image {
					id: mapItemImage;
					source: "qrc:///mapwidget-marker" + (mapHelper.model.selectedUuid === model.uuid ? "-selected" : "");

					SequentialAnimation {
						id: mapItemImageAnimation;
						PropertyAnimation {
							target: mapItemImage; property: "scale"; from: 1.0; to: 0.7; duration: 120;
						}
						PropertyAnimation {
							target: mapItemImage; property: "scale"; from: 0.7; to: 1.0; duration: 80;
						}
					}

					onSourceChanged: {
						if (mapHelper.model.selectedUuid === model.uuid)
							mapItemImageAnimation.restart();
					}
				}

				MouseArea {
					anchors.fill: parent
					onClicked: {
						mapHelper.model.selectedUuid = model.uuid
					}
				}
			}
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

		function centerOnMapLocation(mapLocation) {
			map.newCenter = mapLocation.coordinate;
			map.zoomLevel = 2;
			mapAnimation.restart();
			mapHelper.model.selectedUuid = mapLocation.uuid;
		}
	}
}
