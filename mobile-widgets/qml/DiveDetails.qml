// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.4
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {
	id: diveDetailsPage // but this is referenced as detailsWindow
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
	property alias startpressure: detailsEdit.startpressureText
	property alias endpressure: detailsEdit.endpressureText
	property alias cylinderIndex: detailsEdit.cylinderIndex
	property alias cylinderText: detailsEdit.cylinderText
	property alias cylinderModel: detailsEdit.cylinderModel
	property alias gasmix: detailsEdit.gasmixText
	property alias gpsCheckbox: detailsEdit.gpsCheckbox
	property int updateCurrentIdx: manager.updateSelectedDive
	property alias rating: detailsEdit.rating
	property alias visibility: detailsEdit.visibility

	title: currentItem && currentItem.modelData ? currentItem.modelData.dive.location : qsTr("Dive details")
	state: "view"
	leftPadding: 0
	topPadding: Kirigami.Units.gridUnit * 2 // make room for the title bar
	rightPadding: 0
	bottomPadding: 0

	states: [
		State {
			name: "view"
			PropertyChanges {
				target: diveDetailsPage;
				actions {
					right: deleteAction
					left: currentItem ? (currentItem.modelData && currentItem.modelData.dive.gps !== "" ? mapAction : null) : null
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

	property QtObject deleteAction: Kirigami.Action {
		text: qsTr("Delete dive")
		iconName: "trash-empty"
		onTriggered: {
			var deletedId = currentItem.modelData.dive.id
			var deletedIndex = diveDetailsListView.currentIndex
			manager.deleteDive(deletedId)
			stackView.pop()
			showPassiveNotification("Dive deleted", 3000, "Undo",
						function() {
							diveDetailsListView.currentIndex = manager.undoDelete(deletedId) ? deletedIndex : diveDetailsListView.currentIndex
						});
		}
	}

	property QtObject cancelAction: Kirigami.Action {
		text: qsTr("Cancel edit")
		iconName: "dialog-cancel"
		onTriggered: {
			endEditMode()
		}
	}

	property QtObject mapAction: Kirigami.Action {
		text: qsTr("Show on map")
		iconName: "gps"
		onTriggered: {
			showMap(currentItem.modelData.dive.gps_decimal)
		}
	}

	actions.main: Kirigami.Action {
		iconName: state !== "view" ? "document-save" : "document-edit"
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
			stackView.pop()
			event.accepted = true;
		}
		// if we were in view mode, don't accept the event and pop the page
	}

	onCurrentItemChanged: {
		manager.selectedDiveTimestamp = currentItem.modelData.dive.timestamp
	}

	function showDiveIndex(index) {
		currentIndex = index;
		//diveDetailsListView.positionViewAtIndex(index, ListView.End);
	}

	function endEditMode() {
		// if we were adding a dive, we need to remove it
		if (state === "add")
			manager.addDiveAborted(dive_id)
		// just cancel the edit/add state
		state = "view";
		focus = false;
		Qt.inputMethod.hide();
	}

	function startEditMode() {
		if (!currentItem.modelData) {
			console.log("DiveDetails trying to access undefined currentItem.modelData")
			return
		}

		// set things up for editing - so make sure that the detailsEdit has
		// all the right data (using the property aliases set up above)
		dive_id = currentItem.modelData.dive.id
		number = currentItem.modelData.dive.number
		date = currentItem.modelData.dive.date + " " + currentItem.modelData.dive.time
		location = currentItem.modelData.dive.location
		gps = currentItem.modelData.dive.gps
		gpsCheckbox = false
		duration = currentItem.modelData.dive.duration
		depth = currentItem.modelData.dive.depth
		airtemp = currentItem.modelData.dive.airTemp
		watertemp = currentItem.modelData.dive.waterTemp
		suitIndex = currentItem.modelData.dive.suitList.indexOf(currentItem.modelData.dive.suit)
		if (currentItem.modelData.dive.buddy.indexOf(",") > 0) {
			buddyIndex = currentItem.modelData.dive.buddyList.indexOf(qsTr("Multiple Buddies"));
		} else {
			buddyIndex = currentItem.modelData.dive.buddyList.indexOf(currentItem.modelData.dive.buddy)
		}
		divemasterIndex = currentItem.modelData.dive.divemasterList.indexOf(currentItem.modelData.dive.divemaster)
		notes = currentItem.modelData.dive.notes
		if (currentItem.modelData.dive.singleWeight) {
			// we have only one weight, go ahead, have fun and edit it
			weight = currentItem.modelData.dive.sumWeight
		} else {
			// careful when translating, this text is "magic" in DiveDetailsEdit.qml
			weight = "cannot edit multiple weight systems"
		}
		startpressure = currentItem.modelData.dive.startPressure
		endpressure = currentItem.modelData.dive.endPressure
		gasmix = currentItem.modelData.dive.firstGas
		cylinderIndex = currentItem.modelData.dive.cylinderList.indexOf(currentItem.modelData.dive.getCylinder)
		rating = currentItem.modelData.dive.rating
		visibility = currentItem.modelData.dive.visibility

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
			highlightFollowsCurrentItem: true
			focus: true
			clip: false
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

	DiveDetailsEdit {
		id: detailsEdit
		visible: diveDetailsPage.state != "view" ? true : false
	}
}
