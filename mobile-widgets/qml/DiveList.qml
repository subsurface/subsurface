// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.kirigami 2.2 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: page
	objectName: "DiveList"
	title: qsTr("Dive list")
	verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
	width: subsurfaceTheme.columnWidth
	property int credentialStatus: prefs.credentialStatus
	property int numDives: diveListView.count
	property color textColor: subsurfaceTheme.textColor
	property color secondaryTextColor: subsurfaceTheme.secondaryTextColor
	property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1
	property string activeTrip

	supportsRefreshing: true
	onRefreshingChanged: {
		if (refreshing) {
			if (prefs.credentialStatus === SsrfPrefs.CS_VERIFIED) {
				console.log("User pulled down dive list - syncing with cloud storage")
				detailsWindow.endEditMode()
				manager.saveChangesCloud(true)
				console.log("done syncing, turn off spinner")
				refreshing = false
			} else {
				console.log("sync with cloud storage requested, but credentialStatus is " + prefs.credentialStatus)
				console.log("no syncing, turn off spinner")
				refreshing = false
			}
		}
	}

	Component {
		id: diveDelegate
		Kirigami.AbstractListItem {
			// this looks weird, but it's how we can tell that this dive isn't in a trip
			property bool diveOutsideTrip: dive.tripNrDives === 0
			leftPadding: 0
			topPadding: 0
			id: innerListItem
			enabled: true
			supportsMouseEvents: true
			checked: diveListView.currentIndex === model.index
			width: parent.width
			height: diveOutsideTrip ? diveListEntry.height + Kirigami.Units.smallSpacing : 0
			visible: diveOutsideTrip
			backgroundColor: checked ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
			activeBackgroundColor: subsurfaceTheme.primaryColor
			textColor: checked ? subsurfaceTheme.primaryTextColor : subsurfaceTheme.textColor

			states: [
				State {
					name: "isHidden";
					when: dive.tripMeta !== activeTrip && ! diveOutsideTrip
					PropertyChanges {
						target: innerListItem
						height: 0
						visible: false
					}
				},
				State {
					name: "isVisible";
					when: dive.tripMeta === activeTrip || diveOutsideTrip
					PropertyChanges {
						target: innerListItem
						height: diveListEntry.height + Kirigami.Units.smallSpacing
						visible: true
					}
				}
			]
			transitions: [
				Transition {
					from: "isHidden"
					to: "isVisible"
					SequentialAnimation {
						NumberAnimation {
							property: "visible"
							duration: 1
						}
						NumberAnimation {
							property: "height"
							duration: 200 + 20 * dive.tripNrDives
							easing.type: Easing.InOutQuad
						}
					}
				},
				Transition {
					from: "isVisible"
					to: "isHidden"
					SequentialAnimation {
						NumberAnimation {
							property: "height"
							duration: 200 + 20 * dive.tripNrDives
							easing.type: Easing.InOutQuad
						}
						NumberAnimation {
							property: "visible"
							duration: 1
						}
					}
				}
			]

			// When clicked, the mode changes to details view
			onClicked: {
				if (detailsWindow.state === "view") {
					diveListView.currentIndex = index
					detailsWindow.showDiveIndex(index);
					stackView.push(detailsWindow);
				}
			}

			property bool deleteButtonVisible: false

			onPressAndHold: {
				deleteButtonVisible = true
				timer.restart()
			}
			Item {
				Rectangle {
					id: leftBarDive
					width: dive.tripMeta == "" ? 0 : Kirigami.Units.smallSpacing
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
					width: parent.width - Kirigami.Units.gridUnit * (innerListItem.deleteButtonVisible ? 3 : 1)
					height: Math.ceil(childrenRect.height + Kirigami.Units.smallSpacing)
					anchors.left: leftBarDive.right
					Controls.Label {
						id: locationText
						text: dive.location
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
							text: dive.date + " " + dive.time
							width: Math.max(locationText.width * 0.45, paintedWidth) // helps vertical alignment throughout listview
							font.pointSize: subsurfaceTheme.smallPointSize
							color: innerListItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : secondaryTextColor
						}
						// let's try to show the depth / duration very compact
						Controls.Label {
							text: dive.depth + ' / ' + dive.duration
							width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
							font.pointSize: subsurfaceTheme.smallPointSize
							color: innerListItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : secondaryTextColor
						}
					}
					Controls.Label {
						id: numberText
						text: "#" + dive.number
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
					visible: deleteButtonVisible
					height: diveListEntry.height - Kirigami.Units.smallSpacing
					width: height - 3 * Kirigami.Units.smallSpacing
					color: subsurfaceTheme.contrastAccentColor
					antialiasing: true
					radius: Kirigami.Units.smallSpacing
					anchors {
						left: diveListEntry.right
						right: parent.right
					}
					Kirigami.Icon {
						anchors {
							horizontalCenter: parent.horizontalCenter
							verticalCenter: parent.verticalCenter
						}
						source: ":/icons/trash-empty"
						width: Kirigami.Units.iconSizes.smallMedium
						height: width
					}
					MouseArea {
						anchors.fill: parent
						enabled: parent.visible
						onClicked: {
							deleteButtonVisible = false
							timer.stop()
							manager.deleteDive(dive.id)
						}
					}
				}
				Item {
					Timer {
						id: timer
						interval: 4000
						onTriggered: {
							deleteButtonVisible = false
						}
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
					height: section == "" ? 0 : 2 * Kirigami.Units.gridUnit
					width: section == "" ? 0 : 2.5 * Kirigami.Units.gridUnit
					color: subsurfaceTheme.primaryColor
					radius: Kirigami.Units.smallSpacing * 2
					antialiasing: true
					anchors {
						verticalCenter: parent.verticalCenter
						left: parent.left
						leftMargin: Kirigami.Units.smallSpacing
					}
					Controls.Label {
						text: {	section.replace(/.*\+\+/, "").replace(/::.*/, "").replace("@", "\n'") }
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
						if (activeTrip === section)
							activeTrip = ""
						else
							activeTrip = section
					}
				}
				Controls.Label {
					id: sectionText
					text: {
						// if the tripMeta (which we get as "section") ends in ::-- we know
						// that there's no trip -- otherwise strip the meta information before
						// the :: and show the trip location
						var shownText
						var endsWithDoubleDash = /::--$/;
						if (endsWithDoubleDash.test(section) || section === "--") {
							shownText = ""
						} else {
							shownText = section.replace(/.*::/, "")
						}
						shownText
					}
					wrapMode: Text.WrapAtWordBoundaryOrAnywhere
					visible: text !== ""
					font.weight: Font.Bold
					anchors {
						top: parent.top
						left: dateBox.right
						topMargin: Math.max(2, Kirigami.Units.gridUnit / 2)
						leftMargin: horizontalPadding * 2
						right: parent.right
					}
					color: subsurfaceTheme.lightPrimaryTextColor
				}
				MouseArea {
					anchors.fill: sectionText
					onClicked: {
						if (activeTrip === section)
							activeTrip = ""
						else
							activeTrip = section
					}
				}
			}
			Rectangle {
				height: Math.max(2, Kirigami.Units.gridUnit / 12) // we want a thicker line
				anchors {
					bottom: headingBackground.top
					left: parent.left
					rightMargin: Kirigami.Units.gridUnit * -2
					right: parent.right
				}
				color: subsurfaceTheme.lightPrimaryColor
			}
		}
	}

	StartPage {
		id: startPage
		anchors.fill: parent
		opacity: credentialStatus === SsrfPrefs.CS_NOCLOUD ||
									(credentialStatus === SsrfPrefs.CS_VERIFIED) ? 0 : 1
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
		function setupActions() {
			if (visible) {
				page.actions.main = null
				page.actions.right = null
				page.title = qsTr("Cloud credentials")
			} else if (prefs.credentialStatus === SsrfPrefs.CS_VERIFIED ||
						prefs.credentialStatus === SsrfPrefs.CS_NOCLOUD) {
				page.actions.main = page.downloadFromDCAction
				page.actions.right = page.addDiveAction
				page.title = qsTr("Dive list")
				if (diveListView.count === 0)
					showPassiveNotification(qsTr("Please tap the '+' button to add a dive (or download dives from a supported dive computer)"), 3000)
			} else {
				page.actions.main = null
				page.actions.right = null
				page.title = qsTr("Dive list")
			}
		}
		onVisibleChanged: {
			setupActions();
		}

		Component.onCompleted: {
			manager.finishSetup();
			setupActions();
		}
	}

	Text {
		// make sure this gets pushed far enough down so that it's not obscured by the page title
		// it would be nicer to use Controls.Label, but due to a QML bug that isn't possible with a
		// weird "component versioning" error
		// using this property means that we require Qt 5.6 / QtQuick2.6
		topPadding: Kirigami.Units.iconSizes.large
		leftPadding: Kirigami.Units.iconSizes.large

		text: qsTr("No dives in dive list")
		visible: diveListView.visible && diveListView.count === 0
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		opacity: 1.0 - startPage.opacity
		visible: opacity > 0
		model: diveModel
		currentIndex: -1
		delegate: diveDelegate
		boundsBehavior: Flickable.DragOverBounds
		maximumFlickVelocity: parent.height * 5
		bottomMargin: Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit
		cacheBuffer: 40 // this will increase memory use, but should help with scrolling
		section.property: "dive.tripMeta"
		section.criteria: ViewSection.FullString
		section.delegate: tripHeading
		section.labelPositioning: ViewSection.CurrentLabelAtStart | ViewSection.InlineLabels
		Connections {
			target: detailsWindow
			onCurrentIndexChanged: diveListView.currentIndex = detailsWindow.currentIndex
		}
	}

	function showDownloadPage() {
		downloadFromDc.dcImportModel.clearTable()
		stackView.push(downloadFromDc)
	}

	property QtObject downloadFromDCAction: Kirigami.Action {
		icon {
			name: ":/icons/downloadDC"
			color: subsurfaceTheme.primaryColor
		}
		onTriggered: {
			showDownloadPage()
		}
	}

	property QtObject addDiveAction: Kirigami.Action {
		icon {
			name: ":/icons/list-add"
		}
		onTriggered: {
			startAddDive()
		}
	}

	onBackRequested: {
		if (startPage.visible && diveListView.count > 0 &&
			prefs.credentialStatus !== SsrfPrefs.CS_INCORRECT_USER_PASSWD) {
			prefs.credentialStatus = oldStatus
			event.accepted = true;
		}
		if (!startPage.visible) {
			if (Qt.platform.os != "ios") {
				manager.quit()
			}
			// let's make sure Kirigami doesn't quit on our behalf
			event.accepted = true
		}
	}

	function setCurrentDiveListIndex(idx, noScroll) {
		diveListView.currentIndex = idx
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
