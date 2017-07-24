// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Controls 2.2
import org.kde.kirigami 2.0 as Kirigami

Switch {
	id: root
	indicator: Rectangle {
		implicitWidth: Kirigami.Units.largeSpacing * 2.2
		implicitHeight: Kirigami.Units.largeSpacing * 0.75
		x: root.leftPadding
		y: parent.height / 2 - height / 2
		radius: height / 2
		color: root.checked ?
			subsurfaceTheme.lightPrimaryColor : subsurfaceTheme.backgroundColor
		border.color: subsurfaceTheme.darkerPrimaryColor

		Rectangle {
			x: root.checked ? parent.width - width : 0
			y: parent.height / 2 - height / 2
			width: Kirigami.Units.largeSpacing * 1.1
			height: Kirigami.Units.largeSpacing * 1.1
			radius: height / 2
			color: root.down || root.checked ?
				subsurfaceTheme.primaryColor : subsurfaceTheme.lightPrimaryColor
			border.color: subsurfaceTheme.darkerPrimaryColor
		}
	}
}
