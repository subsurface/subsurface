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
		}
	]
	mainAction: Action {
		iconName: state === "edit" ? "dialog-cancel" : "document-edit"
		onTriggered: {
			if (state === "edit") {
				state = "view"
				return
			}
			detailsEdit.dive_id = diveDetailsListView.currentItem.modelData.dive.id
			detailsEdit.number = diveDetailsListView.currentItem.modelData.dive.number
			detailsEdit.dateText = diveDetailsListView.currentItem.modelData.dive.date + " " + diveDetailsListView.currentItem.modelData.dive.time
			detailsEdit.locationText = diveDetailsListView.currentItem.modelData.dive.location
			detailsEdit.durationText = diveDetailsListView.currentItem.modelData.dive.duration
			detailsEdit.depthText = diveDetailsListView.currentItem.modelData.dive.depth
			detailsEdit.airtempText = diveDetailsListView.currentItem.modelData.dive.airTemp
			detailsEdit.watertempText = diveDetailsListView.currentItem.modelData.dive.waterTemp
			detailsEdit.suitText = diveDetailsListView.currentItem.modelData.dive.suit
			detailsEdit.buddyText = diveDetailsListView.currentItem.modelData.dive.buddy
			detailsEdit.divemasterText = diveDetailsListView.currentItem.modelData.dive.divemaster
			detailsEdit.notesText = diveDetailsListView.currentItem.modelData.dive.notes
			diveDetailsPage.state = "edit"
		}
	}

	function showDiveIndex(index) {
		diveDetailsListView.currentIndex = index;
		diveDetailsListView.positionViewAtIndex(diveDetailsListView.currentIndex, ListView.Beginning);
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
