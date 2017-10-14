// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Controls 2.2
import org.kde.kirigami 2.2 as Kirigami

CheckBox {
	id: root
	indicator: Rectangle {
		implicitWidth: 20
		implicitHeight: 20
		x: root.leftPadding
		y: parent.height / 2 - height / 2
		radius: 4
		border.color: root.down ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
		color: subsurfaceTheme.drawerColor

		Rectangle {
		    width: 12
		    height: 12
		    x: 4
		    y: 4
		    radius: 3
			color: root.down ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
		    visible: root.checked
		}
	}
}
