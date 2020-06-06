// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import org.kde.kirigami 2.4 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: recoverCache
	title: qsTr("Cloud Cache Import")
	objectName: "recoverCache"
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
	width: parent.width
	height: parent.height

	Item {
	TemplateLabel {
		id: header
		text: qsTr("Cloud Cache Import")
		color: subsurfaceTheme.lightPrimaryTextColor
		background: Rectangle { color: subsurfaceTheme.lightPrimaryColor }
		font.pointSize: subsurfaceTheme.regularPointSize * 1.5
		padding: Kirigami.Units.gridUnit
		width: recoverCache.width - 5 * Kirigami.Units.largeSpacing
		height: 3.5 * Kirigami.Units.gridUnit
	}
	Rectangle {
		id: subheader
		z: 5
		width: recoverCache.width - 5 * Kirigami.Units.largeSpacing
		height: 3 * Kirigami.Units.gridUnit
		color: subsurfaceTheme.backgroundColor
		anchors {
			left: header.left
			top: header.bottom
			right: parent.right
		}
		TemplateLabel {
			height: 2 * Kirigami.Units.gridUnit
			text: qsTr("import data from the given cache repo")
			anchors {
				verticalCenter: parent.verticalCenter
				horizontalCenter: parent.horizontalCenter
			}
		}
	}
	Rectangle {
		id: spacer
		anchors.top: subheader.bottom
		height: Kirigami.Units.largeSpacing
		width: recoverCache.width
		color: subsurfaceTheme.backgroundColor
	}

	Rectangle {
		anchors {
			left: header.left
			right: parent.right
			top: spacer.bottom
		}
		z: -5
		ListView {
			height: recoverCache.height - 9 * Kirigami.Units.gridUnit
			width: recoverCache.width
			model: manager.cloudCacheList
			delegate: TemplateButton {
				height: 3 * Kirigami.Units.gridUnit
				width: parent.width - 2 * Kirigami.Units.gridUnit
				text: modelData
				onClicked: {
					console.log("import " + modelData)
					manager.importCacheRepo(modelData)
				}
			}
		}
	}
	}
}
