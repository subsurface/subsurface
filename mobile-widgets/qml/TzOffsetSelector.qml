// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Manual timezone-correction control, shared by the per-dive and the
// whole-divelog FIT export.
//
// offsetSeconds is the value of record: setOffset() seeds it, commit()
// re-reads and re-clamps whatever the user typed and returns it. The text
// field is the input and is only folded back into offsetSeconds by commit(),
// so callers must call commit() before exporting.
ColumnLayout {
	id: tzOffset

	property int offsetSeconds: 0

	function clampOffset(value) {
		if (isNaN(value))
			return 0
		return Math.max(-50400, Math.min(50400, value))
	}

	// Seconds to [+-]hh:mm:ss, always signed and zero-padded so the preview
	// keeps a fixed width while the user types. Not translated: a numeric
	// time offset, not a phrase.
	function formatOffset(seconds) {
		var sign = seconds < 0 ? "-" : "+"
		var abs = Math.abs(seconds)
		var h = String(Math.floor(abs / 3600)).padStart(2, "0")
		var m = String(Math.floor(abs / 60) % 60).padStart(2, "0")
		var s = String(abs % 60).padStart(2, "0")
		return sign + h + ":" + m + ":" + s
	}

	function setOffset(value) {
		offsetSeconds = clampOffset(value)
		offsetField.text = offsetSeconds.toString()
	}

	function commit() {
		offsetSeconds = clampOffset(parseInt(offsetField.text, 10))
		return offsetSeconds
	}

	spacing: Kirigami.Units.smallSpacing * 2

	TemplateLabel {
		text: qsTr("Manual timezone correction (seconds east of UTC)")
		wrapMode: Text.Wrap
		Layout.fillWidth: true
	}
	// The seconds field is the input; the read-only [+-]hh:mm:ss next to it
	// shows what the number means.
	RowLayout {
		Layout.fillWidth: true
		spacing: Kirigami.Units.smallSpacing * 2

		SsrfTextField {
			id: offsetField
			Layout.fillWidth: true
			inputMethodHints: Qt.ImhFormattedNumbersOnly
			validator: IntValidator { bottom: -50400; top: 50400 }
			text: "0"
		}
		TemplateLabel {
			// clamped like commit() does, so the preview can never show an
			// offset that would not be exported
			text: tzOffset.formatOffset(tzOffset.clampOffset(parseInt(offsetField.text, 10)))
		}
	}

	// D012/D013 (user override): most people cannot mentally convert a
	// duration into seconds, so offer fixed-increment adjustments alongside
	// the raw seconds field rather than replacing it. D013 replaces D012's
	// single 7-wide row with a 5-column grid (blank cells reproduce the
	// override's diagram) because a flat row left each button narrower than
	// its label once the app's font size is increased; D016 then rebalanced
	// the column widths so the labels stay legible. Each button only
	// rewrites offsetField.text -- commit() re-reads and re-clamps it.
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
			// don't need a dummy zero-delta button. Layout.fillWidth lets
			// GridLayout shrink a column below its natural size when the row
			// doesn't fit, and TemplateButton's Text.Fit content item
			// shrinks the label to match, so the row fits without depending
			// on the grid's own width (a per-column width computed from the
			// grid's own resolved width is a circular binding that does not
			// reliably settle). The centre column is excluded from the
			// share so "0" and its two blank neighbours don't take space
			// away from the four wide labels ("-30m", "+15m", ...).
			Loader {
				Layout.fillWidth: index % offsetAdjustGrid.columns !== 2
				active: modelData !== null
				sourceComponent: TemplateButton {
					text: modelData.label
					onClicked: {
						if (modelData.delta === 0)
							offsetField.text = "0"
						else
							offsetField.text = tzOffset.clampOffset((parseInt(offsetField.text, 10) || 0) + modelData.delta).toString()
					}
				}
			}
		}
	}
}
