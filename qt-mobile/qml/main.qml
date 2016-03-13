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
	id: rootItem
	title: qsTr("Subsurface-mobile")
	property bool fullscreen: true
	property int oldStatus: -1
	property alias accessingCloud: manager.accessingCloud
	property QtObject notification: null
	onAccessingCloudChanged: {
		if (accessingCloud) {
			showPassiveNotification("Accessing Subsurface Cloud Storage", 500000);
		} else {
			hidePassiveNotification();
		}
	}

	FontMetrics {
		id: fontMetrics
	}

	visible: false
	opacity: 0

	function returnTopPage() {
		for (var i=stackView.depth; i>1; i--) {
			stackView.pop()
		}
		detailsWindow.endEditMode()
	}

	globalDrawer: MobileComponents.GlobalDrawer {
		title: "Subsurface"
		titleIcon: "qrc:/qml/subsurface-mobile-icon.png"

		bannerImageSource: "dive.jpg"
		actions: [
			Action {
				text: "Dive list"
				onTriggered: {
					if (manager.credentialStatus == QMLManager.UNKNOWN) {
						// the user has asked to change credentials - if the credentials before that
						// were valid, go back to dive list
						if (oldStatus == QMLManager.VALID || oldStatus == QMLManager.VALID_EMAIL) {
							manager.credentialStatus = oldStatus
						}
					}
					returnTopPage()
					globalDrawer.close()
				}
			},
			Action {
				text: "Cloud credentials"
				onTriggered: {
					returnTopPage()
					oldStatus = manager.credentialStatus
					if (diveList.numDives > 0) {
						manager.startPageText = "Enter different credentials or return to dive list"
					} else {
						manager.startPageText = "Enter valdi cloud storage credentials"
					}

					manager.credentialStatus = QMLManager.UNKNOWN
				}
			},
			MobileComponents.ActionGroup {
				text: "Manage dives"
				enabled: manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL
			/*
			 * disable for the beta to avoid confusion
				Action {
					text: "Download from computer"
					onTriggered: {
						detailsWindow.endEditMode()
						stackView.push(downloadDivesWindow)
					}
				}
			 */
				Action {
					text: "Add dive manually"
					onTriggered: {
						detailsWindow.state = "add"
						detailsWindow.dive_id = manager.addDive();
						detailsWindow.number = manager.getNumber(detailsWindow.dive_id)
						detailsWindow.date = manager.getDate(detailsWindow.dive_id)
						detailsWindow.airtemp = ""
						detailsWindow.watertemp = ""
						detailsWindow.buddy = ""
						detailsWindow.depth = ""
						detailsWindow.divemaster = ""
						detailsWindow.notes = ""
						detailsWindow.location = ""
						detailsWindow.duration = ""
						detailsWindow.suit = ""
						detailsWindow.weight = ""
						detailsWindow.gasmix = ""
						detailsWindow.startpressure = ""
						detailsWindow.endpressure = ""
						stackView.push(detailsWindow)
					}
				}
				Action {
					text: "Refresh"
					onTriggered: {
						globalDrawer.close()
						detailsWindow.endEditMode()
						manager.loadDives();
					}
				}
				Action {
					text: "Upload to cloud"
					onTriggered: {
						globalDrawer.close()
						detailsWindow.endEditMode()
						manager.saveChanges();
					}
				}
			},

			MobileComponents.ActionGroup {
				text: "GPS"
				enabled: manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL
				Action {
					text: "GPS-tag dives"
					onTriggered: {
						manager.applyGpsData();
					}
				}

				Action {
					text: "Upload GPS data"
					onTriggered: {
						manager.sendGpsData();
					}
				}

				Action {
					text: "Download GPS data"
					onTriggered: {
						manager.downloadGpsData();
					}
				}

				Action {
					text: "Show GPS fixes"
					onTriggered: {
						manager.populateGpsData();
						stackView.push(gpsWindow)
					}
				}

				Action {
					text: "Clear GPS cache"
					onTriggered: {
						manager.clearGpsData();
					}
				}
				Action {
					text: "Preferences"
					onTriggered: {
						stackView.push(prefsWindow)
						detailsWindow.endEditMode()
					}
				}
			},

			MobileComponents.ActionGroup {
				text: "Developer"
				Action {
					text: "App log"
					onTriggered: {
						stackView.push(logWindow)
					}
				}

				Action {
					text: "Theme information"
					onTriggered: {
						stackView.push(themetest)
					}
				}
				Action {
					checkable: true
					checked: manager.verboseEnabled
					text: checked ? "Disable verbose (for adb logcat)" : "Enable verbose (for adb logcat)"
					onToggled: {
						manager.verboseEnabled = checked;
					}
				}
			},
			Action {
				text: "User manual"
				onTriggered: {
					manager.showUserManual()
				}
			},
			Action {
				text: "About"
				onTriggered: {
					stackView.push(aboutWindow)
					detailsWindow.endEditMode()
				}
			}
		] // end actions

		MouseArea {
			height: childrenRect.height
			width: MobileComponents.Units.gridUnit * 10
			CheckBox {
				//text: "Run location service"
				id: locationCheckbox
				anchors {
					left: parent.left
					top: parent.top
				}
				checked: manager.locationServiceEnabled
				onCheckedChanged: {
					manager.locationServiceEnabled = checked;
				}
			}
			MobileComponents.Label {
				x: MobileComponents.Units.gridUnit * 1.5
				anchors {
					left: locationCheckbox.right
					//leftMargin: units.smallSpacing
					verticalCenter: locationCheckbox.verticalCenter
				}
				text: "Run location service"
			}
			onClicked: {
				print("Click.")
				locationCheckbox.checked = !locationCheckbox.checked
			}
		}
	}

	contextDrawer: MobileComponents.ContextDrawer {
		id: contextDrawer
		actions: rootItem.pageStack.currentPage ? rootItem.pageStack.currentPage.contextualActions : null
		title: "Actions"
	}

	QtObject {
		id: subsurfaceTheme
		property int titlePointSize: Math.round(fontMetrics.font.pointSize * 1.5)
		property int smallPointSize: Math.round(fontMetrics.font.pointSize * 0.8)
		property color accentColor: "#2d5b9a"
		property color shadedColor: "#132744"
		property color accentTextColor: "#ececec"
		property int columnWidth: Math.round(rootItem.width/(MobileComponents.Units.gridUnit*30)) > 0 ? Math.round(rootItem.width / Math.round(rootItem.width/(MobileComponents.Units.gridUnit*30))) : rootItem.width
	}

	toolBar: TopBar {
		width: parent.width
		height: Layout.minimumHeight
	}

	property Item stackView: pageStack
	initialPage: DiveList {
		anchors.fill: detailsPage
		id: diveList
		opacity: 0
		Behavior on opacity {
			NumberAnimation {
				duration: 200
				easing.type: Easing.OutQuad
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

	About {
		id: aboutWindow
		visible: false
	}

	DiveDetails {
		id: detailsWindow
		visible: false
		width: parent.width
		height: parent.height
	}

	DownloadFromDiveComputer {
		id: downloadDivesWindow
		visible: false
	}

	Log {
		id: logWindow
		visible: false
	}

	GpsList {
		id: gpsWindow
	}

	ThemeTest {
		id: themetest
		visible: false
	}

	Component.onCompleted: {
		manager.finishSetup();
		rootItem.visible = true
		diveList.opacity = 1
		rootItem.opacity = 1
	}
	Behavior on opacity {
		NumberAnimation {
			duration: 200
			easing.type: Easing.OutQuad
		}
	}
}
