import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

MobileComponents.Page {
	id: page
	objectName: "DiveList"
	color: MobileComponents.Theme.viewBackgroundColor
	flickable: diveListView

	Component {
		id: diveDelegate
		MobileComponents.ListItem {
			id: dive
			enabled: true
			checked: diveListView.currentIndex == model.index

			property real detailsOpacity : 0

			//When clicked, the mode changes to details view

			onClicked: {
				diveListView.currentIndex = model.index
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

			Item {
				width: parent.width - MobileComponents.Units.smallSpacing * 2
				height: childrenRect.height - MobileComponents.Units.smallSpacing

				MobileComponents.Label {
					id: locationText
					text: location
					font.weight: Font.Light
					elide: Text.ElideRight
					maximumLineCount: 1 // needed for elide to work at all
					anchors {
						left: parent.left
						top: parent.top
						right: dateLabel.left
					}
				}
				MobileComponents.Label {
					id: dateLabel
					text: date
					opacity: 0.6
					font.pointSize: subsurfaceTheme.smallPointSize
					anchors {
						right: parent.right
						top: parent.top
					}
				}
				Row {
					id: descriptionText
					anchors {
						left: parent.left
						right: parent.right
						bottom: numberText.bottom
					}
					MobileComponents.Label {
						text: 'Depth: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: depth
						width: Math.max(MobileComponents.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: 'Duration: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: duration
						font.pointSize: subsurfaceTheme.smallPointSize
					}
				}
				MobileComponents.Label {
					id: numberText
					text: "#" + diveNumber
					color: MobileComponents.Theme.textColor
					font.pointSize: subsurfaceTheme.smallPointSize
					opacity: 0.6
					anchors {
						right: parent.right
						top: locationText.bottom
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

			MobileComponents.Heading {
				id: sectionText
				text: section
				anchors {
					top: parent.top
					left: parent.left
					leftMargin: MobileComponents.Units.smallSpacing
					right: parent.right
				}
				level: 2
			}
			Rectangle {
				height: Math.max(2, MobileComponents.Units.gridUnit / 12) // we want a thicker line
				anchors {
					top: sectionText.bottom
					left: parent.left
					leftMargin: MobileComponents.Units.smallSpacing
					right: parent.right
				}
				color: subsurfaceTheme.accentColor
			}
		}
	}

	Connections {
		target: stackView
		onDepthChanged: {
			if (stackView.depth == 1) {
				diveListView.currentIndex = -1;
			}
		}
	}
	ScrollView {
		anchors.fill: parent
		ListView {
			id: diveListView
			anchors.fill: parent
			model: diveModel
			currentIndex: -1
			delegate: diveDelegate
			boundsBehavior: Flickable.StopAtBounds
			//highlight: Rectangle { color: MobileComponents.Theme.highlightColor; width: MobileComponents.Units.smallSpacing }
			focus: true
			clip: true
			section.property: "trip"
			section.criteria: ViewSection.FullString
			section.delegate: tripHeading
		}
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
