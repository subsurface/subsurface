// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
/*
import QtWebView 1.0
*/
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

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
		color: subsurfaceTheme.textColor
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
			text: location
			font.weight: Font.Bold
			font.pointSize: subsurfaceTheme.titlePointSize
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			color: subsurfaceTheme.textColor
			anchors {
				left: parent.left
				top: parent.top
				right: gpsButton.left
				margins: Math.round(Kirigami.Units.gridUnit / 2)
			}
			MouseArea {
				anchors.fill: parent
				enabled: gpsDecimal !== ""
				onClicked: {
					showMap()
					mapPage.centerOnDiveSite(diveSite)
				}
			}
		}
		SsrfButton {
			id: gpsButton
			anchors.right: parent.right
			enabled: gps !== ""
			text: qsTr("Map it")
			onClicked: {
				showMap()
				mapPage.centerOnDiveSite(diveSite)
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
				text: dateTime
				width: Math.max(locationText.width * 0.45, paintedWidth)
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			// let's try to show the depth / duration very compact
			Controls.Label {
				text: depthDuration
				width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth)
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
		}
		Controls.Label {
			id: numberText
			text: "#" + number
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
				source: (rating >= 1) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				source: (rating >= 2) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				source: (rating >= 3) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				source: (rating >= 4) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				source: (rating === 5) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
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
				source: (viz >= 1) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				source: (viz >= 2) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				source: (viz >= 3) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				source: (viz >= 4) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				source: (viz === 5) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
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
			visible: !noDive
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
			visible: noDive
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
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			text: qsTr("Air Temp:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			text: qsTr("Water Temp:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			color: subsurfaceTheme.textColor
		}

		// second row
		//------------
		Controls.Label {
			id: txtSuit
			text: suit
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			id: txtAirTemp
			text: airTemp
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			id: txtWaterTemp
			text: waterTemp
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			color: subsurfaceTheme.textColor
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
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			text: qsTr("Weight:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			text: qsTr("SAC:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}

		// fourth row
		//------------
		Controls.Label {
			id: txtCylinder
			text: cylinder
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			id: txtWeight
			text: sumWeight
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			id: txtSAC
			text: sac
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			color: subsurfaceTheme.textColor
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
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			text: qsTr("Buddy:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 2
			Layout.maximumWidth: detailsView.col2Width + detailsView.col3Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}

		// sixth row
		//-----------
		Controls.Label {
			id: txtDiveMaster
			text: diveMaster
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		Controls.Label {
			id: txtBuddy
			text: buddy
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.columnSpan: 2
			Layout.maximumWidth: detailsView.col2Width + detailsView.col3Width
			color: subsurfaceTheme.textColor
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
			color: subsurfaceTheme.textColor
		}

		Controls.Label {
			id: txtNotes
			text: notes
			focus: true
			Layout.columnSpan: 3
			Layout.fillWidth: true
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			color: subsurfaceTheme.textColor
		}
		Item {
			Layout.columnSpan: 3
			Layout.fillWidth: true
			Layout.minimumHeight: Kirigami.Units.gridUnit * 6
		}
		Component.onCompleted: {
			qmlProfile.setMargin(Kirigami.Units.smallSpacing)
			qmlProfile.diveId = model.id;
			qmlProfile.update();
		}
	}
}
