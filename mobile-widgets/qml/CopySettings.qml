// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	objectName: "CopySettings"
	id: settingsCopy

	title: qsTr("Copy Settings")
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	property real gridWidth: settingsCopy.width - Kirigami.Units.gridUnit

	ColumnLayout {
		width: gridWidth

		GridLayout {
			id: copy_settings
			columns: 2
			Controls.Label {
				text: qsTr("Selection for copy-paste")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Controls.Label {
				text: qsTr("Dive site")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteDiveSite
				onToggled: manager.pasteDiveSite = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Notes")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteNotes
				onToggled: manager.pasteNotes = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Dive guide")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteDiveGuide
				onToggled: manager.pasteDiveGuide = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Buddy")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteBuddy
				onToggled: manager.pasteBuddy = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Suit")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteSuit
				onToggled: manager.pasteSuit = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Rating")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteRating
				onToggled: manager.pasteRating = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Visibility")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteVisibility
				onToggled: manager.pasteVisibility = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Tags")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteTags
				onToggled: manager.pasteTags = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Cylinders")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteCylinders
				onToggled: manager.pasteCylinders = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Weights")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				Component.onCompleted: checked = manager.pasteWeights
				onToggled: manager.pasteWeights = checked
				Layout.preferredWidth: gridWidth * 0.25
			}
		}

		Rectangle {
			color: subsurfaceTheme.darkerPrimaryColor
			height: 1
			opacity: 0.5
			Layout.fillWidth: true
		}

		Item {
			height: Kirigami.Units.gridUnit * 6
		}
	}
}
