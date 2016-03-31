import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

MobileComponents.Page {
	id: page
	objectName: "DiveList"
	color: MobileComponents.Theme.viewBackgroundColor

	property int credentialStatus: manager.credentialStatus
	property int numDives: diveListView.count
	property color textColor: subsurfaceTheme.diveListTextColor
	Component {
		id: diveDelegate
		MobileComponents.ListItem {
			enabled: true
			checked: diveListView.currentIndex === model.index
			width: parent.width

			property real detailsOpacity : 0
			property int horizontalPadding: MobileComponents.Units.gridUnit / 2 - MobileComponents.Units.smallSpacing  + 1

			// When clicked, the mode changes to details view
			onClicked: {
				if (detailsWindow.state === "view") {
					diveListView.currentIndex = index
					detailsWindow.showDiveIndex(index);
					stackView.push(detailsWindow);
				}
			}

			Item {
				width: parent.width - MobileComponents.Units.gridUnit
				height: childrenRect.height - MobileComponents.Units.smallSpacing

				MobileComponents.Label {
					id: locationText
					text: dive.location
					font.weight: Font.Light
					elide: Text.ElideRight
					maximumLineCount: 1 // needed for elide to work at all
					color: textColor
					anchors {
						left: parent.left
						leftMargin: horizontalPadding
						top: parent.top
						right: dateLabel.left
					}
				}
				MobileComponents.Label {
					id: dateLabel
					text: dive.date + " " + dive.time
					font.pointSize: subsurfaceTheme.smallPointSize
					color: textColor
					anchors {
						right: parent.right
						top: parent.top
					}
				}
				Row {
					anchors {
						left: parent.left
						leftMargin: horizontalPadding
						right: parent.right
						rightMargin: horizontalPadding
						bottom: numberText.bottom
					}
					MobileComponents.Label {
						text: 'Depth: '
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
					MobileComponents.Label {
						text: dive.depth
						width: Math.max(MobileComponents.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
					MobileComponents.Label {
						text: 'Duration: '
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
					MobileComponents.Label {
						text: dive.duration
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
				}
				MobileComponents.Label {
					id: numberText
					text: "#" + dive.number
					font.pointSize: subsurfaceTheme.smallPointSize
					color: textColor
					anchors {
						right: parent.right
						top: locationText.bottom
					}
				}
			}
		}
	}

	Component {
		id: tripHeading
		Item {
			width: page.width - MobileComponents.Units.gridUnit
			height: childrenRect.height + MobileComponents.Units.smallSpacing * 2 + Math.max(2, MobileComponents.Units.gridUnit / 2)

			MobileComponents.Heading {
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
				anchors {
					top: parent.top
					left: parent.left
					topMargin: Math.max(2, MobileComponents.Units.gridUnit / 2)
					leftMargin: MobileComponents.Units.gridUnit / 2
					right: parent.right
				}
				color: textColor
				level: 2
			}
			Rectangle {
				height: Math.max(2, MobileComponents.Units.gridUnit / 12) // we want a thicker line
				anchors {
					top: sectionText.bottom
					left: parent.left
					leftMargin: MobileComponents.Units.gridUnit * -2
					rightMargin: MobileComponents.Units.gridUnit * -2
					right: parent.right
				}
				color: subsurfaceTheme.accentColor
			}
		}
	}

	Connections {
		target: stackView
		onDepthChanged: {
			if (stackView.depth === 1) {
				diveListView.currentIndex = -1;
			}
		}
	}

	ScrollView {
		id: outerScrollView
		anchors.fill: parent
		opacity: 0.8 - startPageWrapper.opacity
		visible: opacity > 0
		ListView {
			id: diveListView
			anchors.fill: parent
			model: diveModel
			currentIndex: -1
			delegate: diveDelegate
			boundsBehavior: Flickable.StopAtBounds
			maximumFlickVelocity: parent.height * 5
			bottomMargin: MobileComponents.Units.iconSizes.medium + MobileComponents.Units.gridUnit
			cacheBuffer: 0 // seems to avoid empty rendered profiles
			section.property: "dive.tripMeta"
			section.criteria: ViewSection.FullString
			section.delegate: tripHeading
			header: MobileComponents.Heading {
				x: MobileComponents.Units.gridUnit / 2
				height: paintedHeight + MobileComponents.Units.gridUnit / 2
				verticalAlignment: Text.AlignBottom
				text: "Dive Log"
				color: textColor
			}
			Connections {
				target: detailsWindow
				onCurrentIndexChanged: diveListView.currentIndex = detailsWindow.currentIndex
			}
		}
	}

	property QtObject saveAction: Action {
		iconName: "document-save"
		onTriggered: {
			startPage.saveCredentials();
		}
	}

	onBackRequested: {
		if (startPageWrapper.visible && diveListView.count > 0 && manager.credentialStatus != QMLManager.INVALID) {
			manager.credentialStatus = oldStatus
			event.accepted = true;
		}
	}

	ScrollView {
		id: startPageWrapper
		anchors.fill: parent
		opacity: (diveListView.count > 0 && (credentialStatus == QMLManager.VALID || credentialStatus == QMLManager.VALID_EMAIL)) ? 0 : 1
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: MobileComponents.Units.shortDuration } }
		onVisibleChanged: {
			if (visible) {
				page.mainAction = page.saveAction
			} else {
				page.mainAction = null
			}
		}

		StartPage {
			id: startPage
		}
	}
}
