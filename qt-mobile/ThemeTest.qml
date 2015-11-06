import QtQuick 2.5
import QtQuick.Layouts 1.1

GridLayout {
	id: themetest
	columns: 2

	Label {
		Layout.columnSpan: 2
		Layout.fillHeight: true
		text: "Theme Information"
	}

	FontMetrics {
		id: fm
	}

	Label {
		text: "units.gridUnit:"
	}
	Label {
		text: units.gridUnit
	}

	Label {
		text: "units.devicePixelRatio:"
	}
	Label {
		text: units.devicePixelRatio
	}

	Label {
		text: "FontMetrics pointSize:"
	}
	Label {
		text: fm.font.pointSize
	}

	Label {
		text: "FontMetrics pixelSize:"
	}
	Label {
		text: fm.height

	}

	Label {
		text: "hand-computed devicePixelRatio:"
	}
	Label {
		text: fm.height / fm.font.pointSize
	}

	Text {
		text: "Text item pixelSize:"
	}
	Text {
		text: font.pixelSize
	}

	Text {
		text: "Text item pointSize:"
	}
	Text {
		text: font.pointSize
	}

	Label {
		Layout.columnSpan: 2
		Layout.fillHeight: true
	}
}
