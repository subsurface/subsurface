import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.ApplicationWindow {
	id: rootItem
	title: qsTr("Subsurface-mobile")

	header.minimumHeight: 0
	header.preferredHeight: Kirigami.Units.gridUnit * (Qt.platform.os == "ios" ? 2 : 1)
	header.maximumHeight: Kirigami.Units.gridUnit * 2
	property bool fullscreen: true
	property alias oldStatus: manager.oldStatus
	property alias accessingCloud: manager.accessingCloud
	property QtObject notification: null
	property bool showingDiveList: false
	property alias syncToCloud: manager.syncToCloud
	onAccessingCloudChanged: {
		if (accessingCloud >= 0) {
			// we now keep updating this to show progress, so timing out after 30 seconds is more useful
			// but should still be very conservative
			showPassiveNotification("Accessing Subsurface Cloud Storage " + accessingCloud +"%", 30000);
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

	function scrollToTop() {
		diveList.scrollToTop()
	}

	function showMap(location) {
		var urlPrefix = "https://www.google.com/maps/place/"
		var locationPair = location + "/@" + location
		var urlSuffix = ",5000m/data=!3m1!1e3!4m2!3m1!1s0x0:0x0"
		Qt.openUrlExternally(urlPrefix + locationPair + urlSuffix)

	}

	function startAddDive() {
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

	globalDrawer: Kirigami.GlobalDrawer {
		title: "Subsurface"
		titleIcon: "qrc:/qml/subsurface-mobile-icon.png"

		bannerImageSource: "dive.jpg"
		actions: [
			Kirigami.Action {
				text: "Dive list"
				onTriggered: {
					manager.appendTextToLog("requested dive list with credential status " + manager.credentialStatus)
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
			Kirigami.Action {
				text: "Cloud credentials"
				onTriggered: {
					returnTopPage()
					oldStatus = manager.credentialStatus
					if (diveList.numDives > 0) {
						manager.startPageText = "Enter different credentials or return to dive list"
					} else {
						manager.startPageText = "Enter valid cloud storage credentials"
					}

					manager.credentialStatus = QMLManager.UNKNOWN
				}
			},
			Kirigami.Action {
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
				Kirigami.Action {
					text: "Add dive manually"
					onTriggered: {
						returnTopPage()  // otherwise odd things happen with the page stack
						startAddDive()
					}
				}
				Kirigami.Action {
					text: "Manual sync with cloud"
					onTriggered: {
						globalDrawer.close()
						detailsWindow.endEditMode()
						manager.saveChangesCloud(true);
						globalDrawer.close()
					}
				}
				Kirigami.Action {
					text: syncToCloud ? "Disable auto cloud sync" : "Enable auto cloud sync"
					onTriggered: {
						syncToCloud = !syncToCloud
						if (!syncToCloud) {
							var alertText = "Turning off automatic sync to cloud causes all data\n"
							alertText +=" to only be stored locally.\n"
							alertText += "This can be very useful in situations with\n"
							alertText += " limited or no network access.\n"
							alertText += "Please chose 'Manual sync with cloud' if you have network\n"
							alertText += " connectivity and want to sync your data to cloud storage."
							showPassiveNotification(alertText, 10000)
						}
					}
				}
			},

			Kirigami.Action {
				text: "GPS"
				enabled: manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL
				Kirigami.Action {
					text: "GPS-tag dives"
					onTriggered: {
						manager.applyGpsData();
					}
				}

				Kirigami.Action {
					text: "Upload GPS data"
					onTriggered: {
						manager.sendGpsData();
					}
				}

				Kirigami.Action {
					text: "Download GPS data"
					onTriggered: {
						manager.downloadGpsData();
					}
				}

				Kirigami.Action {
					text: "Show GPS fixes"
					onTriggered: {
						returnTopPage()
						manager.populateGpsData();
						stackView.push(gpsWindow)
					}
				}

				Kirigami.Action {
					text: "Clear GPS cache"
					onTriggered: {
						manager.clearGpsData();
					}
				}
				Kirigami.Action {
					text: "Preferences"
					onTriggered: {
						stackView.push(prefsWindow)
						detailsWindow.endEditMode()
					}
				}
			},

			Kirigami.Action {
				text: "Developer"
				Kirigami.Action {
					text: "App log"
					onTriggered: {
						stackView.push(logWindow)
					}
				}

				Kirigami.Action {
					text: "Theme information"
					onTriggered: {
						stackView.push(themetest)
					}
				}
			},
			Kirigami.Action {
				text: "User manual"
				onTriggered: {
					Qt.openUrlExternally("https://subsurface-divelog.org/documentation/subsurface-mobile-user-manual/")
				}
			},
			Kirigami.Action {
				text: "About"
				onTriggered: {
					stackView.push(aboutWindow)
					detailsWindow.endEditMode()
				}
			}
		] // end actions

		MouseArea {
			height: childrenRect.height
			width: Kirigami.Units.gridUnit * 10
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
			Kirigami.Label {
				x: Kirigami.Units.gridUnit * 1.5
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

	contextDrawer: Kirigami.ContextDrawer {
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
		property color diveListTextColor: "#000000" // the Kirigami theme text color is too light
		property int columnWidth: Math.round(rootItem.width/(Kirigami.Units.gridUnit*30)) > 0 ? Math.round(rootItem.width / Math.round(rootItem.width/(Kirigami.Units.gridUnit*30))) : rootItem.width
	}
/*
	toolBar: TopBar {
		width: parent.width
		height: Layout.minimumHeight
	}
 */

	property Item stackView: pageStack
	pageStack.initialPage: DiveList {
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
		visible: false
	}

	ThemeTest {
		id: themetest
		visible: false
	}

	Component.onCompleted: {
		Kirigami.Theme.highlightColor = subsurfaceTheme.accentColor
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
