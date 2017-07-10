// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.Page {

	title: qsTr("Preferences")
	actions {
		main: Kirigami.Action {
			text: qsTr("Save")
			iconName: "document-save"
			onTriggered: {
				manager.distanceThreshold = distanceThreshold.text
				manager.timeThreshold = timeThreshold.text
				manager.theme = subsurfaceTheme.currentTheme
				manager.savePreferences()
				stackView.pop()
			}
		}
	}

	GridLayout {
		id: themePrefs
		columns: 2
		width: parent.width - Kirigami.Units.gridUnit
		anchors {
			top: parent.top
			left: parent.left
			right: parent.right
			margins: Kirigami.Units.gridUnit / 2
		}

		Kirigami.Heading {
			text: qsTr("Preferences")
			color: subsurfaceTheme.textColor
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
			Layout.columnSpan: 2
		}

		Kirigami.Heading {
			text: qsTr("Theme")
			color: subsurfaceTheme.textColor
			level: 3
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
			Layout.columnSpan: 2
		}

		RadioButton {
			id: bluebutton
			checked: subsurfaceTheme.currentTheme === "Blue"
			onClicked: {
				blueTheme()
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
		Row {
			Label {
				text: qsTr("Blue")
				color: subsurfaceTheme.textColor
				anchors.verticalCenter: blueRect.verticalCenter
				rightPadding: Kirigami.Units.gridUnit
			}
			Rectangle {
				id: blueRect
				color: subsurfaceTheme.blueBackgroundColor
				border.color: "black"
				width: sampleRegularBlue.width + 2 * Kirigami.Units.gridUnit
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
				width: sampleHighlightBlue.width + 2 * Kirigami.Units.gridUnit
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
			id: pinkbutton
			checked: subsurfaceTheme.currentTheme === "Pink"
			onClicked: {
				pinkTheme()
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
		Row {
			Label {
				text: qsTr("Pink")
				color: subsurfaceTheme.textColor
				anchors.verticalCenter: pinkRect.verticalCenter
				rightPadding: Kirigami.Units.gridUnit
			}
			Rectangle {
				id: pinkRect
				color: subsurfaceTheme.pinkBackgroundColor
				border.color: "black"
				width: sampleRegularPink.width + 2 * Kirigami.Units.gridUnit
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
				width: sampleHighlightPink.width + 2 * Kirigami.Units.gridUnit
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
			id: darkbutton
			checked: subsurfaceTheme.currentTheme === "Dark"
			onClicked: {
				darkTheme()
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
		Row {
			Label {
				text: qsTr("Dark")
				color: subsurfaceTheme.textColor
				anchors.verticalCenter: blackRect.verticalCenter
				rightPadding: Kirigami.Units.gridUnit
			}
			Rectangle {
				id: blackRect
				color: subsurfaceTheme.darkBackgroundColor
				border.color: "black"
				width: sampleRegularDark.width + 2 * Kirigami.Units.gridUnit
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
				width: sampleHighlightDark.width + 2 * Kirigami.Units.gridUnit
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
	}
	GridLayout {
		id: gpsPrefs
		columns: 2
		width: parent.width - Kirigami.Units.gridUnit
		anchors {
			top: themePrefs.bottom
			margins: Kirigami.Units.gridUnit / 2
		}

		Kirigami.Heading {
			text: qsTr("Subsurface GPS data webservice")
			color: subsurfaceTheme.textColor
			level: 3
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
			Layout.columnSpan: 2
		}

		Kirigami.Label {
			text: qsTr("Distance threshold (meters)")
			Layout.alignment: Qt.AlignRight
		}

		TextField {
			id: distanceThreshold
			text: manager.distanceThreshold
			Layout.fillWidth: true
		}

		Kirigami.Label {
			text: qsTr("Time threshold (minutes)")
			Layout.alignment: Qt.AlignRight
		}

		TextField {
			id: timeThreshold
			text: manager.timeThreshold
			Layout.fillWidth: true
		}

		Item {
			Layout.fillHeight: true
		}
	}
	GridLayout {
		columns: 2
		width: parent.width - Kirigami.Units.gridUnit
		anchors {
			top: gpsPrefs.bottom
			margins: Kirigami.Units.gridUnit / 2
		}
		Kirigami.Heading {
			text: qsTr("Debug log for download from divecomputer")
			color: subsurfaceTheme.textColor
			level: 3
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
			Layout.columnSpan: 2
		}

		CheckBox {
			id: libdclogButton
			checked: manager.libdcLog
			onClicked: {
				manager.libdcLog = checked
			}
			indicator: Rectangle {
				implicitWidth: 20
				implicitHeight: 20
				x: libdclogButton.leftPadding
				y: parent.height / 2 - height / 2
				radius: 4
				border.color: libdclogButton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
				color: subsurfaceTheme.backgroundColor

				Rectangle {
				    width: 12
				    height: 12
				    x: 4
				    y: 4
				    radius: 3
				    color: libdclogButton.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
				    visible: libdclogButton.checked
				}
			}
		}
		Kirigami.Label {
			text: qsTr("Save detailed log of interaction with the dive computer")
		}

	}
}
