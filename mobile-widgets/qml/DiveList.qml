// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.kirigami 2.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: page
	objectName: "DiveList"
	title: qsTr("Dive list")
	background: Rectangle {
		color: subsurfaceTheme.backgroundColor
	}
	width: subsurfaceTheme.columnWidth
	property int credentialStatus: manager.credentialStatus
	property int numDives: diveListView.count
	property color textColor: subsurfaceTheme.textColor
	property color secondaryTextColor: subsurfaceTheme.secondaryTextColor
	property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1

	supportsRefreshing: true
	onRefreshingChanged: {
		if (refreshing) {
			if (manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL) {
				console.log("User pulled down dive list - syncing with cloud storage")
				detailsWindow.endEditMode()
				manager.saveChangesCloud(true)
				console.log("done syncing, turn off spinner")
				refreshing = false
			} else {
				console.log("sync with cloud storage requested, but credentialStatus is " + manager.credentialStatus)
				console.log("no syncing, turn off spinner")
				refreshing = false
			}
		}
	}

	Component {
		id: diveDelegate
		Kirigami.AbstractListItem {
			leftPadding: 0
			topPadding: 0
			id: innerListItem
			enabled: true
			supportsMouseEvents: true
			checked: diveListView.currentIndex === model.index
			width: parent.width
			height: diveListEntry.height + Kirigami.Units.smallSpacing
			backgroundColor: checked ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
			textColor: checked ? subsurfaceTheme.primaryTextColor : subsurfaceTheme.textColor

			property real detailsOpacity : 0

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
					height: childrenRect.height - Kirigami.Units.smallSpacing
					anchors.left: leftBarDive.right
					Kirigami.Label {
						id: locationText
						text: dive.location
						font.weight: Font.Bold
						elide: Text.ElideRight
						maximumLineCount: 1 // needed for elide to work at all
						color: textColor
						anchors {
							left: parent.left
							leftMargin: horizontalPadding * 2
							top: parent.top
							right: parent.right
						}
					}
					Row {
						anchors {
							left: locationText.left
							top: locationText.bottom
							topMargin: - Kirigami.Units.smallSpacing * 2
							bottom: numberText.bottom
						}

						Kirigami.Label {
							id: dateLabel
							text: dive.date + " " + dive.time
							width: Math.max(locationText.width * 0.45, paintedWidth) // helps vertical alignment throughout listview
							font.pointSize: subsurfaceTheme.smallPointSize
							color: secondaryTextColor
						}
						// let's try to show the depth / duration very compact
						Kirigami.Label {
							text: dive.depth + ' / ' + dive.duration
							width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
							font.pointSize: subsurfaceTheme.smallPointSize
							color: secondaryTextColor
						}
					}
					Kirigami.Label {
						id: numberText
						text: "#" + dive.number
						font.pointSize: subsurfaceTheme.smallPointSize
						color: secondaryTextColor
						anchors {
							right: parent.right
							rightMargin: horizontalPadding
							top: locationText.bottom
							topMargin: - Kirigami.Units.smallSpacing * 2
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
						source: "trash-empty"
					}
					MouseArea {
						anchors.fill: parent
						enabled: parent.visible
						onClicked: {
							parent.visible = false
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
			width: page.width - Kirigami.Units.gridUnit
			height: childrenRect.height - Kirigami.Units.smallSpacing
			Rectangle {
				id: headingBackground
				height: section == "" ? 0 : sectionText.height + Kirigami.Units.gridUnit
				anchors {
					left: parent.left
					right: parent.right
					rightMargin: Kirigami.Units.gridUnit * -2
				}
				color: subsurfaceTheme.lightPrimaryColor
				visible: section != ""
				Kirigami.Label {
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
						left: parent.left
						topMargin: Math.max(2, Kirigami.Units.gridUnit / 2)
						leftMargin: horizontalPadding * 2
						right: parent.right
					}
					color: subsurfaceTheme.lightPrimaryTextColor
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
		opacity: credentialStatus === QMLManager.NOCLOUD || (credentialStatus === QMLManager.VALID || credentialStatus === QMLManager.VALID_EMAIL) ? 0 : 1
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
		function setupActions() {
			if (visible) {
				page.actions.main = page.saveAction
				page.actions.right = page.offlineAction
				title = qsTr("Cloud credentials")
			} else if(manager.credentialStatus === QMLManager.VALID || manager.credentialStatus === QMLManager.VALID_EMAIL || manager.credentialStatus === QMLManager.NOCLOUD) {
				page.actions.main = page.addDiveAction
				page.actions.right = null
				title = qsTr("Dive list")
				if (diveListView.count === 0)
					showPassiveNotification(qsTr("Please tap the '+' button to add a dive"), 3000)
			} else {
				page.actions.main = null
				page.actions.right = null
				title = qsTr("Dive list")
			}
		}
		onVisibleChanged: {
			setupActions();
		}
		Component.onCompleted: {
			setupActions();
		}
	}

	Text {
		// make sure this gets pushed far enough down so that it's not obscured by the page title
		// it would be nicer to use Kirigami.Label, but due to a QML bug that isn't possible with a
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
		//boundsBehavior: Flickable.StopAtBounds
		maximumFlickVelocity: parent.height * 5
		bottomMargin: Kirigami.Units.iconSizes.medium + Kirigami.Units.gridUnit
		cacheBuffer: 0 // seems to avoid empty rendered profiles
		section.property: "dive.tripMeta"
		section.criteria: ViewSection.FullString
		section.delegate: tripHeading
		section.labelPositioning: ViewSection.CurrentLabelAtStart | ViewSection.InlineLabels
		Connections {
			target: detailsWindow
			onCurrentIndexChanged: diveListView.currentIndex = detailsWindow.currentIndex
		}
	}

	property QtObject addDiveAction: Kirigami.Action {
		iconName: "list-add"
		onTriggered: {
			startAddDive()
		}
	}

	property QtObject saveAction: Kirigami.Action {
		iconName: "document-save"
		onTriggered: {
			Qt.inputMethod.hide()
			startPage.saveCredentials();
		}
	}

	property QtObject offlineAction: Kirigami.Action {
		iconName: "qrc:/qml/nocloud.svg"
		onTriggered: {
			manager.syncToCloud = false
			manager.credentialStatus = QMLManager.NOCLOUD
		}
	}

	onBackRequested: {
		if (startPage.visible && diveListView.count > 0 && manager.credentialStatus !== QMLManager.INVALID) {
			manager.credentialStatus = oldStatus
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
}
