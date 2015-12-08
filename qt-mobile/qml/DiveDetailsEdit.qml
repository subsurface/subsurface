import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

Item {

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
				onEditingFinished: {
					suit = text;
				}
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Buddy:"
			}
			TextField {
				id: txtBuddy
				text: buddy
				Layout.fillWidth: true
				onEditingFinished: {
					buddy = text;
				}
			}

			MobileComponents.Label {
				Layout.alignment: Qt.AlignRight
				text: "Dive Master:"
			}
			TextField {
				id: txtDiveMaster
				text: divemaster
				Layout.fillWidth: true
				onEditingFinished: {
					divemaster = text;
				}
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
// there is no onEditingFinished signal... not sure how to get the value
// out of this field when we're done editing
//				onEditingFinished: {
//					diveDetailsWindow.notes = text;
//				}
			}
		}
		Item {
			height: MobileComponents.Units.gridUnit * 3
			width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
		}
	}
}
