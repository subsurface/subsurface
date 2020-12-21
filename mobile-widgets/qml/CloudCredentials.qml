// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.4 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow
	height: outerLayout.height

	property string username: login.text;
	property string password: password.text;
	property bool showPin: (Backend.cloud_verification_status === Enums.CS_NEED_TO_VERIFY)

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
			if (visible) {
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
			color: subsurfaceTheme.textColor
		}

		TemplateLabelSmall {
			text: qsTr("Email")
			visible: !showPin
			color: subsurfaceTheme.secondaryTextColor
		}

		TemplateTextField {
			id: login
			text: PrefCloudStorage.cloud_storage_email
			visible: !showPin
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhEmailCharactersOnly |
					  Qt.ImhNoAutoUppercase
		}

		TemplateLabelSmall {
			text: qsTr("Password")
			visible: !showPin
			color: subsurfaceTheme.secondaryTextColor
		}

		TemplateTextField {
			id: password
			text: PrefCloudStorage.cloud_storage_password
			visible: !showPin
			echoMode: TextInput.PasswordEchoOnEdit
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			Layout.fillWidth: true
		}

		TemplateLabel {
			text: qsTr("PIN")
			visible: showPin
		}
		TemplateTextField {
			id: pin
			text: ""
			Layout.fillWidth: true
			visible: showPin
		}

		RowLayout {
			Layout.fillWidth: true
			Layout.margins: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing
			visible: showPin
			TemplateButton {
				id: registerpin
				text: qsTr("Register")
				onClicked: {
					manager.saveCloudCredentials(login.text, password.text, pin.text)
				}
			}
			Controls.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			TemplateButton {
				id: cancelpin
				text: qsTr("Cancel")
				onClicked: {
					Backend.cloud_verification_status = Enums.CS_UNKNOWN
					manager.startPageText = qsTr("Check credentials...");
				}
			}
		}

		RowLayout {
			Layout.fillWidth: true
			Layout.margins: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing
			visible: !showPin

			TemplateButton {
				id: signin_register_normal
				text: qsTr("Sign-in or Register")
				onClicked: {
					manager.saveCloudCredentials(login.text, password.text, "")
				}
			}
			Controls.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			TemplateButton {
				id: toNoCloudMode
				text: qsTr("No cloud mode")
				onClicked: {
					manager.setGitLocalOnly(true)
					PrefCloudStorage.cloud_auto_sync = false
					manager.oldStatus = Backend.cloud_verification_status
					Backend.cloud_verification_status = Enums.CS_NOCLOUD
					manager.saveCloudCredentials("", "", "")
					manager.openNoCloudRepo()
				}
			}
		}
		TemplateButton {
			Layout.margins: Kirigami.Units.smallSpacing
			id: signin_forgot_password
			text: qsTr("Forgot password?")
			onClicked: {
				Qt.openUrlExternally("https://cloud.subsurface-divelog.org/passwordreset")
			}
		}
	}
}
