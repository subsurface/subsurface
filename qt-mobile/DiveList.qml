import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.subsurfacedivelog.mobile 1.0
import QtQuick.Layouts 1.0

Rectangle {
	id: page
	objectName: "DiveList"

	Component {
		id: diveDelegate
		Item {
			id: dive

			property real detailsOpacity : 0

			width: diveListView.width - units.spacing
			height: childrenRect.height

			//Mouse region: When clicked, the mode changes to details view
			MouseArea {
				anchors.fill: parent
				onClicked: {
					detailsWindow.width = parent.width
					detailsWindow.location = location
					detailsWindow.dive_id = id
					detailsWindow.buddy = buddy
					detailsWindow.suit = suit
					detailsWindow.airtemp = airtemp
					detailsWindow.watertemp = watertemp
					detailsWindow.divemaster = divemaster
					detailsWindow.notes = notes
					detailsWindow.number = diveNumber
					detailsWindow.date = date
					stackView.push(detailsWindow)
				}
			}

			//Layout of the page: (mini profile, dive no, date at the top
			//And other details at the bottom.
			Column {
				x: units.spacing
				width: parent.width - units.spacing * 2
				height: childrenRect.height + units.spacing * 2
				spacing: units.spacing / 2
				anchors.margins: units.spacing

				Text {
					text: diveNumber + ' (' + date + ')'
				}
				Text { text: location; width: parent.width }
				Text { text: '<b>Depth:</b> ' + depth + ' <b>Duration:</b> ' + duration; width: parent.width }
				Rectangle {
					color: theme.textColor
					opacity: .2
					width: parent.width
					height: Math.max(1, units.gridUnit / 24) // we really want a thin line
				}
			}
		}
	}

	Component {
		id: tripHeading
		Item {
			width: page.width - units.spacing * 2
			height: childrenRect.height + units.spacing * 2

			Text {
				id: sectionText
				text: section
				anchors {
					top: parent.top
					left: parent.left
					leftMargin: units.spacing
					right: parent.right
				}
				color: theme.textColor
				font.pointSize: 16
			}
			Rectangle {
				height: Math.max(2, units.gridUnit / 12) // we want a thicker line
				anchors {
					top: sectionText.bottom
					left: parent.left
					leftMargin: units.spacing
					right: parent.right
				}
				color: theme.accentColor
			}
		}
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		model: diveModel
		delegate: diveDelegate
		boundsBehavior: Flickable.StopAtBounds
		//highlight: Rectangle { color: theme.highlightColor; width: units.spacing }
		focus: true
		clip: true
		section.property: "trip"
		section.criteria: ViewSection.FullString
		section.delegate: tripHeading
	}
}
