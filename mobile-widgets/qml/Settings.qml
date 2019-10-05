// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.4 as Kirigami
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

			Controls.Label {
				text: qsTr("Cloud status")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 3
			}
			Controls.Label {
				text: qsTr("Email")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.15
				color: subsurfaceTheme.textColor
			}
			Controls.Label {
				text: prefs.credentialStatus === CloudStatus.CS_NOCLOUD ? qsTr("Not applicable") : prefs.cloudUserName
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.60
				color: subsurfaceTheme.textColor
			}
			SsrfButton {
				id: changeCloudSettings
				text: qsTr("Change")
				onClicked: {
					prefs.cancelCredentialsPinSetup()
					rootItem.returnTopPage()
				}
			}
			Controls.Label {
				text: qsTr("Status")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.15
				Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
				color: subsurfaceTheme.textColor
			}
			Controls.Label {
				text: describe[prefs.credentialStatus]
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.60
				Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
				color: subsurfaceTheme.textColor
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

			Controls.Label {
				text: qsTr("Theme")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 3
			}
			Controls.Label {
				text: qsTr("Blue")
				font.pointSize: subsurfaceTheme.regularPointSize
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
						font.pointSize: subsurfaceTheme.regularPointSize
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
						font.pointSize: subsurfaceTheme.regularPointSize
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
				font.pointSize: subsurfaceTheme.regularPointSize
				rightPadding: Kirigami.Units.gridUnit
				Layout.preferredWidth: gridWidth * 0.15
				color: subsurfaceTheme.textColor
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
						font.pointSize: subsurfaceTheme.regularPointSize
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
						font.pointSize: subsurfaceTheme.regularPointSize
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
				font.pointSize: subsurfaceTheme.regularPointSize
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
						font.pointSize: subsurfaceTheme.regularPointSize
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
						font.pointSize: subsurfaceTheme.regularPointSize
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
			Controls.Label {
				text: qsTr("Scaling")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 3
			}
			Row {
				Layout.preferredWidth: gridWidth * 0.8
				Layout.columnSpan: 3
				spacing: Kirigami.Units.largeSpacing
				SsrfButton {
					text: qsTr("smaller")
					enabled: PrefDisplay.mobile_scale !== 0.85
					onClicked: {
						PrefDisplay.mobile_scale = 0.85
						fontMetrics.font.pointSize = subsurfaceTheme.basePointSize * PrefDisplay.mobile_scale;
					}
				}
				SsrfButton {
					text: qsTr("regular")
					enabled: PrefDisplay.mobile_scale !== 1.0
					onClicked: {
						PrefDisplay.mobile_scale = 1.0
						fontMetrics.font.pointSize = subsurfaceTheme.basePointSize * PrefDisplay.mobile_scale;
					}
				}
				SsrfButton {
					text: qsTr("larger")
					enabled: PrefDisplay.mobile_scale !== 1.15
					onClicked: {
						PrefDisplay.mobile_scale = 1.15
						fontMetrics.font.pointSize = subsurfaceTheme.basePointSize * PrefDisplay.mobile_scale;
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

			Controls.Label {
				text: qsTr("GPS location service")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Controls.Label {
				text: qsTr("Distance threshold (meters)")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}

			Controls.TextField {
				id: distanceThreshold
				text: PrefLocationService.distance_threshold
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.25
				color: subsurfaceTheme.textColor
				onEditingFinished: {
					PrefLocationService.distance_threshold = distanceThreshold.text
				}
			}

			Controls.Label {
				text: qsTr("Time threshold (minutes)")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}

			Controls.TextField {
				id: timeThreshold
				text: PrefLocationService.time_threshold / 60
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.25
				color: subsurfaceTheme.textColor
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
			Layout.rightMargin: Kirigami.Units.gridUnit * 1.5

			Controls.Label {
				text: qsTr("Default Cylinder")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}
			Controls.Label {
				text: qsTr("Cylinder:")
				font.pointSize: subsurfaceTheme.regularPointSize
				color: subsurfaceTheme.textColor
			}
			Controls.ComboBox {
				id: defaultCylinderBox
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredHeight: fontMetrics.height * 2.5
				inputMethodHints: Qt.ImhNoPredictiveText
				Layout.fillWidth: true
				onActivated: {
					PrefGeneral.set_default_cylinder(defaultCylinderBox.currentText)
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
			id: divecomputers
			columns: 2
			Controls.Label {
				text: qsTr("Dive computers")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing
				Layout.columnSpan: 2
			}
			Controls.Label {
				text: qsTr("Forget remembered dive computers")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}
			SsrfButton {
				id: forgetDCButton
				text: qsTr("Forget")
				enabled: PrefDiveComputer.vendor1 !== ""
				onClicked: {
					PrefDiveComputer.vendor1 = PrefDiveComputer.product1 = PrefDiveComputer.device1 = ""
					PrefDiveComputer.vendor2 = PrefDiveComputer.product2 = PrefDiveComputer.device2 = ""
					PrefDiveComputer.vendor3 = PrefDiveComputer.product3 = PrefDiveComputer.device3 = ""
					PrefDiveComputer.vendor4 = PrefDiveComputer.product4 = PrefDiveComputer.device4 = ""
					PrefDiveComputer.vendor = PrefDiveComputer.product = PrefDiveComputer.device = ""
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
			id: unit_system
			columns: 2
			Controls.Label {
				text: qsTr("Units")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Controls.Label {
				text: qsTr("Use Imperial Units")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}
			SsrfSwitch {
				id: imperialButton
				checked: PrefUnits.unit_system === "imperial"
				enabled: PrefUnits.unit_system === "metric"
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					PrefUnits.set_unit_system("imperial")
					manager.changesNeedSaving()
					manager.refreshDiveList()
				}
			}
			Controls.Label {
				text: qsTr("Use Metric Units")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}
			SsrfSwitch {
				id: metricButtton
				checked: PrefUnits.unit_system === "metric"
				enabled: PrefUnits.unit_system === "imperial"
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					PrefUnits.set_unit_system("metric")
					manager.changesNeedSaving()
					manager.refreshDiveList()
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
			id: filterPrefs
			columns: 2

			Controls.Label {
				text: qsTr("Filter preferences")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}
			Controls.Label {
				text: qsTr("Include notes in full text filtering")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}

			SsrfSwitch {
				id: fullTextNotes
				checked: PrefGeneral.filterFullTextNotes
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					PrefGeneral.set_filterFullTextNotes(checked)
				}
			}

			Controls.Label {
				text: qsTr("Match filter case sensitive")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}

			SsrfSwitch {
				id: filterCaseSensitive
				checked: PrefGeneral.filterCaseSensitive
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					PrefGeneral.set_filterCaseSensitive(checked)
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
			id: whichBluetoothDevices
			columns: 2
			Controls.Label {
				text: qsTr("Bluetooth")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Controls.Label {
				text: qsTr("Show all bluetooth devices \neven if not recognized as dive computers")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
			}
			SsrfSwitch {
				id: nonDCButton
				checked: manager.showNonDiveComputers
				Layout.preferredWidth: gridWidth * 0.25
				onClicked: {
					manager.showNonDiveComputers = checked
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
			Controls.Label {
				text: qsTr("Developer")
				font.pointSize: subsurfaceTheme.headingPointSize
				font.weight: Font.Light
				color: subsurfaceTheme.textColor
				Layout.topMargin: Kirigami.Units.largeSpacing
				Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
				Layout.columnSpan: 2
			}

			Controls.Label {
				text: qsTr("Display Developer menu")
				font.pointSize: subsurfaceTheme.regularPointSize
				Layout.preferredWidth: gridWidth * 0.75
				color: subsurfaceTheme.textColor
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
