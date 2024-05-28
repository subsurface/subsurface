// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

TemplatePage {
	title: qsTr("Export Divelog information")

	property int selectedExport: ExportType.EX_DIVES_XML

	FileDialog {
		id: saveAsDialog
		folder: shortcuts.documents
		selectFolder: true
		onAccepted: {
			manager.exportToFile(selectedExport, fileUrls, anonymize.checked)
			pageStack.pop()
			close()
		}
		onRejected: {
			pageStack.pop()
			close()
		}
	}

	GridLayout {
		id: uploadDialog
		visible: false
		anchors {
			top: parent.top
			left: parent.left
			right: parent.right
			margins: Kirigami.Units.gridUnit / 2
		}
		rowSpacing: Kirigami.Units.smallSpacing * 2
		columnSpacing: Kirigami.Units.smallSpacing
		columns: 3
		TemplateLabel {
			text: qsTr("Export credentials")
			Layout.columnSpan: 3
		}
		TemplateLabel {
			id: textUserID
			text: qsTr("User ID")
		}
		TemplateTextField {
			id: fieldUserID
			Layout.columnSpan: 2
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhNoAutoUppercase
		}
		TemplateLabel {
			id: textPassword
			text: qsTr("Password:")
		}
		TemplateTextField {
			id: fieldPassword
			Layout.columnSpan: 2
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			echoMode: TextInput.PasswordEchoOnEdit
		}
		TemplateCheckBox {
			id: fieldPrivate
			visible: selectedExport === ExportType.EX_DIVESHARE
			Layout.fillWidth: true
			text: qsTr("Private")
		}
		ProgressBar {
			id: progress
			value: 0.0
			Layout.columnSpan: 2
		}
		TemplateLabel {
			id: statusText
			Layout.fillWidth: true
			Layout.columnSpan: 3
			wrapMode: Text.Wrap
		}

		TemplateButton {
			text: qsTr("Export")
			onClicked: {
				if (selectedExport === ExportType.EX_DIVELOGS_DE) {
					if (fieldUserID.text !== PrefCloudStorage.divelogde_user) {
						PrefCloudStorage.divelogde_user = fieldUserID.text
					}
					if (fieldPassword.text !== PrefCloudStorage.divelogde_pass)
						PrefCloudStorage.divelogde_pass = fieldPassword.text
					manager.exportToWEB(selectedExport, fieldUserID.text, fieldPassword.text, anonymize.checked)
				} else {
					if (fieldUserID.text !== PrefCloudStorage.diveshare_uid) {
						        PrefCloudStorage.diveshare_uid = fieldUserID.text
					}
					PrefCloudStorage.diveshare_private = fieldPrivate.checked
					manager.exportToWEB(selectedExport, fieldUserID.text, fieldPassword.text, fieldPrivate.checked)
				}
			}
		}
		TemplateButton {
			text: qsTr("Cancel")
			onClicked: {
				pageStack.pop()
			}
		}
		Connections {
			target: manager
			function onUploadFinish(success, text) {
				if (success) {
					pageStack.pop()
				}
				statusText.text = text
				progress.value = 0
			}
			function onUploadProgress(percentage) {
				progress.value = percentage
			}
		}
	}

	// given that there is no native file dialog on Android and that access to
	// the file system is increasingly restrictive in future versions, file based
	// export really doesn't make sense on Android

	ColumnLayout {
		id: exportSelection
		visible: true
		width: parent.width
		spacing: 3
		Layout.margins: Kirigami.Units.gridUnit / 2

		TemplateRadioButton {
			text: qsTr("Export Subsurface XML")
			checked: true
			onClicked: {
				selectedExport = ExportType.EX_DIVES_XML
				explain.text = qsTr("Subsurface native XML format.")
			}
		}
		TemplateRadioButton {
			text: qsTr("Export Subsurface dive sites XML")
			onClicked: {
				selectedExport = ExportType.EX_DIVE_SITES_XML
				explain.text = qsTr("Subsurface dive sites native XML format.")
			}
		}
		TemplateRadioButton {
			text: qsTr("Upload divelogs.de")
			onClicked: {
				selectedExport = ExportType.EX_DIVELOGS_DE
				explain.text = qsTr("Send the dive data to divelogs.de website.")
			}
		}
		TemplateRadioButton {
			text: qsTr("Upload DiveShare")
			onClicked: {
				selectedExport = ExportType.EX_DIVESHARE
				explain.text = qsTr("Send the dive data to dive-share.appspot.com website.")
			}
		}
		Rectangle {
			width: 1
			height: Kirigami.Units.gridUnit
			color: "transparent"
		}
		TemplateLabel {
			id: explain
			Layout.fillWidth: true
			wrapMode: Text.Wrap
		}
		Rectangle {
			width: 1
			height: Kirigami.Units.gridUnit
			color: "transparent"
		}
		TemplateCheckBox {
			id: anonymize
			Layout.fillWidth: true
			text: qsTr("Anonymize")
		}
		TemplateButton {
			text: qsTr("Next")
			onClicked: {
				if (selectedExport === ExportType.EX_DIVELOGS_DE) {
					textUserID.visible = true
					fieldUserID.visible = true
					fieldUserID.text = PrefCloudStorage.divelogde_user
					textPassword.visible = true
					fieldPassword.visible = true
					fieldPassword.text = PrefCloudStorage.divelogde_pass
					anonymize.visible = false
					statusText.text = ""
					exportSelection.visible = false
					uploadDialog.visible = true
				} else if (selectedExport === ExportType.EX_DIVESHARE) {
					textUserID.visible = true
					fieldUserID.visible = true
					fieldUserID.text = PrefCloudStorage.diveshare_uid
					fieldPrivate.visible = true
					fieldPrivate.checked = PrefCloudStorage.diveshare_private
					exportSelection.visible = false
					textPassword.visible = false
					uploadDialog.visible = true
				} else if (Qt.platform.os !== "android" && Qt.platform.os !== "ios") {
					saveAsDialog.open()
				} else {
					manager.appendTextToLog("Send export of type " + selectedExport + " via email.")
					manager.shareViaEmail(selectedExport, anonymize.checked)
					pageStack.pop()
				}
			}
		}
	}
}
