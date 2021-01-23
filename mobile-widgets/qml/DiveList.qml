// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.kirigami 2.5 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: page
	objectName: "DiveList"
	title: qsTr("Dive list")
	verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
	property int dlHorizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1
	property QtObject diveListModel: null
	// we want to use our own colors for Kirigami, so let's define our colorset
	Kirigami.Theme.inherit: false
	Kirigami.Theme.colorSet: Kirigami.Theme.Button
	Kirigami.Theme.backgroundColor: subsurfaceTheme.backgroundColor
	Kirigami.Theme.textColor: subsurfaceTheme.textColor

	supportsRefreshing: true
	onRefreshingChanged: {
		if (refreshing) {
			if (Backend.cloud_verification_status === Enums.CS_VERIFIED) {
				detailsWindow.endEditMode()
				manager.saveChangesCloud(true)
				showPassiveNotification(qsTr("Completed manual sync with cloud\n") + manager.syncState)
				refreshing = false
			} else {
				manager.appendTextToLog("sync with cloud storage requested, but credentialStatus is " + Backend.cloud_verification_status)
				manager.appendTextToLog("no syncing, turn off spinner")
				refreshing = false
			}
		}
	}

	Component {
		id: diveOrTripDelegate
		Kirigami.AbstractListItem {
			// this allows us to access properties of the currentItem from outside
			property variant myData: model
			property var view: ListView.view
			property bool selected: !isTrip && current // don't use 'checked' for this as that confuses QML as it tries
			property bool invalid: isInvalid === true
			id: diveOrTripDelegateItem
			padding: 0
			supportsMouseEvents: true
			anchors {
				left: parent ? parent.left : undefined
				right: parent ? parent.right : undefined
			}
			height: isTrip ? 1 + 8 * Kirigami.Units.smallSpacing : 11 * Kirigami.Units.smallSpacing // delegateInnerItem.height

			onSelectedChanged: {
				if (selected && index !== view.currentIndex)
					view.currentIndex = index;
			}

			// When clicked, a trip expands / unexpands, a dive is opened in DiveDetails
			onClicked: {
				view.currentIndex = index
				if (isTrip) {
					manager.appendTextToLog("clicked on trip " + tripTitle)
					// toggle expand (backend to deal with unexpand other trip)
					diveModel.toggle(model.row);
				} else {
					manager.appendTextToLog("clicked on dive")
					if (detailsWindow.state === "view") {
						manager.selectRow(model.row);
						showPage(detailsWindow)
					}
				}
			}
			// use this to select a dive without switching to dive details; instead open context drawer
			onPressAndHold: {
				view.currentIndex = index
				manager.appendTextToLog("press and hold on trip or dive; open context drawer")
				manager.selectRow(model.row)
				contextDrawer.open()
			}

			// first we look at the trip
			Item {
				id: delegateInnerItem
				width: page.width
				height: childrenRect.height
				Rectangle {
					id: headingBackground
					height: visible ? 8 * Kirigami.Units.smallSpacing : 0
					anchors {
						topMargin: Kirigami.Units.smallSpacing / 2
						left: parent.left
						right: parent.right
					}
					color: subsurfaceTheme.lightPrimaryColor
					visible: isTrip
					Rectangle {
						id: dateBox
						height: 1.5 * Kirigami.Units.gridUnit
						width: 1.8 * Kirigami.Units.gridUnit
						color: subsurfaceTheme.primaryColor
						radius: Kirigami.Units.smallSpacing * 1.5
						antialiasing: true
						anchors {
							verticalCenter: parent.verticalCenter
							left: parent.left
							leftMargin: Kirigami.Units.smallSpacing
						}
						Controls.Label {
							visible: headingBackground.visible
							text: visible ? tripShortDate : ""
							color: subsurfaceTheme.primaryTextColor
							font.pointSize: subsurfaceTheme.smallPointSize * 0.8
							lineHeightMode: Text.FixedHeight
							lineHeight: Kirigami.Units.gridUnit *.6
							horizontalAlignment: Text.AlignHCenter
							height: contentHeight
							anchors {
								horizontalCenter: parent.horizontalCenter
								verticalCenter: parent.verticalCenter
							}
						}
					}
					Controls.Label {
						text: visible ? tripTitle : ""
						elide: Text.ElideRight
						visible: headingBackground.visible
						font.weight: Font.Medium
						font.pointSize: subsurfaceTheme.regularPointSize
						anchors {
							verticalCenter: parent.verticalCenter
							left: dateBox.right
							leftMargin: dlHorizontalPadding * 2
							right: parent.right
						}
						color: subsurfaceTheme.lightPrimaryTextColor
					}
				}

				Rectangle {
					id: diveBackground
					height: visible ? diveListEntry.height + Kirigami.Units.smallSpacing : 0
					anchors {
						left: parent.left
						right: parent.right
					}
					color: selected ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.backgroundColor
					visible: !isTrip
					Item {
						anchors.fill: parent
						Rectangle {
							id: leftBarDive
							width: Kirigami.Units.smallSpacing
							height: isTopLevel ? 0 : diveListEntry.height * 0.8
							color: selected ? subsurfaceTheme.backgroundColor :subsurfaceTheme.darkerPrimaryColor // reverse of the diveBackground
							anchors {
								left: parent.left
								top: parent.top
								leftMargin: Kirigami.Units.smallSpacing
								topMargin: Kirigami.Units.smallSpacing * 2
								bottomMargin: Kirigami.Units.smallSpacing * 2
							}
						}
						Item {
							id: diveListEntry
							height: visible ? 10 * Kirigami.Units.smallSpacing - 1 : 0
							anchors {
								right: parent.right
								left: leftBarDive.right
								verticalCenter: parent.verticalCenter
							}
							Controls.Label {
								id: locationText
								text: (undefined !== location && "" != location) ? location : qsTr("<unnamed dive site>")
								font.weight: Font.Medium
								font.strikeout: invalid
								font.pointSize: subsurfaceTheme.smallPointSize
								elide: Text.ElideRight
								maximumLineCount: 1 // needed for elide to work at all
								color: selected ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.textColor
								anchors {
									left: parent.left
									leftMargin: dlHorizontalPadding * 2
									topMargin: Kirigami.Units.smallSpacing / 2
									top: parent.top
									right: parent.right
								}
							}
							Row {
								anchors {
									left: locationText.left
									top: locationText.bottom
									topMargin: Kirigami.Units.smallSpacing / 2
								}

								Controls.Label {
									id: dateLabel
									text: (undefined !== dateTime) ? dateTime : ""
									width: Math.max(locationText.width * 0.45, paintedWidth) // helps vertical alignment throughout listview
									font.pointSize: subsurfaceTheme.smallPointSize
									font.strikeout: invalid
									color: selected ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.secondaryTextColor
								}
								// spacer, just in case
								Controls.Label {
									text: " "
									width: Kirigami.Units.largeSpacing
								}
								// let's try to show the depth / duration very compact
								Controls.Label {
									text: (undefined !== depthDuration) ? depthDuration : ""
									width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
									font.pointSize: subsurfaceTheme.smallPointSize
									font.strikeout: invalid
									color: selected ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.secondaryTextColor
								}
							}
							Controls.Label {
								id: numberText
								text: "#" + number
								font.pointSize: subsurfaceTheme.smallPointSize
								font.strikeout: invalid
								color: selected ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.secondaryTextColor
								anchors {
									right: parent.right
									rightMargin: Kirigami.Units.smallSpacing
									top: locationText.bottom
									topMargin: Kirigami.Units.smallSpacing / 2
								}
							}
						}
					}
				}

			}
		}
	}

	property alias currentItem: diveListView.currentItem

	property QtObject removeDiveFromTripAction: Kirigami.Action {
		text: visible ? qsTr ("Remove dive %1 from trip").arg(currentItem.myData.number) : ""
		icon { name: ":/icons/chevron_left.svg" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip && currentItem.myData.diveInTrip === true
		onTriggered: {
			manager.removeDiveFromTrip(currentItem.myData.id)
		}
	}
	property QtObject addDiveToTripAboveAction: Kirigami.Action {
		text: visible ? qsTr ("Add dive %1 to trip above").arg(currentItem.myData.number) : ""
		icon { name: ":/icons/expand_less.svg" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip && currentItem.myData.tripAbove !== -1
		onTriggered: {
			manager.addDiveToTrip(currentItem.myData.id, currentItem.myData.tripAbove)
		}
	}
	property QtObject addDiveToTripBelowAction: Kirigami.Action {
		text: visible ? qsTr ("Add dive %1 to trip below").arg(currentItem.myData.number) : ""
		icon { name: ":/icons/expand_more.svg" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip && currentItem.myData.tripBelow !== -1
		onTriggered: {
			manager.addDiveToTrip(currentItem.myData.id, currentItem.myData.tripBelow)
		}
	}
	property QtObject createTripForDiveAction: Kirigami.Action {
		text: visible ? qsTr("Create trip with dive %1").arg(currentItem.myData.number) : ""
		icon { name: ":/icons/list-add" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip && currentItem.myData.isTopLevel
		onTriggered: {
			manager.addTripForDive(currentItem.myData.id)
		}
	}

	property QtObject toggleInvalidAction: Kirigami.Action {
		text: currentItem && currentItem.myData && currentItem.myData.isInvalid ? qsTr("Mark dive as valid") : qsTr("Mark dive as invalid")
		// icon: { name: "TBD" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip
		onTriggered: manager.toggleDiveInvalid(currentItem.myData.id)
	}
	property QtObject deleteAction: Kirigami.Action {
		text: qsTr("Delete dive")
		icon { name: ":/icons/trash-empty.svg" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip
		onTriggered: manager.deleteDive(currentItem.myData.id)
	}
	property QtObject mapAction: Kirigami.Action {
		text: qsTr("Show on map")
		icon { name: ":/icons/gps" }
		visible: currentItem && currentItem.myData && !currentItem.myData.isTrip && currentItem.myData.gps !== ""
		onTriggered: {
			showMap()
			mapPage.centerOnDiveSite(currentItem.myData.diveSite)
		}
	}
	property QtObject tripDetailsEdit: Kirigami.Action {
		text: qsTr("Edit trip details")
		icon { name: ":/icons/trip_details.svg" }
		visible: currentItem && currentItem.myData && currentItem.myData.isTrip
		onTriggered: {
			tripEditWindow.tripId = currentItem.myData.tripId
			tripEditWindow.tripLocation = currentItem.myData.tripLocation
			tripEditWindow.tripNotes = currentItem.myData.tripNotes
			showPage(tripEditWindow)
		}
	}

	property QtObject undoAction: Kirigami.Action {
		text: qsTr("Undo") + " " + manager.undoText
		icon { name: ":/icons/undo.svg" }
		enabled: manager.undoText !== ""
		onTriggered: manager.undo()
	}
	property QtObject redoAction: Kirigami.Action {
		text: qsTr("Redo") + " " + manager.redoText
		icon { name: ":/icons/redo.svg" }
		enabled: manager.redoText !== ""
		onTriggered: manager.redo()
	}
	property variant contextactions: [ removeDiveFromTripAction, createTripForDiveAction, addDiveToTripAboveAction, addDiveToTripBelowAction, toggleInvalidAction, deleteAction, mapAction, tripDetailsEdit, undoAction, redoAction ]

	function setupActions() {
		if (Backend.cloud_verification_status === Enums.CS_VERIFIED || Backend.cloud_verification_status === Enums.CS_NOCLOUD) {
			page.actions.main = page.downloadFromDCAction
			page.actions.right = page.addDiveAction
			page.actions.left = page.filterToggleAction
			page.contextualActions = contextactions
			page.title = qsTr("Dive list")
			if (diveListView.count === 0)
				showPassiveNotification(qsTr("Please tap the '+' button to add a dive (or download dives from a supported dive computer)"), 3000)
		} else {
			page.actions.main = null
			page.actions.right = null
			page.actions.left = null
			page.contextualActions = null
			page.title = qsTr("Cloud credentials")
		}
	}

	Controls.Label {
		property bool showProcessingText: manager.diveListProcessing
		anchors.fill: parent
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		text: diveListModel && !showProcessingText ? qsTr("No dives in dive list") : qsTr("Please wait, updating the dive list")
		visible: diveListView.visible && diveListView.count === 0
		onShowProcessingTextChanged: {
			manager.appendTextToLog("============diveListProcessing is " + showProcessingText)
		}
	}

	Rectangle {
		id: filterHeader
		visible: filterBar.height > 0
		implicitHeight: filterBar.implicitHeight
		implicitWidth: filterBar.implicitWidth
		height: filterBar.height
		anchors {
			top: parent.top
			left: parent.left
			right: parent.right
		}
		color: subsurfaceTheme.backgroundColor
		enabled: rootItem.filterToggle
		RowLayout {
			id: filterBar
			states: [
				State {
					name: "isVisible"
					when: rootItem.filterToggle
					PropertyChanges { target: filterBar; height: sitefilter.implicitHeight }
				},
				State {
					name: "isHidden"
					when: !rootItem.filterToggle
					PropertyChanges { target: filterBar; height: 0 }
				}
			]
			transitions: [
				Transition { NumberAnimation { property: "height"; duration: 400; easing.type: Easing.InOutQuad }}
			]
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.leftMargin: Kirigami.Units.gridUnit / 2
			anchors.rightMargin: Kirigami.Units.gridUnit / 2
			TemplateSlimComboBox {
				visible: filterBar.height === sitefilter.implicitHeight
				id: sitefilterMode
				model: ListModel {
					ListElement {text: qsTr("Fulltext")}
					ListElement {text: qsTr("People")}
					ListElement {text: qsTr("Tags")}
				}
				font.pointSize: subsurfaceTheme.smallPointSize
				Layout.maximumWidth: parent.width * 0.3
				onActivated:  {
					manager.setFilter(sitefilter.text, currentIndex)
				}
			}
			TemplateTextField  {
				id: sitefilter
				verticalAlignment: TextInput.AlignVCenter
				Layout.fillWidth: true
				text: ""
				placeholderText: sitefilterMode.currentText
				placeholderTextColor: subsurfaceTheme.secondaryTextColor
				onAccepted: {
					manager.setFilter(text, sitefilterMode.currentIndex)
				}
				onVisibleChanged: {
					// reset the filter when it gets toggled
					text = ""
					if (visible) {
						forceActiveFocus()
					}
				}
			}
			TemplateLabel {
				id: numShown
				verticalAlignment: Text.AlignVCenter
				text: diveModel.shown
			}
		}
	}
	ListView {
		id: diveListView
		topMargin: filterHeader.height
		anchors.fill: parent
		model: diveListModel
		currentIndex: -1
		delegate: diveOrTripDelegate
		boundsBehavior: Flickable.DragOverBounds
		maximumFlickVelocity: parent.height * 5
		bottomMargin: Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit
		cacheBuffer: 0
		Component.onCompleted: {
			manager.appendTextToLog("finished setting up the diveListView")
		}
		onVisibleChanged: {
			if (visible)
				setupActions()
		}
	}

	property QtObject downloadFromDCAction: Kirigami.Action {
		icon {
			name: ":/icons/downloadDC"
			color: subsurfaceTheme.primaryColor
		}
		text: qsTr("Download dives")
		onTriggered: {
			rootItem.showDownloadPage()
		}
	}

	property QtObject addDiveAction: Kirigami.Action {
		icon {
			name: ":/icons/list-add"
		}
		color: subsurfaceTheme.textColor
		text: qsTr("Add dive")
		onTriggered: {
			startAddDive()
		}
	}

	property QtObject filterToggleAction: Kirigami.Action {
		icon {
			name: ":icons/ic_filter_list"
		}
		color: subsurfaceTheme.textColor
		text: qsTr("Filter dives")
		onTriggered: {
			rootItem.filterToggle = !rootItem.filterToggle
			manager.setFilter("", 0)
			if (rootItem.filterToggle)
				Qt.inputMethod.show()
			else
				Qt.inputMethod.hide()
		}
	}

	onBackRequested: {
		if (startPage.visible && diveListView.count > 0 &&
		    Backend.cloud_verification_status !== Enums.CS_INCORRECT_USER_PASSWD) {
			Backend.cloud_verification_status = oldStatus
			event.accepted = true;
		}
		if (!startPage.visible) {
			if (globalDrawer.visible) {
				globalDrawer.close()
				event.accepted = true
			}
			if (contextDrawer.visible) {
				contextDrawer.close()
				event.accepted = true
			}
			if (event.accepted === false && Qt.platform.os !== "ios") {
				manager.quit()
			}
			// let's make sure Kirigami doesn't quit on our behalf
			event.accepted = true
		}
	}

	function setCurrentDiveListIndex(idx, noScroll) {
		// pick the dive in the dive list and make sure its trip is expanded
		diveListView.currentIndex = idx
		if (diveListModel)
			diveListModel.setActiveTrip(diveListView.currentItem.myData.tripId)

		// updating the index of the ListView triggers a non-linear scroll
		// animation that can be very slow. the fix is to stop this animation
		// by setting contentY to itself and then using positionViewAtIndex().
		// the downside is that the view jumps to the index immediately.
		if (noScroll) {
			diveListView.contentY = diveListView.contentY
			diveListView.positionViewAtIndex(idx, ListView.Center)
		}
	}
}
