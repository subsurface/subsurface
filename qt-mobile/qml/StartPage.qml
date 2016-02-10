import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

Item {
	ColumnLayout {
		id: startpage
		width: subsurfaceTheme.columnWidth
		anchors.fill: parent
		anchors.margins: MobileComponents.Units.gridUnit / 2

		property int buttonWidth: width * 0.9

		MobileComponents.Heading {
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			text: "Subsurface-mobile"
		}
		MobileComponents.Label {
			id: explanationText
			Layout.fillWidth: true
			Layout.bottomMargin: MobileComponents.Units.largeSpacing
			text: "In order to use Subsurface-mobile you need to have a Subsurface cloud storage account " +
			      "(which can be created with the Subsurface desktop application)."
			wrapMode: Text.WordWrap
		}
		MobileComponents.Label {
			id: messageArea
			Layout.fillWidth: true
			text: manager.startPageText
			wrapMode: Text.WordWrap
		}
		CloudCredentials {
			Layout.fillWidth: true
			Layout.margins: MobileComponents.Units.gridUnit
			Layout.topMargin: MobileComponents.Units.gridUnit * 2
			visible: true
			property int headingLevel: 3
		}
		Item {
			id: spacer
			width: parent.width
			Layout.fillHeight: true
		}
	}
}
