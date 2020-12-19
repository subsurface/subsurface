// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.kde.kirigami 2.4 as Kirigami

CheckBox {
	id: cb

	indicator: Rectangle {
		implicitWidth: Kirigami.Units.gridUnit
		implicitHeight: Kirigami.Units.gridUnit
		x: cb.leftPadding
		y: parent.height / 2 - height / 2
		radius: 3
		color: subsurfaceTheme.backgroundColor
		border.color: subsurfaceTheme.textColor

		Rectangle {
			width: parent.width / 2
			height: width
			x: width / 2
			y: width / 2
			radius: 2
			color: subsurfaceTheme.textColor
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
