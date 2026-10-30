// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

TemplatePage {
	title: qsTr("Export and upload dive data")

	property int selectedExport: ExportType.EX_DIVES_XML
	// AI-generated (Claude): on Android/iOS a file export is handed to the platform's
	// own email/save mechanism instead of using a desktop-style folder picker
	property bool mobileFileExport: Qt.platform.os === "android" || Qt.platform.os === "ios"

	FolderDialog {
		id: saveAsDialog
		currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
		onAccepted: {
			manager.exportToFile(selectedExport, selectedFolder, anonymize.checked)
			pageStack.pop()
			close()
		}
		onRejected: {
			pageStack.pop()
			close()
		}
	}

	// AI-generated (Claude): report the outcome manager.shareViaEmail()/
	// saveExportToDevice() actually observed - writing the export file is
	// synchronous, so a failure is known and must not pop the page as though
	// the export succeeded; success only means the file was created and handed
	// to the platform, not that the user completed the email/save.
	Connections {
		target: manager
		function onFileExportFinished(success, text) {
			showPassiveNotification(text, success ? 5000 : 8000)
			if (success)
				pageStack.pop()
		}
	}

	// AI-generated (Claude): the page is grouped by mechanism, not just by data
	// scope - "Export to a file" is the only section where the data choice
	// (dive log vs. dive sites) and anonymization apply, because the action
	// buttons underneath it are the only consumers of selectedExport/anonymize.
	// "Upload the dive log" has no data choice to make: both backends
	// (uploadDiveLogsDE::doUpload(), uploadDiveShare::doUpload()) always send
	// the whole dive log and never honour anonymize, so that section carries
	// its own fixed scope as a note instead of a disabled-with-reason control.
	Column {
		id: exportSelection
		visible: true
		width: parent.width
		spacing: Kirigami.Units.smallSpacing
		Layout.margins: Kirigami.Units.gridUnit / 2

		TemplateSection {
			id: sectionExportToFile
			title: qsTr("Export to a file")
			isExpanded: true
			width: parent.width

			ColumnLayout {
				visible: sectionExportToFile.isExpanded
				width: parent.width
				spacing: 3

				TemplateRadioButton {
					text: qsTr("Dive log")
					checked: true
					onClicked: selectedExport = ExportType.EX_DIVES_XML
				}
				TemplateLabel {
					visible: selectedExport === ExportType.EX_DIVES_XML
					Layout.fillWidth: true
					Layout.leftMargin: Kirigami.Units.gridUnit
					wrapMode: Text.Wrap
					text: qsTr("Subsurface native XML format.")
				}
				TemplateRadioButton {
					text: qsTr("Dive sites")
					onClicked: selectedExport = ExportType.EX_DIVE_SITES_XML
				}
				TemplateLabel {
					visible: selectedExport === ExportType.EX_DIVE_SITES_XML
					Layout.fillWidth: true
					Layout.leftMargin: Kirigami.Units.gridUnit
					wrapMode: Text.Wrap
					text: qsTr("Subsurface dive sites native XML format.")
				}
				Rectangle {
					width: 1
					height: Kirigami.Units.smallSpacing
					color: "transparent"
				}
				TemplateCheckBox {
					id: anonymize
					Layout.fillWidth: true
					text: qsTr("Anonymize")
				}
				// AI-generated (Claude): matches what save_dives_logic()/
				// export_dive_sites_xslt() actually do via show_utf8_blanked()/
				// blankout() (core/save-xml.cpp) - buddy/dive master/suit/notes
				// on dives, and name/description/notes/taxonomy on dive sites.
				// blankout() only replaces ASCII 'A'-'Z'/'a'-'z' bytes; non-ASCII
				// letters (e.g. accented or non-Latin characters), digits, and
				// punctuation pass through unchanged, so this must not be
				// described as removing the text. GPS coordinates are not
				// affected either way.
				TemplateLabel {
					Layout.fillWidth: true
					Layout.leftMargin: Kirigami.Units.gridUnit
					wrapMode: Text.Wrap
					text: qsTr("Replaces ASCII letters with X or x in buddy, dive master, suit, and notes text, and in dive site names, descriptions, and notes, in the exported file. Non-ASCII characters, digits, and punctuation are not changed.")
				}
				TemplateButton {
					text: qsTr("Save to a file...")
					onClicked: {
						if (mobileFileExport) {
							manager.appendTextToLog("Save export of type " + selectedExport + " to a file.")
							manager.saveExportToDevice(selectedExport, anonymize.checked)
						} else {
							saveAsDialog.open()
						}
					}
				}
				TemplateButton {
					text: qsTr("Send by email...")
					visible: mobileFileExport
					onClicked: {
						manager.appendTextToLog("Send export of type " + selectedExport + " via email.")
						manager.shareViaEmail(selectedExport, anonymize.checked)
					}
				}
			}
		}

		TemplateSection {
			id: sectionUploadTheDiveLog
			title: qsTr("Upload the dive log")
			isExpanded: true
			width: parent.width

			ColumnLayout {
				visible: sectionUploadTheDiveLog.isExpanded
				width: parent.width
				spacing: 3

				// AI-generated (Claude): dive site names and coordinates travel
				// embedded in each dive record (xslt/divelogs-export.xslt,
				// core/save-html.cpp) - they are not omitted, there is simply no
				// dive-sites-only upload, and anonymize (file export only) does
				// not apply here either.
				TemplateLabel {
					Layout.fillWidth: true
					wrapMode: Text.Wrap
					text: qsTr("Uploads always send the whole dive log, including dive site names and coordinates; anonymizing is not available for uploads.")
				}
				TemplateButton {
					text: qsTr("Upload to divelogs.de...")
					onClicked: {
						exportUploadWindow.uploadDestination = ExportType.EX_DIVELOGS_DE
						pageStack.push(exportUploadWindow)
					}
				}
				TemplateButton {
					text: qsTr("Upload to DiveShare...")
					onClicked: {
						exportUploadWindow.uploadDestination = ExportType.EX_DIVESHARE
						pageStack.push(exportUploadWindow)
					}
				}
			}
		}
	}
}
