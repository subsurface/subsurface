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
		width: parent.width; height: parent.height
		anchors.fill: parent

		Component {
			id: diveDelegate
			Item {
				id: wrapper
				width: parent.width; height: 55
				Column {
					Text { text: '#:' + diveNumber + "(" + date + ")"  }
					Text { text: 'Duration: ' + duration }
					Text { text: 'Depth: ' + depth }
				}
				MouseArea { anchors.fill: parent; onClicked: diveListView.currentIndex = index }

				states: State {
					name: "Current"
					when: wrapper.ListView.isCurrentItem
					PropertyChanges { target: wrapper; x:20 }
				}
				transitions: Transition {
					NumberAnimation { properties: "x"; duration: 200  }
				}
			}
		}

		Component {
			id: highlightBar
			Rectangle {
				width: parent.width; height: 50
				color: "lightsteelblue"
				radius: 5
				y: diveListView.currentItem.y;
				Behavior on y {  SpringAnimation  { spring: 2; damping: 0.1 } }
			}
		}

		Component {
			id: tripHeading
			Rectangle {
				width: container.width
				height: childrenRect.height
				color: "lightgreen"

				Text {
					text: section
					font.bold: true
				}
			}
		}

		ListView {
			id: diveListView
			width: parent.width; height: parent.height
			anchors.fill: parent
			model: diveModel
			delegate: diveDelegate
			focus: true
			highlight: highlightBar
			highlightFollowsCurrentItem: false

			section.property: "location"
			section.criteria: ViewSection.FullString
			section.delegate: tripHeading
		}
	}
}
