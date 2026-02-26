// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


Item {
	property alias title: myLabel.text
	width: parent.width
	height: myLabel.height + 1

	Label {
		id: myLabel
		color: subsurfaceTheme.textColor
		font.pointSize: subsurfaceTheme.titlePointSize
		font.bold: true
		anchors.horizontalCenter: parent.horizontalCenter
		text: "using the TemplateTitle you need to set the title text"
	}
	TemplateLine {
		anchors.top: myLabel.bottom
		anchors.left: parent.left
		anchors.right: parent.right
	}
}

