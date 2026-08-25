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

	// Most people cannot mentally convert a duration into seconds, so offer
	// fixed-increment adjustments alongside the raw seconds field. A flat
	// 7-wide row left each button narrower than its label once the app font
	// size is increased, hence the 5-column grid with blank cells. Each
	// button only rewrites offsetField.text; commit() re-clamps it.
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
			// A Loader per cell so blank positions cost nothing and don't
			// need a dummy zero-delta button.
			//
			// Layout.fillWidth rather than a computed per-column width:
			// five buttons at their natural width overflow a 360dp-wide
			// phone, and dividing offsetAdjustGrid.width across the columns
			// binds each column to the grid's own resolved width, which
			// GridLayout derives from those same columns -- a circular
			// binding that does not settle. fillWidth lets GridLayout shrink
			// a column below its natural size and TemplateButton scales its
			// label to match.
			//
			// The centre column is excluded: filling all five equally spent
			// a full fifth of the row on "0" and two blanks, shrinking the
			// four wide labels ("-30m", "+15m", ...) past readability.
			Loader {
				Layout.fillWidth: index % offsetAdjustGrid.columns !== 2
				active: modelData !== null
				sourceComponent: TemplateButton {
					text: modelData.label
					// the default half-line-height pad per side inflates a
					// cell past the row share it is granted, shrinking the
					// label; a tight pad keeps the full font size
					labelPadding: Kirigami.Units.smallSpacing
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
