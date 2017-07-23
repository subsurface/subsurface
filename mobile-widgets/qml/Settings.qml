// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	objectName: "Settings"
	id: settingsPage
	title: qsTr("Settings")
	anchors.margins: Kirigami.Units.gridUnit / 2
	property real gridWidth: settingsPage.width - 2 * Kirigami.Units.gridUnit

	ColumnLayout {
		width: parent.width - Kirigami.Units.gridUnit
		CloudCredentials {
			id: cloudCredentials
			Layout.fillWidth: true
			Layout.margins: Kirigami.Units.gridUnit
			Layout.topMargin: 0
			property int headingLevel: 4
		}
		Rectangle {
			color: subsurfaceTheme.darkerPrimaryColor
			height: 1
			opacity: 0.5
			Layout.fillWidth: true
		}
		GridLayout {
			id: themeSettings
			columns: 3
			Layout.bottomMargin: Kirigami.Units.gridUnit

			Kirigami.Heading {
				text: qsTr("Theme")
				color: subsurfaceTheme.textColor
				level: 4
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 3
			}
			Kirigami.Label {
				text: qsTr("Blue")
				color: subsurfaceTheme.textColor
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.2
			}
			Row {
				Layout.preferredWidth: gridWidth * 0.6
				Rectangle {
					id: blueRect
					color: subsurfaceTheme.blueBackgroundColor
					border.color: "black"
					width: sampleRegularBlue.width + Kirigami.Units.gridUnit
					height: Kirigami.Units.gridUnit * 2
					Text {
						id: sampleRegularBlue
						text: qsTr("regular text")
						color: subsurfaceTheme.blueTextColor
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
				Rectangle {
					color: subsurfaceTheme.bluePrimaryColor
					border.color: "black"
					width: sampleHighlightBlue.width + Kirigami.Units.gridUnit
					height: Kirigami.Units.gridUnit * 2
					Text {
						id: sampleHighlightBlue
						text: qsTr("Highlight")
						color: subsurfaceTheme.bluePrimaryTextColor
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
			}
			RadioButton {
				id: bluebutton
				Layout.preferredWidth: gridWidth * 0.125
				Layout.alignment: Qt.AlignRight
				checked: subsurfaceTheme.currentTheme === "Blue"
				onClicked: {
					blueTheme()
					manager.theme = subsurfaceTheme.currentTheme
					manager.savePreferences()
				}
				indicator: Rectangle {
					implicitWidth: 20
					implicitHeight: 20
					x: bluebutton.leftPadding
					y: parent.height / 2 - height / 2
					radius: 4
					border.color: bluebutton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
					color: subsurfaceTheme.backgroundColor

					Rectangle {
						width: 12
						height: 12
						x: 4
						y: 4
						radius: 3
						color: bluebutton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
						visible: bluebutton.checked
					}
				}
			}

			Kirigami.Label {
				text: qsTr("Pink")
				color: subsurfaceTheme.textColor
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.125
			}
			Row {
				Layout.preferredWidth: settingsPage.col2Width + settingsPage.col3Width
				Rectangle {
					id: pinkRect
					color: subsurfaceTheme.pinkBackgroundColor
					border.color: "black"
					width: sampleRegularPink.width + Kirigami.Units.gridUnit
					height: Kirigami.Units.gridUnit * 2
					Text {
						id: sampleRegularPink
						text: qsTr("regular text")
						color: subsurfaceTheme.pinkTextColor
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
				Rectangle {
					color: subsurfaceTheme.pinkPrimaryColor
					border.color: "black"
					width: sampleHighlightPink.width + Kirigami.Units.gridUnit
					height: Kirigami.Units.gridUnit * 2
					Text {
						id: sampleHighlightPink
						text: qsTr("Highlight")
						color: subsurfaceTheme.pinkPrimaryTextColor
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
			}

			RadioButton {
				id: pinkbutton
				checked: subsurfaceTheme.currentTheme === "Pink"
				Layout.preferredWidth: gridWidth * 0.125
				Layout.alignment: Qt.AlignRight
				onClicked: {
					pinkTheme()
					manager.theme = subsurfaceTheme.currentTheme
					manager.savePreferences()
				}
				indicator: Rectangle {
					implicitWidth: 20
					implicitHeight: 20
					x: pinkbutton.leftPadding
					y: parent.height / 2 - height / 2
					radius: 4
					border.color: pinkbutton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
					color: subsurfaceTheme.backgroundColor

					Rectangle {
						width: 12
						height: 12
						x: 4
						y: 4
						radius: 3
						color: pinkbutton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
						visible: pinkbutton.checked
					}
				}
			}

			Kirigami.Label {
				text: qsTr("Dark")
				color: subsurfaceTheme.textColor
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.125
			}
			Row {
				Layout.preferredWidth: settingsPage.col2Width + settingsPage.col3Width
				Rectangle {
					id: blackRect
					color: subsurfaceTheme.darkBackgroundColor
					border.color: "black"
					width: sampleRegularDark.width + Kirigami.Units.gridUnit
					height: Kirigami.Units.gridUnit * 2
					Text {
						id: sampleRegularDark
						text: qsTr("regular text")
						color: subsurfaceTheme.darkTextColor
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
				Rectangle {
					color: subsurfaceTheme.darkPrimaryColor
					border.color: "black"
					width: sampleHighlightDark.width + Kirigami.Units.gridUnit
					height: Kirigami.Units.gridUnit * 2
					Text {
						id: sampleHighlightDark
						text: qsTr("Highlight")
						color: subsurfaceTheme.darkPrimaryTextColor
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
			}
			RadioButton {
				id: darkbutton
				checked: subsurfaceTheme.currentTheme === "Dark"
				Layout.preferredWidth: gridWidth * 0.125
				Layout.alignment: Qt.AlignRight
				onClicked: {
					darkTheme()
					manager.theme = subsurfaceTheme.currentTheme
					manager.savePreferences()
				}
				indicator: Rectangle {
					implicitWidth: 20
					implicitHeight: 20
					x: darkbutton.leftPadding
					y: parent.height / 2 - height / 2
					radius: 4
					border.color: darkbutton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
					color: subsurfaceTheme.backgroundColor

					Rectangle {
						width: 12
						height: 12
						x: 4
						y: 4
						radius: 3
						color: darkbutton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
						visible: darkbutton.checked
					}
				}
			}
		}
		Rectangle {
			color: subsurfaceTheme.darkerPrimaryColor
			height: 1
			opacity: 0.5
			Layout.fillWidth: true
		}
		GridLayout {
			id: gpsPrefs
			columns: 2
			width: parent.width

			Kirigami.Heading {
				text: qsTr("Subsurface GPS data webservice")
				color: subsurfaceTheme.textColor
				level: 4
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Kirigami.Label {
				text: qsTr("Distance threshold (meters)")
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth:gridWidth * 0.75
			}

			TextField {
				id: distanceThreshold
				text: manager.distanceThreshold
				Layout.preferredWidth: gridWidth * 0.25
				onEditingFinished: {
					manager.distanceThreshold = distanceThreshold.text
					manager.savePreferences()
				}
			}

			Kirigami.Label {
				text: qsTr("Time threshold (minutes)")
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.75
			}

			TextField {
				id: timeThreshold
				text: manager.timeThreshold
				Layout.preferredWidth: gridWidth * 0.25
				onEditingFinished: {
					manager.timeThreshold = timeThreshold.text
					manager.savePreferences()
				}
			}

		}
		Rectangle {
			color: subsurfaceTheme.darkerPrimaryColor
			height: 1
			opacity: 0.5
			Layout.fillWidth: true
		}
		GridLayout {
			id: libdclogprefs
			columns: 2
			width: parent.width
			Kirigami.Heading {
				text: qsTr("Dive computer")
				color: subsurfaceTheme.textColor
				level: 4
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Kirigami.Label {
				text: qsTr("Save detailed log")
				Layout.preferredWidth: gridWidth * 0.75
			}
			Switch {
				id: libdclogButton
				checked: manager.libdcLog
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.libdcLog = checked
				}
				indicator: Rectangle {
					implicitWidth: Kirigami.Units.largeSpacing * 3
					implicitHeight: Kirigami.Units.largeSpacing
					x: libdclogButton.leftPadding
					y: parent.height / 2 - height / 2
					radius: Kirigami.Units.largeSpacing * 0.5
					color: libdclogButton.checked ?
						subsurfaceTheme.lightPrimaryColor : subsurfaceTheme.backgroundColor
					border.color: subsurfaceTheme.darkerPrimaryColor

					Rectangle {
						x: libdclogButton.checked ? parent.width - width : 0
						y: parent.height / 2 - height / 2
						width: Kirigami.Units.largeSpacing * 1.5
						height: Kirigami.Units.largeSpacing * 1.5
						radius: height / 2
						color: libdclogButton.down || libdclogButton.checked ?
							subsurfaceTheme.primaryColor : subsurfaceTheme.lightPrimaryColor
						border.color: subsurfaceTheme.darkerPrimaryColor
					}
				}
			}
		}
		Rectangle {
			color: subsurfaceTheme.darkerPrimaryColor
			height: 1
			opacity: 0.5
			Layout.fillWidth: true
		}
		GridLayout {
			id: developer
			columns: 2
			width: parent.width - Kirigami.Units.gridUnit
			Kirigami.Heading {
				text: qsTr("Developer")
				color: subsurfaceTheme.textColor
				level: 4
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Kirigami.Label {
				text: qsTr("Display Developer menu")
				Layout.preferredWidth: gridWidth * 0.75
			}
			Switch {
				id: developerButton
				checked: manager.developer
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.developer = checked
				}
				indicator: Rectangle {
					implicitWidth: Kirigami.Units.largeSpacing * 3
					implicitHeight: Kirigami.Units.largeSpacing
					x: developerButton.leftPadding
					y: parent.height / 2 - height / 2
					radius: Kirigami.Units.largeSpacing * 0.5
					color: developerButton.checked ?
						subsurfaceTheme.lightPrimaryColor : subsurfaceTheme.backgroundColor
					border.color: subsurfaceTheme.darkerPrimaryColor

					Rectangle {
						x: developerButton.checked ? parent.width - width : 0
						y: parent.height / 2 - height / 2
						width: Kirigami.Units.largeSpacing * 1.5
						height: Kirigami.Units.largeSpacing * 1.5
						radius: height / 2
						color: developerButton.down || developerButton.checked ?
							subsurfaceTheme.primaryColor : subsurfaceTheme.lightPrimaryColor
						border.color: subsurfaceTheme.darkerPrimaryColor
					}
				}
			}
		}
		Item {
			height: Kirigami.Units.gridUnit * 6
		}
	}
}
