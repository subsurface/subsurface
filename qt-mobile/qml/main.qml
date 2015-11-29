import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.ApplicationWindow {
	title: qsTr("Subsurface mobile")
	property bool fullscreen: true
	property alias messageText: message.text

	property int titlePointSize: Math.round(fontMetrics.font.pointSize * 1.5)
	property int smallPointSize: Math.round(fontMetrics.font.pointSize * 0.7)

	FontMetrics {
		id: fontMetrics
	}

	visible: true

	globalDrawer: MobileComponents.GlobalDrawer{
		title: "Subsurface"
		titleIcon: "qrc:/qml/subsurface-mobile-icon.png"

		bannerImageSource: "dive.jpg"
		actions: [
		Action {
			text: "Preferences"
			onTriggered: {
				stackView.push(prefsWindow)
			}
		},

		Action {
			text: "Load Dives"
			onTriggered: {
				manager.loadDives();
			}
		},

		Action {
			text: "Download Dives"
			onTriggered: {
				stackView.push(downloadDivesWindow)
			}
		},

		Action {
			text: "Add Dive"
			onTriggered: {
				manager.addDive();
				stackView.push(detailsWindow)
			}
		},

		Action {
			text: "Save Changes"
			onTriggered: {
				manager.saveChanges();
			}
		},

		MobileComponents.ActionGroup {
			text: "GPS"
			Action {
			text: "Run location service"
			checkable: true
			checked: manager.locationServiceEnabled
			onToggled: {
				manager.locationServiceEnabled = checked;
			}
		}
		Action {
				text: "Apply GPS data to dives"
				onTriggered: {
						manager.applyGpsData();
				}
		}

		Action {
				text: "Send GPS data to server"
				onTriggered: {
						manager.sendGpsData();
				}
		}

		Action {
				text: "Clear stored GPS data"
				onTriggered: {
						manager.clearGpsData();
				}
		}
	},

		Action {
			text: "View Log"
			onTriggered: {
				stackView.push(logWindow)
			}
		},

		Action {
			text: "Theme Information"
			onTriggered: {
				stackView.push(themetest)
			}
		}
            ]
	}
// 	MobileComponents.Units {
// 		id: units
//
// 		property int titlePointSize: Math.round(fontMetrics.font.pointSize * 1.5)
// 		property int smallPointSize: Math.round(fontMetrics.font.pointSize * 0.7)
//
// 	}
//
// 	MobileComponents.Theme {
// 		id: theme
// 		/* Added for subsurface */
// 		property color accentColor: "#2d5b9a"
// 		property color accentTextColor: "#ececec"
// 	}

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
			text: "Run location service"
			checkable: true
			checked: manager.locationServiceEnabled
			onToggled: {
				manager.locationServiceEnabled = checked;
			}
		}

		MenuItem {
			text: "Apply GPS data to dives"
			onTriggered: {
				manager.applyGpsData();
			}
		}

		MenuItem {
			text: "Send GPS data to server"
			onTriggered: {
				manager.sendGpsData();
			}
		}

		MenuItem {
			text: "Clear stored GPS data"
			onTriggered: {
				manager.clearGpsData();
			}
		}

		MenuItem {
			text: "View Log"
			onTriggered: {
				stackView.push(logWindow)
			}
		}

		MenuItem {
			text: "Theme Information"
			onTriggered: {
				stackView.push(themetest)
			}
		}
	}

	ColumnLayout {
		anchors.fill: parent

		TopBar {

		}

		StackView {
			id: stackView
			Layout.preferredWidth: parent.width
			Layout.fillHeight: true
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
					spacing: MobileComponents.Units.gridUnit / 2

					Rectangle {
						id: detailsPage
						Layout.fillHeight: true
						Layout.fillWidth: true

						DiveList {
							anchors.fill: detailsPage
							id: diveDetails
							color: MobileComponents.Theme.backgroundColor
						}
					}

					Rectangle {
						id: messageArea
						height: childrenRect.height
						Layout.fillWidth: true
						color: MobileComponents.Theme.backgroundColor

						Text {
							id: message
							color: MobileComponents.Theme.textColor
							wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
							styleColor: MobileComponents.Theme.textColor
							font.pointSize: MobileComponents.Units.smallPointSize
						}
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

	ThemeTest {
		id: themetest
		visible: false
	}

	Component.onCompleted: {
		print("MobileComponents.Units.gridUnit is: " + MobileComponents.Units.gridUnit);
	}
}
