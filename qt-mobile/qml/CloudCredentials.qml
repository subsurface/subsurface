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
	property bool issave: savePassword.checked;

	ColumnLayout {
		id: outerLayout
		width: subsurfaceTheme.columnWidth - 2 * MobileComponents.Units.gridUnit
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

			CheckBox {
				checked: manager.saveCloudPassword
				id: savePassword
			}
			MobileComponents.Label {
				text: "Remember"
			}
		}
		Item { width: MobileComponents.Units.gridUnit; height: width }
		RowLayout {
			Item {
				height: saveButton.height
				width: saveButton.width
				SubsurfaceButton {
					id: saveButton
					text: "Save"
					onClicked: {
						manager.cloudUserName = login.text
						manager.cloudPassword = password.text
						manager.saveCloudPassword = savePassword.checked
						manager.saveCloudCredentials()
					}
				}
			}
			Item {
				height: backButton.height
				width: backButton.width
				visible: diveListView.count > 0 && manager.credentialStatus != QMLManager.INVALID
				SubsurfaceButton {
					id: backButton
					text: "Back to dive list"
					onClicked: {
						manager.credentialStatus = oldStatus
					}
				}
			}
		}

	}
}
