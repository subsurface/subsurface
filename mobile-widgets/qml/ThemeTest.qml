// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import org.kde.kirigami 2.4 as Kirigami
import org.subsurfacedivelog.mobile 1.0

Kirigami.Page {

	title: "Theme Information"
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	FontMetrics {
		id: fontMetrics
	}

	GridLayout {
		id: themetest
		columns: 2
		anchors.margins: Kirigami.Units.gridUnit / 2

		Kirigami.Heading {
			Layout.columnSpan: 2
			text: "Theme Information"
			color: subsurfaceTheme.textColor
		}

		Kirigami.Heading {
			text: "Screen"
			color: subsurfaceTheme.textColor
			Layout.columnSpan: 2
			level: 3
		}

		TemplateLabel {
			text: "Geometry (pixels):"
		}
		TemplateLabel {
			text: rootItem.width + "x" + rootItem.height
		}

		TemplateLabel {
			text: "Geometry (gridUnits):"
		}
		TemplateLabel {
			text: Math.round(rootItem.width / Kirigami.Units.gridUnit) + "x" + Math.round(rootItem.height / Kirigami.Units.gridUnit)
		}

		TemplateLabel {
			text: "Units.gridUnit:"
		}
		TemplateLabel {
			text: Kirigami.Units.gridUnit
		}

		TemplateLabel {
			text: "Units.devicePixelRatio:"
		}
		TemplateLabel {
			text: Screen.devicePixelRatio
		}

		Kirigami.Heading {
			text: "Font Metrics"
			color: subsurfaceTheme.textColor
			level: 3
			Layout.columnSpan: 2
		}

		TemplateLabel {
			text: "basePointSize:"
		}
		TemplateLabel {
			text: subsurfaceTheme.basePointSize
		}

		TemplateLabel {
			text: "FontMetrics pointSize:"
		}
		TemplateLabel {
			text: fontMetrics.font.pointSize
		}

		TemplateLabel {
			text: "FontMetrics pixelSize:"
		}
		TemplateLabel {
			text: Number(fontMetrics.height).toFixed(2)
		}

		TemplateLabel {
			text: "FontMetrics devicePixelRatio:"
		}
		TemplateLabel {
			text: Number(fontMetrics.height / fontMetrics.font.pointSize).toFixed(2)
		}

		TemplateLabel {
			text: "Text item pixelSize:"
		}
		TemplateLabel {
			text: fontMetrics.font.pixelSize
		}

		TemplateLabel {
			text: "Text item pointSize:"
		}
		TemplateLabel {
			text: fontMetrics.font.pointSize
		}

		TemplateLabel {
			text: "Pixel density:"
		}
		TemplateLabel {
			text: Number(Screen.pixelDensity).toFixed(2)
		}

		TemplateLabel {
			text: "Height of default font:"
		}
		TemplateLabel {
			text: Number(fontMetrics.font.pixelSize / Screen.pixelDensity).toFixed(2) + "mm"
		}

		TemplateLabel {
			text: "2cm x 2cm square:"
		}
		Rectangle {
			width: Math.round(Screen.pixelDensity * 20)
			height: Math.round(Screen.pixelDensity * 20)
			color: subsurfaceTheme.textColor
		}
		TemplateLabel {
			text: "text in 4 gridUnit square"
		}
		Rectangle {
			id: backSquare
			width: Kirigami.Units.gridUnit * 4
			height: width
			color: subsurfaceTheme.primaryColor
			border.color: subsurfaceTheme.primaryColor
			border.width: 1

			Controls.Label {
				anchors.top: backSquare.top
				anchors.left: backSquare.left
				color: subsurfaceTheme.primaryTextColor
				font.pointSize: subsurfaceTheme.regularPointSize
				text: "Simply 27 random characters"
			}
			Controls.Label {
				anchors.bottom: backSquare.bottom
				anchors.left: backSquare.left
				color: subsurfaceTheme.primaryTextColor
				font.pointSize: subsurfaceTheme.smallPointSize
				text: "Simply 27 random characters"
			}
		}

		TemplateLabel {
			Layout.columnSpan: 2
			Layout.fillHeight: true
		}
	}
}
