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
				checked: manager.toggleDiveSite(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleDiveSite(true)
				}
			}
			Controls.Label {
				text: qsTr("Notes")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleNotes(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleNotes(true)
				}
			}
			Controls.Label {
				text: qsTr("Dive master")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleDiveMaster(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleDiveMaster(true)
				}
			}
			Controls.Label {
				text: qsTr("Buddy")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleBuddy(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleBuddy(true)
				}
			}
			Controls.Label {
				text: qsTr("Suit")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleSuit(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleSuit(true)
				}
			}
			Controls.Label {
				text: qsTr("Rating")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleRating(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleRating(true)
				}
			}
			Controls.Label {
				text: qsTr("Visibility")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleVisibility(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleVisibility(true)
				}
			}
			Controls.Label {
				text: qsTr("Tags")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleTags(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleTags(true)
				}
			}
			Controls.Label {
				text: qsTr("Cylinders")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleCylinders(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleCylinders(true)
				}
			}
			Controls.Label {
				text: qsTr("Weights")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				checked: manager.toggleWeights(false)
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.toggleWeights(true)
				}
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
