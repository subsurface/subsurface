import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2

ApplicationWindow {
	title: qsTr("Subsurface")
	width: 500;
	height: 700

	FileDialog {
		id: fileOpen
		selectExisting: true
		selectMultiple: true
	}

	menuBar: MenuBar {
		Menu {
			title: qsTr("File")
			MenuItem {
				text: qsTr("Open")
				onTriggered: fileOpen.open()
			}

			MenuItem {
				text: qsTr("Exit")
				onTriggered: Qt.quit();
			}
		}
	}
}
