// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls 2.1
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {
	id: diveComputerDownloadWindow
	anchors.top:parent.top
	width: parent.width
	height: parent.height
	Layout.fillWidth: true;
	title: qsTr("Dive Computer")

/* this can be done by hitting the back key
	contextualActions: [
		Kirigami.Action {
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
		GridLayout {
			columns: 2
			Kirigami.Label { text: qsTr(" Vendor name: ") }  ComboBox { Layout.fillWidth: true }
			Kirigami.Label { text: qsTr(" Dive Computer:") } ComboBox { Layout.fillWidth: true }
		}

		ProgressBar {
			Layout.fillWidth: true
			visible: false
		}

		RowLayout {
			Button {
				text: qsTr("Download")
				onClicked: {
					text: qsTr("Retry")
					stackView.pop();
				}
			}
			Button {
				id:quitbutton
				text: qsTr("Quit")
				onClicked: {
					stackView.pop();
				}
			}
		}
		RowLayout {
			Kirigami.Label {
				text: qsTr(" Downloaded dives")
			}
		}
		QQC1.TableView {
			width: parent.width
			Layout.fillWidth: true  // The tableview should fill
			Layout.fillHeight: true // all remaining vertical space
			height: parent.height   // on this screen
			QQC1.TableViewColumn {
				width: parent.width / 2
				role: "datetime"
				title: qsTr("Date / Time")
			}
			QQC1.TableViewColumn {
				width: parent.width / 4
				role: "duration"
				title: qsTr("Duration")
			}
			QQC1.TableViewColumn {
				width: parent.width / 4
				role: "depth"
				title: qsTr("Depth")
			}
		}
		RowLayout {
			Layout.fillWidth: true
			Button {
				text: qsTr("Accept")
				onClicked: {
				stackView.pop();
				}
			}
			Button {
				text: qsTr("Quit")
				onClicked: {
					stackView.pop();
				}
			}
			Kirigami.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			Button {
				text: qsTr("Select All")
			}
			Button {
				id: unselectbutton
				text: qsTr("Unselect All")
			}
		}
		RowLayout { // spacer to make space for silly button
			Layout.minimumHeight: 1.2 * unselectbutton.height
			Kirigami.Label {
				text:""
			}
		}
	}
}
