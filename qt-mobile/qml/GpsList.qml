import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: gpsListWindow
	width: parent.width - MobileComponents.Units.gridUnit
	anchors.margins: MobileComponents.Units.gridUnit / 2
	objectName: "gpsList"

	contextualActions: [
		Action {
			id: closeLog
			text: "Close GPS list"
			iconName: "view-readermode"
			onTriggered: {
				stackView.pop()
				contextDrawer.close()
			}
		}
	]

	Component {
		id: gpsDelegate
		MobileComponents.ListItemWithActions {
			id: gpsFix
			enabled: true
			width: parent.width
			property int horizontalPadding: MobileComponents.Units.gridUnit / 2 - MobileComponents.Units.smallSpacing  + 1

			Item {
				width: parent.width - MobileComponents.Units.gridUnit
				height: childrenRect.height - MobileComponents.Units.smallSpacing
				GridLayout {
					columns: 4
					id: timeAndName
					anchors {
						left: parent.left
						leftMargin: horizontalPadding
						right: parent.right
						rightMargin: horizontalPadding
					}
					MobileComponents.Label {
						text: 'Date: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: date
						width: Math.max(parent.width / 5, paintedWidth) // helps vertical alignment throughout listview
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: 'Name: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: name
						width: Math.max(parent.width / 5, paintedWidth) // helps vertical alignment throughout listview
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: 'Latitude: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: latitude
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: 'Longitude: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					MobileComponents.Label {
						text: longitude
						font.pointSize: subsurfaceTheme.smallPointSize
					}
				}
			}
			actions: [
				Action {
					iconName: "trash-empty"
					onTriggered: {
						print("delete this!")
					}
				}

			]
		}
	}

	ScrollView {
		anchors.fill: parent
		ListView {
			id: gpsListView
			anchors.fill: parent
			model: gpsModel
			currentIndex: -1
			delegate: gpsDelegate
			boundsBehavior: Flickable.StopAtBounds
			maximumFlickVelocity: parent.height * 5
			cacheBuffer: parent.height *5
			focus: true
			clip: true
			header: MobileComponents.Heading {
				x: MobileComponents.Units.gridUnit / 2
				height: paintedHeight + MobileComponents.Units.gridUnit / 2
				verticalAlignment: Text.AlignBottom
				text: "List of stored GPS fixes"
			}
		}
	}
}
