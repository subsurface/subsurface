import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

ColumnLayout {
	id: detailsView

	spacing: MobileComponents.Units.smallSpacing

	Connections {
		target: diveDetailsWindow
		onDive_idChanged: {
			qmlProfile.diveId = diveDetailsWindow.dive_id
			qmlProfile.update()
		}
	}

	MobileComponents.Heading {
		id: detailsViewHeading
		Layout.columnSpan: 2
		text: location
	}
	RowLayout {
// 			anchors {
// 				left: parent.left
// 				right: parent.right
// 				bottom: numberText.bottom
// 			}
		Layout.columnSpan: 2
		MobileComponents.Label {
			text: 'Depth: '
			opacity: 0.6
		}
		MobileComponents.Label {
			text: depth
			Layout.minimumWidth: Math.max(MobileComponents.Units.gridUnit * 4, paintedWidth) // helps vertical alignment throughout listview
		}
		MobileComponents.Label {
			text: 'Duration: '
			opacity: 0.6
		}
		MobileComponents.Label {
			text: duration
		}
		Item {
			Layout.fillWidth: true
			height: parent.height
		}
		MobileComponents.Label {
			id: numberText
			text: "#" + diveNumber
			color: MobileComponents.Theme.textColor
			//opacity: 0.6
		}
	}

	Item {
		Layout.columnSpan: 2
		Layout.fillWidth: true
		Layout.preferredHeight: qmlProfile.height
		QMLProfile {
			id: qmlProfile
			//height: MobileComponents.Units.gridUnit * 25
			height: width * 0.66
			anchors {
				top: parent.top
				left: parent.left
				right: parent.right
			}
			//Rectangle { color: "green"; opacity: 0.4; anchors.fill: parent } // used for debugging the dive profile sizing, will be removed later
		}
	}
	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Location:"
	}
	MobileComponents.Label {
		id: txtLocation; text: location;
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Air Temp:"
	}
	MobileComponents.Label {
		id: txtAirTemp
		text: airtemp
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Water Temp:"
	}
	MobileComponents.Label {
		id: txtWaterTemp
		text: watertemp
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Suit:"

	}
	MobileComponents.Label {
		id: txtSuit
		text: suit
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Buddy:"
	}
	MobileComponents.Label {
		id: txtBuddy
		text: buddy
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Dive Master:"
	}
	MobileComponents.Label {
		id: txtDiveMaster
		text: divemaster
		Layout.fillWidth: true
	}

	MobileComponents.Label {
		Layout.alignment: Qt.AlignRight
		text: "Notes:"
	}
	MobileComponents.Label {
		id: txtNotes
		text: notes
		focus: true
		Layout.fillWidth: true
		Layout.fillHeight: true
		//selectByMouse: true
		wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
	}
	Item {
		height: MobileComponents.Units.gridUnit * 3
		width: height // just to make sure the spacer doesn't produce scrollbars, but also isn't null
	}
}
