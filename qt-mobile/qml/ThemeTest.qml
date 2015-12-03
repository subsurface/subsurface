import QtQuick 2.5
import QtQuick.Layouts 1.1
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

GridLayout {
	id: themetest
	columns: 2
	anchors.margins: MobileComponents.Units.gridUnit

	MobileComponents.Heading {
		Layout.columnSpan: 2
		text: "Theme Information"
	}

	FontMetrics {
		id: fm
	}

	MobileComponents.Label {
		text: "Geometry:"
	}
	MobileComponents.Label {
		text: rootItem.width + "x" + rootItem.height
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
