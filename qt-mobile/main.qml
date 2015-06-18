import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.subsurfacedivelog.mobile 1.0

ApplicationWindow {
	title: qsTr("Subsurface")
	width: 500;
	height: 700

	FileDialog {
		id: fileOpen
		selectExisting: true
		selectMultiple: true

		onAccepted: {
			manager.setFilename(fileUrl)
		}
	}

	QMLManager {
		id: manager
	}

	menuBar: MenuBar {
		Menu {
			title: qsTr("File")
			MenuItem {
				text: qsTr("Open")
				onTriggered: fileOpen.open()
			}

			MenuItem {
				text: qsTr("Exit")
				onTriggered: Qt.quit();
			}
		}
	}

	Rectangle {
		id: page
		width: parent.width; height: parent.height

		Component {
			id: diveDelegate

			Item {
				id: dive

				property real detailsOpacity : 0

				width: diveListView.width
				height: 70

				//Bounded rect for the background
				Rectangle {
					id: background
					x: 2; y: 2; width: parent.width - x*2; height: parent.height - y*2;
					color: "ivory"
					border.color: "orange"
					radius: 5
				}

				//Mouse region: When clicked, the mode changes to details view
				MouseArea {
					anchors.fill: parent
					onClicked: dive.state = 'Details'
				}

				//Layout of the page: (mini profile, dive no, date at the tio
				//And other details at the bottom.
				Row {
					id: topLayout
					x: 10; y: 10; height: 60; width: parent.width
					spacing: 10

					Column {
						width: background.width; height: 60
						spacing: 5

						Text {
							text: diveNumber + ' (' + date + ')'
						}
						Text { text: location; width: details.width }
						Text { text: '<b>Depth:</b> ' + depth + ' <b>Duration:</b>' + duration; width: details.width }
					}
				}

				Item {
					id: details
					x: 10; width: parent.width - 20
					anchors { top: topLayout.bottom; topMargin: 10; bottom:parent.bottom; bottomMargin: 10 }
					opacity: dive.detailsOpacity

					Text {
						id: detailsTitle
						anchors.top: parent.top
						text: "Dive Details"
						font.pointSize: 12; font.bold: true
					}

					Flickable {
						id: flick
						width: parent.width
						anchors { top: detailsTitle.bottom; bottom: parent.bottom }
						contentHeight: detailsView.height
						clip: true
						Row {
							Text { text:
								    '<b>Location: </b>' + location +
								    '<br><b>Air temp: </b>' + airtemp + ' <b> Water temp: </b>' + watertemp +
								    '<br><b>Suit: </b>' + suit +
								    '<br><b>Buddy: </b>' + buddy +
								    '<br><b>Dive Master: </b>' + divemaster +
								    '<br/><b>Notes:</b><br/>' + notes; wrapMode: Text.WordWrap; width: details.width }
						}
					}
				}

				TextButton {
					y: 10
					anchors { right: background.right; rightMargin: 10 }
					opacity: dive.detailsOpacity
					text: "Close"

					onClicked: dive.state = '';
				}

				states: State {
					name: "Details"

					PropertyChanges {
						target: background
						color: "white"
					}

					PropertyChanges {
						target: dive
						detailsOpacity: 1; x:0 //Make details visible
						height: diveListView.height //Fill entire list area with the details
					}

					//Move the list so that this item is at the top
					PropertyChanges {
						target: dive.ListView.view
						explicit: true
						contentY: dive.y
					}

					//Disable flicking while we are in detailed view
					PropertyChanges {
						target: dive.ListView.view
						interactive: false
					}
				}

				transitions: Transition {
					//make the state changes smooth
					ParallelAnimation {
						ColorAnimation {
							property: "color"
							duration: 500
						}
						NumberAnimation {
							duration: 300
							properties: "detailsOpacity,x,contentY,height,width"
						}
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
}
