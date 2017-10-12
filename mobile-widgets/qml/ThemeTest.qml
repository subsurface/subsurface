// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0 as Controls
import QtQuick.Window 2.2
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {

	title: "Theme Information"

	GridLayout {
		id: themetest
		columns: 2
		anchors.margins: Kirigami.Units.gridUnit / 2

		Kirigami.Heading {
			Layout.columnSpan: 2
			text: "Theme Information"
		}

		Kirigami.Heading {
			text: "Screen"
			Layout.columnSpan: 2
			level: 3
		}
		FontMetrics {
			id: fm
		}

		Controls.Label {
			text: "Geometry (pixels):"
		}
		Controls.Label {
			text: rootItem.width + "x" + rootItem.height
		}

		Controls.Label {
			text: "Geometry (gridUnits):"
		}
		Controls.Label {
			text: Math.round(rootItem.width / Kirigami.Units.gridUnit) + "x" + Math.round(rootItem.height / Kirigami.Units.gridUnit)
		}

		Controls.Label {
			text: "Units.gridUnit:"
		}
		Controls.Label {
			text: Kirigami.Units.gridUnit
		}

		Controls.Label {
			text: "Units.devicePixelRatio:"
		}
		Controls.Label {
			text: Screen.devicePixelRatio
		}

		Kirigami.Heading {
			text: "Font Metrics"
			level: 3
			Layout.columnSpan: 2
		}

		Controls.Label {
			text: "FontMetrics pointSize:"
		}
		Controls.Label {
			text: fm.font.pointSize
		}

		Controls.Label {
			text: "FontMetrics pixelSize:"
		}
		Controls.Label {
			text: Number(fm.height).toFixed(2)
		}

		Controls.Label {
			text: "FontMetrics devicePixelRatio:"
		}
		Controls.Label {
			text: Number(fm.height / fm.font.pointSize).toFixed(2)
		}

		Controls.Label {
			text: "Text item pixelSize:"
		}
		Text {
			text: font.pixelSize
		}

		Controls.Label {
			text: "Text item pointSize:"
		}
		Text {
			text: font.pointSize
		}

		Controls.Label {
			text: "Pixel density:"
		}
		Text {
			text: Number(Screen.pixelDensity).toFixed(2)
		}

		Controls.Label {
			text: "Height of default font:"
		}
		Text {
			text: Number(font.pixelSize / Screen.pixelDensity).toFixed(2) + "mm"
		}

		Controls.Label {
			text: "2cm x 2cm square:"
		}
		Rectangle {
			width: Math.round(Screen.pixelDensity * 20)
			height: Math.round(Screen.pixelDensity * 20)
			color: "black"
		}

		Controls.Label {
			Layout.columnSpan: 2
			Layout.fillHeight: true
		}
	}
}
