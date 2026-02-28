// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	id: logWindow
	objectName: "Log"
	title: qsTr("Application Log")
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	ListView {
		anchors.fill: parent
		model: logModel
		currentIndex: -1
		boundsBehavior: Flickable.StopAtBounds
		maximumFlickVelocity: parent.height * 5
		cacheBuffer: 100 // cache up to 100 lines above / below what is shown for smoother scrolling
		focus: true
		clip: true
		delegate : Text {
			width: logWindow.width
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			color: subsurfaceTheme.textColor
			text : message
			font.pointSize: subsurfaceTheme.smallPointSize
			leftPadding: Kirigami.Units.gridUnit / 2
		}
	}
}
