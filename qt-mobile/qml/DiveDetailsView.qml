import QtQuick 2.3
/*
import QtWebView 1.0
*/
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

Item {
	id: detailsView
	property int labelWidth: MobileComponents.Units.gridUnit * 10
	width: SubsurfaceTheme.columnWidth
	height: mainLayout.implicitHeight + bottomLayout.implicitHeight + MobileComponents.Units.iconSizes.large
	Rectangle {
	    z: 99
		color: MobileComponents.Theme.textColor
		opacity: 0.3
		width: MobileComponents.Units.smallSpacing/4
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
		    margins: MobileComponents.Units.gridUnit
		}
		columns: 4
		rowSpacing: MobileComponents.Units.smallSpacing * 2
		columnSpacing: MobileComponents.Units.smallSpacing

		MobileComponents.Heading {
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
		/*
		Rectangle {
			id: mapView
			width: parent.width
			height: parents.width * 0.7
			WebView {
				id: webView
				anchors.fill: parent
				url: "http://www.google.com"
			}
		}
	*/
		MobileComponents.Label {
			id: dateLabel
			text: "Date: "
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			text: dive.date + " " + dive.time
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 2
		}
		MobileComponents.Label {
			id: numberText
			text: "#" + dive.number
			color: MobileComponents.Theme.textColor
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		MobileComponents.Label {
			id: depthLabel
			text: "Depth: "
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			text: dive.depth
			Layout.fillWidth: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}
		MobileComponents.Label {
			text: "Duration: "
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			text: dive.duration
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		QMLProfile {
			id: qmlProfile
			Layout.fillWidth: true
			Layout.preferredHeight: Layout.minimumHeight
			Layout.minimumHeight: width * 0.75
			Layout.columnSpan: 4
			clip: false
			devicePixelRatio: MobileComponents.Units.devicePixelRatio
			Rectangle {
				color: "transparent"
				opacity: 0.6
				border.width: 1
				border.color: MobileComponents.Theme.textColor;
				anchors.fill: parent
			}
		}
	}
	GridLayout {
		id: bottomLayout
		anchors {
		    top: mainLayout.bottom
		    left: parent.left
		    right: parent.right
		    margins: MobileComponents.Units.gridUnit
		}
		columns: 4
		rowSpacing: MobileComponents.Units.smallSpacing * 2
		columnSpacing: MobileComponents.Units.smallSpacing

		MobileComponents.Heading {
			Layout.fillWidth: true
			level: 3
			text: "Dive Details"
			Layout.columnSpan: 4
		}

		// first row - here we set up the column widths - total is 90% of width
		MobileComponents.Label {
			text: "Suit:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: subsurfaceTheme.columnWidth * 0.18
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtSuit
			text: dive.suit
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: subsurfaceTheme.columnWidth * 0.36
		}

		MobileComponents.Label {
			text: "Air Temp:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.maximumWidth: subsurfaceTheme.columnWidth * 0.18
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtAirTemp
			text: dive.airTemp
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: subsurfaceTheme.columnWidth * 0.18
		}

		MobileComponents.Label {
			text: "Cylinder:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtCylinder
			text: dive.getCylinder
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		MobileComponents.Label {
			text: "Water Temp:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtWaterTemp
			text: dive.waterTemp
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		MobileComponents.Label {
			text: "Dive Master:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtDiveMaster
			text: dive.divemaster
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		MobileComponents.Label {
			text: "Weight:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtWeight
			text: dive.sumWeight
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		MobileComponents.Label {
			text: "Buddy:"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			opacity: 0.6
			Layout.alignment: Qt.AlignRight
		}
		MobileComponents.Label {
			id: txtBuddy
			text: dive.buddy
			Layout.columnSpan: 3
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		MobileComponents.Heading {
			Layout.fillWidth: true
			level: 3
			text: "Notes"
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 4
		}

		MobileComponents.Label {
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
			Layout.minimumHeight: MobileComponents.Units.gridUnit * 3
		}
		Component.onCompleted: {
			qmlProfile.setMargin(MobileComponents.Units.smallSpacing)
			qmlProfile.diveId = model.dive.id;
			qmlProfile.update();
		}
	}
}
