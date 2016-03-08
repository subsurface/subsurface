import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 1.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

ColumnLayout {
	id: startpage
	width: subsurfaceTheme.columnWidth

	property int buttonWidth: width * 0.9

	function saveCredentials() { cloudCredentials.saveCredentials() }

	Kirigami.Heading {
		Layout.margins: Kirigami.Units.gridUnit
		text: "Subsurface-mobile"
	}
	Kirigami.Label {
		id: explanationText
		Layout.fillWidth: true
		Layout.margins: Kirigami.Units.gridUnit
		text: "In order to use Subsurface-mobile you need to have a Subsurface cloud storage account " +
		      "(which can be created with the Subsurface desktop application)."
		wrapMode: Text.WordWrap
	}
	Kirigami.Label {
		id: messageArea
		Layout.fillWidth: true
		Layout.margins: Kirigami.Units.gridUnit
		text: manager.startPageText
		wrapMode: Text.WordWrap
	}
	CloudCredentials {
		id: cloudCredentials
		Layout.fillWidth: true
		Layout.margins: Kirigami.Units.gridUnit
		Layout.topMargin: Kirigami.Units.gridUnit * 2
		property int headingLevel: 3
	}
}
