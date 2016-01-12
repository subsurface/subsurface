import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

Item {
	id: detailsEdit
	property int dive_id
	property int number
	property alias dateText: txtDate.text
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
			margins: MobileComponents.Units.gridUnit
		}
		spacing: MobileComponents.Units.smallSpacing


		GridLayout {
			id: editorDetails
			width: parent.width
			columns: 2

			MobileComponents.Heading {
				Layout.columnSpan: 2
				text: "Dive " + number
			}
			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Date:"
			}
			TextField {
				id: txtDate;
				Layout.fillWidth: true
			}
			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Location:"
			}
			TextField {
				id: txtLocation;
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
				Layout.fillWidth: true
			}
			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Duration:"
			}
			TextField {
				id: txtDuration
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Air Temp:"
			}
			TextField {
				id: txtAirTemp
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Water Temp:"
			}
			TextField {
				id: txtWaterTemp
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Suit:"
			}
			TextField {
				id: txtSuit
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Buddy:"
			}
			TextField {
				id: txtBuddy
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Dive Master:"
			}
			TextField {
				id: txtDiveMaster
				Layout.fillWidth: true
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Notes:"
			}
			TextArea {
				id: txtNotes
				textFormat: TextEdit.RichText
				focus: true
				Layout.fillWidth: true
				Layout.fillHeight: true
				Layout.minimumHeight: MobileComponents.Units.gridUnit * 6
				selectByMouse: true
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			}
		}
		Button {
			anchors.horizontalCenter: parent.horizontalCenter
			text: "Save"
			onClicked: {
				// apply the changes to the dive_table
				manager.commitChanges(dive_id, detailsEdit.dateText, detailsEdit.locationText, detailsEdit.gpsText, detailsEdit.durationText,
						      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText, detailsEdit.suitText,
						      detailsEdit.buddyText, detailsEdit.divemasterText, detailsEdit.notesText)
				// apply the changes to the dive detail view
				diveListView.currentItem.modelData.date = detailsEdit.dateText
				diveListView.currentItem.modelData.location = detailsEdit.locationText
				diveListView.currentItem.modelData.duration = detailsEdit.durationText
				diveListView.currentItem.modelData.depth = detailsEdit.depthText
				diveListView.currentItem.modelData.airtemp = detailsEdit.airtempText
				diveListView.currentItem.modelData.watertemp = detailsEdit.watertempText
				diveListView.currentItem.modelData.suit = detailsEdit.suitText
				diveListView.currentItem.modelData.buddy = detailsEdit.buddyText
				diveListView.currentItem.modelData.divemaster = detailsEdit.divemasterText
				diveListView.currentItem.modelData.notes = detailsEdit.notesText
				editDrawer.close()
			}
		}
		Item {
			height: MobileComponents.Units.gridUnit * 3
			width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
		}
	}
}
