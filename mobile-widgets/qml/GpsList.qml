import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.ScrollablePage {
	id: gpsListWindow
	width: parent.width - Kirigami.Units.gridUnit
	anchors.margins: Kirigami.Units.gridUnit / 2
	objectName: "gpsList"
	title: "GPS Fixes"

	Component {
		id: gpsDelegate
		Kirigami.SwipeListItem {
			id: gpsFix
			enabled: true
			width: parent.width
			property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1

			Kirigami.BasicListItem {
				supportsMouseEvents: true
				width: parent.width - Kirigami.Units.gridUnit
				height: childrenRect.height - Kirigami.Units.smallSpacing
				GridLayout {
					columns: 4
					id: timeAndName
					anchors {
						left: parent.left
						leftMargin: horizontalPadding
						right: parent.right
						rightMargin: horizontalPadding
					}
					Kirigami.Label {
						text: 'Date: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: date
						Layout.preferredWidth: Math.max(parent.width / 5, paintedWidth)
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: 'Name: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: name
						Layout.preferredWidth: Math.max(parent.width / 5, paintedWidth)
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: 'Latitude: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: latitude
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: 'Longitude: '
						opacity: 0.6
						font.pointSize: subsurfaceTheme.smallPointSize
					}
					Kirigami.Label {
						text: longitude
						font.pointSize: subsurfaceTheme.smallPointSize
					}
				}
			}
			actions: [
				Kirigami.Action {
					iconName: "trash-empty"
					onTriggered: {
						print("delete this!")
						manager.deleteGpsFix(when)
					}
				},
				Kirigami.Action {
					iconName: "gps"
					onTriggered: {
						showMap(latitude + " " + longitude)
					}
				}

			]
		}
	}

	ListView {
		id: gpsListView
		anchors.fill: parent
		model: gpsModel
		currentIndex: -1
		delegate: gpsDelegate
		boundsBehavior: Flickable.StopAtBounds
		maximumFlickVelocity: parent.height * 5
		cacheBuffer: Math.max(5000, parent.height * 5)
		focus: true
		clip: true
		header: Kirigami.Heading {
			x: Kirigami.Units.gridUnit / 2
			height: paintedHeight + Kirigami.Units.gridUnit / 2
			verticalAlignment: Text.AlignBottom
			text: "List of stored GPS fixes"
		}
	}
}
