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
	property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1
	property QtObject diveListModel: mobileListModel

	supportsRefreshing: true
	onRefreshingChanged: {
		if (refreshing) {
			if (Backend.cloud_verification_status === Enums.CS_VERIFIED) {
				detailsWindow.endEditMode()
				manager.saveChangesCloud(true)
				refreshing = false
			} else {
				manager.appendTextToLog("sync with cloud storage requested, but credentialStatus is " + Backend.cloud_verification_status)
				manager.appendTextToLog("no syncing, turn off spinner")
				refreshing = false
			}
		}
	}

	Component {
		id: diveOrTripDelegate
		Kirigami.AbstractListItem {
			// this allows us to access properties of the currentItem from outside
			property variant myData: model
			id: diveOrTripDelegateItem
			padding: 0
			supportsMouseEvents: true
			checked: !isTrip && current
			anchors {
				left: parent.left
				right: parent.right
			}
			height: (isTrip ? 9 : 11) * Kirigami.Units.smallSpacing // delegateInnerItem.height

			// When clicked, a trip expands / unexpands, a dive is opened in DiveDetails
			onClicked: {
				if (isTrip) {
					manager.appendTextToLog("clicked on trip " + tripTitle)
					// toggle expand (backend to deal with unexpand other trip)
					manager.toggle(model.row);
				} else {
					manager.appendTextToLog("clicked on dive")
					if (detailsWindow.state === "view") {
						detailsWindow.showDiveIndex(id); // we need access to dive->id
						// switch to detailsWindow (or push it if it's not in the stack)
						var i = rootItem.pageIndex(detailsWindow)
						if (i === -1)
							pageStack.push(detailsWindow)
						else
							pageStack.currentIndex = i
					}
				}
			}

			// first we look at the trip
			Item {
				id: delegateInnerItem
				width: page.width
				height: childrenRect.height
				Rectangle {
					id: headingBackground
					height: visible ? 8 * Kirigami.Units.smallSpacing : 0
					anchors {
						topMargin: Kirigami.Units.smallSpacing / 2
						left: parent.left
						right: parent.right
					}
					color: subsurfaceTheme.lightPrimaryColor
					visible: isTrip
					Rectangle {
						id: dateBox
						height: 1.8 * Kirigami.Units.gridUnit
						width: 2.2 * Kirigami.Units.gridUnit
						color: subsurfaceTheme.primaryColor
						radius: Kirigami.Units.smallSpacing * 2
						antialiasing: true
						anchors {
							verticalCenter: parent.verticalCenter
							left: parent.left
							leftMargin: Kirigami.Units.smallSpacing
						}
						Controls.Label {
							visible: headingBackground.visible
							text: visible ? tripShortDate : ""
							color: subsurfaceTheme.primaryTextColor
							font.pointSize: subsurfaceTheme.smallPointSize * 0.8
							lineHeightMode: Text.FixedHeight
							lineHeight: Kirigami.Units.gridUnit *.8
							horizontalAlignment: Text.AlignHCenter
							height: contentHeight
							anchors {
								horizontalCenter: parent.horizontalCenter
								verticalCenter: parent.verticalCenter
							}
						}
					}
					Controls.Label {
						text: visible ? tripTitle : ""
						elide: Text.ElideRight
						visible: headingBackground.visible
						font.weight: Font.Medium
						font.pointSize: subsurfaceTheme.regularPointSize
						anchors {
							verticalCenter: parent.verticalCenter
							left: dateBox.right
							leftMargin: horizontalPadding * 2
							right: parent.right
						}
						color: subsurfaceTheme.lightPrimaryTextColor
					}
				}
				Rectangle {
					id: headingBottomLine
					height: visible ? Kirigami.Units.smallSpacing : 0
					visible: headingBackground.visible
					anchors {
						left: parent.left
						right: parent.right
						top: headingBackground.bottom
					}
					color: "#B2B2B2"
				}

				Rectangle {
					id: diveBackground
					height: visible ? diveListEntry.height + Kirigami.Units.smallSpacing : 0
					anchors {
						left: parent.left
						right: parent.right
					}
					color: subsurfaceTheme.backgroundColor
					visible: !isTrip
					Item {
						anchors.fill: parent
						Rectangle {
							id: leftBarDive
							width: Kirigami.Units.smallSpacing
							height: isTopLevel ? 0 : diveListEntry.height * 0.8
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
							height: visible ? 10 * Kirigami.Units.smallSpacing : 0
							anchors {
								right: parent.right
								left: leftBarDive.right
								verticalCenter: parent.verticalCenter
							}
							Controls.Label {
								id: locationText
								text: (undefined !== location && "" != location) ? location : qsTr("<unnamed dive site>")
								font.weight: Font.Medium
								font.pointSize: subsurfaceTheme.smallPointSize
								elide: Text.ElideRight
								maximumLineCount: 1 // needed for elide to work at all
								color: subsurfaceTheme.textColor
								anchors {
									left: parent.left
									leftMargin: horizontalPadding * 2
									topMargin: Kirigami.Units.smallSpacing / 2
									top: parent.top
									right: parent.right
								}
							}
							Row {
								anchors {
									left: locationText.left
									top: locationText.bottom
									topMargin: Kirigami.Units.smallSpacing / 2
								}

								Controls.Label {
									id: dateLabel
									text: (undefined !== dateTime) ? dateTime : ""
									width: Math.max(locationText.width * 0.45, paintedWidth) // helps vertical alignment throughout listview
									font.pointSize: subsurfaceTheme.smallPointSize
									color: diveOrTripDelegateItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.secondaryTextColor
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
									color: diveOrTripDelegateItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.secondaryTextColor
								}
							}
							Controls.Label {
								id: numberText
								text: "#" + number
								font.pointSize: subsurfaceTheme.smallPointSize
								color: diveOrTripDelegateItem.checked ? subsurfaceTheme.darkerPrimaryTextColor : subsurfaceTheme.secondaryTextColor
								anchors {
									right: parent.right
									rightMargin: Kirigami.Units.smallSpacing
									top: locationText.bottom
									topMargin: Kirigami.Units.smallSpacing / 2
								}
							}
						}
					}
				}

			}
		}
	}

	StartPage {
		id: startPage
		anchors.fill: parent
		opacity: (Backend.cloud_verification_status === Enums.CS_NOCLOUD ||
			 Backend.cloud_verification_status === Enums.CS_VERIFIED) ? 0 : 1
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
		function setupActions() {
			if (Backend.cloud_verification_status === Enums.CS_VERIFIED ||
					Backend.cloud_verification_status === Enums.CS_NOCLOUD) {
				page.actions.main = page.downloadFromDCAction
				page.actions.right = page.addDiveAction
				page.actions.left = page.filterToggleAction
				page.title = qsTr("Dive list")
				if (diveListView.count === 0)
					showPassiveNotification(qsTr("Please tap the '+' button to add a dive (or download dives from a supported dive computer)"), 3000)
			} else {
				page.actions.main = null
				page.actions.right = null
				page.actions.left = null
				page.title = qsTr("Cloud credentials")
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

	Controls.Label {
		anchors.fill: parent
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		text: diveListModel ? qsTr("No dives in dive list") : qsTr("Please wait, updating the dive list")
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
					Transition { NumberAnimation { property: "height"; duration: 400; easing.type: Easing.InOutQuad }}
				]
				anchors.left: parent.left
				anchors.right: parent.right
				anchors.leftMargin: Kirigami.Units.gridUnit / 2
				anchors.rightMargin: Kirigami.Units.gridUnit / 2
				TemplateComboBox {
					id: sitefilterMode
					editable: false
					model: ListModel {
						ListElement {text: qsTr("Fulltext")}
						ListElement {text: qsTr("People")}
						ListElement {text: qsTr("Tags")}
					}
					font.pointSize: subsurfaceTheme.smallPointSize
					Layout.preferredWidth: parent.width * 0.2
					Layout.maximumWidth: parent.width * 0.3
					onActivated:  {
						manager.setFilter(sitefilter.text, currentIndex)
					}
				}
				Controls.TextField  {
					id: sitefilter
					z: 10
					verticalAlignment: TextInput.AlignVCenter
					Layout.fillWidth: true
					text: ""
					placeholderText: sitefilterMode.currentText
					onAccepted: {
						manager.setFilter(text, sitefilterMode.currentIndex)
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
					text: diveModel.shown
				}
			}
		}
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		opacity: 1.0 - startPage.opacity
		visible: opacity > 0
		model: diveListModel
		currentIndex: -1
		delegate: diveOrTripDelegate
		header: filterHeader
		headerPositioning: ListView.OverlayHeader
		boundsBehavior: Flickable.DragOverBounds
		maximumFlickVelocity: parent.height * 5
		bottomMargin: Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit
		cacheBuffer: 40 // this will increase memory use, but should help with scrolling
		Component.onCompleted: {
			manager.appendTextToLog("finished setting up the diveListView")
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
			manager.setFilter("", 0)
		}
	}

	onBackRequested: {
		if (startPage.visible && diveListView.count > 0 &&
			Backend.cloud_verification_status !== Enums.CS_INCORRECT_USER_PASSWD) {
			Backend.cloud_verification_status = oldStatus
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
