// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.5
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0


Kirigami.ScrollablePage {
	id: startpage

	function saveCredentials() { cloudCredentials.saveCredentials() }

	ColumnLayout {
		Kirigami.Label {
			id: explanationText
			Layout.fillWidth: true
			Layout.margins: Kirigami.Units.gridUnit
			Layout.topMargin: Kirigami.Units.gridUnit * 3
			text: qsTr("To use Subsurface-mobile with Subsurface cloud storage, please enter your cloud credentials.\n") +
				qsTr("If this is the first time you use Subsurface cloud storage, enter a valid email (all lower case) " +
				"and a password of your choice (letters and numbers). " +
				"The server will send a PIN to the email address provided that you will have to enter here.\n\n") +
				qsTr("To use Subsurface-mobile only with local data on this device, tap " +
				"on the no cloud icon below.")
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
}
