import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

MobileComponents.Page {
	id: aboutPage
	property int pageWidth: subsurfaceTheme.columnWidth - MobileComponents.Units.smallSpacing

	ScrollView {
		anchors.fill: parent

		ColumnLayout {
			spacing: MobileComponents.Units.largeSpacing
			width: aboutPage.width
			Layout.margins: MobileComponents.Units.gridUnit / 2

			MobileComponents.Heading {
				text: "About Subsurface-mobile"
				Layout.alignment: Qt.AlignHCenter
				Layout.maximumWidth: pageWidth
				Layout.topMargin: MobileComponents.Units.gridUnit
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			}
			Image {
				id: image
				source: "qrc:/qml/subsurface-mobile-icon.png"
				width: pageWidth / 2
				height: width
				fillMode: Image.Stretch
				Layout.alignment: Qt.AlignCenter
				horizontalAlignment: Image.AlignHCenter
			}
			MobileComponents.Heading {
				text: "A mobile version of the free Subsurface divelog software.\n" +
				      "View your dive logs while on the go."
				level: 4
				Layout.alignment: Qt.AlignHCenter
				Layout.topMargin: MobileComponents.Units.largeSpacing * 3
				Layout.maximumWidth: pageWidth
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				anchors.horizontalCenter: parent.Center
				horizontalAlignment: Text.AlignHCenter
			}

			MobileComponents.Heading {
				text: "Version: " + manager.getVersion() + "\n\n© Subsurface developer team\n2011-2016"
				level: 5
				font.pointSize: subsurfaceTheme.smallPointSize + 1
				Layout.alignment: Qt.AlignHCenter
				Layout.topMargin: MobileComponents.Units.largeSpacing
				Layout.maximumWidth: pageWidth
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				anchors.horizontalCenter: parent.Center
				horizontalAlignment: Text.AlignHCenter
			}
		}
	}
}
