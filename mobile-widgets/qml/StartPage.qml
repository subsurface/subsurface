import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 1.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

ColumnLayout {
	id: startpage
	width: subsurfaceTheme.columnWidth

	function saveCredentials() { cloudCredentials.saveCredentials() }

	Kirigami.Label {
		id: explanationText
		Layout.fillWidth: true
		Layout.margins: Kirigami.Units.gridUnit
		Layout.topMargin: Kirigami.Units.gridUnit * 3
		text: "In order to use Subsurface-mobile you need to have a Subsurface cloud storage account " +
		      "(which can be created with the Subsurface desktop application)."
		wrapMode: Text.WordWrap
	}
	Kirigami.Label {
		id: messageArea
		Layout.fillWidth: true
		Layout.margins: Kirigami.Units.gridUnit
		Layout.topMargin: 0
		text: manager.startPageText
		wrapMode: Text.WordWrap
	}
	CloudCredentials {
		id: cloudCredentials
		Layout.fillWidth: true
		Layout.margins: Kirigami.Units.gridUnit
		Layout.topMargin: 0
		property int headingLevel: 3
	}
}
