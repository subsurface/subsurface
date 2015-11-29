import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

Rectangle {
	id: page
	objectName: "DiveList"

	Component {
		id: diveDelegate
		Item {
			id: dive

			property real detailsOpacity : 0

			width: diveListView.width - MobileComponents.Units.smallSpacing
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
			Item {
				x: MobileComponents.Units.smallSpacing
				width: parent.width - MobileComponents.Units.smallSpacing * 2
				height: childrenRect.height + MobileComponents.Units.smallSpacing * 2
				//spacing: MobileComponents.Units.smallSpacing / 2
				anchors.margins: MobileComponents.Units.smallSpacing

				Text {
					id: locationText
					text: location
					color: MobileComponents.Theme.textColor
					//font.pointSize: Math.round(MobileComponents.Units.fontMetrics.pointSize * 1.2) // why this doesn't work is a mystery to me, so ...
					scale: 1.2 // Let's see how this works, otherwise, we'll need the default point size somewhere
					transformOrigin: Item.TopLeft
					elide: Text.ElideRight
					maximumLineCount: 1 // needed for elide to work at all
					anchors {
						left: parent.left
						top: parent.top
						right: dateLabel.left
					}
				}
				Text {
					id: dateLabel
					text: date
					opacity: 0.6
					color: MobileComponents.Theme.textColor
					font.pointSize: smallPointSize
					anchors {
						right: parent.right
						top: parent.top
						bottomMargin: MobileComponents.Units.smallSpacing / 2
					}
				}
				Row {
					id: descriptionText
					anchors {
						left: parent.left
						right: parent.right
						bottom: numberText.bottom
					}
					Text {
						text: 'Depth: '
						opacity: 0.6
						color: MobileComponents.Theme.textColor
					}
					Text {
						text: depth
						width: Math.max(MobileComponents.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
						color: MobileComponents.Theme.textColor
					}
					Text {
						text: 'Duration: '
						opacity: 0.6
						color: MobileComponents.Theme.textColor
					}
					Text {
						text: duration
						color: MobileComponents.Theme.textColor
					}
				}
				Text {
					id: numberText
					text: "#" + diveNumber
					color: MobileComponents.Theme.textColor
					scale: 1.2
					transformOrigin: Item.BottomRight
					opacity: 0.4
					anchors {
						right: parent.right
						topMargin: MobileComponents.Units.smallSpacing
						top: locationText.bottom
					}
				}
				//Text { text: location; width: parent.width }
				Rectangle {
					color: MobileComponents.Theme.textColor
					opacity: .2
					height: Math.max(1, MobileComponents.Units.gridUnit / 24) // we really want a thin line
					anchors {
						left: parent.left
						right: parent.right
						top: numberText.bottom
					}
				}
			}
		}
	}

	Component {
		id: tripHeading
		Item {
			width: page.width - MobileComponents.Units.smallSpacing * 2
			height: childrenRect.height + MobileComponents.Units.smallSpacing * 2

			Text {
				id: sectionText
				text: section
				anchors {
					top: parent.top
					left: parent.left
					leftMargin: MobileComponents.Units.smallSpacing
					right: parent.right
				}
				color: MobileComponents.Theme.textColor
				font.pointSize: 16
			}
			Rectangle {
				height: Math.max(2, MobileComponents.Units.gridUnit / 12) // we want a thicker line
				anchors {
					top: sectionText.bottom
					left: parent.left
					leftMargin: MobileComponents.Units.smallSpacing
					right: parent.right
				}
				color: MobileComponents.Theme.accentColor
			}
		}
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		model: diveModel
		delegate: diveDelegate
		boundsBehavior: Flickable.StopAtBounds
		//highlight: Rectangle { color: MobileComponents.Theme.highlightColor; width: MobileComponents.Units.smallSpacing }
		focus: true
		clip: true
		section.property: "trip"
		section.criteria: ViewSection.FullString
		section.delegate: tripHeading
	}
	StartPage {
		anchors.fill: parent
		opacity: (diveListView.count == 0) ? 1.0 : 0
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: MobileComponents.Units.shortDuration } }
		Component.onCompleted: {
			print("diveListView.count " + diveListView.count);
		}
	}
}
