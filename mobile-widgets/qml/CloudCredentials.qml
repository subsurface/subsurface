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
			visible: !rootItem.showPin
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.secondaryTextColor
		}

		TextField {
			id: login
			text: manager.cloudUserName
			visible: !rootItem.showPin
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhEmailCharactersOnly |
					  Qt.ImhNoAutoUppercase
		}

		Kirigami.Label {
			text: qsTr("Password")
			visible: !rootItem.showPin
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.secondaryTextColor
		}

		TextField {
			id: password
			text: manager.cloudPassword
			visible: !rootItem.showPin
			echoMode: TextInput.PasswordEchoOnEdit
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			Layout.fillWidth: true
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

		RowLayout {
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.margins: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing
			visible: rootItem.showPin
			SsrfButton {
				id: registerpin
				text: qsTr("Register") 
				onClicked: {
					saveCredentials()
				}
			}
			Kirigami.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SsrfButton {
				id: cancelpin
				text: qsTr("Cancel")
				onClicked: {
					manager.cancelCredentialsPinSetup()
					rootItem.returnTopPage()
				}
			}
		}

		RowLayout {
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.margins: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing
			visible: !rootItem.showPin

			SsrfButton {
				id: signin_register_normal
				text: qsTr("Sign-in or Register")
				onClicked: {
					saveCredentials()
				}
			}
			Kirigami.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SsrfButton {
				id: toNoCloudMode
				text: qsTr("No cloud mode")
				onClicked: {
					manager.syncToCloud = false
					manager.credentialStatus = QMLManager.CS_NOCLOUD
					manager.saveCloudCredentials()
				}
			}
		}
	}
}
