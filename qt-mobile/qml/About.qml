import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 1.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: aboutPage
	property int pageWidth: subsurfaceTheme.columnWidth - Kirigami.Units.gridUnit
	title: "About Subsurface-mobile"

	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing
		width: aboutPage.width
		Layout.margins: Kirigami.Units.gridUnit / 2

		Kirigami.Heading {
			text: "About Subsurface-mobile"
			Layout.margins: Kirigami.Units.largeSpacing / 2
			Layout.alignment: Qt.AlignHCenter
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
		}

		Rectangle {
			color: "transparent"
			Layout.margins: Kirigami.Units.largeSpacing
			Layout.fillWidth: true
			Layout.minimumHeight: childrenRect.height
			Image {
				id: image
				source: "qrc:/qml/subsurface-mobile-icon.png"
				width: parent.width - Kirigami.Units.largeSpacing
				fillMode: Image.PreserveAspectFit
				horizontalAlignment: Image.AlignHCenter
			}
		}

		Kirigami.Heading {
			text: "A mobile version of the free Subsurface divelog software.\n" +
				"View your dive logs while on the go."
			level: 4
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing * 3
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			anchors.horizontalCenter: parent.Center
			horizontalAlignment: Text.AlignHCenter
		}

		Kirigami.Heading {
			text: "Version: " + manager.getVersion() + "\n\n© Subsurface developer team\n2011-2016"
			level: 5
			font.pointSize: subsurfaceTheme.smallPointSize + 1
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			anchors.horizontalCenter: parent.Center
			horizontalAlignment: Text.AlignHCenter
		}
	}
}
