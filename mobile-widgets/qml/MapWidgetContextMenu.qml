// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.7

Item {
	id: container
	signal actionSelected(int action)

	readonly property var actions: {
		"OPEN_LOCATION_IN_GOOGLE_MAPS": 0,
		"COPY_LOCATION_DECIMAL":        1,
		"COPY_LOCATION_SEXAGESIMAL":    2
	}
	readonly property var menuItemData: [
		{ idx: actions.OPEN_LOCATION_IN_GOOGLE_MAPS, itemText: qsTr("Open location in Google Maps") },
		{ idx: actions.COPY_LOCATION_DECIMAL,        itemText: qsTr("Copy location to clipboard (decimal)") },
		{ idx: actions.COPY_LOCATION_SEXAGESIMAL,    itemText: qsTr("Copy location to clipboard (sexagesimal)") }
	]
	readonly property real itemTextPadding: 10.0
	readonly property real itemHeight: 30.0
	readonly property int itemAnimationDuration: 100
	readonly property color colorItemBackground: "#dedede"
	readonly property color colorItemBackgroundSelected: "grey"
	readonly property color colorItemText: "black"
	readonly property color colorItemTextSelected: "#dedede"
	readonly property color colorItemBorder: "black"
	property int listViewIsVisible: -1
	property real maxItemWidth: 0.0

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
				listViewIsVisible = (listViewIsVisible !== 1) ? 1 : 0
			}
		}
	}

	ListModel {
		id: listModel
		property int selectedIdx: -1
		Component.onCompleted: {
			for (var i = 0; i < menuItemData.length; i++)
				append(menuItemData[i]);
		}
	}

	Component {
		id: listItemDelegate
		Rectangle {
			color: model.idx === listModel.selectedIdx ? colorItemBackgroundSelected : colorItemBackground
			width: maxItemWidth
			height: itemHeight
			border.color: colorItemBorder
			Text {
				x: itemTextPadding
				height: itemHeight
				verticalAlignment: Text.AlignVCenter
				text: model.itemText
				color: model.idx === listModel.selectedIdx ? colorItemTextSelected : colorItemText
				onWidthChanged: {
					if (width + itemTextPadding * 2.0 > maxItemWidth)
						maxItemWidth = width + itemTextPadding * 2.0
				}
				Behavior on color { ColorAnimation { duration: itemAnimationDuration }}
			}
			Behavior on color { ColorAnimation { duration: itemAnimationDuration }}
		}
	}

	ListView {
		id: listView
		y: contextMenuImage.y + contextMenuImage.height + 10;
		width: maxItemWidth;
		height: listModel.count * itemHeight
		visible: false
		opacity: 0.0
		interactive: false
		model: listModel
		delegate: listItemDelegate

		onCountChanged:	x = -maxItemWidth
		onVisibleChanged: listModel.selectedIdx = -1
		onOpacityChanged: visible = opacity != 0.0

		Timer {
			id: timerListViewVisible
			running: false
			repeat: false
			interval: itemAnimationDuration + 50
			onTriggered: listViewIsVisible = 0
		}

		MouseArea {
			anchors.fill: parent
			onClicked: {
				if (opacity < 1.0)
					return;
				var idx = listView.indexAt(mouseX, mouseY)
				listModel.selectedIdx = idx
				container.actionSelected(idx)
				timerListViewVisible.restart()
			}
		}
		states: [
			State { when: listViewIsVisible === 1; PropertyChanges { target: listView; opacity: 1.0 }},
			State { when: listViewIsVisible === 0; PropertyChanges { target: listView; opacity: 0.0 }}
		]
		transitions: Transition {
			NumberAnimation { properties: "opacity"; easing.type: Easing.InOutQuad }
		}
	}
}
