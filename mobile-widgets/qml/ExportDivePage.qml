// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

// A pushed page rather than a popup: a modal Dialog positioned by x/y
// bindings against parent.width/height loses activeFocus on Android when the
// soft keyboard resizes the window, which made the offset field practically
// uneditable. A pushed TemplatePage also gives room for the format picker.
TemplatePage {
	id: exportDivePage
	objectName: "ExportDivePage"
	title: qsTr("Export dive")

	property int diveId: -1
	property int offsetSeconds: 0

	// Data-driven so a future format (e.g. SSRF XML) is one entry here plus
	// one dispatch branch in runExport(), not a hardcoded control/title.
	readonly property var formats: [
		{ label: qsTr("FIT"), key: "fit" }
	]

	// The only API call sites should use: sets up the page for a given dive
	// and resets the transient selection/offset state.
	function openFor(id) {
		diveId = id
		formatCombo.currentIndex = 0
		tzOffset.setOffset(manager.fitDefaultTzOffset(id))
	}

	function currentFormatKey() {
		return formatCombo.currentIndex >= 0 && formatCombo.currentIndex < formats.length ?
			formats[formatCombo.currentIndex].key : ""
	}

	function runExport(destination) {
		offsetSeconds = tzOffset.commit()
		var key = currentFormatKey()
		if (key === "fit") {
			if (destination === "share") {
				manager.shareFitForDive(diveId, offsetSeconds)
				pageStack.pop()
			} else {
				fitFileDialog.currentFile = manager.fitSuggestedFileName(diveId)
				fitFileDialog.open()
			}
		} else {
			// A future format registered in 'formats' without a matching
			// branch here must fail loudly, not silently do nothing.
			manager.appendTextToLog("Export dive: no export performed for unrecognised format '" + key + "'")
		}
	}

	FileDialog {
		id: fitFileDialog
		fileMode: FileDialog.SaveFile
		nameFilters: [ qsTr("FIT files") + " (*.fit)" ]
		onAccepted: {
			// Round-3 finding 8: don't trust transient QML state here, read
			// the dive id back from the page's own property. If it was lost,
			// still hand off to exportFitForDive() rather than returning
			// silently -- its "no such dive" branch removes the SAF document
			// that the picker already materialised for this accepted file.
			if (exportDivePage.diveId === -1)
				manager.appendTextToLog("Export FIT: lost export context")
			manager.exportFitForDive(exportDivePage.diveId, selectedFile, exportDivePage.offsetSeconds)
			pageStack.pop()
		}
		onRejected: {
			// a cancelled picker must not have any file side effects
			pageStack.pop()
		}
	}

	ColumnLayout {
		anchors {
			top: parent.top
			left: parent.left
			right: parent.right
			margins: Kirigami.Units.gridUnit / 2
		}
		spacing: Kirigami.Units.smallSpacing * 2

		TemplateLabel {
			text: qsTr("Format")
		}
		TemplateComboBox {
			id: formatCombo
			Layout.fillWidth: true
			model: exportDivePage.formats.map(function(f) { return f.label })
		}

		TzOffsetSelector {
			id: tzOffset
			Layout.fillWidth: true
		}

		Rectangle {
			width: 1
			height: Kirigami.Units.gridUnit
			color: "transparent"
		}

		RowLayout {
			Layout.fillWidth: true
			TemplateButton {
				text: qsTr("Share")
				onClicked: exportDivePage.runExport("share")
			}
			TemplateButton {
				text: qsTr("Save")
				onClicked: exportDivePage.runExport("save")
			}
			TemplateButton {
				text: qsTr("Cancel")
				onClicked: pageStack.pop()
			}
		}
	}
}
