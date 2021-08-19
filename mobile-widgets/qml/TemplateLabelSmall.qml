// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4

Label {
	id: myLabel
	color: subsurfaceTheme.textColor
	font.pointSize: subsurfaceTheme.smallPointSize
	lineHeight: 0.7
	property alias colorBackground: myLabelBackground.color

	background: Rectangle {
		id: myLabelBackground
		implicitWidth: myLabel.width
		implicitHeight: myLabel.width
		color: subsurfaceTheme.backgroundColor
	}
}

