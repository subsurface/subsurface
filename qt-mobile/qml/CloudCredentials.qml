import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow
	height: outerLayout.height + 2 * MobileComponents.Units.gridUnit

	property string username: login.text;
	property string password: password.text;

	function saveCredentials() {
		manager.cloudUserName = login.text
		manager.cloudPassword = password.text
		manager.saveCloudCredentials()
	}

	ColumnLayout {
		id: outerLayout
		width: subsurfaceTheme.columnWidth - 2 * MobileComponents.Units.gridUnit

		onVisibleChanged: {
			if (visible) {
				manager.appendTextToLog("Credential scrn: show kbd was: " + (Qt.inputMethod.isVisible ? "visible" : "invisible"))
				Qt.inputMethod.show()
				login.forceActiveFocus()
			} else {
				manager.appendTextToLog("Credential scrn: hide kbd was: " + (Qt.inputMethod.isVisible ? "visible" : "invisible"))
				Qt.inputMethod.hide()
			}
		}

		MobileComponents.Heading {
			text: "Cloud credentials"
			level: headingLevel
			Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		}

		MobileComponents.Label {
			text: "Email"
		}

		TextField {
			id: login
			text: manager.cloudUserName
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhEmailCharactersOnly |
					  Qt.ImhNoAutoUppercase
		}

		MobileComponents.Label {
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
			MobileComponents.Label {
				text: "Show password"
			}
		}
		Item { width: MobileComponents.Units.gridUnit; height: width }
	}
}
