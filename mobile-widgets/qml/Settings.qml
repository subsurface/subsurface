// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.2 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	objectName: "Settings"
	id: settingsPage
	property alias defaultCylinderModel: defaultCylinderBox.model
	property alias defaultCylinderIndex: defaultCylinderBox.currentIndex

	title: qsTr("Settings")
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	property real gridWidth: settingsPage.width - Kirigami.Units.gridUnit
	property var describe: [qsTr("Undefined"),
		qsTr("Incorrect username/password combination"),
		qsTr("Credentials need to be verified"),
		qsTr("Credentials verified"),
		qsTr("No cloud mode")]

	ColumnLayout {
		width: gridWidth
		GridLayout {
			id: cloudSetting
			columns: 3
			Layout.bottomMargin: Kirigami.Units.gridUnit

			Kirigami.Heading {
				text: qsTr("Cloud status")
				color: subsurfaceTheme.textColor
				level: 4
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 3
			}
			Controls.Label {
				text: qsTr("Email")
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.15
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2
			}
			Controls.Label {
				text: prefs.credentialStatus === CloudStatus.CS_NOCLOUD ? qsTr("Not applicable") : PrefCloudStorage.cloud_storage_email
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.60
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2
			}
			SsrfButton {
				id: changeCloudSettings
				Layout.alignment: Qt.AlignRight
				text: qsTr("Change")
				onClicked: {
					prefs.cancelCredentialsPinSetup()
					rootItem.returnTopPage()
				}
			}
			Controls.Label {
				text: qsTr("Status")
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.15
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2
			}
			Controls.Label {
				text: describe[prefs.credentialStatus]
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.60
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2
			}
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
			Controls.Label {
				text: qsTr("Blue")
				color: subsurfaceTheme.textColor
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.15
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
			SsrfSwitch {
				id: blueButton
				Layout.preferredWidth: gridWidth * 0.25
				checked: subsurfaceTheme.currentTheme === "Blue"
				enabled: subsurfaceTheme.currentTheme !== "Blue"
				onClicked: {
					blueTheme()
					PrefDisplay.theme = subsurfaceTheme.currentTheme
				}
			}

			Controls.Label {
				id: pinkLabel
				text: qsTr("Pink")
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.15
			}
			Row {
				Layout.preferredWidth: gridWidth * 0.6
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

			SsrfSwitch {
				id: pinkButton
				Layout.preferredWidth: gridWidth * 0.25
				checked: subsurfaceTheme.currentTheme === "Pink"
				enabled: subsurfaceTheme.currentTheme !== "Pink"
				onClicked: {
					pinkTheme()
					PrefDisplay.theme = subsurfaceTheme.currentTheme
				}
			}

			Controls.Label {
				text: qsTr("Dark")
				color: subsurfaceTheme.textColor
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.15
			}
			Row {
				Layout.preferredWidth: gridWidth * 0.6
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
			SsrfSwitch {
				id: darkButton
				Layout.preferredWidth: gridWidth * 0.25
				checked: subsurfaceTheme.currentTheme === "Dark"
				enabled: subsurfaceTheme.currentTheme !== "Dark"
				onClicked: {
					darkTheme()
					PrefDisplay.theme = subsurfaceTheme.currentTheme
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

			Controls.Label {
				text: qsTr("Distance threshold (meters)")
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.75
			}

			Controls.TextField {
				id: distanceThreshold
				text: PrefLocationService.distance_threshold
				Layout.preferredWidth: gridWidth * 0.25
				onEditingFinished: {
					PrefLocationService.distance_threshold = distanceThreshold.text
				}
			}

			Controls.Label {
				text: qsTr("Time threshold (minutes)")
				Layout.alignment: Qt.AlignRight
				Layout.preferredWidth: gridWidth * 0.75
			}

			Controls.TextField {
				id: timeThreshold
				text: PrefLocationService.time_threshold / 60
				Layout.preferredWidth: gridWidth * 0.25
				onEditingFinished: {
					PrefLocationService.time_threshold = timeThreshold.text * 60
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
			id: defaultCylinder
			columns: 2
			width: parent.width - Kirigami.Units.gridUnit

			Kirigami.Heading {
				text: qsTr("Default Cylinder")
				color: subsurfaceTheme.textColor
				level: 4
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}
			Controls.Label {
				Layout.alignment: Qt.AlignRight
				text: qsTr("Cylinder:")
				font.pointSize: subsurfaceTheme.smallPointSize
			}
			Controls.ComboBox {
				id: defaultCylinderBox
				flat: true
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
					PrefGeneral.default_cylinder = defaultCylinderBox.currentText
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

			Controls.Label {
				text: qsTr("Display Developer menu")
				Layout.preferredWidth: gridWidth * 0.75
			}
			SsrfSwitch {
				id: developerButton
				checked: PrefDisplay.show_developer
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					PrefDisplay.show_developer = checked
				}
			}
		}
		Item {
			height: Kirigami.Units.gridUnit * 6
		}
	}
}
