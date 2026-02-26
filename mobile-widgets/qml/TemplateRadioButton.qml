// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

RadioButton {
	id: rb
	Layout.fillWidth: true
	indicator: Rectangle {
		implicitWidth: Kirigami.Units.gridUnit
		implicitHeight: Kirigami.Units.gridUnit
		x: rb.leftPadding
		y: parent.height / 2 - height / 2
		radius: width / 2
		color: subsurfaceTheme.backgroundColor
		border.color: subsurfaceTheme.textColor

		Rectangle {
			width: parent.width / 2
			height: width
			x: width / 2
			y: width / 2
			radius: width / 2
			color: subsurfaceTheme.textColor
			visible: rb.checked
		}
	}
	contentItem: Text {
		color: subsurfaceTheme.textColor
		font.pointSize: subsurfaceTheme.regularPointSize
		text: rb.text
		leftPadding: rb.indicator.width + rb.spacing
	}
}
