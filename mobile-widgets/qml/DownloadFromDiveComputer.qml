import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.Page {
	id: diveComputerDownloadWindow
	anchors.top:parent.top
	width: parent.width
	height: parent.height
	Layout.fillWidth: true;
	title: qsTr("Dive Computer")

/* this can be done by hitting the back key
	contextualActions: [
		Action {
			text: qsTr("Close Preferences")
			iconName: "dialog-cancel"
			onTriggered: {
				stackView.pop()
				contextDrawer.close()
			}
		}
	]
 */
	ColumnLayout {
		anchors.top: parent.top
		height: parent.height
		width: parent.width
		Layout.fillWidth: true
		RowLayout {
			anchors.top:parent.top
			Layout.fillWidth: true
			Text { text: qsTr(" Vendor name : ") }
			ComboBox { Layout.fillWidth: true }
		}
		RowLayout {
			Text { text: qsTr(" Dive Computer:") }
			ComboBox { Layout.fillWidth: true }
		}
		RowLayout {
			Text { text: " Progress:" }
			Layout.fillWidth: true
			ProgressBar { Layout.fillWidth: true }
		}
		RowLayout {
			SubsurfaceButton {
				text: qsTr("Download")
				onClicked: {
					text: qsTr("Retry")
					stackView.pop();
				}
			}
			SubsurfaceButton {
				id:quitbutton
				text: qsTr("Quit")
				onClicked: {
					stackView.pop();
				}
			}
		}
		RowLayout {
			Text {
				text: qsTr(" Downloaded dives")
			}
		}
		TableView {
			width: parent.width
			Layout.fillWidth: true  // The tableview should fill
			Layout.fillHeight: true // all remaining vertical space
			height: parent.height   // on this screen
			TableViewColumn {
				width: parent.width / 2
				role: "datetime"
				title: qsTr("Date / Time")
			}
			TableViewColumn {
				width: parent.width / 4
				role: "duration"
				title: qsTr("Duration")
			}
			TableViewColumn {
				width: parent.width / 4
				role: "depth"
				title: qsTr("Depth")
			}
			}
		RowLayout {
			Layout.fillWidth: true
			SubsurfaceButton {
				text: qsTr("Accept")
				onClicked: {
				stackView.pop();
				}
			}
			SubsurfaceButton {
				text: qsTr("Quit")
				onClicked: {
					stackView.pop();
				}
			}
			Text {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SubsurfaceButton {
				text: qsTr("Select All")
			}
			SubsurfaceButton {
				id: unselectbutton
				text: qsTr("Unselect All")
			}
		}
		RowLayout { // spacer to make space for silly button
			Layout.minimumHeight: 1.2 * unselectbutton.height
			Text {
				text:""
			}
		}
	}
}
