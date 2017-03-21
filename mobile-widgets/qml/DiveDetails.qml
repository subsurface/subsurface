import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

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
	property alias buddyModel: detailsEdit.buddyModel
	property alias divemasterIndex: detailsEdit.divemasterIndex
	property alias divemasterModel: detailsEdit.divemasterModel
	property alias depth: detailsEdit.depthText
	property alias duration: detailsEdit.durationText
	property alias location: detailsEdit.locationText
	property alias gps: detailsEdit.gpsText
	property alias notes: detailsEdit.notesText
	property alias suitIndex: detailsEdit.suitIndex
	property alias suitModel: detailsEdit.suitModel
	property alias weight: detailsEdit.weightText
	property alias startpressure: detailsEdit.startpressureText
	property alias endpressure: detailsEdit.endpressureText
	property alias cylinderIndex: detailsEdit.cylinderIndex
	property alias cylinderModel: detailsEdit.cylinderModel
	property alias gasmix: detailsEdit.gasmixText
	property alias gpsCheckbox: detailsEdit.gpsCheckbox
	property int updateCurrentIdx: manager.updateSelectedDive

	title: diveDetailsListView.currentItem ? diveDetailsListView.currentItem.modelData.dive.location : qsTr("Dive details")
	state: "view"
	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0

	states: [
		State {
			name: "view"
			PropertyChanges {
				target: diveDetailsPage;
				actions {
					right: deleteAction
					left: diveDetailsListView.currentItem ? (diveDetailsListView.currentItem.modelData.dive.gps !== "" ? mapAction : null) : null
				}
			}
			PropertyChanges { target: detailsEditScroll; opened: false }
			PropertyChanges { target: pageStack.contentItem; interactive: true }
		},
		State {
			name: "edit"
			PropertyChanges { target: detailsEditScroll; opened: true }
			PropertyChanges { target: pageStack.contentItem; interactive: false }
		},
		State {
			name: "add"
			PropertyChanges { target: detailsEditScroll; opened: true }
			PropertyChanges { target: pageStack.contentItem; interactive: false }
		}

	]

	property QtObject deleteAction: Action {
		text: qsTr("Delete dive")
		iconName: "trash-empty"
		onTriggered: {
			contextDrawer.close()
			var deletedId = diveDetailsListView.currentItem.modelData.dive.id
			var deletedIndex = diveDetailsListView.currentIndex
			manager.deleteDive(deletedId)
			stackView.pop()
			showPassiveNotification("Dive deleted", 3000, "Undo",
						function() {
							diveDetailsListView.currentIndex = manager.undoDelete(deletedId) ? deletedIndex : diveDetailsListView.currentIndex
						});
			contextDrawer.close() // at least one iPhone user has the drawer pop open after delete
		}
	}

	property QtObject mapAction: Action {
		text: qsTr("Show on map")
		iconName: "gps"
		onTriggered: {
			showMap(diveDetailsListView.currentItem.modelData.dive.gps_decimal)
		}
	}

	actions.main: Action {
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
		manager.selectedDiveTimestamp = diveDetailsListView.currentItem.modelData.dive.timestamp
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
		Qt.inputMethod.hide();
	}

	function startEditMode() {
		// set things up for editing - so make sure that the detailsEdit has
		// all the right data (using the property aliases set up above)
		dive_id = diveDetailsListView.currentItem.modelData.dive.id
		number = diveDetailsListView.currentItem.modelData.dive.number
		date = diveDetailsListView.currentItem.modelData.dive.date + " " + diveDetailsListView.currentItem.modelData.dive.time
		location = diveDetailsListView.currentItem.modelData.dive.location
		gps = diveDetailsListView.currentItem.modelData.dive.gps
		gpsCheckbox = false
		duration = diveDetailsListView.currentItem.modelData.dive.duration
		depth = diveDetailsListView.currentItem.modelData.dive.depth
		airtemp = diveDetailsListView.currentItem.modelData.dive.airTemp
		watertemp = diveDetailsListView.currentItem.modelData.dive.waterTemp
		suitIndex = diveDetailsListView.currentItem.modelData.dive.suitList.indexOf(diveDetailsListView.currentItem.modelData.dive.suit)
		if (diveDetailsListView.currentItem.modelData.dive.buddy.indexOf(",") > 0) {
			buddyIndex = diveDetailsListView.currentItem.modelData.dive.buddyList.indexOf(qsTr("Multiple Buddies"));
		} else {
			buddyIndex = diveDetailsListView.currentItem.modelData.dive.buddyList.indexOf(diveDetailsListView.currentItem.modelData.dive.buddy)
		}
		divemasterIndex = diveDetailsListView.currentItem.modelData.dive.divemasterList.indexOf(diveDetailsListView.currentItem.modelData.dive.divemaster)
		notes = diveDetailsListView.currentItem.modelData.dive.notes
		if (diveDetailsListView.currentItem.modelData.dive.singleWeight) {
			// we have only one weight, go ahead, have fun and edit it
			weight = diveDetailsListView.currentItem.modelData.dive.sumWeight
		} else {
			// careful when translating, this text is "magic" in DiveDetailsEdit.qml
			weight = "cannot edit multiple weight systems"
		}
		startpressure = diveDetailsListView.currentItem.modelData.dive.startPressure
		endpressure = diveDetailsListView.currentItem.modelData.dive.endPressure
		gasmix = diveDetailsListView.currentItem.modelData.dive.firstGas
		cylinderIndex = diveDetailsListView.currentItem.modelData.dive.cylinderList.indexOf(diveDetailsListView.currentItem.modelData.dive.getCylinder)

		diveDetailsPage.state = "edit"
	}

	//onWidthChanged: diveDetailsListView.positionViewAtIndex(diveDetailsListView.currentIndex, ListView.Beginning);

	Item {
		anchors.fill: parent
		ScrollView {
			id: diveDetailList
			anchors.fill: parent
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
				//cacheBuffer: parent.width * 3 // cache one item on either side (this is in pixels)
				snapMode: ListView.SnapOneItem
				highlightRangeMode: ListView.StrictlyEnforceRange
				onMovementEnded: {
					currentIndex = indexAt(contentX+1, 1);
				}
				delegate: ScrollView {
					id: internalScrollView
					width: diveDetailsListView.width
					height: diveDetailsListView.height
					property var modelData: model
					Flickable {
						//contentWidth: parent.width
						contentHeight: diveDetails.height
						boundsBehavior: Flickable.StopAtBounds
						DiveDetailsView {
							id: diveDetails
							width: internalScrollView.width
						}
					}
				}
			}
		}
		Kirigami.OverlaySheet {
			id: detailsEditScroll
			anchors.fill: parent
			onOpenedChanged: {
				if (!opened) {
					endEditMode()
				}
			}
			DiveDetailsEdit {
				id: detailsEdit
			}
		}
	}
}
