// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
/*
import QtWebView 1.0
*/
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0 as Controls
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami

Item {
	id: detailsView
	property real gridWidth: detailsView.width - 2 * Kirigami.Units.gridUnit
	property real col1Width: gridWidth * 0.40
	property real col2Width: gridWidth * 0.30
	property real col3Width: gridWidth * 0.30

	width: diveDetailsPage.width - diveDetailsPage.leftPadding - diveDetailsPage.rightPadding
	height: divePlate.implicitHeight + bottomLayout.implicitHeight + Kirigami.Units.iconSizes.large
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
	Item {
		id: divePlate
		width: parent.width - Kirigami.Units.gridUnit
		height: childrenRect.height - Kirigami.Units.smallSpacing
		anchors.left: parent.left
		Controls.Label {
			id: locationText
			text: dive.location
			font.weight: Font.Bold
			font.pointSize: subsurfaceTheme.titlePointSize
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			color: Kirigami.Theme.textColor
			anchors {
				left: parent.left
				top: parent.top
				right: gpsButton.left
				margins: Math.round(Kirigami.Units.gridUnit / 2)
			}
			MouseArea {
				anchors.fill: parent
				enabled: dive.gps_decimal !== ""
				onClicked: {
					if (dive.gps_decimal !== "")
						showMap(dive.gps_decimal)
				}
			}
		}
		SsrfButton {
			id: gpsButton
			anchors.right: parent.right
			enabled: dive.gps !== ""
			text: qsTr("Map it")
			onClicked: {
				if (dive.gps_decimal !== "")
					showMap(dive.gps_decimal)
			}
		}
		Row {
			id: dateRow
			anchors {
				left: locationText.left
				top: locationText.bottom
				topMargin: Kirigami.Units.smallSpacing
				bottom: numberText.bottom

			}

			Controls.Label {
				text: dive.date + " " + dive.time
				width: Math.max(locationText.width * 0.45, paintedWidth)
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			// let's try to show the depth / duration very compact
			Controls.Label {
				text: dive.depth + ' / ' + dive.duration
				width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth)
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
		}
		Controls.Label {
			id: numberText
			text: "#" + dive.number
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.textColor
			anchors {
				right: parent.right
				top: locationText.bottom
				topMargin: Kirigami.Units.smallSpacing
			}
		}
		Row {
			anchors {
				left: dateRow.left
				top: numberText.bottom
				topMargin: Kirigami.Units.smallSpacing
			}
			Controls.Label {
				id: ratingText
				text: qsTr("Rating:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter 
				source: (dive.rating >= 1) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter 
				source: (dive.rating >= 2) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter 
				source: (dive.rating >= 3) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter 
				source: (dive.rating >= 4) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter 
				source: (dive.rating === 5) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
		}
		Row {
			anchors {
				right: numberText.right
				top: numberText.bottom
				topMargin: Kirigami.Units.smallSpacing
			}
			Controls.Label {
				id: visibilityText
				text: qsTr("Visibility:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter 
				source: (dive.visibility >= 1) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter 
				source: (dive.visibility >= 2) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}	
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter 
				source: (dive.visibility >= 3) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter 
				source: (dive.visibility >= 4) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter 
				source: (dive.visibility === 5) ? "icons/ic_star.svg" : "icons/ic_star_border.svg"
			}
		}

	}
	GridLayout {
		id: bottomLayout
		anchors {
			top: divePlate.bottom
			left: parent.left
			right: parent.right
			margins: Math.round(Kirigami.Units.gridUnit / 2)
			topMargin: Kirigami.Units.gridUnit
		}
		columns: 3
		rowSpacing: Kirigami.Units.smallSpacing * 2
		columnSpacing: Kirigami.Units.smallSpacing

		QMLProfile {
			id: qmlProfile
			visible: !dive.noDive
			Layout.fillWidth: true
			Layout.preferredHeight: Layout.minimumHeight
			Layout.minimumHeight: width * 0.75
			Layout.columnSpan: 3
			clip: false
			Rectangle {
				color: "transparent"
				opacity: 0.6
				border.width: 1
				border.color: subsurfaceTheme.primaryColor
				anchors.fill: parent
			}
		}
		Controls.Label {
			id: noProfile
			visible: dive.noDive
			Layout.fillWidth: true
			Layout.columnSpan: 3
			Layout.margins: Kirigami.Units.gridUnit
			horizontalAlignment: Text.AlignHCenter
			text: qsTr("No profile to show")
		}

		// first row
		//-----------
		Controls.Label {
			text: qsTr("Suit:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
		}
		Controls.Label {
			text: qsTr("Air Temp:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
		}
		Controls.Label {
			text: qsTr("Water Temp:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
		}

		// second row
		//------------
		Controls.Label {
			id: txtSuit
			text: dive.suit
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
		}
		Controls.Label {
			id: txtAirTemp
			text: dive.airTemp
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
		}
		Controls.Label {
			id: txtWaterTemp
			text: dive.waterTemp
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
		}

		Rectangle {
			color: subsurfaceTheme.primaryColor
			height: 1
			opacity: 0.5
			Layout.columnSpan: 3
			Layout.fillWidth: true
		}

		// thrid row
		//------------
		Controls.Label {
			text: qsTr("Cylinder:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			Layout.bottomMargin: 0
		}
		Controls.Label {
			text: qsTr("Weight:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.bottomMargin: 0
		}
		Controls.Label {
			text: qsTr("SAC:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			Layout.bottomMargin: 0
		}

		// fourth row
		//------------
		Controls.Label {
			id: txtCylinder
			text: dive.getCylinder
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
		}
		Controls.Label {
			id: txtWeight
			text: dive.sumWeight
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
		}
		Controls.Label {
			id: txtSAC
			text: dive.sac
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
		}

		Rectangle {
			color: subsurfaceTheme.primaryColor
			height: 1
			opacity: 0.5
			Layout.columnSpan: 3
			Layout.fillWidth: true
		}

		// fifth row
		//-----------
		Controls.Label {
			text: qsTr("Divemaster:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			Layout.bottomMargin: 0
		}
		Controls.Label {
			text: qsTr("Buddy:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 2
			Layout.maximumWidth: detailsView.col2Width + detailsView.col3Width
			Layout.bottomMargin: 0
		}

		// sixth row
		//-----------
		Controls.Label {
			id: txtDiveMaster
			text: dive.divemaster
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
		}
		Controls.Label {
			id: txtBuddy
			text: dive.buddy
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 2
			Layout.maximumWidth: detailsView.col2Width + detailsView.col3Width
		}

		Rectangle {
			color: subsurfaceTheme.primaryColor
			height: 1
			opacity: 0.5
			Layout.columnSpan: 3
			Layout.fillWidth: true
		}


		Controls.Label {
			Layout.fillWidth: true
			opacity: 0.6
			text: qsTr("Notes")
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 3
		}

		Controls.Label {
			id: txtNotes
			text: dive.notes
			focus: true
			Layout.columnSpan: 3
			Layout.fillWidth: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}
		Item {
			Layout.columnSpan: 3
			Layout.fillWidth: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 6
		}
		Component.onCompleted: {
			qmlProfile.setMargin(Kirigami.Units.smallSpacing)
			qmlProfile.diveId = model.dive.id;
			qmlProfile.update();
		}
	}
}
