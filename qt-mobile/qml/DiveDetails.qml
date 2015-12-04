import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: diveDetailsWindow
	width: parent.width
	objectName: "DiveDetails"
	flickable: flick

	property string location
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

	contextualActions: [
		Action {
			text: "Save Changes"
			enabled: diveDetailsWindow.state == "edit"
			iconName: "document-save"
			onTriggered: {
				manager.saveChanges();
			}
		},
		Action {
			text: checked ? "View" : "Edit"
			checkable: true
			iconName: checked ? "view-readermode" : "document-edit"
			onTriggered: {
				diveDetailsWindow.state = checked ? "edit" : "view"
			}
		}

	]

	ScrollView {
		anchors.fill: parent
		Flickable {
			id: flick
			anchors.fill: parent
			contentHeight: content.height
			clip: true
			Item {
				id: content
				width: flick.width
				height: childrenRect.height + MobileComponents.Units.smallSpacing * 2

				Button {
					checkable: true
					text: "Edit"
					z: 999
					anchors {
						top: parent.top
						right: parent.right
						margins: MobileComponents.Units.gridUnit / 2
					}
					onCheckedChanged: {
						diveDetailsWindow.state = checked ? "edit" : "view"
					}
				}

				DiveDetailsEdit {
					id: detailsEdit
					anchors {
						left: parent.left
						right: parent.right
						top: parent.top
						margins: MobileComponents.Units.smallSpacing
					}

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
						margins: MobileComponents.Units.smallSpacing
					}

					Behavior on opacity {
						NumberAnimation { duration: MobileComponents.Units.shortDuration }
					}

				}
			}
		}
	}
}
