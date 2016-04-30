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
		text: "To use Subsurface-mobile with Subsurface cloud storage, please enter " +
		      "your cloud credentials.\n\n" +
		      "To use Subsurface-mobile only with local data on this device, tap " +
		      "on the no cloud icon below."
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
