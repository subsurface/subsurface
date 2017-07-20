// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.7

Item {
	Image {
		id: contextMenuImage
		source: "qrc:///mapwidget-context-menu"

		SequentialAnimation {
			id:contextMenuImageAnimation
			PropertyAnimation {
				target: contextMenuImage; property: "scale"; from: 1.0; to: 0.8; duration: 80;
			}
			PropertyAnimation {
				target: contextMenuImage; property: "scale"; from: 0.8; to: 1.0; duration: 60;
			}
		}

		MouseArea {
			anchors.fill: parent
			onClicked: {
				contextMenuImageAnimation.restart()
			}
		}
	}
}
