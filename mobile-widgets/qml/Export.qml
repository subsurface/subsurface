// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	title: qsTr("Export Divelog information")

	property int selectedExport: ExportType.EX_DIVES_XML

	FileDialog {
		id: saveAsDialog
		folder: shortcuts.documents
		selectFolder: true
		title: radioGroup.current.text
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

	Dialog {
		id: uploadDialog
		title: radioGroup.current.text
		standardButtons: StandardButton.Apply | StandardButton.Cancel

		GridLayout {
			rowSpacing: Kirigami.Units.smallSpacing * 2
			columnSpacing: Kirigami.Units.smallSpacing
			columns: 2

			Text {
				id: textUserID
				text: qsTr("User ID")
			}
			TextField {
				id: fieldUserID
				Layout.fillWidth: true
				inputMethodHints: Qt.ImhNoAutoUppercase
			}
			Text {
				id: textPassword
				text: qsTr("Password:")
			}
			TextField {
				id: fieldPassword
				Layout.fillWidth: true
				inputMethodHints: Qt.ImhSensitiveData |
						  Qt.ImhHiddenText |
						  Qt.ImhNoAutoUppercase
				echoMode: TextInput.PasswordEchoOnEdit
			}
			CheckBox {
				id: fieldPrivate
				Layout.fillWidth: true
				text: qsTr("Private")
			}
			ProgressBar {
				id: progress
				value: 0.0
			}
			Text {
				id: statusText
				Layout.fillWidth: true
				Layout.columnSpan: 2
				wrapMode: Text.Wrap
			}

		}

		onApply: {
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
		onRejected: {
			pageStack.pop()
			close()
		}

		Connections {
			target: manager
			onUploadFinish: {
				if (success) {
					pageStack.pop()
					uploadDialog.close()
				}
				statusText.text = text
				progress.value = 0
			}
			onUploadProgress: {
				progress.value = percentage
			}
		}
	}

	ColumnLayout {
		width: parent.width
		spacing: 1
		Layout.margins: 10

		ExclusiveGroup { id: radioGroup }
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export Subsurface XML")
			checked: true
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_DIVES_XML
				explain.text = qsTr("Subsurface native XML format.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export Subsurface dive sites XML")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_DIVE_SITES_XML
				explain.text = qsTr("Subsurface dive sites native XML format.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export UDDF")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_UDDF
				explain.text = qsTr("Generic format that is used for data exchange between a variety of diving related programs.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Upload divelogs.de")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_DIVELOGS_DE
				explain.text = qsTr("Send the dive data to divelogs.de website.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Upload DiveShare")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_DIVESHARE
				explain.text = qsTr("Send the dive data to dive-share.appspot.com website.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export CSV dive profile")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_CSV_DIVE_PROFILE
				explain.text = qsTr("Comma separated values describing the dive profile.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export CSV dive details")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_CSV_DETAILS
				explain.text = qsTr("Comma separated values of the dive information. This includes most of the dive details but no profile information.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export CSV profile data")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_CSV_PROFILE
				explain.text = qsTr("Write profile data to a CSV file.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			visible: false // TEMPORARY MEASURE, until a non UI PNG generation is ready
			text: qsTr("Export Dive profile")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_PROFILE_PNG
				explain.text = qsTr("Write the profile image as PNG file.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export Worldmap")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_WORLD_MAP
				explain.text = qsTr("HTML export of the dive locations, visualized on a world map.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export TeX")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_TEX
				explain.text = qsTr("Write dive as TeX macros to file.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export LaTeX")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_LATEX
				explain.text = qsTr("Write dive as LaTeX macros to file.")
			}
		}
		RadioButton {
			Layout.fillWidth: true
			text: qsTr("Export Image depths")
			exclusiveGroup: radioGroup
			onClicked: {
				selectedExport = ExportType.EX_IMAGE_DEPTHS
				explain.text = qsTr("Write depths of images to file.")
			}
		}
		Text {
			id: explain
			Layout.fillWidth: true
			wrapMode: Text.Wrap
		}
		CheckBox {
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
					fieldPrivate.visible = false
					uploadDialog.open()
				} else if (selectedExport === ExportType.EX_DIVESHARE) {
					textUserID.visible = true
					fieldUserID.visible = true
					fieldUserID.text = PrefCloudStorage.diveshare_uid
					fieldPrivate.visible = true
					fieldPrivate.checked = PrefCloudStorage.diveshare_private
					textPassword.visible = false
					fieldPassword.visible = false
					uploadDialog.open()
				} else {
					saveAsDialog.open()
				}
			}
		}
	}
}
