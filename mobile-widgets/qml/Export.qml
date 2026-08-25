// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

TemplatePage {
	title: qsTr("Export Divelog information")

	property int selectedExport: ExportType.EX_DIVES_XML

	// AI-generated (Claude)
	// The remaining export types are website uploads, which need the
	// credential step instead of a destination.
	readonly property bool fileBasedExport: selectedExport === ExportType.EX_DIVES_XML ||
						selectedExport === ExportType.EX_DIVE_SITES_XML ||
						selectedExport === ExportType.EX_FIT

	// AI-generated (Claude)
	// A whole-divelog FIT export is many files, so unlike the XML exports it
	// needs a granularity choice and the same manual timezone correction the
	// per-dive export offers.
	readonly property bool fitExport: selectedExport === ExportType.EX_FIT
	readonly property bool fitAsArchive: fitGranularity.currentIndex === 1
	// fitBulkDiveCount() has no change notification, so cache it on entry to
	// the page instead of leaving bindings that go stale after an import.
	property int fitDiveCount: 0
	// Warn before the user taps Share rather than failing after they picked a
	// target app (see fitShareMaxIndividual in qmlmanager.cpp).
	readonly property bool fitShareTooMany: fitExport && !fitAsArchive &&
						fitDiveCount > manager.fitShareIndividualLimit()

	// AI-generated (Claude)
	// Mirrors ExportDivePage's Share/Save split for the whole divelog:
	// "share" hands a fixed file to the native share sheet, "save" lets the
	// user pick a destination.
	function runFileExport(destination) {
		if (fitExport) {
			// the text field is only folded into offsetSeconds by commit()
			var offset = fitTzOffset.commit()
			if (destination === "share") {
				if (fitAsArchive)
					manager.shareFitArchive(offset)
				else
					manager.shareFitAll(offset)
				pageStack.pop()
			} else if (fitAsArchive) {
				exportFileDialog.currentFile = manager.exportSuggestedFileName(selectedExport)
				exportFileDialog.open()
			} else {
				// one file per dive: the destination is a directory
				fitFolderDialog.open()
			}
			return
		}
		if (destination === "share") {
			manager.appendTextToLog("Send export of type " + selectedExport + " via share sheet.")
			manager.shareViaEmail(selectedExport, anonymize.checked)
			pageStack.pop()
		} else if (Qt.platform.os !== "android" && Qt.platform.os !== "ios") {
			saveAsDialog.open()
		} else {
			exportFileDialog.currentFile = manager.exportSuggestedFileName(selectedExport)
			exportFileDialog.open()
		}
	}

	// AI-generated (Claude)
	// exportWindow is a single, pre-instantiated page (main.qml), so the
	// visibility flags below survive a pageStack.pop(). Without resetting
	// them, reopening Export lands on the credential pane with no way back
	// to the export type list.
	function resetToSelection() {
		uploadDialog.visible = false
		exportSelection.visible = true
		fitDiveCount = manager.fitBulkDiveCount()
		statusText.text = ""
		progress.value = 0
	}

	onVisibleChanged: {
		if (visible)
			resetToSelection()
	}

	FileDialog {
		id: exportFileDialog
		fileMode: FileDialog.SaveFile
		nameFilters: selectedExport === ExportType.EX_FIT ?
				[ qsTr("ZIP archives") + " (*.zip)" ] :
			selectedExport === ExportType.EX_DIVES_XML ?
				[ qsTr("Subsurface files") + " (*.ssrf)" ] : [ qsTr("XML files") + " (*.xml)" ]
		onAccepted: {
			if (selectedExport === ExportType.EX_FIT)
				manager.exportFitArchiveToUrl(selectedFile, fitTzOffset.offsetSeconds)
			else
				manager.exportToUrl(selectedExport, selectedFile, anonymize.checked)
			pageStack.pop()
		}
		onRejected: {
			// a cancelled picker must not have any file side effects
			pageStack.pop()
		}
	}

	// AI-generated (Claude)
	// Destination for the one-file-per-dive FIT export. On Android this is the
	// SAF "open document tree" picker; the tree URI it returns goes to QFile
	// as-is (see exportFitAllToFolder).
	FolderDialog {
		id: fitFolderDialog
		currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
		onAccepted: {
			manager.exportFitAllToFolder(selectedFolder, fitTzOffset.offsetSeconds)
			pageStack.pop()
			close()
		}
		onRejected: {
			// a cancelled picker must not have any file side effects
			pageStack.pop()
			close()
		}
	}

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
		SsrfTextField {
			id: fieldUserID
			Layout.columnSpan: 2
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhNoAutoUppercase
		}
		TemplateLabel {
			id: textPassword
			text: qsTr("Password:")
		}
		SsrfTextField {
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
			// AI-generated (Claude): back to the export type list, not out of
			// the page
			onClicked: {
				resetToSelection()
			}
		}
		Connections {
			target: manager
			function onUploadFinish(success, text) {
				if (success) {
					resetToSelection()
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

	// On Android the destination is picked through SAF (FileDialog's SaveFile
	// mode), so file based export offers a "Save" next to the share sheet
	// rather than being share-only.

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
			text: qsTr("Export all dives as FIT")
			onClicked: {
				selectedExport = ExportType.EX_FIT
				explain.text = qsTr("Garmin FIT activity files, one per dive.")
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
		// AI-generated (Claude)
		// FIT carries no names or dive sites, so "Anonymize" has nothing to
		// strip there.
		TemplateCheckBox {
			id: anonymize
			visible: !fitExport
			Layout.fillWidth: true
			text: qsTr("Anonymize")
		}

		// AI-generated (Claude)
		ColumnLayout {
			id: fitOptions
			visible: fitExport
			Layout.fillWidth: true
			spacing: Kirigami.Units.smallSpacing * 2

			TemplateLabel {
				text: qsTr("Files")
			}
			TemplateComboBox {
				id: fitGranularity
				Layout.fillWidth: true
				model: [ qsTr("Individual files"), qsTr("ZIP archive") ]
			}
			TemplateLabel {
				Layout.fillWidth: true
				wrapMode: Text.Wrap
				text: fitShareTooMany ?
					qsTr("%1 dives - too many to share as individual files, use a ZIP archive or Save.").arg(fitDiveCount) :
					qsTr("%1 dives will be exported.").arg(fitDiveCount)
			}
			TzOffsetSelector {
				id: fitTzOffset
				Layout.fillWidth: true
			}
		}
		RowLayout {
			Layout.fillWidth: true
			// AI-generated (Claude)
			// A website upload needs the credential step, so it keeps the
			// "Next" button; a file based export goes straight to a
			// destination.
			TemplateButton {
				text: qsTr("Next")
				visible: !fileBasedExport
				onClicked: {
					if (selectedExport === ExportType.EX_DIVELOGS_DE) {
						textUserID.visible = true
						fieldUserID.visible = true
						fieldUserID.text = PrefCloudStorage.divelogde_user
						textPassword.visible = true
						fieldPassword.visible = true
						fieldPassword.text = PrefCloudStorage.divelogde_pass
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
					}
				}
			}
			TemplateButton {
				text: qsTr("Share")
				visible: fileBasedExport
				enabled: !fitShareTooMany
				onClicked: runFileExport("share")
			}
			TemplateButton {
				text: qsTr("Save")
				visible: fileBasedExport
				onClicked: runFileExport("save")
			}
			TemplateButton {
				text: qsTr("Cancel")
				onClicked: pageStack.pop()
			}
		}
	}
}
