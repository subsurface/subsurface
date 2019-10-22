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
	property alias locationModel: detailsEdit.locationModel
	property alias locationIndex: detailsEdit.locationIndex
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
	property int updateCurrentIdx: manager.updateSelectedDive
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

	states: [
		State {
			name: "view"
			PropertyChanges {
				target: diveDetailsPage;
				actions {
					right: deleteAction
					left: currentItem ? (currentItem.modelData && currentItem.modelData.gps !== "" ? mapAction : null) : null
				}
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

	property QtObject deleteAction: Kirigami.Action {
		text: qsTr("Delete dive")
		icon {
			name: ":/icons/trash-empty.svg"
		}
		onTriggered: {
			var deletedId = currentItem.modelData.id
			var deletedIndex = diveDetailsListView.currentIndex
			manager.deleteDive(deletedId)
			pageStack.pop()
			showPassiveNotification("Dive deleted", 5000, "Undo",
						function() {
							diveDetailsListView.currentIndex = manager.undoDelete(deletedId) ? deletedIndex : diveDetailsListView.currentIndex
						});
		}
	}

	property QtObject cancelAction: Kirigami.Action {
		text: qsTr("Cancel edit")
		icon {
			name: ":/icons/dialog-cancel.svg"
		}
		onTriggered: {
			endEditMode()
		}
	}

	property QtObject mapAction: Kirigami.Action {
		text: qsTr("Show on map")
		icon {
			name: ":/icons/gps"
		}
		onTriggered: {
			showMap()
			mapPage.centerOnDiveSite(currentItem.modelData.diveSite)
		}
	}

	actions.main: Kirigami.Action {
		icon {
			name: state !== "view" ? ":/icons" + subsurfaceTheme.iconStyle + "/document-save.svg" :
						 ":/icons" + subsurfaceTheme.iconStyle + "/document-edit.svg"
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
		if (state === "edit") {
			endEditMode()
			event.accepted = true;
		} else if (state === "add") {
			endEditMode()
			pageStack.pop()
			event.accepted = true;
		}
		// if we were in view mode, don't accept the event and pop the page
	}

	onCurrentItemChanged: {
		if (currentItem && currentItem.modelData) {
			// this is used when loading dives to maintain relative position in the dive list
			manager.selectedDiveTimestamp = currentItem.modelData.date
			// make sure the core data structures reflect that this dive is selected
			manager.selectDive(currentItem.modelData.id)
			// update the map to show the highlighted flag and center on it
			if (rootItem.pageIndex(mapPage) !== -1) {
				mapPage.reloadMap()
				mapPage.centerOnDiveSite(currentItem.modelData.diveSite)
			}
		}
	}

	function showDiveIndex(index) {
		currentIndex = index;
		diveDetailsListView.positionViewAtIndex(index, ListView.End);
	}

	function endEditMode() {
		// if we were adding a dive, we need to remove it
		if (state === "add")
			manager.addDiveAborted(dive_id)
		// just cancel the edit/add state
		state = "view";
		focus = false;
		Qt.inputMethod.hide();
		detailsEdit.clearDetailsEdit();
	}

	function startEditMode() {
		if (!currentItem.modelData) {
			console.log("DiveDetails trying to access undefined currentItem.modelData")
			return
		}

		// set things up for editing - so make sure that the detailsEdit has
		// all the right data (using the property aliases set up above)
		var dive = currentItem.modelData.dive
		var modelData = currentItem.modelData
		dive_id = modelData.id
		number = modelData.number
		date = modelData.dateTime
		location = modelData.location !== undefined ? location : ""
		locationIndex = manager.locationList.indexOf(modelData.location)
		gps = modelData.gps
		gpsCheckbox = false
		duration = modelData.duration
		depth = modelData.depth
		airtemp = modelData.airTemp
		watertemp = modelData.waterTemp
		suitIndex = manager.suitList.indexOf(modelData.suit)
		if (modelData.buddy.indexOf(",") > 0) {
			buddyIndex = manager.buddyList.indexOf(modelData.buddy.split(",", 1).toString())
		} else {
			buddyIndex = manager.buddyList.indexOf(modelData.buddy)
		}
		buddyText = modelData.buddy;
		if (modelData.diveMaster.indexOf(",") > 0) {
			divemasterIndex = manager.divemasterList.indexOf(modelData.diveMaster.split(",", 1).toString())
		} else {
			divemasterIndex = manager.divemasterList.indexOf(modelData.diveMaster)
		}
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

		diveDetailsPage.state = "edit"
	}

	Item {
		anchors.fill: parent
		visible: diveDetailsPage.state == "view"
		ListView {
			id: diveDetailsListView
			anchors.fill: parent
			model: diveModel
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
				}
				ScrollBar.vertical: ScrollBar { }
			}
			ScrollIndicator.horizontal: ScrollIndicator { }
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
