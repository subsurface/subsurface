// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: diveDetailsPage // but this is referenced as detailsWindow
	objectName: "DiveDetails"
	property alias currentIndex: diveDetailsListView.currentIndex
	property alias currentItem: diveDetailsListView.currentItem
	property alias dive_id: detailsEdit.dive_id
	property alias number: detailsEdit.number
	property alias date: detailsEdit.dateText
	property alias airtemp: detailsEdit.airtempText
	property alias watertemp: detailsEdit.watertempText
	property alias buddyIndex: detailsEdit.buddyIndex
	property alias buddyText: detailsEdit.buddyText
	property alias buddyModel: detailsEdit.buddyModel
	property alias divemasterIndex: detailsEdit.divemasterIndex
	property alias divemasterText: detailsEdit.divemasterText
	property alias divemasterModel: detailsEdit.divemasterModel
	property alias depth: detailsEdit.depthText
	property alias duration: detailsEdit.durationText
	property alias location: detailsEdit.locationText
	property alias gps: detailsEdit.gpsText
	property alias notes: detailsEdit.notesText
	property alias suitIndex: detailsEdit.suitIndex
	property alias suitText: detailsEdit.suitText
	property alias suitModel: detailsEdit.suitModel
	property alias weight: detailsEdit.weightText
	property alias startpressure: detailsEdit.startpressure
	property alias endpressure: detailsEdit.endpressure
	property alias cylinderIndex0: detailsEdit.cylinderIndex0
	property alias cylinderIndex1: detailsEdit.cylinderIndex1
	property alias cylinderIndex2: detailsEdit.cylinderIndex2
	property alias cylinderIndex3: detailsEdit.cylinderIndex3
	property alias cylinderIndex4: detailsEdit.cylinderIndex4
	property alias usedGas: detailsEdit.usedGas
	property alias gpsCheckbox: detailsEdit.gpsCheckbox
	property alias rating: detailsEdit.rating
	property alias visibility: detailsEdit.visibility
	property alias usedCyl: detailsEdit.usedCyl
	property alias cylinderModel0: detailsEdit.cylinderModel0
	property alias cylinderModel1: detailsEdit.cylinderModel1
	property alias cylinderModel2: detailsEdit.cylinderModel2
	property alias cylinderModel3: detailsEdit.cylinderModel3
	property alias cylinderModel4: detailsEdit.cylinderModel4

	title: currentItem && currentItem.modelData && currentItem.modelData.location && "" !== currentItem.modelData.location ?
		       currentItem.modelData.location : qsTr("Dive details")
	state: "view"
	leftPadding: 0
	topPadding: Kirigami.Units.gridUnit / 2
	rightPadding: 0
	bottomPadding: 0
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
	width: rootItem.colWidth

	// we want to use our own colors for Kirigami, so let's define our colorset
	Kirigami.Theme.inherit: false
	Kirigami.Theme.colorSet: Kirigami.Theme.Button
	Kirigami.Theme.backgroundColor: subsurfaceTheme.backgroundColor
	Kirigami.Theme.textColor: subsurfaceTheme.textColor

	property QtObject removeDiveFromTripAction: Kirigami.Action {
		text: qsTr ("Remove this dive from trip")
		icon { name: ":/icons/chevron_left.svg" }
		enabled: currentItem && currentItem.modelData && currentItem.modelData.diveInTrip
		onTriggered: {
			manager.appendTextToLog("remove dive #" + currentItem.modelData.number + " from its trip")
			manager.removeDiveFromTrip(currentItem.modelData.id)
		}
	}
	property QtObject addDiveToTripAboveAction: Kirigami.Action {
		text: qsTr ("Add dive to trip above")
		icon { name: ":/icons/expand_less.svg" }
		enabled: currentItem && currentItem.modelData && !currentItem.modelData.diveInTrip && currentItem.modelData.tripAbove !== -1
		onTriggered: {
			manager.appendTextToLog("add dive #" + currentItem.modelData.number + " to trip with id " + currentItem.modelData.tripAbove)
			manager.addDiveToTrip(currentItem.modelData.id, currentItem.modelData.tripAbove)
		}
	}
	property QtObject addDiveToTripBelowAction: Kirigami.Action {
		text: qsTr ("Add dive to trip below")
		icon { name: ":/icons/expand_more.svg" }
		enabled: currentItem && currentItem.modelData && !currentItem.modelData.diveInTrip && currentItem.modelData.tripBelow !== -1
		onTriggered: {
			manager.appendTextToLog("add dive #" + currentItem.modelData.number + " to trip with id " + currentItem.modelData.tripBelow)
			manager.addDiveToTrip(currentItem.modelData.id, currentItem.modelData.tripBelow)
		}
	}
	property QtObject createTripForDiveAction: Kirigami.Action {
		text: qsTr("Create trip with dive")
		icon { name: ":/icons/list-add" }
		enabled: currentItem && currentItem.modelData && !currentItem.modelData.isTrip && currentItem.modelData.isTopLevel
		onTriggered: manager.addTripForDive(currentItem.modelData.id)
	}
	property QtObject toggleInvalidAction: Kirigami.Action {
		text: currentItem && currentItem.modelData && currentItem.modelData.isInvalid ? qsTr("Mark dive as valid") : qsTr("Mark dive as invalid")
		visible: currentItem && currentItem.modelData
		onTriggered: manager.toggleDiveInvalid(currentItem.modelData.id)
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
	property variant contextactions: [ removeDiveFromTripAction, addDiveToTripAboveAction, addDiveToTripBelowAction, deleteAction, undoAction, redoAction ]

	states: [
		State {
			name: "view"
			PropertyChanges {
				target: diveDetailsPage;
				actions {
					right: null
					left: currentItem ? (currentItem.modelData && currentItem.modelData.gps !== "" ? mapAction : null) : null
				}
				contextualActions: contextactions
			}
		},
		State {
			name: "edit"
			PropertyChanges {
				target: diveDetailsPage;
				actions {
					right: cancelAction
					left: null
				}
				contextualActions: []
			}
		},
		State {
			name: "add"
			PropertyChanges {
				target: diveDetailsPage;
				actions {
					right: cancelAction
					left: null
				}
				contextualActions: []
			}
		}
	]
	transitions: [
		Transition {
			from: "view"
			to: "*"
			ParallelAnimation {
				SequentialAnimation {
					NumberAnimation {
						target: detailsEditFlickable
						properties: "visible"
						from: 0
						to: 1
						duration: 10
					}
					ScaleAnimator {
						target: detailsEditFlickable
						from: 0.3
						to: 1
						duration: 400
						easing.type: Easing.InOutQuad
					}
				}

				NumberAnimation {
					target: detailsEditFlickable
					property: "contentY"
					to: 0
					duration: 200
					easing.type: Easing.InOutQuad
				}
			}
		},
		Transition {
			from: "*"
			to: "view"
			SequentialAnimation {
				ScaleAnimator {
					target: detailsEditFlickable
					from: 1
					to: 0.3
					duration: 400
					easing.type: Easing.InOutQuad
				}
				NumberAnimation {
					target: detailsEditFlickable
					properties: "visible"
					from: 1
					to: 0
					duration: 10
				}
			}

		}
	]

	Component.onCompleted: {
		// when we first create this page, we are in "view" mode and shouldn't show
		// a virtual keyboard. This should be unnecessary, but in some circumstances,
		// when the user editied a dive, went back to the dive list, and then tried to
		// view a different dive, the Android keyboard would pop up
		Qt.inputMethod.hide()
	}

	onHeightChanged: {
		// even with the explicit attempt to hide the keyboard above, it STILL sometimes
		// pops up when first showing a dive. So let's get more aggressive
		// QML doesn't let me trigger this based on the visible property of the inputMethod,
		// but when it becomes visible, the height of the page changes, so let's use that
		if (Qt.inputMethod.visible && state === "view")
			Qt.inputMethod.hide()
	}

	property QtObject deleteAction: Kirigami.Action {
		text: qsTr("Delete dive")
		icon {
			name: ":/icons/trash-empty.svg"
		}
		color: subsurfaceTheme.textColor
		onTriggered: manager.deleteDive(currentItem.modelData.id)
	}

	property QtObject cancelAction: Kirigami.Action {
		text: qsTr("Cancel edit")
		icon {
			name: ":/icons/dialog-cancel.svg"
		}
		color: subsurfaceTheme.textColor
		onTriggered: {
			endEditMode()
		}
	}

	property QtObject mapAction: Kirigami.Action {
		text: qsTr("Show on map")
		icon {
			name: ":/icons/gps"
		}
		color: subsurfaceTheme.textColor
		onTriggered: {
			showMap()
			mapPage.centerOnDiveSite(currentItem.modelData.diveSite)
		}
	}

	actions.main: Kirigami.Action {
		icon {
			name: state !== "view" ? ":/icons/document-save.svg" :
						 ":/icons/document-edit.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: state !== "view" ? qsTr("Save edits") : qsTr("Edit dive")
		onTriggered: {
			manager.appendTextToLog("save/edit button triggered")
			if (state === "edit" || state === "add") {
				detailsEdit.saveData()
			} else {
				startEditMode()
			}
		}
	}

	onBackRequested: {
		// if one of the drawers/menus is open, the back button should close those
		if (globalDrawer.visible) {
			globalDrawer.close()
			event.accepted = true
		}
		if (contextDrawer.visible) {
			contextDrawer.close()
			event.accepted = true
		}
		// if we didn't close either of the drawer, check if we need to close add/edit
		if (event.accepted === false) {
			if (state === "edit") {
				endEditMode()
				event.accepted = true;
			} else if (state === "add") {
				endEditMode()
				pageStack.pop()
				event.accepted = true;
			}
		}
		// if we were in view mode and no menus are open, don't accept the event and pop the page
	}

	onCurrentItemChanged: {
		if (currentItem && currentItem.modelData) {
			// make sure the core data structures reflect that this dive is selected
			manager.selectDive(currentItem.modelData.id)
			// update the map to show the highlighted flag and center on it
			if (rootItem.pageIndex(mapPage) !== -1) {
				mapPage.reloadMap()
				mapPage.centerOnDiveSite(currentItem.modelData.diveSite)
			}
		}
	}

	function endEditMode() {
		// we need to clean up either an edit or an add - and in case this
		// was an add, we need to undo the addDive action that created the empty dive
		// and we should also go back to the DiveDetails where we came from...
		manager.appendTextToLog("endEditMode called with state " + state)
		if (state === "add") {
			manager.undo()
			pageStack.pop()
		}
		// now all that is left is to cancel the edit/add state
		state = "view";
		focus = false;
		Qt.inputMethod.hide();
		detailsEdit.clearDetailsEdit();
	}

	function startEditMode() {
		if (!currentItem.modelData) {
			manager.appendTextToLog("DiveDetails trying to access undefined currentItem.modelData")
			return
		}

		// set things up for editing - so make sure that the detailsEdit has
		// all the right data (using the property aliases set up above)
		var modelData = currentItem.modelData
		dive_id = modelData.id
		number = modelData.number
		date = modelData.dateTime
		var locationText = modelData.location !== undefined ? modelData.location : ""
		var locationIndex = manager.locationList.indexOf(modelData.location)
		if (locationIndex >= 0) {
			detailsEdit.locationIndex = locationIndex
			detailsEdit.locationText  = manager.locationList[locationIndex] // this shouldn't be necessary, but apparently it is
		} else {
			detailsEdit.locationText = locationText
		}
		gps = modelData.gps
		gpsCheckbox = false
		duration = modelData.duration
		depth = modelData.depth
		airtemp = modelData.airTemp
		watertemp = modelData.waterTemp
		suitIndex = manager.suitList.indexOf(modelData.suit)
		buddyText = modelData.buddy;
		divemasterText = modelData.diveMaster
		notes = modelData.notes
		if (modelData.singleWeight) {
			// we have only one weight, go ahead, have fun and edit it
			weight = modelData.sumWeight
		} else {
			// careful when translating, this text is "magic" in DiveDetailsEdit.qml
			weight = "cannot edit multiple weight systems"
		}
		startpressure = modelData.startPressure
		endpressure = modelData.endPressure
		usedGas = modelData.firstGas
		usedCyl = modelData.getCylinder
		cylinderIndex0 = modelData.cylinderList.indexOf(usedCyl[0])
		cylinderIndex1 = modelData.cylinderList.indexOf(usedCyl[1])
		cylinderIndex2 = modelData.cylinderList.indexOf(usedCyl[2])
		cylinderIndex3 = modelData.cylinderList.indexOf(usedCyl[3])
		cylinderIndex4 = modelData.cylinderList.indexOf(usedCyl[4])
		rating = modelData.rating
		visibility = modelData.viz
		detailsEdit.focusReset()
		diveDetailsPage.state = "edit"
	}

	Item {
		anchors.fill: parent
		visible: diveDetailsPage.state == "view"
		ListView {
			id: diveDetailsListView
			anchors.fill: parent
			model: swipeModel
			currentIndex: -1
			boundsBehavior: Flickable.StopAtBounds
			maximumFlickVelocity: parent.width * 5
			orientation: ListView.Horizontal
			highlightFollowsCurrentItem: false
			focus: true
			clip: true
			snapMode: ListView.SnapOneItem
			highlightRangeMode: ListView.StrictlyEnforceRange
			onMovementEnded: {
				currentIndex = indexAt(contentX+1, 1);
				manager.selectSwipeRow(currentIndex)
			}
			delegate: Flickable {
				id: internalScrollView
				width: diveDetailsListView.width
				height: diveDetailsListView.height
				contentHeight: diveDetails.height
				boundsBehavior: Flickable.StopAtBounds
				property var modelData: model
				DiveDetailsView {
					id: diveDetails
					width: internalScrollView.width
					myId: model.id
				}
				ScrollBar.vertical: ScrollBar { }
			}
			ScrollIndicator.horizontal: ScrollIndicator { }
			Connections {
				target: swipeModel
				onCurrentDiveChanged: {
					currentIndex = index.row
					diveDetailsListView.positionViewAtIndex(currentIndex, ListView.End)
				}
			}
		}
	}
	Flickable {
		id: detailsEditFlickable
		anchors.fill: parent
		leftMargin: Kirigami.Units.smallSpacing
		rightMargin: Kirigami.Units.smallSpacing
		contentHeight: detailsEdit.height
		// start invisible and scaled down, to get the transition
		// off to the right start
		visible: false
		scale: 0.3
		DiveDetailsEdit {
			id: detailsEdit
		}
		ScrollBar.vertical: ScrollBar { }
	}
}
