// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2
import org.kde.kirigami 2.4 as Kirigami

Switch {
	id: root
	indicator: Rectangle {
		implicitWidth: 36 * PrefDisplay.mobile_scale
		implicitHeight: 12 * PrefDisplay.mobile_scale
		x: root.leftPadding
		y: parent.height / 2 - height / 2
		radius: height / 2
		color: root.checked ?
			subsurfaceTheme.lightPrimaryColor : subsurfaceTheme.backgroundColor
		border.color: subsurfaceTheme.darkerPrimaryColor

		Rectangle {
			x: root.checked ? parent.width - width : 0
			y: parent.height / 2 - height / 2
			width: 20 * PrefDisplay.mobile_scale
			height: 20 * PrefDisplay.mobile_scale
			radius: height / 2
			color: root.down || root.checked ?
				subsurfaceTheme.primaryColor : subsurfaceTheme.lightPrimaryColor
			border.color: subsurfaceTheme.darkerPrimaryColor
		}
	}
}
