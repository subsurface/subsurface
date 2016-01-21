import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: diveDetailsPage
	property alias currentIndex: diveListView.currentIndex

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
		}
	]
	mainAction: Action {
		iconName: state === "edit" ? "dialog-cancel" : "document-edit"
		onTriggered: {
			if (state === "edit") {
				state = "view"
				return
			}
			// After saving, the list may be shuffled, so first of all make sure that
			// the listview's currentIndex is the visible item
			// This makes sure that we always edit the currently visible item
			diveListView.currentIndex = diveListView.indexAt(diveListView.contentX+1, 1);
			detailsEdit.dive_id = diveListView.currentItem.modelData.dive.id
			detailsEdit.number = diveListView.currentItem.modelData.dive.number
			detailsEdit.dateText = diveListView.currentItem.modelData.dive.date + " " + diveListView.currentItem.modelData.dive.time
			detailsEdit.locationText = diveListView.currentItem.modelData.dive.location
			detailsEdit.durationText = diveListView.currentItem.modelData.dive.duration
			detailsEdit.depthText = diveListView.currentItem.modelData.dive.depth
			detailsEdit.airtempText = diveListView.currentItem.modelData.dive.airTemp
			detailsEdit.watertempText = diveListView.currentItem.modelData.dive.waterTemp
			detailsEdit.suitText = diveListView.currentItem.modelData.dive.suit
			detailsEdit.buddyText = diveListView.currentItem.modelData.dive.buddy
			detailsEdit.divemasterText = diveListView.currentItem.modelData.dive.divemaster
			detailsEdit.notesText = diveListView.currentItem.modelData.dive.notes
			detailsEdit.forcedWidth = diveDetailsPage.width
			diveDetailsPage.state = "edit"
		}
	}

	function showDiveIndex(index) {
		diveListView.currentIndex = index;
		diveListView.positionViewAtIndex(diveListView.currentIndex, ListView.Beginning);
	}
	onWidthChanged: diveListView.positionViewAtIndex(diveListView.currentIndex, ListView.Beginning);

	ScrollView {
		id: diveDetailList
		anchors.fill: parent
		ListView {
			id: diveListView
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
				width: diveListView.width
				height: diveListView.height
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
