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
	Layout.minimumHeight: MobileComponents.Units.gridUnit * 2
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
				width: MobileComponents.Units.gridUnit
				height: width
			}
			MobileComponents.Label {
				text: qsTr("Subsurface")
				height: subsurfaceLogo.height
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
	}
}
