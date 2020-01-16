// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2
import QtQuick.Controls 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

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
