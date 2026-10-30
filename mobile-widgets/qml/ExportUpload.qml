// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

// AI-generated (Claude): the upload credentials step, pulled out of Export.qml
// onto its own page so it has its own identity (title, confirming action,
// back/cancel behaviour) instead of being a hidden sibling layout toggled
// visible in place of the export page's own content.
TemplatePage {
	id: exportUploadPage
	title: qsTr("Upload credentials")

	// Set by Export.qml before pushing this page.
	property int uploadDestination: ExportType.EX_DIVELOGS_DE

	// AI-generated (Claude): the upload backends (uploadDiveLogsDE,
	// uploadDiveShare, core/uploadDiveShare.cpp) each keep only a single
	// in-flight QNetworkReply, so starting a second upload before the first
	// completes overwrites it and silently drops that request's completion.
	// This page is a singleton reused across visits (main.qml), so the guard
	// has to persist across Cancel-and-return-later, not just within one
	// click handler.
	property bool uploadInProgress: false

	// AI-generated (Claude): exportUploadPage is a singleton, so Cancel-and-
	// reopen can make pageStack.currentItem === exportUploadPage true again for
	// a *different* visit than the one that started the still-in-flight
	// request. Without tracking which visit started the upload, that stale
	// request's completion would pop the reopened page and discard whatever
	// the user is doing on this new visit.
	property bool uploadStartedForCurrentVisit: false

	// AI-generated (Claude): manager.uploadFinish(success, text) carries no
	// destination of its own (it is shared by both backends, qmlmanager.cpp),
	// so a result arriving while uploadDestination has since changed (the page
	// was reopened for the other destination) would otherwise be reported
	// without saying which upload it belongs to. Capture the destination the
	// in-flight request was actually started for.
	property int uploadDestinationForCurrentUpload: ExportType.EX_DIVELOGS_DE

	// AI-generated (Claude): this page is instantiated once in main.qml and reused
	// across visits, so populate the fields relevant to the chosen destination,
	// and clear any leftover status/progress from a previous upload, each time
	// it is (re)shown.
	onVisibleChanged: {
		if (visible) {
			uploadStartedForCurrentVisit = false
			statusText.text = ""
			progress.value = 0
			if (uploadDestination === ExportType.EX_DIVELOGS_DE) {
				fieldUserID.text = PrefCloudStorage.divelogde_user
				fieldPassword.text = PrefCloudStorage.divelogde_pass
			} else {
				fieldUserID.text = PrefCloudStorage.diveshare_uid
				fieldPrivate.checked = PrefCloudStorage.diveshare_private
			}
		}
	}

	GridLayout {
		width: parent.width
		Layout.margins: Kirigami.Units.gridUnit / 2
		rowSpacing: Kirigami.Units.smallSpacing * 2
		columnSpacing: Kirigami.Units.smallSpacing
		columns: 3

		TemplateLabel {
			text: qsTr("User ID")
		}
		SsrfTextField {
			id: fieldUserID
			Layout.columnSpan: 2
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhNoAutoUppercase
		}
		// divelogs.de needs a password; DiveShare doesn't.
		TemplateLabel {
			visible: uploadDestination === ExportType.EX_DIVELOGS_DE
			text: qsTr("Password:")
		}
		SsrfTextField {
			id: fieldPassword
			visible: uploadDestination === ExportType.EX_DIVELOGS_DE
			Layout.columnSpan: 2
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhSensitiveData |
					  Qt.ImhHiddenText |
					  Qt.ImhNoAutoUppercase
			echoMode: TextInput.PasswordEchoOnEdit
		}
		// AI-generated (Claude): this only selects the "?private=true" upload URL
		// (uploadDiveShare::doUpload(), core/uploadDiveShare.cpp) - a dive-share
		// listing/visibility flag. The uploaded payload is identical either way;
		// divelogs.de has no equivalent option.
		TemplateCheckBox {
			id: fieldPrivate
			visible: uploadDestination === ExportType.EX_DIVESHARE
			Layout.columnSpan: 3
			Layout.fillWidth: true
			text: qsTr("Don't list this upload publicly on dive-share")
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
			text: qsTr("Upload")
			enabled: !uploadInProgress
			onClicked: {
				uploadInProgress = true
				uploadStartedForCurrentVisit = true
				uploadDestinationForCurrentUpload = uploadDestination
				if (uploadDestination === ExportType.EX_DIVELOGS_DE) {
					if (fieldUserID.text !== PrefCloudStorage.divelogde_user)
						PrefCloudStorage.divelogde_user = fieldUserID.text
					if (fieldPassword.text !== PrefCloudStorage.divelogde_pass)
						PrefCloudStorage.divelogde_pass = fieldPassword.text
					manager.exportToDivelogsDE(fieldUserID.text, fieldPassword.text)
				} else {
					if (fieldUserID.text !== PrefCloudStorage.diveshare_uid)
						PrefCloudStorage.diveshare_uid = fieldUserID.text
					PrefCloudStorage.diveshare_private = fieldPrivate.checked
					manager.exportToDiveShare(fieldUserID.text, fieldPrivate.checked)
				}
			}
		}
		TemplateButton {
			text: qsTr("Cancel")
			onClicked: pageStack.pop()
		}

		// AI-generated (Claude): this page is a singleton Connections target, so a
		// request started here can complete after the user has cancelled and
		// navigated elsewhere - pop only if this page is still the one on top of
		// the stack, and report the result through a passive notification so it
		// remains visible even if the page is no longer current when this fires.
		Connections {
			target: manager
			function onUploadFinish(success, text) {
				uploadInProgress = false
				var destination = uploadDestinationForCurrentUpload === ExportType.EX_DIVELOGS_DE
						? qsTr("divelogs.de") : qsTr("DiveShare")
				showPassiveNotification(qsTr("Upload to %1: %2").arg(destination).arg(text),
							 success ? 5000 : 8000)
				if (success && uploadStartedForCurrentVisit && pageStack.currentItem === exportUploadPage)
					pageStack.pop()
				if (uploadStartedForCurrentVisit)
					statusText.text = text
				progress.value = 0
			}
			function onUploadProgress(percentage) {
				progress.value = percentage
			}
		}
	}
}
