import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import org.kde.kirigami 1.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: page
	objectName: "DiveList"
	title: "Subsurface-mobile"
	background: Rectangle {
		color: Kirigami.Theme.viewBackgroundColor
	}

	property int credentialStatus: manager.credentialStatus
	property int numDives: diveListView.count
	property color textColor: subsurfaceTheme.diveListTextColor

	function scrollToTop() {
		diveListView.positionViewAtBeginning()
	}

	Component {
		id: diveDelegate
		Kirigami.AbstractListItem {
			enabled: true
			supportsMouseEvents: true
			checked: diveListView.currentIndex === model.index
			width: parent.width

			property real detailsOpacity : 0
			property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1

			// When clicked, the mode changes to details view
			onClicked: {
				if (detailsWindow.state === "view") {
					diveListView.currentIndex = index
					detailsWindow.showDiveIndex(index);
					stackView.push(detailsWindow);
				}
			}

			Item {
				width: parent.width - Kirigami.Units.gridUnit
				height: childrenRect.height - Kirigami.Units.smallSpacing

				Kirigami.Label {
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
				Kirigami.Label {
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
						topMargin: - Kirigami.Units.smallSpacing * 2
						bottom: numberText.bottom
					}
					Kirigami.Label {
						text: 'Depth: '
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
					Kirigami.Label {
						text: dive.depth
						width: Math.max(Kirigami.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
					Kirigami.Label {
						text: 'Duration: '
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
					Kirigami.Label {
						text: dive.duration
						font.pointSize: subsurfaceTheme.smallPointSize
						color: textColor
					}
				}
				Kirigami.Label {
					id: numberText
					text: "#" + dive.number
					font.pointSize: subsurfaceTheme.smallPointSize
					color: textColor
					anchors {
						right: parent.right
						top: locationText.bottom
						topMargin: - Kirigami.Units.smallSpacing * 2
					}
				}
			}
		}
	}

	Component {
		id: tripHeading
		Item {
			width: page.width - Kirigami.Units.gridUnit
			height: childrenRect.height + Kirigami.Units.smallSpacing * 2 + Math.max(2, Kirigami.Units.gridUnit / 2)

			Kirigami.Heading {
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
					topMargin: Math.max(2, Kirigami.Units.gridUnit / 2)
					leftMargin: Kirigami.Units.gridUnit / 2
					right: parent.right
				}
				color: textColor
				level: 2
			}
			Rectangle {
				height: Math.max(2, Kirigami.Units.gridUnit / 12) // we want a thicker line
				anchors {
					top: sectionText.bottom
					left: parent.left
					leftMargin: Kirigami.Units.gridUnit * -2
					rightMargin: Kirigami.Units.gridUnit * -2
					right: parent.right
				}
				color: subsurfaceTheme.accentColor
			}
		}
	}

	ScrollView {
		id: startPageWrapper
		anchors.fill: parent
		opacity: (diveListView.count > 0 && (credentialStatus == QMLManager.VALID || credentialStatus == QMLManager.VALID_EMAIL)) ? 0 : 1
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration } }
		onVisibleChanged: {
			if (visible) {
				page.mainAction = page.saveAction
			} else {
				page.mainAction = page.addDiveAction
			}
		}

		StartPage {
			id: startPage
		}
	}

	ListView {
		id: diveListView
		anchors.fill: parent
		opacity: 0.8 - startPageWrapper.opacity
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
		header: Kirigami.Heading {
			x: Kirigami.Units.gridUnit / 2
			height: paintedHeight + Kirigami.Units.gridUnit / 2
			verticalAlignment: Text.AlignBottom
			text: "Dive Log"
		}
		Connections {
			target: detailsWindow
			onCurrentIndexChanged: diveListView.currentIndex = detailsWindow.currentIndex
		}
		Connections {
			target: stackView
			onDepthChanged: {
				if (stackView.depth === 1) {
					diveListView.currentIndex = -1;
				}
			}
		}
	}

	property QtObject addDiveAction: Action {
		iconName: "list-add"
		onTriggered: {
			startAddDive()
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
}
