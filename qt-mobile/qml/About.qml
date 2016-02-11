import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

MobileComponents.Page {

GridLayout {
	columns: 2
	width: parent.width - MobileComponents.Units.gridUnit
	anchors {
		fill: parent
		margins: MobileComponents.Units.gridUnit / 2
	}

	MobileComponents.Heading {
		text: "About"
		Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		Layout.columnSpan: 2
		Layout.alignment: Qt.AlignLeft
	}

	Image {
		source:"qrc:/qml/subsurface-mobile-icon.png"
	}

	MobileComponents.Heading {
		text: "A mobile version of Subsurface divelog software.\nView your dive logs while on the go."
		level: 3
		Layout.topMargin: MobileComponents.Units.largeSpacing
		Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		Layout.columnSpan: 2
	}

	MobileComponents.Label {
		text: "Version: " + manager.getVersion()
	}

	MobileComponents.Heading {
		text: "\n\n© Subsurface developer team, 2016"
		level: 3
		Layout.topMargin: MobileComponents.Units.largeSpacing
		Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		Layout.columnSpan: 2
	}

	Item {
		Layout.fillHeight: true
	}
}
}
