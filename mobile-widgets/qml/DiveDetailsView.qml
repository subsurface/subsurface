// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.10
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
	property int myId: -1

	width: diveDetailsPage.width - diveDetailsPage.leftPadding - diveDetailsPage.rightPadding
	height: divePlate.implicitHeight + bottomLayout.implicitHeight + Kirigami.Units.iconSizes.large

	Connections {
		target: rootItem
		onSettingsChanged: {
			qmlProfile.update()
		}
	}

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
		anchors.top: detailsView.top
		anchors.topMargin: - Kirigami.Units.smallSpacing
		Controls.Label {
			id: locationText
			text: (undefined !== location && "" !== location) ? location : qsTr("<unnamed dive site>")
			font.weight: Font.Bold
			font.pointSize: subsurfaceTheme.titlePointSize
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			color: subsurfaceTheme.textColor
			anchors {
				left: parent.left
				top: parent.top
				right: parent.right
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
		Row {
			id: dateRow
			anchors {
				left: locationText.left
				top: locationText.bottom
				topMargin: Kirigami.Units.smallSpacing
				bottom: numberText.bottom

			}

			TemplateLabel {
				text: dateTime
				width: Math.max(locationText.width * 0.45, paintedWidth)
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			// spacer, just in case
			TemplateLabel {
				text: " "
				width: Kirigami.Units.largeSpacing
			}
			// let's try to show the depth / duration very compact
			TemplateLabel {
				text: depthDuration
				width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth)
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
		}
		TemplateLabel {
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
			TemplateLabelSmall {
				id: ratingText
				text: qsTr("Rating:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (rating >= 1) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (rating >= 2) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (rating >= 3) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (rating >= 4) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: ratingText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
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
			TemplateLabelSmall {
				id: visibilityText
				text: qsTr("Visibility:")
				font.pointSize: subsurfaceTheme.smallPointSize
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (viz >= 1) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (viz >= 2) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (viz >= 3) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
				source: (viz >= 4) ? ":/icons/ic_star.svg" : ":/icons/ic_star_border.svg"
				color: subsurfaceTheme.textColor
			}
			Kirigami.Icon {
				width: height
				height: subsurfaceTheme.regularPointSize
				anchors.verticalCenter: visibilityText.verticalCenter
				anchors.verticalCenterOffset: height * 0.2
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

		Rectangle {
			Layout.fillWidth: true
			Layout.preferredHeight: Layout.minimumHeight
			Layout.minimumHeight: width * 0.75
			Layout.columnSpan: 3
			clip: true

			QMLProfile {
				id: qmlProfile
				visible: !noDive
				anchors.fill: parent
				clip: true
				property real lastScale: 1.0 // final scale at the end of previous pinch
				diveId: detailsView.myId
				Rectangle {
					color: "transparent"
					opacity: 0.6
					border.width: 1
					border.color: subsurfaceTheme.primaryColor
					anchors.fill: parent
				}
				PinchArea {
					anchors.fill: parent
					pinch.dragAxis: Pinch.XAndYAxis
					onPinchStarted: {
						// it's possible that we thought this was a pan and reduced opacity
						// before realizing that this is actually a pinch/zoom. So let's reset this
						// just in case
						qmlProfile.opacity = 1.0
						if (manager.verboseEnabebled)
							manager.appendTextToLog("pinch started w/ previousScale " + qmlProfile.lastScale)
					}
					onPinchUpdated: {
						if (pinch.scale * qmlProfile.lastScale < 1.0)
							qmlProfile.lastScale = 1.0 / pinch.scale // this way we never shrink and the changes stay smooth
						// the underlying widget deals with the scaling, no need to send an update request
						qmlProfile.scale = pinch.scale * qmlProfile.lastScale
						if (manager.verboseEnabled)
							manager.appendTextToLog("pinch updated to scale " + qmlProfile.scale);
					}
					onPinchFinished: {
						// remember the final scale value so we can continue from there next time the user pinches
						qmlProfile.lastScale = pinch.scale * qmlProfile.lastScale
					}

					MouseArea {
						// we want to pan the profile if we are zoomed in, but we want to immediately
						// pass the mouse events through to the ListView if we are not. That way you
						// can swipe through the dive list, even if you happen to swipe the profile
						property bool isZoomed: qmlProfile.scale - 1.0 > 0.02

						// this indicates that we are actually dragging
						property bool dragging: false
						// cursor/finger position as we start dragging
						property real initialX
						property real initialY
						// the offset previously used to show the profile
						property real oldXOffset
						property real oldYOffset

						// if the profile is not scaled in, don't start panning
						// but if the profile is scaled in, then start almost immediately
						pressAndHoldInterval: isZoomed ? 50 : 50000

						// pass events through to the parent and eventually into the ListView
						propagateComposedEvents: true

						// for testing / debugging on a desktop
						scrollGestureEnabled: true
						onWheel: {
							manager.appendTextToLog("wheel " + wheel.angleDelta)
							if (wheel.angleDelta.y > 0)
								qmlProfile.scale += 0.2
							if (wheel.angleDelta.y < 0 && qmlProfile.scale > 1.1)
								qmlProfile.scale -= 0.2
						}

						anchors.fill: parent
						drag.target: qmlProfile
						drag.axis: Drag.XAndYAxis
						drag.smoothed: true
						onPressed: {
							if (!isZoomed)
								mouse.accepted = false
						}
						onPressAndHold: {
							dragging = true;
							oldXOffset = qmlProfile.xOffset
							oldYOffset = qmlProfile.yOffset
							initialX = mouse.x
							initialY = mouse.y
							if (manager.verboseEnabled)
								manager.appendTextToLog("press and hold at mouse" + Math.round(10 * mouse.x) / 10 + " / " + Math.round(10 * mouse.y) / 10)
							// give visual feedback to the user that they now can drag
							qmlProfile.opacity = 0.5
						}
						onPositionChanged: {
							if (dragging) {
								var x = (mouse.x - initialX) / qmlProfile.scale
								var y = (mouse.y - initialY) / qmlProfile.scale
								if (manager.verboseEnabled)
									manager.appendTextToLog("drag mouse "  + Math.round(10 * mouse.x) / 10 + " / " + Math.round(10 * mouse.y) / 10 + " delta " + Math.round(x) + " / " + Math.round(y))
								qmlProfile.xOffset = oldXOffset + x
								qmlProfile.yOffset = oldYOffset + y
								qmlProfile.update()
							} else {
								mouse.accepted = false
							}
						}
						onReleased: {
							if (dragging) {
								// reset things
								dragging = false
								qmlProfile.opacity = 1.0
							}
							mouse.accepted = false
						}
						onClicked: {
							// reset the position if not zoomed in
							if (!isZoomed) {
								qmlProfile.xOffset = qmlProfile.yOffset = oldXOffset = oldYOffset = 0
								mouse.accepted = false
							}
						}
					}
				}
			}
		}
		TemplateLabelSmall {
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
		TemplateLabelSmall {
			text: qsTr("Suit:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
			text: qsTr("Air Temp:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
			text: qsTr("Water Temp:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			color: subsurfaceTheme.textColor
		}

		// second row
		//------------
		TemplateLabelSmall {
			id: txtSuit
			text: suit
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
			id: txtAirTemp
			text: airTemp
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
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
		TemplateLabelSmall {
			text: qsTr("Cylinder:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
			text: qsTr("Weight:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
			text: qsTr("SAC:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col3Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}

		// fourth row
		//------------
		TemplateLabelSmall {
			id: txtCylinder
			text: cylinder
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
			id: txtWeight
			text: sumWeight
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col2Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
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
		TemplateLabelSmall {
			text: qsTr("Divemaster:")
			opacity: 0.6
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
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
		TemplateLabelSmall {
			id: txtDiveMaster
			text: diveMaster
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			Layout.maximumWidth: detailsView.col1Width
			color: subsurfaceTheme.textColor
		}
		TemplateLabelSmall {
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

		// seventh row
		//------------
		TemplateLabelSmall {
			text: qsTr("Tags:")
			opacity: 0.6
			Layout.columnSpan: 3
			Layout.maximumWidth: detailsView.gridWidth
			Layout.bottomMargin: 0
			color: subsurfaceTheme.textColor
		}

		// eighth row
		//------------
		TemplateLabelSmall {
			id: txtTags
			text: tags
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			elide: Text.ElideRight
			maximumLineCount: 3
			Layout.maximumWidth: detailsView.gridWidth
			height: Kirigami.Units.gridUnit * 3
			Layout.columnSpan: 3
			color: subsurfaceTheme.textColor
		}

		Rectangle {
			color: subsurfaceTheme.primaryColor
			height: 1
			opacity: 0.5
			Layout.columnSpan: 3
			Layout.fillWidth: true
		}

		// Notes on the bottom
		//--------------------

		TemplateLabelSmall {
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
	}
}
