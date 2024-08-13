// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.4 as Kirigami
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
				checked: manager.pasteDiveSite
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Notes")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteNotes
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Dive guide")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteDiveGuide
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Buddy")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteBuddy
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Suit")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteSuit
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Rating")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteRating
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Visibility")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteVisibility
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Tags")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteTags
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Cylinders")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteCylinders
				Layout.preferredWidth: gridWidth * 0.25
			}
			Controls.Label {
				text: qsTr("Weights")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.pasteWeights
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
