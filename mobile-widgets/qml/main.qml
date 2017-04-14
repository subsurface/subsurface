import QtQuick 2.4
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.ApplicationWindow {
	id: rootItem
	title: qsTr("Subsurface-mobile")

	header: Kirigami.ApplicationHeader {
		minimumHeight: 0
		preferredHeight: Kirigami.Units.gridUnit * (Qt.platform.os == "ios" ? 2 : 1)
		maximumHeight: Kirigami.Units.gridUnit * 2
	}
	property bool fullscreen: true
	property alias oldStatus: manager.oldStatus
	property alias accessingCloud: manager.accessingCloud
	property QtObject notification: null
	property bool showingDiveList: false
	property alias syncToCloud: manager.syncToCloud
	property alias showPin: manager.showPin

	onAccessingCloudChanged: {
		// >= 0 for updating cloud, -1 for hide, < -1 for local storage
		if (accessingCloud >= 0) {
			// we now keep updating this to show progress, so timing out after 30 seconds is more useful
			// but should still be very conservative
			showPassiveNotification("Accessing Subsurface cloud storage " + accessingCloud +"%", 30000);
		} else if (accessingCloud < -1) {
			// local storage should be much faster, so timeout of 5 seconds
			// the offset of 2 is so that things start 0 again
			showPassiveNotification("Accessing local storage " + -(accessingCloud + 2) +"%", 5000);
		} else {
			hidePassiveNotification();
		}
	}
	FontMetrics {
		id: fontMetrics
		/* this shouldn't be needed anymore
		Component.onCompleted: {
			if (Math.round(rootItem.width / Kirigami.Units.gridUnit) < 20) {
				fontMetrics.font.pointSize = fontMetrics.font.pointSize * 2 / 3
				Kirigami.Theme.defaultFont.pointSize = fontMetrics.font.pointSize
				console.log("Reduce font size for narrow screens: " + fontMetrics.font.pointSize)
			}
		}
		*/
	}
	visible: false

	// TODO: Verify where the opacity went to.
	// opacity: 0

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
		detailsWindow.buddyModel = manager.buddyInit
		detailsWindow.buddyIndex = -1
		detailsWindow.depth = ""
		detailsWindow.divemasterModel = manager.divemasterInit
		detailsWindow.divemasterIndex = -1
		detailsWindow.notes = ""
		detailsWindow.location = ""
		detailsWindow.gps = ""
		detailsWindow.duration = ""
		detailsWindow.suitModel = manager.suitInit
		detailsWindow.suitIndex = -1
		detailsWindow.weight = ""
		detailsWindow.gasmix = ""
		detailsWindow.startpressure = ""
		detailsWindow.endpressure = ""
		detailsWindow.gpsCheckbox = false
		stackView.push(detailsWindow)
	}

	globalDrawer: Kirigami.GlobalDrawer {
		title: qsTr("Subsurface")
		titleIcon: "qrc:/qml/subsurface-mobile-icon.png"

		bannerImageSource: "dive.jpg"

		actions: [
			Kirigami.Action {
				text: qsTr("Dive list")
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
				text: qsTr("Cloud credentials")
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
				text: qsTr("Manage dives")
				Kirigami.Action {
					text: qsTr("Add dive manually")
					enabled: manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL || manager.credentialStatus === QMLManager.NOCLOUD
					onTriggered: {
						returnTopPage()  // otherwise odd things happen with the page stack
						startAddDive()
					}
				}
				Kirigami.Action {
					text: qsTr("Manual sync with cloud")
					enabled: manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL || manager.credentialStatus === QMLManager.NOCLOUD
					onTriggered: {
						if (manager.credentialStatus === QMLManager.NOCLOUD) {
							returnTopPage()
							oldStatus = manager.credentialStatus
							manager.startPageText = "Enter valid cloud storage credentials"
							manager.credentialStatus = QMLManager.UNKNOWN
							globalDrawer.close()
						} else {
							globalDrawer.close()
							detailsWindow.endEditMode()
							manager.saveChangesCloud(true);
							globalDrawer.close()
						}
					}
				}
				Kirigami.Action {
				text: syncToCloud ? qsTr("Offline mode") : qsTr("Enable auto cloud sync")
					enabled: manager.credentialStatus !== QMLManager.NOCLOUD
					onTriggered: {
						syncToCloud = !syncToCloud
						if (!syncToCloud) {
							var alertText = "Turning off automatic sync to cloud causes all data\n"
							alertText +="to only be stored locally.\n"
							alertText += "This can be very useful in situations with\n"
							alertText += "limited or no network access.\n"
							alertText += "Please chose 'Manual sync with cloud' if you have network\n"
							alertText += "connectivity and want to sync your data to cloud storage."
							showPassiveNotification(alertText, 10000)
						}
					}
				}
			},
			Kirigami.Action {
				text: qsTr("GPS")
				enabled: manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL
				visible: (Qt.platform.os !== "ios")
				Kirigami.Action {
					text: qsTr("GPS-tag dives")
					onTriggered: {
						manager.applyGpsData();
					}
				}

				Kirigami.Action {
					text: qsTr("Upload GPS data")
					onTriggered: {
						manager.sendGpsData();
					}
				}

				Kirigami.Action {
					text: qsTr("Download GPS data")
					onTriggered: {
						manager.downloadGpsData();
					}
				}

				Kirigami.Action {
					text: qsTr("Show GPS fixes")
					onTriggered: {
						returnTopPage()
						manager.populateGpsData();
						stackView.push(gpsWindow)
					}
				}

				Kirigami.Action {
					text: qsTr("Clear GPS cache")
					onTriggered: {
						manager.clearGpsData();
					}
				}
				Kirigami.Action {
					text: qsTr("Preferences")
					onTriggered: {
						stackView.push(prefsWindow)
						detailsWindow.endEditMode()
					}
				}
			},
			Kirigami.Action {
				text: qsTr("Developer")
				Kirigami.Action {
					text: qsTr("App log")
					onTriggered: {
						stackView.push(logWindow)
					}
				}

				Kirigami.Action {
					text: qsTr("Theme information")
					onTriggered: {
						stackView.push(themetest)
					}
				}
			},
			Kirigami.Action {
				text: qsTr("User manual")
				onTriggered: {
					Qt.openUrlExternally("https://subsurface-divelog.org/documentation/subsurface-mobile-user-manual/")
				}
			},
			Kirigami.Action {
				text: qsTr("About")
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
				//text: qsTr("Run location service")
				id: locationCheckbox
				visible: manager.locationServiceAvailable
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
				text: Qt.platform.os == "ios" ? "" : manager.locationServiceAvailable ? qsTr("Run location service") : qsTr("No GPS source available")
			}
			onClicked: {
				print("Click.")
				locationCheckbox.checked = !locationCheckbox.checked
			}
		}
	}

	QtObject {
		id: subsurfaceTheme
		property int titlePointSize: Math.round(fontMetrics.font.pointSize * 1.5)
		property int smallPointSize: Math.round(fontMetrics.font.pointSize * 0.8)
		property color accentColor: "#2d5b9a"
		property color shadedColor: "#132744"
		property color accentTextColor: "#ececec"
		property color diveListTextColor: "#000000" // the Kirigami theme text color is too light
		property int columnWidth: Math.round(rootItem.width/(Kirigami.Units.gridUnit*28)) > 0 ? Math.round(rootItem.width / Math.round(rootItem.width/(Kirigami.Units.gridUnit*28))) : rootItem.width
	}

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
		anchors.fill: parent
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
		pageStack.defaultColumnWidth = Kirigami.Units.gridUnit * 28
	}
	/* TODO: Verify where opacity went to.
	Behavior on opacity {
		NumberAnimation {
			duration: 200
			easing.type: Easing.OutQuad
		}
	}
	*/
}
