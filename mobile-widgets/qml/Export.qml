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
	// A whole-divelog FIT export is many files, so unlike the XML exports it
	// needs a granularity choice and the same manual timezone correction the
	// per-dive export offers.
	readonly property bool fitExport: selectedExport === ExportType.EX_FIT
	readonly property bool fitAsArchive: fitGranularity.currentIndex === 1
	// fitBulkDiveCount() has no change notification, so cache it on entry to
	// the page instead of leaving a binding that goes stale after an import.
	property int fitDiveCount: 0
	// Warn before the user taps Share rather than failing after they picked a
	// target app (see fitShareIndividualLimit() in qmlmanager.cpp).
	readonly property bool fitShareTooMany: fitExport && !fitAsArchive &&
						fitDiveCount > manager.fitShareIndividualLimit()

	onVisibleChanged: {
		if (visible)
			fitDiveCount = manager.fitBulkDiveCount()
	}

	// AI-generated (Claude)
	// Mirrors ExportDivePage's Share/Save split for the whole divelog:
	// "share" hands a fixed file to the native share sheet, "save" lets the
	// user pick a destination.
	function runFitExport(destination) {
		var offset = fitTzOffset.commit()
		if (destination === "share") {
			if (fitAsArchive)
				manager.shareFitArchive(offset)
			else
				manager.shareFitAll(offset)
			pageStack.pop()
		} else if (fitAsArchive) {
			fitArchiveDialog.open()
		} else {
			// one file per dive: the destination is a directory
			fitFolderDialog.open()
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

	// AI-generated (Claude)
	// Destination for the one-file-per-dive FIT export. On Android this is the
	// SAF "open document tree" picker; the tree URI it returns goes straight to
	// exportFitAllToFolder(), same as fitFileDialog's selectedFile does for the
	// per-dive export.
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

	FileDialog {
		id: fitArchiveDialog
		fileMode: FileDialog.SaveFile
		currentFile: "subsurface_fit_export.zip"
		nameFilters: [ qsTr("ZIP archives") + " (*.zip)" ]
		onAccepted: {
			manager.exportFitArchiveToUrl(selectedFile, fitTzOffset.offsetSeconds)
			pageStack.pop()
		}
		onRejected: {
			// a cancelled picker must not have any file side effects
			pageStack.pop()
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
			// "Next" button; FIT goes straight to a destination.
			TemplateButton {
				text: qsTr("Next")
				visible: !fitExport
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
			TemplateButton {
				text: qsTr("Share")
				visible: fitExport
				enabled: !fitShareTooMany
				onClicked: runFitExport("share")
			}
			TemplateButton {
				text: qsTr("Save")
				visible: fitExport
				onClicked: runFitExport("save")
			}
		}
	}
}
