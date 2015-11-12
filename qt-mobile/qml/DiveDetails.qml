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


			GridLayout {
				id: editorDetails
				width: parent.width
				columns: 2

				Label {
					Layout.columnSpan: 2
					font.pointSize: units.titlePointSize
					text: "Dive " + number + " (" + date + ")"
				}

				Item {
					Layout.columnSpan: 2
					Layout.fillWidth: true
					Layout.preferredHeight: qmlProfile.visible ? qmlProfile.height : profileHideButton.height
					QMLProfile {
						id: qmlProfile
						height: units.gridUnit * 25
						anchors {
							top: parent.top
							left: parent.left
							right: parent.right
						}
						//Rectangle { color: "green"; opacity: 0.4; anchors.fill: parent } // used for debugging the dive profile sizing, will be removed later
					}
					Button {
						id: profileHideButton
						anchors {
							right: parent.right
							top: parent.top
						}
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
				}
				Label {
					text: "Location:"
				}
				TextField {
					id: txtLocation; text: location;
					Layout.fillWidth: true
				}

				Label {
					text: "Air Temp:"
				}
				TextField {
					id: txtAirTemp
					text: airtemp
					Layout.fillWidth: true
				}

				Label {
					text: "Water Temp:"
				}
				TextField {
					id: txtWaterTemp
					text: watertemp
					Layout.fillWidth: true
				}

				Label {
					text: "Suit:"

				}
				TextField {
					id: txtSuit
					text: suit
					Layout.fillWidth: true
				}

				Label {
					text: "Buddy:"
				}
				TextField {
					id: txtBuddy
					text: buddy
					Layout.fillWidth: true
				}

				Label {
					text: "Dive Master:"
				}
				TextField {
					id: txtDiveMaster
					text: divemaster
					Layout.fillWidth: true
				}

				Label {
					text: "Notes:"
				}
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
