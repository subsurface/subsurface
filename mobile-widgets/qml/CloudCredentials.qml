// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow
	height: outerLayout.height

	property string username: login.text;
	property string password: password.text;

	function saveCredentials() {
		manager.cloudUserName = login.text
		manager.cloudPassword = password.text
		manager.cloudPin = pin.text
		manager.saveCloudCredentials()
	}

	ColumnLayout {
		id: outerLayout
		width: loginWindow.width - Kirigami.Units.gridUnit // to ensure the full input fields are visible

		function goToNext() {
			for (var i = 0; i < children.length; ++i)
				if (children[i].focus) {
					children[i].nextItemInFocusChain().forceActiveFocus()
					break
				}
		}

		Keys.onReturnPressed: goToNext()
		Keys.onTabPressed: goToNext()

		onVisibleChanged: {
			if (visible && manager.accessingCloud < 0) {
				Qt.inputMethod.show()
				login.forceActiveFocus()
			} else {
				Qt.inputMethod.hide()
			}
		}

		Kirigami.Heading {
			text: qsTr("Cloud credentials")
			level: headingLevel
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
		}

		Kirigami.Label {
			text: qsTr("Email")
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.secondaryTextColor
		}

		TextField {
			id: login
			text: manager.cloudUserName
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhEmailCharactersOnly |
					  Qt.ImhNoAutoUppercase
			onEditingFinished: {
				saveCredentials()
			}
		}

		Kirigami.Label {
			text: qsTr("Password")
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.secondaryTextColor
		}

		TextField {
			id: password
			text: manager.cloudPassword
			echoMode: TextInput.PasswordEchoOnEdit
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			Layout.fillWidth: true
			onEditingFinished: {
				saveCredentials()
			}
		}

		Kirigami.Label {
			text: qsTr("PIN")
			visible: rootItem.showPin
		}
		TextField {
			id: pin
			text: ""
			Layout.fillWidth: true
			visible: rootItem.showPin
		}
	}
}
