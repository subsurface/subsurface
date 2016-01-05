import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

Rectangle {
	id: topPart

	property bool goBack: (stackView.depth > 1)

	color: subsurfaceTheme.accentColor
	Layout.minimumHeight: MobileComponents.Units.gridUnit * 2.5
	Layout.fillWidth: true
	Layout.margins: 0
	RowLayout {
		anchors.bottom: topPart.bottom
		anchors.bottomMargin: MobileComponents.Units.smallSpacing
		anchors.left: topPart.left
		anchors.leftMargin: MobileComponents.Units.smallSpacing
		anchors.right: topPart.right
		anchors.rightMargin: MobileComponents.Units.smallSpacing
		Item {
			id: mainMenu
			anchors.left: parent.left
			Layout.preferredHeight: mainMenuIcon.height
			width: mainMenuIcon.width
			Image {
				id: mainMenuIcon
				source: "qrc:/qml/main-menu.png"
				width: MobileComponents.Units.gridUnit * 1.5
				height: width
				anchors {
					top: parent.top
					topMargin: MobileComponents.Units.smallSpacing * -1
					leftMargin: MobileComponents.Units.smallSpacing

				}
			}
			MouseArea {
				height: parent.height
				width: parent.width
				onClicked: {
					globalDrawer.open()
				}
			}
		}
		Item {
			Layout.preferredHeight: subsurfaceLogo.height
			Rectangle { color: "green"; anchors.fill: parent; }
			Image {
				id: subsurfaceLogo
				source: "qrc:/qml/subsurface-mobile-icon.png"
				anchors {
					top: parent.top
					topMargin: MobileComponents.Units.smallSpacing * -1
					left: parent.left
				}
				width: MobileComponents.Units.gridUnit * 1.7
				height: width
			}
			MobileComponents.Label {
				text: qsTr("Subsurface-mobile")
				font.pointSize: MobileComponents.Theme.defaultFont.pointSize * 1.3
				height: subsurfaceLogo.height * 2
				anchors {
					left: subsurfaceLogo.right
					bottom: subsurfaceLogo.bottom
					leftMargin: MobileComponents.Units.gridUnit / 2
				}
				font.weight: Font.Light
				verticalAlignment: Text.AlignBottom
				Layout.fillWidth: false
				color: subsurfaceTheme.accentTextColor
			}
		}
		Item {
			Layout.fillWidth: true
		}
		Item {
			id: contextMenu
			visible: contextDrawer.enabled
			anchors.right: parent.right
			anchors.top: parent.top
			Layout.preferredHeight: contextMenuIcon.height
			width: contextMenuIcon.width
			Image {
				id: contextMenuIcon
				source: "qrc:/qml/context-menu.png"
				width: MobileComponents.Units.gridUnit * 1.5
				height: width
				anchors {
					top: parent.top
					right: parent.right
					topMargin: MobileComponents.Units.smallSpacing * -1
					rightMargin: MobileComponents.Units.smallSpacing
				}
			}
			MouseArea {
				height: parent.height
				width: parent.width
				onClicked: {
					contextDrawer.open()
				}
			}
		}

	}
}
