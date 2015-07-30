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
		width: parent.width
		anchors { top: parent.top; bottom: parent.bottom }
		contentHeight: parent.height
		clip: true
		ColumnLayout {
			width: parent.width
			Rectangle {
				id: topBar
				color: "#2C4882"
				Layout.fillWidth: true
				Layout.margins: 0
				height: backButton.height * 1.2
				RowLayout {
					Button {
						id: backButton
						Layout.margins: 0.1 * height
						Layout.preferredWidth: Screen.width * 0.1
						text: "\u2190"
						style: ButtonStyle {
							background: Rectangle {
								color: "#4C68A2"
								implicitWidth: 50
							}
							label: Text {
								id: txt
								color: "white"
								font.pointSize: 24
								font.bold: true
								text: control.text
							}
						}
						onClicked: {
							manager.commitChanges(
										dive_id,
										suit,
										buddy,
										divemaster,
										notes
										)
							stackView.pop();
						}
					}
					Text {
						text: qsTr("Subsurface mobile")
						font.pointSize: 18
						font.bold: true
						color: "white"
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
