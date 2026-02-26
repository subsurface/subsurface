// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
	width: parent.width
	property string title: "no title"
	property bool isExpanded: false

	Button {
		width: parent.width

		background: Rectangle {
			id: buttonBackground
			antialiasing: true
			height: buttonText.height + 2
			width: parent.width
			border.color: "black"
			border.width: 1
			color: subsurfaceTheme.backgroundColor
			opacity: 0.5
		}
		contentItem: Text {
			id: buttonText
			font.pointSize: subsurfaceTheme.regularPointSize
			anchors.centerIn: buttonBackground
			color: subsurfaceTheme.textColor
			text: (isExpanded ? "- " : "+ ") + title
			font.bold: true
		}
		onClicked: {
			isExpanded = !isExpanded
		}
	}
}
