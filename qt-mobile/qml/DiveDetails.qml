import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: diveDetailsPage
	property alias currentIndex: diveDetailsListView.currentIndex
	property alias dive_id: detailsEdit.dive_id
	property alias number: detailsEdit.number
	property alias date: detailsEdit.dateText
	property alias airtemp: detailsEdit.airtempText
	property alias watertemp: detailsEdit.watertempText
	property alias buddy: detailsEdit.buddyText
	property alias divemaster: detailsEdit.divemasterText
	property alias depth: detailsEdit.depthText
	property alias duration: detailsEdit.durationText
	property alias location: detailsEdit.locationText
	property alias notes: detailsEdit.notesText
	property alias suit: detailsEdit.suitText
	property alias weight: detailsEdit.weightText

	state: "view"

	states: [
		State {
			name: "view"
			PropertyChanges { target: diveDetailList; visible: true }
			PropertyChanges { target: detailsEditScroll; visible: false }
		},
		State {
			name: "edit"
			PropertyChanges { target: diveDetailList; visible: false }
			PropertyChanges { target: detailsEditScroll; visible: true }
		},
		State {
			name: "add"
			PropertyChanges { target: diveDetailList; visible: false }
			PropertyChanges { target: detailsEditScroll; visible: true }
		}

	]
	mainAction: Action {
		iconName: state !== "view" ? "dialog-cancel" : "document-edit"
		onTriggered: {
			if (state === "edit") {
				// just cancel the edit state
				state = "view"
				Qt.inputMethod.hide()
			} else if (state === "add") {
				// edit was canceled - so remove the dive from the dive list
				manager.addDiveAborted(dive_id)
				state = "view"
				Qt.inputMethod.hide()
			} else {
				// set things up for editing - so make sure that the detailsEdit has
				// all the right data (using the property aliases set up above)
				dive_id = diveDetailsListView.currentItem.modelData.dive.id
				number = diveDetailsListView.currentItem.modelData.dive.number
				date = diveDetailsListView.currentItem.modelData.dive.date + " " + diveDetailsListView.currentItem.modelData.dive.time
				location = diveDetailsListView.currentItem.modelData.dive.location
				duration = diveDetailsListView.currentItem.modelData.dive.duration
				depth = diveDetailsListView.currentItem.modelData.dive.depth
				airtemp = diveDetailsListView.currentItem.modelData.dive.airTemp
				watertemp = diveDetailsListView.currentItem.modelData.dive.waterTemp
				suit = diveDetailsListView.currentItem.modelData.dive.suit
				buddy = diveDetailsListView.currentItem.modelData.dive.buddy
				divemaster = diveDetailsListView.currentItem.modelData.dive.divemaster
				notes = diveDetailsListView.currentItem.modelData.dive.notes
				weight = diveDetailsListView.currentItem.modelData.dive.sumWeight
				diveDetailsPage.state = "edit"
			}
		}
	}

	function showDiveIndex(index) {
		currentIndex = index;
		diveDetailsListView.positionViewAtIndex(index, ListView.Beginning);
	}
	onWidthChanged: diveDetailsListView.positionViewAtIndex(diveDetailsListView.currentIndex, ListView.Beginning);

	ScrollView {
		id: diveDetailList
		anchors.fill: parent
		ListView {
			id: diveDetailsListView
			anchors.fill: parent
			model: diveModel
			currentIndex: -1
			boundsBehavior: Flickable.StopAtBounds
			maximumFlickVelocity: parent.width/4
			orientation: ListView.Horizontal
			focus: true
			clip: true
			snapMode: ListView.SnapOneItem
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
	Flickable {
		id: detailsEditScroll
		anchors.fill: parent
		anchors.margins: MobileComponents.Units.gridUnit
		contentWidth: contentItem.childrenRect.width;
		contentHeight: contentItem.childrenRect.height
		clip: true
		bottomMargin: MobileComponents.Units.gridUnit * 3
		DiveDetailsEdit {
			id: detailsEdit
		}
	}
}
