// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.7

Item {
	Image {
		id: contextMenuImage
		x: -width
		source: "qrc:///mapwidget-context-menu"

		SequentialAnimation {
			id:contextMenuImageAnimation
			PropertyAnimation {
				target: contextMenuImage; property: "scale"; from: 1.0; to: 0.8; duration: 80;
			}
			PropertyAnimation {
				target: contextMenuImage; property: "scale"; from: 0.8; to: 1.0; duration: 60;
			}
		}

		MouseArea {
			anchors.fill: parent
			onClicked: {
				contextMenuImageAnimation.restart()
			}
		}
	}

	readonly property var menuItemIndex: {
		"OPEN_LOCATION_IN_GOOGLE_MAPS": 0,
		"COPY_LOCATION_DECIMAL":        1,
		"COPY_LOCATION_SEXAGESIMAL":    2
	}

	readonly property var menuItemData: [
		{ idx: menuItemIndex.OPEN_LOCATION_IN_GOOGLE_MAPS, itemText: qsTr("Open location in Google Maps") },
		{ idx: menuItemIndex.COPY_LOCATION_DECIMAL,        itemText: qsTr("Copy location to clipboard (decimal)") },
		{ idx: menuItemIndex.COPY_LOCATION_SEXAGESIMAL,    itemText: qsTr("Copy location to clipboard (sexagesimal)") }
	]

	ListModel {
		id: listModel
		property int selectedIdx: -1
		Component.onCompleted: {
			for (var i = 0; i < menuItemData.length; i++)
				append(menuItemData[i]);
		}
	}
}
