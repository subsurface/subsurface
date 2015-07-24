import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0

Item {
	id: loginWindow

	signal accept

	property string username: login.text;
	property string password: password.text;
	property bool issave: savePassword.checked;

	GridLayout {
		columns: 2
		anchors.centerIn: parent
		width: parent.width

		Label {
			text: "Enter your Subsurface cloud credentials"
			Layout.columnSpan: 2
		}

		Label {
			text: "Email Address:"
		}

		TextField {
			id: login
			text: manager.cloudUserName
			Layout.fillWidth: true
		}

		Label {
			text: "Password"
		}

		TextField {
			id: password
			text: manager.cloudPassword
			echoMode: TextInput.Password
			Layout.fillWidth: true
		}

		Label {
			text: "Save Password locally"
		}

		CheckBox {
			checked: manager.saveCloudPassword
			id: savePassword
		}

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
					manager.savePreferences()
					stackView.pop()
				}
			}
		}

		Item {
			height: cancelButton.height
			width: cancelButton.width
			Button {
				id: cancelButton
				text: "Cancel"

				onClicked: {
					stackView.pop();
				}
			}
		}
	}
}
