import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0

Item {
	id: logWindow
	width: parent.width
	height: parent.height
	objectName: "Log"

	ColumnLayout {
		width: parent.width
		height: parent.height
		spacing: 8

		Rectangle {
			Layout.fillHeight: true
			Layout.fillWidth: true
			Text {
				anchors.fill: parent
				wrapMode: Text.WrapAnywhere
				text: manager.logText
			}
		}
	}
}
