import QtQuick 2.5
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

GridLayout {
	id: themetest
	columns: 2

	MobileComponents.Label {
		Layout.columnSpan: 2
		Layout.fillHeight: true
		text: "Theme Information"
	}

	FontMetrics {
		id: fm
	}

	MobileComponents.Label {
		text: "MobileComponents.Units.gridUnit:"
	}
	MobileComponents.Label {
		text: MobileComponents.Units.gridUnit
	}

	MobileComponents.Label {
		text: "MobileComponents.Units.devicePixelRatio:"
	}
	MobileComponents.Label {
		text: MobileComponents.Units.devicePixelRatio
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
		text: "hand-computed devicePixelRatio:"
	}
	MobileComponents.Label {
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

	MobileComponents.Label {
		Layout.columnSpan: 2
		Layout.fillHeight: true
	}
}
