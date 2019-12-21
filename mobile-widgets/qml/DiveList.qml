// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.kirigami 2.5 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: page
	objectName: "DiveList"
	title: qsTr("Dive list")
	verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
	property int credentialStatus: prefs.credentialStatus
	property int numDives: diveListView.count
	property color textColor: subsurfaceTheme.textColor
	property color secondaryTextColor: subsurfaceTheme.secondaryTextColor
	property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1
	property QtObject diveListModel: diveTripModel
	property string numShownText

	opacity: 0
	Behavior on opacity {
		NumberAnimation {
			duration: 200
			easing.type: Easing.OutQuad
		}
	}

	supportsRefreshing: true
	onRefreshingChanged: {
		if (refreshing) {
			if (prefs.credentialStatus === CloudStatus.CS_VERIFIED) {
				detailsWindow.endEditMode()
				manager.saveChangesCloud(true)
				refreshing = false
			} else {
				manager.appendTextToLog("sync with cloud storage requested, but credentialStatus is " + prefs.credentialStatus)
				manager.appendTextToLog("no syncing, turn off spinner")
				refreshing = false
			}
		}
	}
//	this will be: onVisibleChanged: {
	function simulateVisibleChanged() {
		console.log("---> DiveList changed visibility to " + visible)
		if (visible) {
			page.actions.main = page.downloadFromDCAction
			page.actions.right = page.addDiveAction
			page.actions.left = page.filterToggleAction
			page.title = qsTr("Dive list")
			if (diveListView.count === 0)
				showPassiveNotification(qsTr("Please tap the '+' button to add a dive (or download dives from a supported dive computer)"), 3000)
		}
	}

	Component {
		id: diveDelegate
		Kirigami.AbstractListItem {
			// this allows us to access properties of the currentItem from outside
			property variant myData: model

			leftPadding: 0
			topPadding: 0
			id: innerListItem
			enabled: true
			supportsMouseEvents: true
			checked: model.selected
			width: parent.width
			height: (collapsed & 1) ? diveListEntry.height + Kirigami.Units.smallSpacing : 0
			visible: collapsed & 1
			backgroundColor: checked ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
			activeBackgroundColor: subsurfaceTheme.primaryColor
			textColor: checked ? subsurfaceTheme.primaryTextColor : subsurfaceTheme.textColor

			states: [
				State {
					name: "isHidden";
					when: (collapsed & 1) == 0
					PropertyChanges {
						target: innerListItem
						height: 0
						visible: false
					}
				},
				State {
					name: "isVisible";
					when: (collapsed & 1) == 1
					PropertyChanges {
						target: innerListItem
						height: diveListEntry.height + Kirigami.Units.smallSpacing
						visible: true
					}
				}
			]
			// When clicked, the mode changes to details view
			onClicked: {
				if (detailsWindow.state === "view") {
					//diveListView.currentIndex = index
					detailsWindow.showDiveIndex(id);
					// switch to detailsWindow (or push it if it's not in the stack)
					var i = rootItem.pageIndex(detailsWindow)
					if (i === -1)
						pageStack.push(detailsWindow)
					else
						pageStack.currentIndex = i
				}
			}

			property bool deleteButtonVisible: false
			property bool copyButtonVisible: false
			property bool pasteButtonVisible: false

			onPressAndHold: {
				deleteButtonVisible = true
				copyButtonVisible = true
				pasteButtonVisible = true
				timer.restart()
			}
			Item {
				Rectangle {
					id: leftBarDive
					width: tripId == "" ? 0 : Kirigami.Units.smallSpacing
					height: diveListEntry.height * 0.8
					color: subsurfaceTheme.lightPrimaryColor
					anchors {
						left: parent.left
						top: parent.top
						leftMargin: Kirigami.Units.smallSpacing
						topMargin: Kirigami.Units.smallSpacing * 2
						bottomMargin: Kirigami.Units.smallSpacing * 2
					}
				}
				Item {
					id: diveListEntry
					width: parent.width - Kirigami.Units.gridUnit * (innerListItem.deleteButtonVisible ? 3 * 3 : 1)
					height: Math.ceil(childrenRect.height + Kirigami.Units.smallSpacing)
					anchors.left: leftBarDive.right
					Controls.Label {
						id: locationText
						text: (undefined !== location && "" != location) ? location : qsTr("<unnamed dive site>")
						font.weight: Font.Bold
						font.pointSize: subsurfaceTheme.regularPointSize
						elide: Text.ElideRight
						maximumLineCount: 1 // needed for elide to work at all
						color: textColor
						anchors {
							left: parent.left
							leftMargin: horizontalPadding * 2
							topMargin: Kirigami.Units.smallSpacing
							top: parent.top
							right: parent.right
						}
					}
					Row {
						anchors {
							left: locationText.left
							top: locationText.bottom
							topMargin: Kirigami.Units.smallSpacing
							bottom: numberText.bottom
						}

						Controls.Label {
							id: dateLabel
							text: (undefined !== dateTime) ? dateTime : ""
							width: Math.max(locationText.width * 0.45, paintedWidth) // helps vertical alignment throughout listview
							font.pointSize: subsurfaceTheme.smallPointSize
							color: innerListItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : secondaryTextColor
						}
						// spacer, just in case
						Controls.Label {
							text: " "
							width: Kirigami.Units.largeSpacing
						}
						// let's try to show the depth / duration very compact
						Controls.Label {
							text: (undefined !== depthDuration) ? depthDuration : ""
							width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
							font.pointSize: subsurfaceTheme.smallPointSize
							color: innerListItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : secondaryTextColor
						}
					}
					Controls.Label {
						id: numberText
						text: "#" + number
						font.pointSize: subsurfaceTheme.smallPointSize
						color: innerListItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : secondaryTextColor
						anchors {
							right: parent.right
							rightMargin: horizontalPadding
							top: locationText.bottom
							topMargin: Kirigami.Units.smallSpacing
						}
					}
				}
				Rectangle {
					id: copyButton
					visible: copyButtonVisible
					height: diveListEntry.height - 2 * Kirigami.Units.smallSpacing
					width: height
					color: subsurfaceTheme.lightDrawerColor
					antialiasing: true
					radius: Kirigami.Units.smallSpacing
					anchors {
						left: diveListEntry.right
						verticalCenter: diveListEntry.verticalCenter
						verticalCenterOffset: Kirigami.Units.smallSpacing / 2
						rightMargin: horizontalPadding * 2
						leftMargin: horizontalPadding * 2
					}
					Kirigami.Icon {
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
						source: ":/icons/edit-copy"
						width: parent.height
						height: width
					}
					MouseArea {
						anchors.fill: parent
						enabled: parent.visible
						onClicked: {
							deleteButtonVisible = false
							copyButtonVisible = false
							pasteButtonVisible = false
							timer.stop()
							manager.copyDiveData(id)
						}
						onPressAndHold: {
								globalDrawer.close()
								manager.copyDiveData(id)
								pageStack.push(settingsCopyWindow)
						}
					}
				}
				Rectangle {
					id: pasteButton
					visible: pasteButtonVisible
					height: diveListEntry.height - 2 * Kirigami.Units.smallSpacing
					width: height
					color: subsurfaceTheme.lightDrawerColor
					antialiasing: true
					radius: Kirigami.Units.smallSpacing
					anchors {
						left: copyButton.right
						verticalCenter: diveListEntry.verticalCenter
						verticalCenterOffset: Kirigami.Units.smallSpacing / 2
						rightMargin: horizontalPadding * 2
						leftMargin: horizontalPadding * 2
					}
					Kirigami.Icon {
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
						source: ":/icons/edit-paste"
						width: parent.height
						height: width
					}
					MouseArea {
						anchors.fill: parent
						enabled: parent.visible
						onClicked: {
							deleteButtonVisible = false
							copyButtonVisible = false
							pasteButtonVisible = false
							timer.stop()
							manager.pasteDiveData(id)
						}
					}
				}
				Rectangle {
					id: deleteButton
					visible: deleteButtonVisible
					height: diveListEntry.height - 2 * Kirigami.Units.smallSpacing
					width: height
					color: subsurfaceTheme.contrastAccentColor
					antialiasing: true
					radius: Kirigami.Units.smallSpacing
					anchors {
						left: pasteButton.right
						right: parent.right
						verticalCenter: diveListEntry.verticalCenter
						verticalCenterOffset: Kirigami.Units.smallSpacing / 2
						rightMargin: horizontalPadding * 2
						leftMargin: horizontalPadding * 2
					}
					Kirigami.Icon {
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
						source: ":/icons/trash-empty"
						width: parent.height
						height: width
					}
					MouseArea {
						anchors.fill: parent
						enabled: parent.visible
						onClicked: {
							deleteButtonVisible = false
							copyButtonVisible = false
							pasteButtonVisible = false
							timer.stop()
							manager.deleteDive(id)
						}
					}
				}
				Timer {
					id: timer
					interval: 4000
					onTriggered: {
						deleteButtonVisible = false
						copyButtonVisible = false
						pasteButtonVisible = false
					}
				}
			}
		}
	}

	Component {
		id: tripHeading
		Item {
			width: page.width
			height: childrenRect.height
			Rectangle {
				id: headingBackground
				height: section == "" ? 0 : sectionText.height + Kirigami.Units.gridUnit
				anchors {
					left: parent.left
					right: parent.right
				}
				color: subsurfaceTheme.lightPrimaryColor
				visible: section != ""
				Rectangle {
					id: dateBox
					visible: section != ""
					height: section == "" ? 0 : parent.height - Kirigami.Units.smallSpacing
					width: section == "" ? 0 : 2.5 * Kirigami.Units.gridUnit * PrefDisplay.mobile_scale
					color: subsurfaceTheme.primaryColor
					radius: Kirigami.Units.smallSpacing * 2
					antialiasing: true
					anchors {
						verticalCenter: parent.verticalCenter
						left: parent.left
						leftMargin: Kirigami.Units.smallSpacing
					}
					Controls.Label {
						text: {
							diveListModel ? diveListModel.tripShortDate(section) : "no data model"
						}
						color: subsurfaceTheme.primaryTextColor
						font.pointSize: subsurfaceTheme.smallPointSize
						lineHeightMode: Text.FixedHeight
						lineHeight: Kirigami.Units.gridUnit *.9
						horizontalAlignment: Text.AlignHCenter
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
					}
				}
				MouseArea {
					anchors.fill: headingBackground
					onClicked: {
						if (diveListModel) {
							if (diveListModel.activeTrip()  === section)
								diveListModel.setActiveTrip("")
							else
								diveListModel.setActiveTrip(section)
						}
					}
				}
				Controls.Label {
					id: sectionText
					text: {
						diveListModel ? diveListModel.tripTitle(section) : "no data model"
					}
					wrapMode: Text.WrapAtWordBoundaryOrAnywhere
					visible: text !== ""
					font.weight: Font.Bold
					font.pointSize: subsurfaceTheme.regularPointSize
					anchors {
						top: parent.top
						left: dateBox.right
						topMargin: Math.max(2, Kirigami.Units.gridUnit / 2)
						leftMargin: horizontalPadding * 2
						right: parent.right
					}
					color: subsurfaceTheme.lightPrimaryTextColor
				}
			}
			Rectangle {
				height: section == "" ? 0 : 1
				width: parent.width
				anchors.top: headingBackground.bottom
				color: "#B2B2B2"
			}
		}
	}

	StartPage {
		id: startPage
		anchors.fill: parent
		opacity: (credentialStatus === CloudStatus.CS_NOCLOUD ||
			 credentialStatus === CloudStatus.CS_VERIFIED) ? 0 : 1
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
		onVisibleChanged: {
			console.log("---> Startpage changed visibility to " + visible)
			if (visible) {
				page.actions.main = null
				page.actions.right = null
				page.actions.left = null
				page.title = qsTr("Cloud credentials")
			} else {
				// This is to be removed when StartPage becomes a proper page
				page.simulateVisibleChanged()
			}
		}
	}

	Controls.Label {
		anchors.fill: parent
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		text: diveListModel ? qsTr("No dives in dive list") : qsTr("Please wait, filtering dive list")
		visible: diveListView.visible && diveListView.count === 0
	}
	Component {
		id: filterHeader
		Rectangle {
			id: filterRectangle
			visible: filterBar.height > 0
			implicitHeight: filterBar.implicitHeight
			implicitWidth: filterBar.implicitWidth
			height: filterBar.height
			anchors.left: parent.left
			anchors.right: parent.right
			color: subsurfaceTheme.backgroundColor
			enabled: rootItem.filterToggle
			RowLayout {
				id: filterBar
				z: 5 //make sure it sits on top
				states: [
					State {
						name: "isVisible"
						when: rootItem.filterToggle
						PropertyChanges { target: filterBar; height: sitefilter.implicitHeight }
					},
					State {
						name: "isHidden"
						when: !rootItem.filterToggle
						PropertyChanges { target: filterBar; height: 0 }
					}

				]
				transitions: [
					Transition {
						NumberAnimation { property: "height"; duration: 400; easing.type: Easing.InOutQuad }
					}
				]

				anchors.left: parent.left
				anchors.right: parent.right
				anchors.leftMargin: Kirigami.Units.gridUnit / 2
				anchors.rightMargin: Kirigami.Units.gridUnit / 2
				Controls.TextField  {
					id: sitefilter
					z: 10
					verticalAlignment: TextInput.AlignVCenter
					Layout.fillWidth: true
					text: ""
					placeholderText: "Full text search"
					onAccepted: {
						manager.setFilter(text)
					}
					onEnabledChanged: {
						// reset the filter when it gets toggled
						text = ""
						if (visible) {
							forceActiveFocus()
						}
					}
				}
				Controls.Label {
					id: numShown
					z: 10
					verticalAlignment: Text.AlignVCenter
					text: numShownText
				}
			}
		}
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		model: diveListModel
		currentIndex: -1
		delegate: diveDelegate
		header: filterHeader
		headerPositioning: ListView.OverlayHeader
		boundsBehavior: Flickable.DragOverBounds
		maximumFlickVelocity: parent.height * 5
		bottomMargin: Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit
		cacheBuffer: 40 // this will increase memory use, but should help with scrolling
		section.property: "tripId"
		section.criteria: ViewSection.FullString
		section.delegate: tripHeading
		section.labelPositioning: ViewSection.CurrentLabelAtStart | ViewSection.InlineLabels
		onModelChanged: {
			numShownText = diveModel.shown()
		}
		Connections {
			target: detailsWindow
			onCurrentIndexChanged: diveListView.currentIndex = detailsWindow.currentIndex
		}
	}

	function showDownloadPage(vendor, product, connection) {
		downloadFromDc.dcImportModel.clearTable()
		pageStack.push(downloadFromDc)
		if (vendor !== undefined && product !== undefined && connection !== undefined) {
			/* set up the correct values on the download page */
			if (vendor !== -1)
				downloadFromDc.vendor = vendor
			if (product !== -1)
				downloadFromDc.product = product
			if (connection !== -1)
				downloadFromDc.connection = connection
		}
	}

	property QtObject downloadFromDCAction: Kirigami.Action {
		icon {
			name: ":/icons/downloadDC"
			color: subsurfaceTheme.primaryColor
		}
		text: qsTr("Download dives")
		onTriggered: {
			showDownloadPage()
		}
	}

	property QtObject addDiveAction: Kirigami.Action {
		icon {
			name: ":/icons/list-add"
		}
		text: qsTr("Add dive")
		onTriggered: {
			startAddDive()
		}
	}

	property QtObject filterToggleAction: Kirigami.Action {
		icon {
			name: ":icons/ic_filter_list"
		}
		text: qsTr("Filter dives")
		onTriggered: {
			rootItem.filterToggle = !rootItem.filterToggle
			manager.setFilter("")
			numShownText = diveModel.shown()
		}
	}

	onBackRequested: {
		if (Qt.platform.os != "ios")
			manager.quit()

		// let's make sure Kirigami doesn't quit on our behalf
		event.accepted = true
	}

	function setCurrentDiveListIndex(idx, noScroll) {
		// pick the dive in the dive list and make sure its trip is expanded
		diveListView.currentIndex = idx
		if (diveListModel)
			diveListModel.setActiveTrip(diveListView.currentItem.myData.tripId)

		// update the diveDetails page to also show that dive
		detailsWindow.showDiveIndex(idx)

		// updating the index of the ListView triggers a non-linear scroll
		// animation that can be very slow. the fix is to stop this animation
		// by setting contentY to itself and then using positionViewAtIndex().
		// the downside is that the view jumps to the index immediately.
		if (noScroll) {
			diveListView.contentY = diveListView.contentY
			diveListView.positionViewAtIndex(idx, ListView.Center)
		}
	}
}
