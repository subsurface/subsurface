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

	color: subsurfaceTheme.accentColor
	Layout.minimumHeight: Math.round(MobileComponents.Units.gridUnit * 1.5)
	Layout.fillWidth: true
	Layout.margins: 0
	RowLayout {
		anchors.verticalCenter: topPart.verticalCenter
		Item {
			Layout.preferredHeight: subsurfaceLogo.height
			Layout.leftMargin: MobileComponents.Units.gridUnit / 4
			Image {
				id: subsurfaceLogo
				source: "qrc:/qml/subsurface-mobile-icon.png"
				anchors {
					verticalCenter: parent.Center
					left: parent.left
				}
				width: Math.round(MobileComponents.Units.gridUnit)
				height: width
			}
			MobileComponents.Label {
				text: qsTr("Subsurface-mobile")
				font.pointSize: Math.round(MobileComponents.Theme.defaultFont.pointSize)
				height: subsurfaceLogo.height
				anchors {
					left: subsurfaceLogo.right
					leftMargin: Math.round(MobileComponents.Units.gridUnit / 2)
				}
				font.weight: Font.Light
				verticalAlignment: Text.AlignVCenter
				Layout.fillWidth: false
				color: subsurfaceTheme.accentTextColor
			}
		}
		Item {
			Layout.fillWidth: true
		}
	}
}
