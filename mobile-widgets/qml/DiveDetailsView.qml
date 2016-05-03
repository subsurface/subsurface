import QtQuick 2.3
/*
import QtWebView 1.0
*/
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

Item {
	id: detailsView
	property real gridWidth: detailsView.width - 2 * Kirigami.Units.gridUnit
	property real col1Width: gridWidth * 0.23
	property real col2Width: gridWidth * 0.37
	property real col3Width: gridWidth * 0.20
	property real col4Width: gridWidth * 0.20

	width: diveDetailsPage.width - diveDetailsPage.leftPadding - diveDetailsPage.rightPadding
	height: mainLayout.implicitHeight + bottomLayout.implicitHeight + Kirigami.Units.iconSizes.large
	Rectangle {
		z: 99
		color: Kirigami.Theme.textColor
		opacity: 0.3
		width: Kirigami.Units.smallSpacing/4
		anchors {
			right: parent.right
			top: parent.top
			bottom: parent.bottom
		}
	}
	GridLayout {
		id: mainLayout
		anchors {
			top: parent.top
			left: parent.left
			right: parent.right
			margins: Kirigami.Units.gridUnit
		}
		columns: 4
		rowSpacing: Kirigami.Units.smallSpacing * 2
		columnSpacing: Kirigami.Units.smallSpacing

		Kirigami.Heading {
			id: detailsViewHeading
			Layout.fillWidth: true
			text: dive.location
			font.underline: dive.gps !== ""
			Layout.columnSpan: 4
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.topMargin: Kirigami.Units.largeSpacing
			MouseArea {
				anchors.fill: parent
				onClicked: {
					if (dive.gps !== "")
						showMap(dive.gps)
				}
			}
		}
		Kirigami.Label {
			id: dateLabel
			text: qsTr("Date: ")
			opacity: 0.6
		}
		Kirigami.Label {
			text: dive.date + " " + dive.time
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 2
		}
		Kirigami.Label {
			id: numberText
			text: "#" + dive.number
			color: Kirigami.Theme.textColor
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		Kirigami.Label {
			id: depthLabel
			text: qsTr("Depth: ")
			opacity: 0.6
		}
		Kirigami.Label {
			text: dive.depth
			Layout.fillWidth: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}
		Kirigami.Label {
			text: qsTr("Duration: ")
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			text: dive.duration
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		QMLProfile {
			id: qmlProfile
			visible: !dive.noDive
			Layout.fillWidth: true
			Layout.preferredHeight: Layout.minimumHeight
			Layout.minimumHeight: width * 0.75
			Layout.columnSpan: 4
			clip: false
			Rectangle {
				color: "transparent"
				opacity: 0.6
				border.width: 1
				border.color: Kirigami.Theme.textColor;
				anchors.fill: parent
			}
		}
		Kirigami.Label {
			id: noProfile
			visible: dive.noDive
			Layout.fillWidth: true
			Layout.columnSpan: 4
			Layout.margins: Kirigami.Units.gridUnit
			horizontalAlignment: Text.AlignHCenter
			text: qsTr("No profile to show")
		}
	}
	GridLayout {
		id: bottomLayout
		anchors {
			top: mainLayout.bottom
			left: parent.left
			right: parent.right
			margins: Math.round(Kirigami.Units.gridUnit / 2)
		}
		columns: 4
		rowSpacing: Kirigami.Units.smallSpacing * 2
		columnSpacing: Kirigami.Units.smallSpacing

		Kirigami.Heading {
			Layout.fillWidth: true
			level: 3
			text: qsTr("Dive Details")
			Layout.columnSpan: 4
		}

		Kirigami.Label {
			text: qsTr("Suit:")
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			width: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtSuit
			text: dive.suit
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.fillWidth: true
		}

		Kirigami.Label {
			text: qsTr("Air Temp:")
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			width: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtAirTemp
			text: dive.airTemp
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			width: detailsView.col4Width
		}

		Kirigami.Label {
			text: qsTr("Cylinder:")
			opacity: 0.6
			width: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtCylinder
			text: dive.getCylinder
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.fillWidth: true
		}

		Kirigami.Label {
			text: qsTr("Water Temp:")
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			width: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtWaterTemp
			text: dive.waterTemp
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			width: detailsView.col4Width
		}

		Kirigami.Label {
			text: qsTr("Dive Master:")
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			width: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtDiveMaster
			text: dive.divemaster
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.fillWidth: true
		}

		Kirigami.Label {
			text: qsTr("Weight:")
			opacity: 0.6
			width: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtWeight
			text: dive.sumWeight
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			width: detailsView.col4Width
		}

		Kirigami.Label {
			text: qsTr("Buddy:")
			opacity: 0.6
			width: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtBuddy
			text: dive.buddy
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.fillWidth: true
		}

		Kirigami.Label {
			text: qsTr("SAC:")
			opacity: 0.6
			width: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtSAC
			text: dive.sac
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			width: detailsView.col4Width
		}

		Kirigami.Heading {
			Layout.fillWidth: true
			level: 3
			text: qsTr("Notes")
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 4
		}

		Kirigami.Label {
			id: txtNotes
			text: dive.notes
			focus: true
			Layout.columnSpan: 4
			Layout.fillWidth: true
			//selectByMouse: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}
		Item {
			Layout.columnSpan: 4
			Layout.fillWidth: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 3
		}
		Component.onCompleted: {
			qmlProfile.setMargin(Kirigami.Units.smallSpacing)
			qmlProfile.diveId = model.dive.id;
			qmlProfile.update();
		}
	}
}
