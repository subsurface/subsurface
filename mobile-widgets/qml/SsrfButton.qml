// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2
import org.kde.kirigami 2.2 as Kirigami

Button {
	id: root
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
		anchors.centerIn: buttonBackground
		color: root.pressed ? subsurfaceTheme.darkerPrimaryTextColor :subsurfaceTheme.primaryTextColor
	}
}
