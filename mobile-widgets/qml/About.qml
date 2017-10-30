// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.2 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.ScrollablePage {
	id: aboutPage
	property int pageWidth: aboutPage.width - aboutPage.leftPadding - aboutPage.rightPadding
	title: qsTr("About Subsurface-mobile")

	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing
		width: aboutPage.width
		Layout.margins: Kirigami.Units.gridUnit / 2


		Kirigami.Heading {
			text: qsTr("About Subsurface-mobile")
			Layout.topMargin: Kirigami.Units.gridUnit
			Layout.alignment: Qt.AlignHCenter
			Layout.maximumWidth: pageWidth
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

		Kirigami.Heading {
			text: qsTr("A mobile version of the free Subsurface divelog software.\n") +
				qsTr("View your dive logs while on the go.")
			level: 4
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing * 3
			Layout.maximumWidth: pageWidth
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			anchors.horizontalCenter: parent.Center
			horizontalAlignment: Text.AlignHCenter
		}

		Kirigami.Heading {
			text: qsTr("Version: %1\n\n© Subsurface developer team\n2011-2017").arg(manager.getVersion())
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
