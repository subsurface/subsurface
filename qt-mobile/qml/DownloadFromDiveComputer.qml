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
							SubsurfaceButton { text: "..." }
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
							SubsurfaceButton { text: "Download" }
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
						SubsurfaceButton {
							text: "Select All"
						}
						SubsurfaceButton {
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
				SubsurfaceButton {
					text: "OK"

					onClicked: {
						stackView.pop();
					}
				}
				SubsurfaceButton {
					text: "Cancel"

					onClicked: {
						stackView.pop();
					}
				}
			}
		}
	}
}
