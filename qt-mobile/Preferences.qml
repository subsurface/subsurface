import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0

Window {
	id: loginWindow

	signal accept

	property string username: login.text;
	property string password: password.text;
	property bool issave: savePassword.checked;

	flags: Qt.Dialog
	modality: Qt.WindowModal
	width: 400
	height: 160

	minimumHeight: 160
	minimumWidth: 400

	title: "Enter your Subsurface cloud credentials"

	GridLayout {
		columns: 2
		anchors.fill: parent
		anchors.margins: 10
		rowSpacing: 10
		columnSpacing: 10

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
			id: savePassword
		}

		Item {
			Layout.columnSpan: 2
			Layout.fillWidth: true
			implicitHeight: saveButton.height

			Button {
				id: cancelButton
				text: "Cancel"

				onClicked: {
					loginWindow.close();
				}
			}

			Button {
				id: saveButton
				text: "Save"
				anchors.centerIn: parent
				onClicked: {
					manager.cloudUserName = login.text
					manager.cloudPassword = password.text
					manager.savePreferences()
					loginWindow.close();
					loginWindow.accept();
				}
			}
		}
	}
}
