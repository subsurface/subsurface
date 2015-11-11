import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0

Item {
	id: diveDetailsWindow
	width: parent.width
	objectName: "DiveDetails"

	property string location
	property string dive_id
	property string airtemp
	property string watertemp
	property string suit
	property string buddy
	property string divemaster;
	property string notes;
	property string date
	property string number

	onDive_idChanged: {
		qmlProfile.diveId = dive_id
		qmlProfile.update()
	}

	Flickable {
		id: flick
		anchors.fill: parent
		contentHeight: parent.height
		clip: true
		ColumnLayout {
			width: parent.width
			spacing: 8

			Button {
				text: "Hide Dive Profile"
				onClicked: {
					qmlProfile.visible = !qmlProfile.visible
					if (qmlProfile.visible) {
						text = "Hide Dive Profile"
					} else {
						text = "Show Dive Profile"
					}
				}
			}

			GridLayout {
				id: editorDetails
				width: parent.width
				columns: 2
				Text {
					Layout.columnSpan: 2
					Layout.alignment: Qt.AlignHCenter
					text: "Dive " + number + " (" + date + ")"; font.bold: true
				}
				QMLProfile {
					Layout.columnSpan: 2
					Layout.fillWidth: true
					id: qmlProfile
					height: 500
				}
				Text { text: "Location:"; font.bold: true }
				TextField { id: txtLocation; text: location; Layout.fillWidth: true }
				Text { text: "Air Temp:"; font.bold: true }
				TextField { id: txtAirTemp; text: airtemp; Layout.fillWidth: true }
				Text { text: "Water Temp:"; font.bold: true }
				TextField { id: txtWaterTemp; text: watertemp; Layout.fillWidth: true }
				Text { text: "Suit:"; font.bold: true }
				TextField { id: txtSuit; text: suit; Layout.fillWidth: true }
				Text { text: "Buddy:"; font.bold: true }
				TextField { id: txtBuddy; text: buddy; Layout.fillWidth: true }
				Text { text: "Dive Master:"; font.bold: true }
				TextField { id: txtDiveMaster; text: divemaster; Layout.fillWidth: true}
				Text { text: "Notes:"; font.bold: true }
				TextEdit{
					id: txtNotes
					text: notes
					focus: true
					Layout.fillWidth: true
					Layout.fillHeight: true
					selectByMouse: true
					wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				}
			}
		}
	}
}
