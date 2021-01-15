// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2
import org.kde.kirigami 2.4 as Kirigami

Button {
	id: root
	property double fontSize: subsurfaceTheme.regularPointSize
	background: Rectangle {
		id: buttonBackground
		color: root.enabled? (root.pressed ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor) : "gray"
		antialiasing: true
		radius: Kirigami.Units.smallSpacing * 2
		height: buttonText.height * 2
	}
	contentItem: Text {
		id: buttonText
		text: root.text
		font.pointSize: root.fontSize
		anchors.centerIn: buttonBackground
		color: root.pressed ? subsurfaceTheme.darkerPrimaryTextColor :subsurfaceTheme.primaryTextColor
	}
}
