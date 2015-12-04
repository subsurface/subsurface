import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow

	signal accept

	property string username: login.text;
	property string password: password.text;
	property bool issave: savePassword.checked;

	ColumnLayout {
		anchors.fill: parent
		anchors.margins: MobileComponents.Units.gridUnit

		MobileComponents.Heading {
			text: "Cloud credentials"
			Layout.bottomMargin: MobileComponents.Units.largeSpacing / 2
		}

		MobileComponents.Label {
			text: "Email"
		}

		TextField {
			id: login
			text: manager.cloudUserName
			Layout.fillWidth: true
		}

		MobileComponents.Label {
			text: "Password"
		}

		TextField {
			id: password
			text: manager.cloudPassword
			echoMode: TextInput.Password
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

			CheckBox {
				checked: manager.saveCloudPassword
				id: savePassword
			}
			MobileComponents.Label {
				text: "Remember"
			}
		}
		Item { width: MobileComponents.Units.gridUnit; height: width }
		Item {
			height: saveButton.height
			width: saveButton.width
			Button {
				id: saveButton
				text: "Save"
				anchors.centerIn: parent
				onClicked: {
					manager.cloudUserName = login.text
					manager.cloudPassword = password.text
					manager.saveCloudPassword = savePassword.checked
					manager.saveCloudCredentials()
					stackView.pop()
				}
			}
		}

		Item {
			Layout.fillHeight: true
		}
	}
}
