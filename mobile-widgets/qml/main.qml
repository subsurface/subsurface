// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.1
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami
import QtGraphicalEffects 1.0

Kirigami.ApplicationWindow {
	id: rootItem
	title: qsTr("Subsurface-mobile")
	reachableModeEnabled: false // while it's a good idea, it seems to confuse more than help
	wideScreen: false // workaround for probably Kirigami bug. See commits.
	header: Kirigami.ApplicationHeader {
		minimumHeight: 0
		preferredHeight: Math.round(Kirigami.Units.gridUnit * (Qt.platform.os == "ios" ? 2 : 1.5))
		maximumHeight: Kirigami.Units.gridUnit * 2
		background: Rectangle { color: subsurfaceTheme.primaryColor }
	}
	property alias oldStatus: prefs.oldStatus
	property alias notificationText: manager.notificationText
	property alias syncToCloud: manager.syncToCloud
	property alias locationServiceEnabled: manager.locationServiceEnabled
	property alias pluggedInDeviceName: manager.pluggedInDeviceName
	property alias showPin: prefs.showPin
	onNotificationTextChanged: {
		if (notificationText != "") {
			// there's a risk that we have a >5 second gap in update events;
			// still, keep the timeout at 5s to avoid odd unchanging notifications
			showPassiveNotification(notificationText, 5000)
		} else {
			// hiding the notification right away may be a mistake as it hides the last warning / error
			hidePassiveNotification();
		}
	}
	FontMetrics {
		id: fontMetrics
		Component.onCompleted: {
			console.log("Using the following font: " + fontMetrics.font.family)
		}

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

	function showMap() {
		globalDrawer.close()
		returnTopPage()
		stackView.push(mapPage)
	}

	function startAddDive() {
		detailsWindow.state = "add"
		detailsWindow.dive_id = manager.addDive();
		detailsWindow.number = manager.getNumber(detailsWindow.dive_id)
		detailsWindow.date = manager.getDate(detailsWindow.dive_id)
		detailsWindow.airtemp = ""
		detailsWindow.watertemp = ""
		detailsWindow.buddyModel = manager.buddyList
		detailsWindow.buddyIndex = -1
		detailsWindow.buddyText = ""
		detailsWindow.depth = ""
		detailsWindow.divemasterModel = manager.divemasterList
		detailsWindow.divemasterIndex = -1
		detailsWindow.divemasterText = ""
		detailsWindow.notes = ""
		detailsWindow.location = ""
		detailsWindow.gps = ""
		detailsWindow.duration = ""
		detailsWindow.suitModel = manager.suitList
		detailsWindow.suitIndex = -1
		detailsWindow.suitText = ""
		detailsWindow.cylinderModel = manager.cylinderInit
		detailsWindow.cylinderIndex = -1
		detailsWindow.cylinderText = ""
		detailsWindow.weight = ""
		detailsWindow.gasmix = ""
		detailsWindow.startpressure = ""
		detailsWindow.endpressure = ""
		detailsWindow.gpsCheckbox = false
		stackView.push(detailsWindow)
	}

	globalDrawer: Kirigami.GlobalDrawer {
		topContent: Image {
			source: "qrc:/qml/icons/dive.jpg"
			Layout.fillWidth: true
			sourceSize.width: parent.width
			fillMode: Image.PreserveAspectFit
			LinearGradient {
				anchors {
					left: parent.left
					right: parent.right
					top: parent.top
				}
				height: textblock.height * 2
				start: Qt.point(0, 0)
				end: Qt.point(0, height)
				gradient: Gradient {
					GradientStop {
						position: 0.0
						color: Qt.rgba(0, 0, 0, 0.8)
					}
					GradientStop {
						position: 1.0
						color: "transparent"
					}
				}
			}
			ColumnLayout {
				id: textblock
				anchors {
					left: parent.left
					top: parent.top
				}
				RowLayout {
					width: Math.min(implicitWidth, parent.width)
					Layout.margins: Kirigami.Units.smallSpacing
					Image {
						source: "qrc:/qml/subsurface-mobile-icon.png"
						fillMode: Image.PreserveAspectCrop
						sourceSize.width: Kirigami.Units.iconSizes.large
						width: Kirigami.Units.iconSizes.large
						Layout.margins: Kirigami.Units.smallSpacing
					}
					Kirigami.Heading {
						Layout.fillWidth: true
						visible: text.length > 0
						level: 1
						color: "white"
						text: "Subsurface"
						wrapMode: Text.NoWrap
						elide: Text.ElideRight
						font.weight: Font.Normal
						Layout.margins: Kirigami.Units.smallSpacing
					}
				}
				RowLayout {
					Layout.margins: Kirigami.Units.smallSpacing
					Kirigami.Heading {
						Layout.fillWidth: true
						visible: text.length > 0
						level: 3
						color: "white"
						text: prefs.cloudUserName
						wrapMode: Text.NoWrap
						elide: Text.ElideRight
						font.weight: Font.Normal
					}
				}
			}
		}

		resetMenuOnTriggered: false

		actions: [
			Kirigami.Action {
				icon {
					name: ":/icons/ic_home.svg"
				}
				text: qsTr("Dive list")
				onTriggered: {
					manager.appendTextToLog("requested dive list with credential status " + prefs.credentialStatus)
					if (prefs.credentialStatus == SsrfPrefs.CS_UNKNOWN) {
						// the user has asked to change credentials - if the credentials before that
						// were valid, go back to dive list
						if (oldStatus == SsrfPrefs.CS_VERIFIED) {
							prefs.credentialStatus = oldStatus
						}
					}
					returnTopPage()
					globalDrawer.close()
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/map-globe.svg"
				}
				text: mapPage.title
				onTriggered: {
					showMap()
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/ic_sync.svg"
				}
				text: qsTr("Dive management")
				Kirigami.Action {
					icon {
						name: ":/icons/ic_add.svg"
					}
					text: qsTr("Add dive manually")
					enabled: prefs.credentialStatus === SsrfPrefs.CS_VERIFIED ||
							prefs.credentialStatus === SsrfPrefs.CS_NOCLOUD
					onTriggered: {
						globalDrawer.close()
						returnTopPage()  // otherwise odd things happen with the page stack
						startAddDive()
					}
				}
				Kirigami.Action {
					// this of course assumes a white background - theming means this needs to change again
					icon {
						name: ":/icons/downloadDC-black.svg"
					}
					text: qsTr("Download from DC")
					enabled: true
					onTriggered: {
						globalDrawer.close()
						downloadFromDc.dcImportModel.clearTable()
						stackView.push(downloadFromDc)
					}
				}
				Kirigami.Action {
					icon {
						name: ":/icons/ic_add_location.svg"
					}
					text: qsTr("Apply GPS fixes")
					onTriggered: {
						manager.applyGpsData();
					}
				}
				Kirigami.Action {
					icon {
						name: ":/icons/cloud_sync.svg"
					}
					text: qsTr("Manual sync with cloud")
					enabled: prefs.credentialStatus === SsrfPrefs.CS_VERIFIED ||
							prefs.credentialStatus === SsrfPrefs.CS_NOCLOUD
					onTriggered: {
						if (prefs.credentialStatus === SsrfPrefs.CS_NOCLOUD) {
							returnTopPage()
							oldStatus = prefs.credentialStatus
							manager.startPageText = "Enter valid cloud storage credentials"
							prefs.credentialStatus = SsrfPrefs.CS_UNKNOWN
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
				icon {
					name: syncToCloud ?  ":/icons/ic_cloud_off.svg" : ":/icons/ic_cloud_done.svg"
				}
				text: syncToCloud ? qsTr("Disable auto cloud sync") : qsTr("Enable auto cloud sync")
					enabled: prefs.credentialStatus !== SsrfPrefs.CS_NOCLOUD
					onTriggered: {
						syncToCloud = !syncToCloud
						if (!syncToCloud) {
							showPassiveNotification(qsTr("Turning off automatic sync to cloud causes all data to only be \
stored locally. This can be very useful in situations with limited or no network access. Please choose 'Manual sync with cloud' \
if you have network connectivity and want to sync your data to cloud storage."), 10000)
						}
					}
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/ic_place.svg"
				}
				text: qsTr("GPS")
				visible: true

				Kirigami.Action {
					icon {
						name: ":/icons/ic_cloud_upload.svg"
					}
					text: qsTr("Upload GPS data")
					onTriggered: {
						globalDrawer.close();
						manager.sendGpsData();
					}
				}

				Kirigami.Action {
					icon {
						name: ":/icons/ic_cloud_download.svg"
					}
					text: qsTr("Download GPS data")
					onTriggered: {
						globalDrawer.close();
						manager.downloadGpsData();
					}
				}

				Kirigami.Action {
					icon {
						name:":/icons/ic_gps_fixed.svg"
					}
					text: qsTr("Show GPS fixes")
					onTriggered: {
						globalDrawer.close()
						returnTopPage()
						manager.populateGpsData();
						stackView.push(gpsWindow)
					}
				}

				Kirigami.Action {
					icon {
						name: ":/icons/ic_clear.svg"
					}
					text: qsTr("Clear GPS cache")
					onTriggered: {
						globalDrawer.close();
						manager.clearGpsData();
					}
				}

				Kirigami.Action {
					icon {
						name: locationServiceEnabled ?  ":/icons/ic_location_off.svg" : ":/icons/ic_place.svg"
					}
					text: locationServiceEnabled ? qsTr("Disable location service") : qsTr("Run location service")
					onTriggered: {
						globalDrawer.close();
						locationServiceEnabled = !locationServiceEnabled
					}
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/ic_info_outline.svg"
				}
				text: qsTr("About")
				onTriggered: {
					globalDrawer.close()
					stackView.push(aboutWindow)
					detailsWindow.endEditMode()
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/ic_settings.svg"
				}
				text: qsTr("Settings")
				onTriggered: {
					globalDrawer.close()
					stackView.push(settingsWindow)
					detailsWindow.endEditMode()
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/ic_adb.svg"
				}
				text: qsTr("Developer")
				visible: prefs.developer
				Kirigami.Action {
					text: qsTr("App log")
					onTriggered: {
						globalDrawer.close()
						stackView.push(logWindow)
					}
				}

				Kirigami.Action {
					text: qsTr("Theme information")
					onTriggered: {
						globalDrawer.close()
						stackView.push(themetest)
					}
				}
			},
			Kirigami.Action {
				icon {
					name: ":/icons/ic_help_outline.svg"
				}
				text: qsTr("Help")
				onTriggered: {
					Qt.openUrlExternally("https://subsurface-divelog.org/documentation/subsurface-mobile-v2-user-manual/")
				}
			}
		] // end actions
		Kirigami.Icon {
			source: ":/icons/" + (subsurfaceTheme.currentTheme != "" ? subsurfaceTheme.currentTheme : "Blue") + "_gps.svg"
			enabled: false
			visible: locationServiceEnabled
		}

	}

	function blueTheme() {
		Material.theme = Material.Light
		Material.accent = subsurfaceTheme.bluePrimaryColor
		subsurfaceTheme.currentTheme = "Blue"
		subsurfaceTheme.darkerPrimaryColor = subsurfaceTheme.blueDarkerPrimaryColor
		subsurfaceTheme.darkerPrimaryTextColor= subsurfaceTheme.blueDarkerPrimaryTextColor
		subsurfaceTheme.primaryColor = subsurfaceTheme.bluePrimaryColor
		subsurfaceTheme.primaryTextColor = subsurfaceTheme.bluePrimaryTextColor
		subsurfaceTheme.lightPrimaryColor = subsurfaceTheme.blueLightPrimaryColor
		subsurfaceTheme.lightPrimaryTextColor = subsurfaceTheme.blueLightPrimaryTextColor
		subsurfaceTheme.backgroundColor = subsurfaceTheme.blueBackgroundColor
		subsurfaceTheme.textColor = subsurfaceTheme.blueTextColor
		subsurfaceTheme.secondaryTextColor = subsurfaceTheme.blueSecondaryTextColor
		manager.setStatusbarColor(subsurfaceTheme.darkerPrimaryColor)
		subsurfaceTheme.drawerColor = subsurfaceTheme.lightDrawerColor
	}

	function pinkTheme() {
		Material.theme = Material.Light
		Material.accent = subsurfaceTheme.pinkPrimaryColor
		subsurfaceTheme.currentTheme = "Pink"
		subsurfaceTheme.darkerPrimaryColor = subsurfaceTheme.pinkDarkerPrimaryColor
		subsurfaceTheme.darkerPrimaryTextColor = subsurfaceTheme.pinkDarkerPrimaryTextColor
		subsurfaceTheme.primaryColor = subsurfaceTheme.pinkPrimaryColor
		subsurfaceTheme.primaryTextColor = subsurfaceTheme.pinkPrimaryTextColor
		subsurfaceTheme.lightPrimaryColor = subsurfaceTheme.pinkLightPrimaryColor
		subsurfaceTheme.lightPrimaryTextColor = subsurfaceTheme.pinkLightPrimaryTextColor
		subsurfaceTheme.backgroundColor = subsurfaceTheme.pinkBackgroundColor
		subsurfaceTheme.textColor = subsurfaceTheme.pinkTextColor
		subsurfaceTheme.secondaryTextColor = subsurfaceTheme.pinkSecondaryTextColor
		manager.setStatusbarColor(subsurfaceTheme.darkerPrimaryColor)
		subsurfaceTheme.drawerColor = subsurfaceTheme.lightDrawerColor
	}

	function darkTheme() {
		Material.theme = Material.Dark
		Material.accent = subsurfaceTheme.darkerPrimaryColor
		subsurfaceTheme.currentTheme = "Dark"
		subsurfaceTheme.darkerPrimaryColor = subsurfaceTheme.darkDarkerPrimaryColor
		subsurfaceTheme.darkerPrimaryTextColor= subsurfaceTheme.darkDarkerPrimaryTextColor
		subsurfaceTheme.primaryColor = subsurfaceTheme.darkPrimaryColor
		subsurfaceTheme.primaryTextColor = subsurfaceTheme.darkPrimaryTextColor
		subsurfaceTheme.lightPrimaryColor = subsurfaceTheme.darkLightPrimaryColor
		subsurfaceTheme.lightPrimaryTextColor = subsurfaceTheme.darkLightPrimaryTextColor
		subsurfaceTheme.backgroundColor = subsurfaceTheme.darkBackgroundColor
		subsurfaceTheme.textColor = subsurfaceTheme.darkTextColor
		subsurfaceTheme.secondaryTextColor = subsurfaceTheme.darkSecondaryTextColor
		manager.setStatusbarColor(subsurfaceTheme.darkerPrimaryColor)
		subsurfaceTheme.drawerColor = subsurfaceTheme.darkDrawerColor
	}

	QtObject {
		id: subsurfaceTheme
		property int regularPointSize: fontMetrics.font.pointSize
		property int titlePointSize: Math.round(regularPointSize * 1.5)
		property int smallPointSize: Math.round(regularPointSize * 0.8)

		// colors currently in use
		property string currentTheme
		property color darkerPrimaryColor
		property color darkerPrimaryTextColor
		property color primaryColor
		property color primaryTextColor
		property color lightPrimaryColor
		property color lightPrimaryTextColor
		property color backgroundColor
		property color textColor
		property color secondaryTextColor
		property color drawerColor

		// colors for the blue theme
		property color blueDarkerPrimaryColor: "#303F9f"
		property color blueDarkerPrimaryTextColor: "#ECECEC"
		property color bluePrimaryColor: "#3F51B5"
		property color bluePrimaryTextColor: "#FFFFFF"
		property color blueLightPrimaryColor: "#C5CAE9"
		property color blueLightPrimaryTextColor: "#212121"
		property color blueBackgroundColor: "#eff0f1"
		property color blueTextColor: blueLightPrimaryTextColor
		property color blueSecondaryTextColor: "#757575"

		// colors for the pink theme
		property color pinkDarkerPrimaryColor: "#C2185B"
		property color pinkDarkerPrimaryTextColor: "#ECECEC"
		property color pinkPrimaryColor: "#FF69B4"
		property color pinkPrimaryTextColor: "#212121"
		property color pinkLightPrimaryColor: "#FFDDF4"
		property color pinkLightPrimaryTextColor: "#212121"
		property color pinkBackgroundColor: "#eff0f1"
		property color pinkTextColor: pinkLightPrimaryTextColor
		property color pinkSecondaryTextColor: "#757575"

		// colors for the dark theme
		property color darkDarkerPrimaryColor: "#303F9f"
		property color darkDarkerPrimaryTextColor: "#ECECEC"
		property color darkPrimaryColor: "#3F51B5"
		property color darkPrimaryTextColor: "#ECECEC"
		property color darkLightPrimaryColor: "#C5CAE9"
		property color darkLightPrimaryTextColor: "#212121"
		property color darkBackgroundColor: "#303030"
		property color darkTextColor: darkPrimaryTextColor
		property color darkSecondaryTextColor: "#757575"

		property color contrastAccentColor: "#FF5722" // used for delete button
		property color lightDrawerColor: "#FFFFFF"
		property color darkDrawerColor: "#424242"

		property int columnWidth: Math.round(rootItem.width/(Kirigami.Units.gridUnit*28)) > 0 ? Math.round(rootItem.width / Math.round(rootItem.width/(Kirigami.Units.gridUnit*28))) : rootItem.width
		Component.onCompleted: {
			// this needs to pick the theme from persistent preference settings
			var theme = prefs.theme
			if (theme == "Blue")
				blueTheme()
			else if (theme == "Pink")
				pinkTheme()
			else
				darkTheme()
		}
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

	pageStack.onCurrentItemChanged: {
	// This is called whenever the user navigates using the breadcrumbs in the header

		// disable the left swipe to go back when on the map page
		stackView.interactive = pageStack.currentItem.objectName !== mapPage.objectName

		// is there a better way to reload the map markers instead of doing that
		// every time the map page is shown - e.g. link to the dive list model somehow?
		if (pageStack.currentItem.objectName === mapPage.objectName)
			mapPage.reloadMap()

		// In case we land on any page, not being the DiveDetails (which can be
		// in multiple states, such as add, edit or view), just end the edit/add mode
		if (pageStack.currentItem.objectName !== "DiveDetails" &&
				(detailsWindow.state === 'edit' || detailsWindow.state === 'add')) {
				detailsWindow.endEditMode()
		}
	}

	QMLPrefs {
		id: prefs
	}

	QMLManager {
		id: manager
	}

	Settings {
		id: settingsWindow
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

	DownloadFromDiveComputer {
		id: downloadFromDc
		visible: false
	}

	MapPage {
		id: mapPage
		visible: false
		anchors.fill: parent
	}

	ThemeTest {
		id: themetest
		visible: false
	}

	onPluggedInDeviceNameChanged: {
		if (detailsWindow.state === 'edit' || detailsWindow.state === 'add') {
			/* we're in the middle of editing / adding a dive */
			console.log("Download page requested by Android Intent, but adding/editing dive; no action taken")
		} else {
			console.log("Show download page for device " + pluggedInDeviceName)
			diveList.showDownloadPage()
		}
	}

	Component.onCompleted: {
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
