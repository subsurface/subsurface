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
					Layout.minimumHeight: prefsButton.height * 1.2
					Layout.fillWidth: true
					anchors.bottom: detailsPage.top
					anchors.bottomMargin: prefsButton.height * 0.1

					RowLayout {
						anchors.bottom: topPart.bottom
						anchors.bottomMargin: prefsButton.height * 0.1
						anchors.left: topPart.left
						anchors.leftMargin: prefsButton.height * 0.1
						Button {
							id: prefsButton
							text: "Preferences"
							onClicked: {
								stackView.push(prefsWindow)
							}
						}

						Button {
							id: loadDivesButton
							text: "Load Dives"
							onClicked: {
								manager.loadDives();
							}
						}

						Button {
							id: saveChanges
							text: "Save Changes"
							onClicked: {
								manager.saveChanges();
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

}
