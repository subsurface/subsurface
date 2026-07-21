// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls

Label {
	id: myLabel
	color: subsurfaceTheme.textColor
	font.pointSize: subsurfaceTheme.regularPointSize
	lineHeight: 0.8
	property alias colorBackground: myLabelBackground.color

	background: Rectangle {
		id: myLabelBackground
		implicitWidth: myLabel.width
		implicitHeight: myLabel.width
		color: subsurfaceTheme.backgroundColor
	}
}

