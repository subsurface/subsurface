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

	property real gridWidth: loginWindow.width - Kirigami.Units.gridUnit
	property real col1Width: gridWidth * 0.80
	property real col2Width: gridWidth * 0.20

	function saveCredentials() {
		manager.cloudUserName = login.text
		manager.cloudPassword = password.text
		manager.cloudPin = pin.text
		manager.saveCloudCredentials()
	}

	ColumnLayout {
		id: outerLayout
		width: loginWindow.width - loginWindow.leftPadding - loginWindow.rightPadding - 2 * Kirigami.Units.gridUnit

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
				manager.appendTextToLog("Credential scrn: show kbd was: " + (Qt.inputMethod.isVisible ? "visible" : "invisible"))
				Qt.inputMethod.show()
				login.forceActiveFocus()
			} else {
				manager.appendTextToLog("Credential scrn: hide kbd was: " + (Qt.inputMethod.isVisible ? "visible" : "invisible"))
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
		}

		TextField {
			id: password
			text: manager.cloudPassword
			echoMode: TextInput.Password
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			Layout.fillWidth: true
			onEditingFinished: {
				saveCredentials()
			}
		}

		GridLayout {
			columns: 2

			Kirigami.Label {
				text: qsTr("Show password")
				Layout.preferredWidth: col1Width
			}
			SsrfSwitch {
				checked: false
				id: showPassword
				Layout.preferredWidth: col2Width
				onCheckedChanged: {
					if (checked)
						password.text = "" // don't show a hidden password
					password.echoMode = checked ? TextInput.Normal : TextInput.Password
				}
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
