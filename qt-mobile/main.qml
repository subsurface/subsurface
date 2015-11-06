import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import "qrc:/qml/theme" as Theme


ApplicationWindow {
	title: qsTr("Subsurface mobile")
	property bool fullscreen: true
	property alias messageText: message.text
	visible: true

	Theme.Units {
		id: units
		property int spacing: Math.ceil(gridUnit / 3)
	}

	Theme.Theme {
		id: theme
		/* Added for subsurface */
		property color accentColor: "#2d5b9a"
		property color accentTextColor: "#ececec"
	}

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
			text: "Add Dive"
			onTriggered: {
				manager.addDive();
				stackView.push(detailsWindow)
			}
		}

		MenuItem {
			text: "Save Changes"
			onTriggered: {
				manager.saveChanges();
			}
		}

		MenuItem {
			text: "View Log"
			onTriggered: {
				stackView.push(logWindow)
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
				spacing: units.gridUnit / 2
				Rectangle {
					id: topPart
					color: theme.accentColor
					Layout.minimumHeight: units.gridUnit * 2 + units.spacing * 2
					Layout.fillWidth: true
					Layout.margins: 0
					RowLayout {
						anchors.bottom: topPart.bottom
						anchors.bottomMargin: units.spacing
						anchors.left: topPart.left
						anchors.leftMargin: units.spacing
						anchors.right: topPart.right
						anchors.rightMargin: units.spacing
						Image {
							source: "qrc:/qml/subsurface-mobile-icon.png"
							Layout.maximumWidth: units.gridUnit * 2
							Layout.preferredWidth: units.gridUnit * 2
							Layout.preferredHeight: units.gridUnit * 2
						}
						Text {
							text: qsTr("Subsurface mobile")
							font.pointSize: 18
							Layout.fillWidth: false
							color: theme.accentTextColor
						}
						Item {
							Layout.fillWidth: true
						}
						Button {
							id: prefsButton
							text: "\u22ee"
							anchors.right: parent.right
							Layout.preferredWidth: units.gridUnit * 2
							Layout.preferredHeight: units.gridUnit * 2
							style: ButtonStyle {
								background: Rectangle {
									implicitWidth: units.gridUnit * 2
									color: theme.accentColor
								}
								label: Text {
									id: txt
									color: theme.accentTextColor
									font.pointSize: 18
									font.bold: true
									text: control.text
									horizontalAlignment: Text.AlignHCenter
								}
							}
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
						color: theme.backgroundColor
					}
				}

				Rectangle {
					id: messageArea
					height: childrenRect.height
					Layout.fillWidth: true

					Text {
						id: message
						color: theme.textColor
						text: ""
						styleColor: theme.textColor
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

	Log {
		id: logWindow
		visible: false
	}

	Component.onCompleted: {
		print("units.gridUnit is: " + units.gridUnit);
	}
}
