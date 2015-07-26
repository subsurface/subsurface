import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0

ApplicationWindow {
	title: qsTr("Subsurface mobile")
	property bool fullscreen: true
	property alias messageText: message.text
	visible: true

	Menu {
		id: prefsMenu
		title: "Menu"

		MenuItem {
			text: "Preferences"
			onTriggered: {
				stackView.push(prefsWindow)
			}
		}

		MenuItem {
			text: "Load Dives"
			onTriggered: {
				manager.loadDives();
			}
		}

		MenuItem {
			text: "Download Dives"
			onTriggered: {
				stackView.push(downloadDivesWindow)
			}
		}
		MenuItem {
			text: "Save Changes"
			onTriggered: {
				manager.saveChanges();
			}
		}
	}

	StackView {
		id: stackView
		anchors.fill: parent
		focus: true
		Keys.onReleased: if (event.key == Qt.Key_Back && stackView.depth > 1) {
					 stackView.pop()
					 event.accepted = true;
				 }
		initialItem: Item {
			width: parent.width
			height: parent.height

			ColumnLayout {
				id: awLayout
				anchors.fill: parent
				spacing: prefsButton.height * 0.1
				Rectangle {
					id: topPart
					color: "#2C4882"
					Layout.minimumHeight: prefsButton.height * 1.2
					Layout.fillWidth: true
					anchors.bottom: detailsPage.top
					anchors.bottomMargin: prefsButton.height * 0.1

					RowLayout {
						anchors.bottom: topPart.bottom
						anchors.bottomMargin: prefsButton.height * 0.1
						anchors.left: topPart.left
						anchors.leftMargin: prefsButton.height * 0.1
						anchors.right: topPart.right
						anchors.rightMargin: prefsButton.height * 0.1
						Button {
							id: prefsButton
							text: "\u22ee"
							anchors.right: parent.right
							onClicked: {
								prefsMenu.popup()
							}
						}

					}

				}

				Rectangle {
					id: detailsPage
					Layout.fillHeight: true
					Layout.fillWidth: true

					DiveList {
						anchors.fill: detailsPage
						id: diveDetails
						color: "#2C4882"
					}
				}

				Rectangle {
					id: messageArea
					height: childrenRect.height
					Layout.fillWidth: true

					Text {
						id: message
						color: "#000000"
						text: ""
						styleColor: "#ff0000"
						font.pointSize: 10
					}
				}

			}
		}
	}

	QMLManager {
		id: manager
	}

	Preferences {
		id: prefsWindow
		visible: false
	}

	DiveDetails {
		id: detailsWindow
		visible: false
	}

	DownloadFromDiveComputer {
		id: downloadDivesWindow
		visible: false
	}
}
