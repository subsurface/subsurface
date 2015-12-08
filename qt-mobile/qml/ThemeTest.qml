import QtQuick 2.5
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

GridLayout {
	id: themetest
	columns: 2
	anchors.margins: MobileComponents.Units.gridUnit / 2

	MobileComponents.Heading {
		Layout.columnSpan: 2
		text: "Theme Information"
	}

	MobileComponents.Heading {
		text: "Screen"
		Layout.columnSpan: 2
		level: 3
	}
	FontMetrics {
		id: fm
	}

	MobileComponents.Label {
		text: "Geometry (pixels):"
	}
	MobileComponents.Label {
		text: rootItem.width + "x" + rootItem.height
	}

	MobileComponents.Label {
		text: "Geometry (gridUnits):"
	}
	MobileComponents.Label {
		text: Math.round(rootItem.width / MobileComponents.Units.gridUnit) + "x" + Math.round(rootItem.height / MobileComponents.Units.gridUnit)
	}

	MobileComponents.Label {
		text: "Units.gridUnit:"
	}
	MobileComponents.Label {
		text: MobileComponents.Units.gridUnit
	}

	MobileComponents.Label {
		text: "Units.devicePixelRatio:"
	}
	MobileComponents.Label {
		text: MobileComponents.Units.devicePixelRatio
	}

	MobileComponents.Heading {
		text: "Font Metrics"
		level: 3
		Layout.columnSpan: 2
	}

	MobileComponents.Label {
		text: "FontMetrics pointSize:"
	}
	MobileComponents.Label {
		text: fm.font.pointSize
	}

	MobileComponents.Label {
		text: "FontMetrics pixelSize:"
	}
	MobileComponents.Label {
		text: fm.height
	}

	MobileComponents.Label {
		text: "FontMetrics devicePixelRatio:"
	}
	MobileComponents.Label {
		text: fm.height / fm.font.pointSize
	}

	MobileComponents.Label {
		text: "Text item pixelSize:"
	}
	Text {
		text: font.pixelSize
	}

	MobileComponents.Label {
		text: "Text item pointSize:"
	}
	Text {
		text: font.pointSize
	}

	MobileComponents.Label {
		Layout.columnSpan: 2
		Layout.fillHeight: true
	}
}
