import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

Item {
	property alias locationText: txtLocation.text
	property string gpsText
	property alias airtempText: txtAirTemp.text
	property alias watertempText: txtWaterTemp.text
	property alias suitText: txtSuit.text
	property alias buddyText: txtBuddy.text
	property alias divemasterText: txtDiveMaster.text
	property alias notesText: txtNotes.text
	property alias durationText: txtDuration.text
	property alias depthText: txtDepth.text
	ColumnLayout {
		anchors {
			left: parent.left
			right: parent.right
			top: parent.top
		}
		spacing: MobileComponents.Units.smallSpacing


		GridLayout {
			id: editorDetails
			width: parent.width
			columns: 2

			MobileComponents.Heading {
				Layout.columnSpan: 2
				text: "Dive " + number + " (" + date + ")"
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Location:"
			}
			TextField {
				id: txtLocation; text: location;
				Layout.fillWidth: true
			}

			// we should add a checkbox here that allows the user
			// to add the current location as the dive location
			// (think of someone adding a dive while on the boat or
			//  at the dive site)
			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Use current\nGPS location:"
			}
			CheckBox {
				id: checkboxGPS
				onCheckedChanged: {
					if (checked)
						gpsText = manager.getCurrentPosition()
				}
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Depth:"
			}
			TextField {
				id: txtDepth
				text: depth
				Layout.fillWidth: true
			}
			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Duration:"
			}
			TextField {
				id: txtDuration
				text: duration
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Air Temp:"
			}
			TextField {
				id: txtAirTemp
				text: airtemp
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Water Temp:"
			}
			TextField {
				id: txtWaterTemp
				text: watertemp
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Suit:"
			}
			TextField {
				id: txtSuit
				text: suit
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Buddy:"
			}
			TextField {
				id: txtBuddy
				text: buddy
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Dive Master:"
			}
			TextField {
				id: txtDiveMaster
				text: divemaster
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Notes:"
			}
			TextArea {
				id: txtNotes
				text: notes
				focus: true
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.minimumHeight: MobileComponents.Units.gridUnit * 6
				selectByMouse: true
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			}
		}
		Item {
			height: MobileComponents.Units.gridUnit * 3
			width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
		}
	}
}
