// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

CheckBox {
	id: cb
	indicator: Rectangle {
		implicitWidth: Kirigami.Units.gridUnit
		implicitHeight: Kirigami.Units.gridUnit
		x: cb.leftPadding
		y: parent.height / 2 - height / 2
		radius: 4
		border.color: cb.down ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
		border.width: 2
		color: subsurfaceTheme.backgroundColor
		Rectangle {
			width: parent.width / 2
			height: width
			x: width / 2
			y: width / 2
			radius: 3
			color: cb.down ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
			visible: cb.checked
		}
	}
	contentItem: Text {
		color: subsurfaceTheme.textColor
		font.pointSize: subsurfaceTheme.regularPointSize
		text: cb.text
		leftPadding: cb.indicator.width + cb.spacing
	}
}
