// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

CheckBox {
	id: root
	indicator: Rectangle {
		implicitWidth: 20 * PrefDisplay.mobile_scale
		implicitHeight: 20 * PrefDisplay.mobile_scale
		x: root.leftPadding
		y: parent.height / 2 - height / 2
		radius: 4
		border.color: root.down ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
		border.width: 2
		color: subsurfaceTheme.backgroundColor

		Rectangle {
			width: 12 * PrefDisplay.mobile_scale
			height: 12 * PrefDisplay.mobile_scale
			x: (parent.width - width) / 2
			y: (parent.height - height) / 2
			radius: 3
			color: root.down ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
			visible: root.checked
		}
	}
}
