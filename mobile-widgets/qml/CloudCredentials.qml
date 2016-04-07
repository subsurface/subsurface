import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.kirigami 1.0 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow
	height: outerLayout.height + 2 * Kirigami.Units.gridUnit

	property string username: login.text;
	property string password: password.text;

	function saveCredentials() {
		manager.cloudUserName = login.text
		manager.cloudPassword = password.text
		manager.saveCloudCredentials()
	}

	ColumnLayout {
		id: outerLayout
		width: loginWindow.width - loginWindow.leftPadding - loginWindow.rightPadding - 2 * Kirigami.Units.gridUnit

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
			text: "Cloud credentials"
			level: headingLevel
			Layout.bottomMargin: Kirigami.Units.largeSpacing / 2
		}

		Kirigami.Label {
			text: "Email"
		}

		TextField {
			id: login
			text: manager.cloudUserName
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhEmailCharactersOnly |
					  Qt.ImhNoAutoUppercase
		}

		Kirigami.Label {
			text: "Password"
		}

		TextField {
			id: password
			text: manager.cloudPassword
			echoMode: TextInput.Password
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			Layout.fillWidth: true
		}
		GridLayout {
			columns: 2

			CheckBox {
				checked: false
				id: showPassword
				onCheckedChanged: {
					password.echoMode = checked ? TextInput.Normal : TextInput.Password
				}
			}
			Kirigami.Label {
				text: "Show password"
			}
		}
		Item { width: Kirigami.Units.gridUnit; height: width }
	}
}
