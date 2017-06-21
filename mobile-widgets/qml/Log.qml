// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.ScrollablePage {
	id: logWindow
	width: parent.width - Kirigami.Units.gridUnit
	anchors.margins: Kirigami.Units.gridUnit / 2
	objectName: "Log"
	title: qsTr("Application Log")

	ListView {
		anchors.fill: parent
		model: logModel
		currentIndex: -1
		boundsBehavior: Flickable.StopAtBounds
		maximumFlickVelocity: parent.height * 5
		cacheBuffer: Math.max(5000, parent.height * 5)
		focus: true
		clip: true
		header : Kirigami.Heading {
			text: qsTr("Application Log")
		}
		delegate : Text {
			width: logWindow.width
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			color: Kirigami.Theme.textColor
			text : message
		}
	}
}
