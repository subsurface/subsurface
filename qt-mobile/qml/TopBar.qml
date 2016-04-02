import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.kde.kirigami 1.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Rectangle {
	id: topPart

	color: subsurfaceTheme.accentColor
	Layout.minimumHeight: Math.round(Kirigami.Units.gridUnit * 1.5)
	Layout.fillWidth: true
	Layout.margins: 0
	RowLayout {
		anchors.verticalCenter: topPart.verticalCenter
		Item {
			Layout.preferredHeight: subsurfaceLogo.height
			Layout.leftMargin: Kirigami.Units.gridUnit / 4
			Image {
				id: subsurfaceLogo
				source: "qrc:/qml/subsurface-mobile-icon.png"
				anchors {
					verticalCenter: parent.Center
					left: parent.left
				}
				width: Math.round(Kirigami.Units.gridUnit)
				height: width
			}
			Kirigami.Label {
				text: qsTr("Subsurface-mobile")
				font.pointSize: Math.round(Kirigami.Theme.defaultFont.pointSize)
				height: subsurfaceLogo.height
				anchors {
					left: subsurfaceLogo.right
					leftMargin: Math.round(Kirigami.Units.gridUnit / 2)
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
	MouseArea {
		anchors.fill: topPart
		onClicked: {
			if (stackView.depth == 1 && showingDiveList) {
				scrollToTop()
			}
		}
	}
}
