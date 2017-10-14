// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami

Kirigami.ScrollablePage {
	id: gpsListWindow
	objectName: "gpsList"
	title: qsTr("GPS Fixes")

	Component {
		id: gpsDelegate
		Kirigami.SwipeListItem {
			id: gpsFix
			enabled: true
			width: parent.width
			GridLayout {
				columns: 4
				id: timeAndName
				width: parent.width
				Controls.Label {
					text: qsTr('Date: ')
					color: subsurfaceTheme.textColor
					opacity: 0.6
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: date
					color: subsurfaceTheme.textColor
					Layout.preferredWidth: Math.max(parent.width / 5, paintedWidth)
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: qsTr('Name: ')
					color: subsurfaceTheme.textColor
					opacity: 0.6
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: name
					color: subsurfaceTheme.textColor
					Layout.preferredWidth: Math.max(parent.width / 5, paintedWidth)
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: qsTr('Latitude: ')
					color: subsurfaceTheme.textColor
					opacity: 0.6
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: latitude
					color: subsurfaceTheme.textColor
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: qsTr('Longitude: ')
					color: subsurfaceTheme.textColor
					opacity: 0.6
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Controls.Label {
					text: longitude
					color: subsurfaceTheme.textColor
					font.pointSize: subsurfaceTheme.smallPointSize
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
	}
}
