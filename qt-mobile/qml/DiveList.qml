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

	Component {
		id: diveDelegate
		MobileComponents.ListItem {
			enabled: true
			checked: diveListView.currentIndex == model.index
			width: parent.width

			property real detailsOpacity : 0
			property int horizontalPadding: MobileComponents.Units.gridUnit / 2 - MobileComponents.Units.smallSpacing  + 1

			// When clicked, the mode changes to details view
			onClicked: {
				diveListView.currentIndex = model.index
				detailsWindow.showDiveIndex(index);
				stackView.push(detailsWindow);
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
					opacity: 0.6
					font.pointSize: subsurfaceTheme.smallPointSize
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
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: dive.depth
						width: Math.max(MobileComponents.Units.gridUnit * 3, paintedWidth) // helps vertical alignment throughout listview
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: 'Duration: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: dive.duration
						font.pointSize: subsurfaceTheme.smallPointSize
					}
				}
				MobileComponents.Label {
					id: numberText
					text: "#" + dive.number
					color: MobileComponents.Theme.textColor
					font.pointSize: subsurfaceTheme.smallPointSize
					opacity: 0.6
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
			height: childrenRect.height + MobileComponents.Units.smallSpacing * 2

			MobileComponents.Heading {
				id: sectionText
				text: section
				anchors {
					top: parent.top
					left: parent.left
					leftMargin: MobileComponents.Units.gridUnit / 2
					right: parent.right
				}
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
			if (stackView.depth == 1) {
				diveListView.currentIndex = -1;
			}
		}
	}

	ScrollView {
		anchors.fill: parent
		ListView {
			id: diveListView
			anchors.fill: parent
			model: diveModel
			currentIndex: -1
			delegate: diveDelegate
			boundsBehavior: Flickable.StopAtBounds
			maximumFlickVelocity: parent.height * 5
			cacheBuffer: 0 // seems to avoid empty rendered profiles
			section.property: "trip" // FIXME: "dive.trip" gives weird section labels.
			section.criteria: ViewSection.FullString
			section.delegate: tripHeading
			header: MobileComponents.Heading {
				x: MobileComponents.Units.gridUnit / 2
				height: paintedHeight + MobileComponents.Units.gridUnit / 2
				verticalAlignment: Text.AlignBottom

				text: "Dive Log"
				opacity: 0.8 - startPage.opacity
				visible: opacity > 0
			}
			Connections {
				target: detailsWindow
				onCurrentIndexChanged: diveListView.currentIndex = detailsWindow.currentIndex
			}
		}
	}
	StartPage {
		id: startPage
		anchors.fill: parent
		opacity: (diveListView.count == 0) ? 1.0 : 0
		visible: opacity > 0
		Behavior on opacity { NumberAnimation { duration: MobileComponents.Units.shortDuration } }
	}
}
