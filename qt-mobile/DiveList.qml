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

			width: diveListView.width
			height: childrenRect.height

			//Bounded rect for the background
			Rectangle {
				id: background
				x: 2; y: 2; width: parent.width - x*2; height: parent.height - y*2;
				color: "ivory"
				border.color: "lightblue"
				radius: 5
			}

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
			Row {
				id: topLayout
				x: 10; y: 10; height: childrenRect.height; width: parent.width

				Column {
					width: background.width; height: childrenRect.height * 1.1
					spacing: 5

					Text {
						text: diveNumber + ' (' + date + ')'
					}
					Text { text: location; width: parent.width }
					Text { text: '<b>Depth:</b> ' + depth + ' <b>Duration:</b> ' + duration; width: parent.width }
				}
			}
		}
	}

	Component {
		id: tripHeading
		Rectangle {
			width: page.width
			height: childrenRect.height
			color: "lightsteelblue"

			Text {
				text: section
				font.bold: true
				font.pointSize: 16
			}
		}
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		model: diveModel
		delegate: diveDelegate
		focus: true

		section.property: "trip"
		section.criteria: ViewSection.FullString
		section.delegate: tripHeading
	}
}
