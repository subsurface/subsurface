// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.2 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow
	height: outerLayout.height

	property string username: login.text;
	property string password: password.text;

	function saveCredentials() {
		QMLPrefs.cloudUserName = login.text
		QMLPrefs.cloudPassword = password.text
		QMLPrefs.cloudPin = pin.text
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

		Controls.Label {
			text: qsTr("Email")
			visible: !QMLPrefs.showPin
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.secondaryTextColor
		}

		Controls.TextField {
			id: login
			text: QMLPrefs.cloudUserName
			visible: !QMLPrefs.showPin
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhEmailCharactersOnly |
					  Qt.ImhNoAutoUppercase
		}

		Controls.Label {
			text: qsTr("Password")
			visible: !QMLPrefs.showPin
			font.pointSize: subsurfaceTheme.smallPointSize
			color: subsurfaceTheme.secondaryTextColor
		}

		Controls.TextField {
			id: password
			text: QMLPrefs.cloudPassword
			visible: !QMLPrefs.showPin
			echoMode: TextInput.PasswordEchoOnEdit
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			Layout.fillWidth: true
		}

		Controls.Label {
			text: qsTr("PIN")
			visible: QMLPrefs.showPin
		}
		Controls.TextField {
			id: pin
			text: ""
			Layout.fillWidth: true
			visible: QMLPrefs.showPin
		}

		RowLayout {
			Layout.fillWidth: true
			Layout.margins: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing
			visible: QMLPrefs.showPin
			SsrfButton {
				id: registerpin
				text: qsTr("Register") 
				onClicked: {
					saveCredentials()
				}
			}
			Controls.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SsrfButton {
				id: cancelpin
				text: qsTr("Cancel")
				onClicked: {
					prefs.cancelCredentialsPinSetup()
					rootItem.returnTopPage()
				}
			}
		}

		RowLayout {
			Layout.fillWidth: true
			Layout.margins: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing
			visible: !QMLPrefs.showPin

			SsrfButton {
				id: signin_register_normal
				text: qsTr("Sign-in or Register")
				onClicked: {
					saveCredentials()
				}
			}
			Controls.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SsrfButton {
				id: toNoCloudMode
				text: qsTr("No cloud mode")
				onClicked: {
					manager.syncToCloud = false
					prefs.credentialStatus = CloudStatus.CS_NOCLOUD
					manager.saveCloudCredentials()
					manager.openNoCloudRepo()
				}
			}
		}
	}
}
