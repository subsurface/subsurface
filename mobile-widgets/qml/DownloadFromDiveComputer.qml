// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.0
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
			acceptButton.enabled = true
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
			}
		}

		ProgressBar {
			Layout.fillWidth: true
			visible: false
		}

		RowLayout {
			Button {
				text: qsTr("Download")
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
					downloadThread.start()
				}
			}
			Button {
				id:quitbutton
				text: qsTr("Quit")
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
				datetime: model.datetime
				duration: model.duration
				depth: model.depth
				selected: model.selected

				backgroundColor: selectAll ? Kirigami.Theme.highlightColor : Kirigami.Theme.viewBackgroundColor

				onClicked : {
					console.log("Selecting index" + index);
					importModel.selectRow(index)
				}
			}
		}

		RowLayout {
			Layout.fillWidth: true
			Button {
				id: acceptButton
				text: qsTr("Accept")
				enabled: false
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
				text: qsTr("Select All")
				onClicked : {
					selectAll = true
					importModel.selectAll()
				}
			}
			Button {
				text: qsTr("Unselect All")
				onClicked : {
					selectAll = false
					importModel.selectNone()
				}
			}
		}
	}
}
