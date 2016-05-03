import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.kde.kirigami 1.0 as Kirigami

Kirigami.Page {

	title: "Theme Information"
/* this can be done by hitting the back key
	contextualActions: [
		Action {
			text: qsTr("Close Theme info")
			iconName: "dialog-cancel"
			onTriggered: {
				stackView.pop()
				contextDrawer.close()
			}
		}
	]
 */
	GridLayout {
		id: themetest
		columns: 2
		anchors.margins: Kirigami.Units.gridUnit / 2

		Kirigami.Heading {
			Layout.columnSpan: 2
			text: qsTr("Theme Information")
		}

		Kirigami.Heading {
			text: qsTr("Screen")
			Layout.columnSpan: 2
			level: 3
		}
		FontMetrics {
			id: fm
		}

		Kirigami.Label {
			text: qsTr("Geometry (pixels):")
		}
		Kirigami.Label {
			text: rootItem.width + "x" + rootItem.height
		}

		Kirigami.Label {
			text: qsTr("Geometry (gridUnits):")
		}
		Kirigami.Label {
			text: Math.round(rootItem.width / Kirigami.Units.gridUnit) + "x" + Math.round(rootItem.height / Kirigami.Units.gridUnit)
		}

		Kirigami.Label {
			text: qsTr("Units.gridUnit:")
		}
		Kirigami.Label {
			text: Kirigami.Units.gridUnit
		}

		Kirigami.Label {
			text: qsTr("Units.devicePixelRatio:")
		}
		Kirigami.Label {
			text: Screen.devicePixelRatio
		}

		Kirigami.Heading {
			text: qsTr("Font Metrics")
			level: 3
			Layout.columnSpan: 2
		}

		Kirigami.Label {
			text: qsTr("FontMetrics pointSize:")
		}
		Kirigami.Label {
			text: fm.font.pointSize
		}

		Kirigami.Label {
			text: qsTr("FontMetrics pixelSize:")
		}
		Kirigami.Label {
			text: fm.height
		}

		Kirigami.Label {
			text: qsTr("FontMetrics devicePixelRatio:")
		}
		Kirigami.Label {
			text: fm.height / fm.font.pointSize
		}

		Kirigami.Label {
			text: qsTr("Text item pixelSize:")
		}
		Text {
			text: font.pixelSize
		}

		Kirigami.Label {
			text: qsTr("Text item pointSize:")
		}
		Text {
			text: font.pointSize
		}

		Kirigami.Label {
			text: qsTr("Pixel density:")
		}
		Text {
			text: Screen.pixelDensity
		}

		Kirigami.Label {
			text: qsTr("Height of default font:")
		}
		Text {
			text: font.pixelSize / Screen.pixelDensity + "mm"
		}

		Kirigami.Label {
			Layout.columnSpan: 2
			Layout.fillHeight: true
		}
	}
}
