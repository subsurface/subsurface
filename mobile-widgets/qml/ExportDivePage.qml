// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

// Pushed page (D009) replacing the popup-based fitTzDialog: a modal Dialog
// positioned by x/y bindings against parent.width/height loses activeFocus
// on Android when the soft keyboard resizes the window, which made the
// offset field practically uneditable. A pushed TemplatePage doesn't have
// that problem and also gives room for the format picker (D007).
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
		offsetSeconds = manager.fitDefaultTzOffset(id)
		offsetField.text = offsetSeconds.toString()
	}

	function clampOffset(value) {
		if (isNaN(value))
			return 0
		return Math.max(-50400, Math.min(50400, value))
	}

	function currentFormatKey() {
		return formatCombo.currentIndex >= 0 && formatCombo.currentIndex < formats.length ?
			formats[formatCombo.currentIndex].key : ""
	}

	function runExport(destination) {
		offsetSeconds = clampOffset(parseInt(offsetField.text, 10))
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
			// a cancelled picker is a no-op with respect to file side effects (R010)
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

		TemplateLabel {
			text: qsTr("Manual timezone correction (seconds east of UTC)")
			wrapMode: Text.Wrap
			Layout.fillWidth: true
		}
		SsrfTextField {
			id: offsetField
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhFormattedNumbersOnly
			validator: IntValidator { bottom: -50400; top: 50400 }
			text: "0"
		}

		// D012/D013 (user override): most people cannot mentally convert a
		// duration into seconds, so offer fixed-increment adjustments
		// alongside the raw seconds field rather than replacing it. D013
		// replaces D012's single 7-wide row with a 5-column grid (blank
		// cells reproduce the override's diagram) because a flat row left
		// each button narrower than its label once the app's font size is
		// increased; D016 then rebalanced the column widths so the labels
		// stay legible. Each button only rewrites offsetField.text --
		// runExport() already re-reads and re-clamps it.
		GridLayout {
			id: offsetAdjustGrid
			Layout.fillWidth: true
			columns: 5
			columnSpacing: Kirigami.Units.smallSpacing
			rowSpacing: Kirigami.Units.smallSpacing

			readonly property var cells: [
				{ label: qsTr("-1h"), delta: -3600 }, { label: qsTr("-30m"), delta: -1800 }, null, { label: qsTr("+30m"), delta: 1800 }, { label: qsTr("+1h"), delta: 3600 },
				{ label: qsTr("-15m"), delta: -900 }, { label: qsTr("-5m"), delta: -300 }, null, { label: qsTr("+5m"), delta: 300 }, { label: qsTr("+15m"), delta: 900 },
				null, { label: qsTr("-1m"), delta: -60 }, { label: qsTr("0"), delta: 0 }, { label: qsTr("+1m"), delta: 60 }, null
			]

			Repeater {
				model: offsetAdjustGrid.cells
				// A Loader per cell so blank grid positions cost nothing and
				// don't need a dummy zero-delta button. On-device UAT (T09)
				// found that forcing every column to its button's natural
				// (unsqueezed) width overflowed the page on a 360dp-wide
				// phone -- five real-width buttons per row simply don't fit.
				// A previous attempt divided offsetAdjustGrid.width evenly
				// across columns via Layout.preferredWidth, but that binds each
				// column's width to the grid's own resolved width, which
				// GridLayout derives from those same columns -- a circular
				// binding that does not reliably settle (device UAT still
				// showed the same overflow with it in place).
				// Layout.fillWidth lets GridLayout shrink a column below its
				// natural size when the row doesn't fit, and TemplateButton's
				// Text.Fit content item shrinks the label to match, so the row
				// fits without depending on the grid's own width.
				//
				// D016: filling all five columns equally left every label but
				// "0" shrunk to the point of being hard to read -- the four
				// wide labels ("-30m", "+15m", ...) each got only a fifth of
				// the width, while the centre column spent an equal share on
				// "0" and two blanks. The centre column is therefore sized to
				// its own content and only the four outer columns share the
				// remaining width, which is the override's "top two rows
				// expand to the centre, third row expands to the sides".
				Loader {
					Layout.fillWidth: index % offsetAdjustGrid.columns !== 2
					active: modelData !== null
					sourceComponent: TemplateButton {
						text: modelData.label
						onClicked: {
							if (modelData.delta === 0)
								offsetField.text = "0"
							else
								offsetField.text = exportDivePage.clampOffset((parseInt(offsetField.text, 10) || 0) + modelData.delta).toString()
						}
					}
				}
			}
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
