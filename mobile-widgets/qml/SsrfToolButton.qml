// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.subsurfacedivelog.mobile 1.0

// A styled icon-only toolbar button for use in page footers.
// Usage:
//   SsrfToolButton { iconSource: "qrc:/icons/ic_add.svg"; onClicked: doSomething() }
//   SsrfToolButton { iconSource: "qrc:/icons/document-save.svg"; highlighted: true; onClicked: save() }
// Note: both app and Breeze icons are at qrc:/icons/...
Controls.ToolButton {
	id: root

	// highlighted (inherited from AbstractButton): set true to mark the primary/featured action.
	// Primary buttons use contrastAccentColor background instead of lighter primaryColor.

	property url iconSource
	property int diameter: root.highlighted ? 3 : 2
	width: Kirigami.Units.gridUnit * diameter
	height: Kirigami.Units.gridUnit * diameter

	contentItem: Image {
		source: root.iconSource
		sourceSize.width: Kirigami.Units.iconSizes.smallMedium
		sourceSize.height: Kirigami.Units.iconSizes.smallMedium
		fillMode: Image.PreserveAspectFit
		anchors.centerIn: parent
	}

	background: Rectangle {
		readonly property color baseColor: root.highlighted
			? subsurfaceTheme.primaryColor
			: Qt.lighter(subsurfaceTheme.primaryColor, 1.6)
		color: root.pressed ? Qt.darker(baseColor, 1.5) : baseColor
		radius: height / 2
	}
}
