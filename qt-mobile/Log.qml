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

	ColumnLayout {
		width: parent.width
		spacing: 8

		TopBar {
			height: childrenRect.height
		}

		TextEdit {
			anchors.fill: height
			text: manager.logText
		}
	}
}
