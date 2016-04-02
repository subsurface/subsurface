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
	property real gridWidth: subsurfaceTheme.columnWidth - 2 * Kirigami.Units.gridUnit
	property real col1Width: gridWidth * 0.23
	property real col2Width: gridWidth * 0.37
	property real col3Width: gridWidth * 0.20
	property real col4Width: gridWidth * 0.20

	width: SubsurfaceTheme.columnWidth
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
		    margins: Math.round(Kirigami.Units.gridUnit / 2)
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
			MouseArea {
				anchors.fill: parent
				onClicked: {
					if (dive.gps !== "")
						manager.showMap(dive.gps)
				}
			}
		}
		Kirigami.Label {
			id: dateLabel
			text: "Date: "
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
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
			text: "Depth: "
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			text: dive.depth
			Layout.fillWidth: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}
		Kirigami.Label {
			text: "Duration: "
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
			text: "No profile to show"
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
			text: "Dive Details"
			Layout.columnSpan: 4
		}

		// first row - here we set up the column widths - total is 90% of width
		Kirigami.Label {
			text: "Suit:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col1Width
			Layout.preferredWidth: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtSuit
			text: dive.suit
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.preferredWidth: detailsView.col2Width
		}

		Kirigami.Label {
			text: "Air Temp:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col3Width
			Layout.preferredWidth: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtAirTemp
			text: dive.airTemp
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col4Width
			Layout.preferredWidth: detailsView.col4Width
		}

		Kirigami.Label {
			text: "Cylinder:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col1Width
			Layout.preferredWidth: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtCylinder
			text: dive.getCylinder
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.preferredWidth: detailsView.col2Width
		}

		Kirigami.Label {
			text: "Water Temp:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col3Width
			Layout.preferredWidth: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtWaterTemp
			text: dive.waterTemp
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col4Width
			Layout.preferredWidth: detailsView.col4Width
		}

		Kirigami.Label {
			text: "Dive Master:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col1Width
			Layout.preferredWidth: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtDiveMaster
			text: dive.divemaster
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.preferredWidth: detailsView.col2Width
		}

		Kirigami.Label {
			text: "Weight:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col3Width
			Layout.preferredWidth: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtWeight
			text: dive.sumWeight
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col4Width
			Layout.preferredWidth: detailsView.col4Width
		}

		Kirigami.Label {
			text: "Buddy:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col1Width
			Layout.preferredWidth: detailsView.col1Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtBuddy
			text: dive.buddy
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.preferredWidth: detailsView.col2Width
		}

		Kirigami.Label {
			text: "SAC:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: detailsView.col3Width
			Layout.preferredWidth: detailsView.col3Width
			Layout.alignment: Qt.AlignRight
		}
		Kirigami.Label {
			id: txtSAC
			text: dive.sac
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col4Width
			Layout.preferredWidth: detailsView.col4Width
		}

		Kirigami.Heading {
			Layout.fillWidth: true
			level: 3
			text: "Notes"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 4
		}

		Kirigami.Label {
			id: txtNotes
			text: dive.notes
			focus: true
			Layout.columnSpan: 4
			Layout.fillWidth: true
			Layout.fillHeight: true
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
