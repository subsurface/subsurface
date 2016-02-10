import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
Item {
	id: diveComputerDownloadWindow
	anchors.top:parent.top
	width: parent.width
	height: parent.height
	Layout.fillWidth: true;

	ColumnLayout {
		anchors.top: parent.top
		height: parent.height
		width: parent.width
		Layout.fillWidth: true
		RowLayout {
			anchors.top:parent.top
			Layout.fillWidth: true
			Text { text: " Vendor name : " }
			ComboBox { Layout.fillWidth: true }
		}
		RowLayout {
			Text { text: " Dive Computer:" }
			ComboBox { Layout.fillWidth: true }
		}
		RowLayout {
			Text { text: " Progress:" }
			Layout.fillWidth: true
			ProgressBar { Layout.fillWidth: true }
		}
		RowLayout {
			SubsurfaceButton {
				text: "Download"
				onClicked: {
					text: "Retry"
					stackView.pop();
				}
			}
			SubsurfaceButton {
				id:quitbutton
				text: "Quit"
				onClicked: {
					stackView.pop();
				}
			}
		}
		RowLayout {
			Text {
				text: " Downloaded dives"
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
				title: "Date / Time"
			}
			TableViewColumn {
				width: parent.width / 4
				role: "duration"
				title: "Duration"
			}
			TableViewColumn {
				width: parent.width / 4
				role: "depth"
				title: "Depth"
			}
			}
		RowLayout {
			Layout.fillWidth: true
			SubsurfaceButton {
				text: "Accept"
				onClicked: {
				stackView.pop();
				}
			}
			SubsurfaceButton {
				text: "Quit"
				onClicked: {
					stackView.pop();
				}
			}
			Text {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SubsurfaceButton {
				text: "Select All"
			}
			SubsurfaceButton {
				id: unselectbutton
				text: "Unselect All"
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
}
