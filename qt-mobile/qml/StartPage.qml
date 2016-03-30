import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

ColumnLayout {
	id: startpage
	width: subsurfaceTheme.columnWidth

	function saveCredentials() { cloudCredentials.saveCredentials() }

	MobileComponents.Heading {
		Layout.margins: MobileComponents.Units.gridUnit
		text: "Subsurface-mobile"
	}
	MobileComponents.Label {
		id: explanationText
		Layout.fillWidth: true
		Layout.margins: MobileComponents.Units.gridUnit
		Layout.topMargin: 0
		text: "In order to use Subsurface-mobile you need to have a Subsurface cloud storage account " +
		      "(which can be created with the Subsurface desktop application)."
		wrapMode: Text.WordWrap
	}
	MobileComponents.Label {
		id: messageArea
		Layout.fillWidth: true
		Layout.margins: MobileComponents.Units.gridUnit
		Layout.topMargin: 0
		text: manager.startPageText
		wrapMode: Text.WordWrap
	}
	CloudCredentials {
		id: cloudCredentials
		Layout.fillWidth: true
		Layout.margins: MobileComponents.Units.gridUnit
		Layout.topMargin: 0
		property int headingLevel: 3
	}
}
