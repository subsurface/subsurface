// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2
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

	property bool selectAll : false
	property alias dcImportModel: importModel
	property bool divesDownloaded: false

	DCDownloadThread {
		id: downloadThread
		deviceData.vendor : comboVendor.currentText
		deviceData.product : comboProduct.currentText

		//TODO: make this dynamic?
		deviceData.devName : "/tmp/ttyS1"

		//TODO: Make this the default on the C++
		deviceData.bluetoothMode : isBluetooth.checked
		deviceData.forceDownload : false
		deviceData.createNewTrip : false
		deviceData.deviceId : 0
		deviceData.diveId : 0
		deviceData.saveDump : false
		deviceData.saveLog : false

		onFinished : {
			importModel.repopulate()
			progressBar.visible = false
			if (dcImportModel.rowCount() > 0) {
				console.log(dcImportModel.rowCount() + " dive downloaded")
				divesDownloaded = true
			} else {
				console.log("no new dives downloaded")
				divesDownloaded = false
			}
			manager.appendTextToLog("DCDownloadThread finished")
		}
	}

	DCImportModel {
		id: importModel
	}

	ColumnLayout {
		anchors.top: parent.top
		height: parent.height
		width: parent.width
		Layout.fillWidth: true
		GridLayout {
			columns: 2
			Kirigami.Label { text: qsTr(" Vendor name: ") }
			property var vendoridx: downloadThread.data().getDetectedVendorIndex("")
			ComboBox {
				id: comboVendor
				Layout.fillWidth: true
				model: vendorList
				currentIndex: parent.vendoridx
				onCurrentTextChanged: {
					comboProduct.model = downloadThread.data().getProductListFromVendor(currentText)
					if (currentIndex == downloadThread.data().getDetectedVendorIndex(currentText))
						comboProduct.currentIndex = downloadThread.data().getDetectedProductIndex(currentText, comboProduct.currentText)
				}
			}
			Kirigami.Label { text: qsTr(" Dive Computer:") }
			ComboBox {
				id: comboProduct
				property var productidx: downloadThread.data().getDetectedProductIndex(comboVendor.currentText, currentText)
				Layout.fillWidth: true
				model: null
				currentIndex: productidx
				onModelChanged: {
					currentIndex = productidx
				}
			}
			Kirigami.Label { text: qsTr("Bluetooth download:") }
			CheckBox {
				id: isBluetooth
				checked: downloadThread.data().getDetectedVendorIndex(ComboBox.currentText) != -1
				indicator: Rectangle {
					implicitWidth: 20
					implicitHeight: 20
					x: isBluetooth.leftPadding
					y: parent.height / 2 - height / 2
					radius: 4
					border.color: isBluetooth.down ? subsurfaceTheme.PrimaryColor : subsurfaceTheme.darkerPrimaryColor
					color: subsurfaceTheme.backgroundColor

					Rectangle {
					    width: 12
					    height: 12
					    x: 4
					    y: 4
					    radius: 3
					    color: isBluetooth.down ? subsurfaceTheme.PrimaryColor : subsurfaceTheme.darkerPrimaryColor
					    visible: isBluetooth.checked
					}
				}
			}
		}

		ProgressBar {
			id: progressBar
			Layout.fillWidth: true
			indeterminate: true
			visible: false
		}

		RowLayout {
			Button {
				id: download
				background: Rectangle {
					color: subsurfaceTheme.darkerPrimaryColor
					antialiasing: true
					radius: Kirigami.Units.smallSpacing * 2
				}
				text: qsTr("Download")
				contentItem: Text {
					text: download.text
					color: subsurfaceTheme.darkerPrimaryTextColor
				}
				onClicked: {
					text = qsTr("Retry")
					if (downloadThread.deviceData.bluetoothMode) {
						var addr = downloadThread.data().getDetectedDeviceAddress(comboVendor.currentText,
													  comboProduct.currentText)
						if (addr !== "")
							downloadThread.deviceData.devName = addr
						var vendor = downloadThread.deviceData.getDeviceDescriptorVendor(comboVendor.currentText,
														 comboProduct.currentText)
						downloadThread.deviceData.vendor = vendor;

						var product = downloadThread.deviceData.getDeviceDescriptorProduct(comboVendor.currentText,
														   comboProduct.currentText)
						downloadThread.deviceData.product = product;
					}
					manager.appendTextToLog("DCDownloadThread started for " + downloadThread.deviceData.devName)
					progressBar.visible = true
					downloadThread.start()
				}
			}
			Button {
				id:quitbutton
				background: Rectangle {
					color: subsurfaceTheme.darkerPrimaryColor
					antialiasing: true
					radius: Kirigami.Units.smallSpacing * 2
				}
				text: qsTr("Quit")
				contentItem: Text {
					text: quitbutton.text
					color: subsurfaceTheme.darkerPrimaryTextColor
				}
				onClicked: {
					manager.appendTextToLog("exit DCDownload screen")
					stackView.pop();
				}
			}
		}

		Kirigami.Label {
			text: qsTr(" Downloaded dives")
		}

		ListView {
			Layout.fillWidth: true
			Layout.fillHeight: true

			model : importModel
			delegate : DownloadedDiveDelegate {
				id: delegate
				datetime: model.datetime ? model.datetime : ""
				duration: model.duration ? model.duration : ""
				depth: model.depth ? model.depth : ""
				selected: model.selected ? model.selected : false

				backgroundColor: selectAll ? subsurfaceTheme.darkPrimaryColor : subsurfaceTheme.backgroundColor

				onClicked : {
					console.log("Selecting index" + index);
					importModel.selectRow(index)
				}
			}
		}

		RowLayout {
			Layout.fillWidth: true
			Kirigami.Label {
				text: ""  // Spacer on the left for hamburger menu
				Layout.fillWidth: true
			}
			Button {
				id: acceptButton
				enabled: divesDownloaded
				background: Rectangle {
					color: enabled ? subsurfaceTheme.darkerPrimaryColor : "gray"
					antialiasing: true
					radius: Kirigami.Units.smallSpacing * 2
				}
				text: qsTr("Accept")
				contentItem: Text {
					text: acceptButton.text
					color: subsurfaceTheme.darkerPrimaryTextColor
				}
				onClicked: {
					manager.appendTextToLog("Save downloaded dives that were selected")
					importModel.recordDives()
					manager.saveChangesLocal()
					diveModel.clear()
					diveModel.addAllDives()
					stackView.pop();
				}
			}
			Kirigami.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			Button {
				id: select
				enabled: divesDownloaded
				background: Rectangle {
					color: enabled ? subsurfaceTheme.darkerPrimaryColor : "gray"
					antialiasing: true
					radius: Kirigami.Units.smallSpacing * 2
				}
				text: qsTr("Select All")
				contentItem: Text {
					text: select.text
					color: subsurfaceTheme.darkerPrimaryTextColor
				}
				onClicked : {
					selectAll = true
					importModel.selectAll()
				}
			}
			Button {
				id: unselect
				enabled: divesDownloaded
				background: Rectangle {
					color: enabled ? subsurfaceTheme.darkerPrimaryColor : "gray"
					antialiasing: true
					radius: Kirigami.Units.smallSpacing * 2
				}
				text: qsTr("Unselect All")
				contentItem: Text {
					text: unselect.text
					color: subsurfaceTheme.darkerPrimaryTextColor
				}
				onClicked : {
					selectAll = false
					importModel.selectNone()
				}
			}
		}
	}
}
