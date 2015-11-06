import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import org.subsurfacedivelog.mobile 1.0

Item {
	id: diveComputerDownloadWindow
	anchors.top: parent.top
	width: parent.width
	height: parent.height

	GridLayout {
		columns: 2
		anchors.top: parent.top
		width: parent.width
		height: parent.height
		ColumnLayout {
			height: parent.height
			width: parent.width
			ColumnLayout {
				width: parent.width
				Layout.fillHeight: true
				ColumnLayout {
					Layout.fillHeight: true
					Layout.fillWidth: true
					ColumnLayout {
						height: parent.height
						Layout.fillWidth: true
						Text { text: "Vendor" }
						ComboBox { Layout.fillWidth: true }
						Text { text: "Dive Computer" }
						ComboBox { Layout.fillWidth: true }
						Text { text: "Device or mount point" }
						RowLayout {
							Layout.fillWidth: true
							TextField { Layout.fillWidth: true }
							Button { text: "..." }
						}
						GridLayout {
							columns: 2
							CheckBox { text: "Force download of all dives" }
							CheckBox { text: "Always prefer downloaded dives" }
							CheckBox { text: "Download into new trip" }
							CheckBox { text: "Save libdivecomputer logfile" }
							CheckBox { text: "Save libdivecomputer dumpfile" }
							CheckBox { text: "Choose Bluetooth download mode" }
						}

						RowLayout {
							Layout.fillWidth: true
							ProgressBar { Layout.fillWidth: true }
							Button { text: "Download" }
						}
					}
				}
				ColumnLayout {
					height: parent.height
					Layout.fillWidth: true
					RowLayout {
						Text {
							text: "Downloaded dives"
						}
						Button {
							text: "Select All"
						}
						Button {
							text: "Unselect All"
						}
					}
					TableView {
						Layout.fillWidth: true
						Layout.fillHeight: true
					}
				}
			}
			RowLayout {
				width: parent.width
				Button {
					text: "OK"

					onClicked: {
						stackView.pop();
					}
				}
				Button {
					text: "Cancel"

					onClicked: {
						stackView.pop();
					}
				}
			}
		}
	}
}
