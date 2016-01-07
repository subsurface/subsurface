import QtQuick 2.3
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: diveDetailsWindow
	width: parent.width
	objectName: "DiveDetails"

	property string location
	property string gps
	property string depth
	property string dive_id
	property string diveNumber
	property string duration
	property string airtemp
	property string watertemp
	property string suit
	property int rating
	property string buddy
	property string divemaster;
	property string notes;
	property string date
	property string number
	property string weight
	state: "view"

	states: [
		State {
			name: "view"
			PropertyChanges { target: detailsView; opacity: 1 }
			PropertyChanges { target: detailsEdit; opacity: 0 }
		},
		State {
			name: "edit"
			PropertyChanges { target: detailsView; opacity: 0 }
			PropertyChanges { target: detailsEdit; opacity: 1 }
		}
	]
	property list<Action> viewActions: [
		Action {
			id: editSelector
			text: "Edit"
			iconName: "document-edit"
			onTriggered: {
				diveDetailsWindow.state = "edit"
				contextDrawer.close()
			}
		}
	]
	property list<Action> editActions: [
		Action {
			id: cancelSelector
			text: "Cancel"
			iconName: "dialog-cancel"
			onTriggered: {
				// reset the fields in the edit screen
				detailsEdit.dateText = date
				detailsEdit.locationText = location
				detailsEdit.durationText = duration
				detailsEdit.depthText = depth
				detailsEdit.airtempText = airtemp
				detailsEdit.watertempText = watertemp
				detailsEdit.suitText = suit
				detailsEdit.buddyText = buddy
				detailsEdit.divemasterText = divemaster
				detailsEdit.notesText = notes
				// back to view state and close the drawer
				diveDetailsWindow.state = "view"
				contextDrawer.close()
			}
		},
		Action {
			id: saveSelector
			text: "Save"
			iconName: "document-save"
			onTriggered: {
				// apply the changes to the dive_table
				manager.commitChanges(dive_id, detailsEdit.dateText, detailsEdit.locationText, detailsEdit.gpsText, detailsEdit.durationText,
						      detailsEdit.depthText, detailsEdit.airtempText, detailsEdit.watertempText, detailsEdit.suitText,
						      detailsEdit.buddyText, detailsEdit.divemasterText, detailsEdit.notesText)
				// apply the changes to the dive detail view
				date = detailsEdit.dateText
				location = detailsEdit.locationText
				duration = detailsEdit.durationText
				depth = detailsEdit.depthText
				airtemp = detailsEdit.airtempText
				watertemp = detailsEdit.watertempText
				suit = detailsEdit.suitText
				buddy = detailsEdit.buddyText
				divemaster = detailsEdit.divemasterText
				notes = detailsEdit.notesText
				// back to view state and close the drawer
				diveDetailsWindow.state = "view"
				contextDrawer.close()
			}
		}
	]
	contextualActions: diveDetailsWindow.state === "view" ? viewActions : editActions

	ScrollView {
		anchors.fill: parent
		Flickable {
			id: flick
			anchors.fill: parent
			contentHeight: content.height
			interactive: contentHeight > height
			clip: true
			Item {
				id: content
				width: flick.width
				height: childrenRect.height + MobileComponents.Units.smallSpacing * 2

				DiveDetailsEdit {
					id: detailsEdit
					anchors {
						left: parent.left
						right: parent.right
						top: parent.top
						margins: MobileComponents.Units.gridUnit / 2
					}
					visible: opacity > 0

					Behavior on opacity {
						NumberAnimation { duration: MobileComponents.Units.shortDuration }
					}
				}
				DiveDetailsView {
					id: detailsView
					anchors {
						left: parent.left
						right: parent.right
						top: parent.top
						margins: MobileComponents.Units.gridUnit / 2
					}
					visible: opacity > 0

					Behavior on opacity {
						NumberAnimation { duration: MobileComponents.Units.shortDuration }
					}

				}
			}
		}
	}
}
