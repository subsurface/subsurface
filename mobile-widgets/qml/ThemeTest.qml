import QtQuick 2.5
import QtQuick.Layouts 1.1
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

		Kirigami.Label {
			text: "Geometry (pixels):"
		}
		Kirigami.Label {
			text: rootItem.width + "x" + rootItem.height
		}

		Kirigami.Label {
			text: "Geometry (gridUnits):"
		}
		Kirigami.Label {
			text: Math.round(rootItem.width / Kirigami.Units.gridUnit) + "x" + Math.round(rootItem.height / Kirigami.Units.gridUnit)
		}

		Kirigami.Label {
			text: "Units.gridUnit:"
		}
		Kirigami.Label {
			text: Kirigami.Units.gridUnit
		}

		Kirigami.Label {
			text: "Units.devicePixelRatio:"
		}
		Kirigami.Label {
			text: Screen.devicePixelRatio
		}

		Kirigami.Heading {
			text: "Font Metrics"
			level: 3
			Layout.columnSpan: 2
		}

		Kirigami.Label {
			text: "FontMetrics pointSize:"
		}
		Kirigami.Label {
			text: fm.font.pointSize
		}

		Kirigami.Label {
			text: "FontMetrics pixelSize:"
		}
		Kirigami.Label {
			text: Number(fm.height).toFixed(2)
		}

		Kirigami.Label {
			text: "FontMetrics devicePixelRatio:"
		}
		Kirigami.Label {
			text: Number(fm.height / fm.font.pointSize).toFixed(2)
		}

		Kirigami.Label {
			text: "Text item pixelSize:"
		}
		Text {
			text: font.pixelSize
		}

		Kirigami.Label {
			text: "Text item pointSize:"
		}
		Text {
			text: font.pointSize
		}

		Kirigami.Label {
			text: "Pixel density:"
		}
		Text {
			text: Number(Screen.pixelDensity).toFixed(2)
		}

		Kirigami.Label {
			text: "Height of default font:"
		}
		Text {
			text: Number(font.pixelSize / Screen.pixelDensity).toFixed(2) + "mm"
		}

		Kirigami.Label {
			text: "2cm x 2cm square:"
		}
		Rectangle {
			width: Math.round(Screen.pixelDensity * 20)
			height: Math.round(Screen.pixelDensity * 20)
			color: "black"
		}

		Kirigami.Label {
			Layout.columnSpan: 2
			Layout.fillHeight: true
		}
	}
}
