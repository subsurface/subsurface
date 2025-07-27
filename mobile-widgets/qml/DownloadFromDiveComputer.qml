// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: diveComputerDownloadWindow
	leftPadding: Kirigami.Units.gridUnit / 2
	rightPadding: Kirigami.Units.gridUnit / 2
	topPadding: 0
	bottomPadding: 0
	title: qsTr("Dive Computer")
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	property alias dcImportModel: importModel
	property bool divesDownloaded: false
	property bool btEnabled: manager.btEnabled
	property string btMessage: manager.btEnabled ? "" : qsTr("Bluetooth is not enabled")
	property alias vendor: comboVendor.currentIndex
	property alias product: comboProduct.currentIndex
	property alias connection: comboConnection.currentIndex
	property bool setupUSB: false
	property int firmwareUpdateState: 0
	property double progress: manager.progress

	DCImportModel {
		id: importModel

		onDownloadFinished : {
			progressBar.visible = false
			if (rowCount() > 0) {
				manager.appendTextToLog(rowCount() + " dive downloaded")
				divesDownloaded = true
			} else {
				manager.appendTextToLog("no new dives downloaded")
				divesDownloaded = false
			}
			manager.appendTextToLog("DCDownloadThread finished")
		}
	}

	function closePage() {
		pageStack.pop(downloadFromDc)
		rootItem.showDiveList()
		acceptButton.text = qsTr("Accept")
		downloadButton.text = qsTr("Download")
		divesDownloaded = false
		firmwareUpdateState = 0
		progressBar.visible = false
		progressBar.indeterminate = true
		manager.progress = 0.0
		manager.progressMessage = ""

		manager.appendTextToLog("exit DCDownload screen")
	}

	function checkFirmwareUpdateAvailable() {
		if (manager.checkFirmwareAvailable(importModel)) {
			manager.appendTextToLog("Firmware update found")
			downloadButton.text = qsTr("Update firmware")
			firmwareUpdateNoteLabel.firmwareOnDevice = manager.getFirmwareOnDevice()
			firmwareUpdateNoteLabel.latestFirmwareAvailable = manager.getLatestFirmwareAvailable()
			firmwareUpdateState = 1

			return true
		}

		return false
	}

	ColumnLayout {
		anchors.top: parent.top
		height: parent.height
		width: parent.width
		GridLayout {
			id: buttonGrid
			Layout.alignment: Qt.AlignTop
			Layout.topMargin: Kirigami.Units.smallSpacing * 4
			columns: 2
			rowSpacing: 0
			TemplateLabel {
				text: qsTr(" Vendor name: ")
			}
			TemplateComboBox {
				id: comboVendor
				Layout.fillWidth: true
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
				model: vendorList
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboVendor.width
					height: Kirigami.Units.gridUnit * 2.5
					contentItem: Text {
						text: modelData
						font.pointSize: subsurfaceTheme.regularPointSize
						color: subsurfaceTheme.textColor
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}
					highlighted: comboVendor.highlightedIndex === index
				}
				contentItem: Text {
					text: comboVendor.displayText
					font.pointSize: subsurfaceTheme.regularPointSize
					color: subsurfaceTheme.textColor
					leftPadding: Kirigami.Units.gridUnit * 0.5
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
				onCurrentTextChanged: {
					manager.DC_vendor = currentText
					comboProduct.model = manager.getProductListFromVendor(currentText)
					// try to be clever if there is just one BT/BLE dive computer paired
					if (currentIndex === manager.getDetectedVendorIndex())
						comboProduct.currentIndex = manager.getDetectedProductIndex(currentText)
				}
			}
			TemplateLabel {
				text: qsTr(" Dive Computer:")
			}
			TemplateComboBox {
				id: comboProduct
				Layout.fillWidth: true
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
				model: null
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboProduct.width
					height: Kirigami.Units.gridUnit * 2.5
					contentItem: Text {
						text: modelData
						font.pointSize: subsurfaceTheme.regularPointSize
						color: subsurfaceTheme.textColor
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}
					highlighted: comboProduct.highlightedIndex === index
				}
				contentItem: Text {
					text: comboProduct.displayText
					font.pointSize: subsurfaceTheme.regularPointSize
					color: subsurfaceTheme.textColor
					leftPadding: Kirigami.Units.gridUnit * 0.5
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
				onCurrentTextChanged: {
					manager.DC_product = currentText
					var newIdx = manager.getMatchingAddress(comboVendor.currentText, currentText)
					if (newIdx != -1)
						comboConnection.currentIndex = newIdx
				}

				onModelChanged: {
					currentIndex = manager.getDetectedProductIndex(comboVendor.currentText)
				}
			}
			TemplateLabel {
				text: qsTr(" Connection:")
			}
			TemplateComboBox {
				id: comboConnection
				Layout.fillWidth: true
				Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
				model: connectionListModel
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboConnection.width
					height: Kirigami.Units.gridUnit * 2.5
					contentItem: Text {
						text: modelData
						font.pointSize: subsurfaceTheme.smallPointSize
						color: subsurfaceTheme.textColor
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}
					highlighted: comboConnection.highlightedIndex === index
				}
				contentItem: Text {
					text: comboConnection.displayText
					font.pointSize: subsurfaceTheme.smallPointSize
					color: subsurfaceTheme.textColor
					leftPadding: Kirigami.Units.gridUnit * 0.5
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
				onCountChanged: {
					// ensure that pick the first entry once we have any
					// entries in the connection list
					if (count === 0)
						currentIndex = -1
					else if (currentIndex === -1)
						currentIndex = 0

				}

				onCurrentTextChanged: {
					var curVendor
					var curProduct
					var curDevice
					dc1.enabled = dc2.enabled = dc3.enabled = dc4.enabled = true
					for (var i = 1; i < 5; i++) {
						switch (i) {
							case 1:
								curVendor = PrefDiveComputer.vendor1
								curProduct = PrefDiveComputer.product1
								curDevice = PrefDiveComputer.device1
								break
							case 2:
								curVendor = PrefDiveComputer.vendor2
								curProduct = PrefDiveComputer.product2
								curDevice = PrefDiveComputer.device2
								break
							case 3:
								curVendor = PrefDiveComputer.vendor3
								curProduct = PrefDiveComputer.product3
								curDevice = PrefDiveComputer.device3
								break
							case 4:
								curVendor = PrefDiveComputer.vendor4
								curProduct = PrefDiveComputer.product4
								curDevice = PrefDiveComputer.device4
								break
						}

						if (comboProduct.currentIndex === -1 && currentText === "FTDI"){
							if ( curVendor === comboVendor.currentText && curDevice.toUpperCase() === currentText)
								rememberedDCsGrid.setDC(curVendor, curProduct, curDevice)
						}else if (comboProduct.currentIndex !== -1 && currentText === "FTDI") {
							if ( curVendor === comboVendor.currentText && curProduct === comboProduct.currentText && curDevice.toUpperCase() === currentText) {
								disableDC(i)
								break
							}
						}else if ( curVendor === comboVendor.currentText && curProduct === comboProduct.currentText && curProduct +" " + curDevice === currentText) {
							disableDC(i)
							break
						}else if ( curVendor === comboVendor.currentText && curProduct === comboProduct.currentText && curDevice === currentText) {
							disableDC(i)
							break
						}
					}
					downloadButton.text = qsTr("Download")
				}
				function disableDC(inx) {
					switch (inx) {
						case 1:
							dc1.enabled = false
							break;
						case 2:
							dc2.enabled = false
							break;
						case 3:
							dc3.enabled = false
							break;
						case 4:
							dc4.enabled = false
							break;
					}
				}
			}
		}

		TemplateLabel {
			text: qsTr(" Previously used dive computers: ")
			visible: PrefDiveComputer.vendor1 !== ""
		}
		Flow {
			id: rememberedDCsGrid
			visible: PrefDiveComputer.vendor1 !== ""
			Layout.alignment: Qt.AlignTop
			Layout.topMargin: Kirigami.Units.smallSpacing * 2
			spacing: Kirigami.Units.smallSpacing;
			Layout.fillWidth: true
			function setDC(vendor, product, device) {
				manager.appendTextToLog("setDC called with " + vendor + "/" + product + "/" + device)
				comboVendor.currentIndex = comboVendor.find(vendor);
				comboProduct.currentIndex = comboProduct.find(product);
				comboConnection.currentIndex = manager.getConnectionIndex(device);
			}

			TemplateButton {
				id: dc1
				visible: PrefDiveComputer.vendor1 !== ""
				text: PrefDiveComputer.vendor1 + " - " + PrefDiveComputer.product1
				onClicked: {
					// update comboboxes
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor1, PrefDiveComputer.product1, PrefDiveComputer.device1)
				}
			}
			TemplateButton {
				id: dc2
				visible: PrefDiveComputer.vendor2 !== ""
				text: PrefDiveComputer.vendor2 + " - " + PrefDiveComputer.product2
				onClicked: {
					// update comboboxes
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor2, PrefDiveComputer.product2, PrefDiveComputer.device2)
				}
			}
			TemplateButton {
				id: dc3
				visible: PrefDiveComputer.vendor3 !== ""
				text: PrefDiveComputer.vendor3 + " - " + PrefDiveComputer.product3
				onClicked: {
					// update comboboxes
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor3, PrefDiveComputer.product3, PrefDiveComputer.device3)
				}
			}
			TemplateButton {
				id: dc4
				visible: PrefDiveComputer.vendor4 !== ""
				text: PrefDiveComputer.vendor4 + " - " + PrefDiveComputer.product4
				onClicked: {
					// update comboboxes
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor4, PrefDiveComputer.product4, PrefDiveComputer.device4)
				}
			}
		}

		Controls.ProgressBar {
			id: progressBar
			Layout.topMargin: Kirigami.Units.smallSpacing * 4
			Layout.fillWidth: true
			indeterminate: true
			visible: false
		}

		RowLayout {
			id: buttonBar
			Layout.fillWidth: true
			Layout.topMargin: Kirigami.Units.smallSpacing
			spacing: Kirigami.Units.smallSpacing


			function doDownload() {
				var message = "DCDownloadThread started for " + manager.DC_vendor + " " + manager.DC_product + " on " + manager.DC_devName;
				message += " downloading " + (manager.DC_forceDownload ? "all" : "only new" ) + " dives";
				manager.appendTextToLog(message)
				progressBar.visible = true
				divesDownloaded = false // this allows the progressMessage to be displayed

				manager.createFirmwareUpdater(manager.DC_product)

				// Make sure the setting is applied to the configuration data for the current download
				Backend.sync_dc_time = syncTimeWithDiveComputer.checked

				importModel.startDownload()
			}

			Connections {
				target: manager

				function onRestartDownloadSignal() {
					buttonBar.doDownload()
				}

				function onErrorSignal() {
					if (firmwareUpdateState != 0) {
						firmwareUpdateState = 3
						progressBar.visible = false
					}
				}
			}

			TemplateButton {
				id: downloadButton
				text: qsTr("Download")
				enabled: comboVendor.currentIndex != -1 && comboProduct.currentIndex != -1 && comboConnection.currentIndex != -1 && !progressBar.visible && firmwareUpdateState < 2
				onClicked: {
					if (firmwareUpdateState == 0) {
						text = qsTr("Retry")

						var connectionString = comboConnection.currentText
						// separate BT address and BT name (if applicable)
						// pattern that matches BT addresses
						var btAddr = "(LE:)?([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}";

						// On iOS we store UUID instead of device address.
						if (Qt.platform.os === 'ios')
							btAddr = "(LE:)?\{?[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\}";

						var pattern = new RegExp(btAddr);
						var devAddress = "";
						devAddress = pattern.exec(connectionString);
						if (devAddress !== null) {
							manager.DC_bluetoothMode = true;
							manager.DC_devName = devAddress[0]; // exec returns an array with the matched text in element 0
							manager.retrieveBluetoothName();
							manager.appendTextToLog("setting btName to " + manager.DC_devBluetoothName);
						} else {
							manager.DC_bluetoothMode = false;
							manager.DC_devName = connectionString;
						}
						buttonBar.doDownload()
					} else if (firmwareUpdateState == 1) {
						progressBar.visible = true
						progressBar.indeterminate = false
						progressBar.to = 100.0
						firmwareUpdateState = 2

						manager.startFirmwareUpdate()
					}
				}
			}
			TemplateButton {
				id: quitButton
				text: progressBar.visible ? qsTr("Cancel") : qsTr("Quit")
				enabled: firmwareUpdateState != 2
				onClicked: {
					if (firmwareUpdateState == 0) {
						manager.cancelDownloadDC()

						manager.appendTextToLog("cancel download")
					}

					if (!progressBar.visible) {
						if (firmwareUpdateState != 0 || !checkFirmwareUpdateAvailable()) {
							closePage();
						}
					}
				}
			}
			TemplateButton {
				id:rescanbutton
				text: qsTr("Rescan")
				enabled: manager.btEnabled && !progressBar.visible && firmwareUpdateState == 0
				onClicked: {
					// refresh both USB and BT/BLE and make sure a reasonable entry is selected
					var current = comboConnection.currentText
					manager.rescanConnections()
					// check if the same entry is still available; if not pick the first entry
					var idx = comboConnection.find(current)
					if (idx === -1)
						idx = 0
					comboConnection.currentIndex = idx
				}
			}

			TemplateLabel {
				Layout.fillWidth: true
				text: (divesDownloaded && firmwareUpdateState == 0) ? qsTr(" Downloaded dives") :
							(manager.progressMessage != "" ? qsTr("Info:") + " " + manager.progressMessage : btMessage)
				wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			}
		}

		TemplateLabel {
			id: firmwareUpdateNoteLabel
			property string firmwareOnDevice: ""
			property string latestFirmwareAvailable: ""
			Layout.fillWidth: true
			visible: firmwareUpdateState == 1 || firmwareUpdateState == 2
			text: qsTr("<br><br>A firmware update for your dive computer is available: you have version %1 but the latest stable version is %2. <br><br><font color=\"red\">If your device uses Bluetooth, enable Bluetooth on the dive computer and do the same preparations as for a logbook download before continuing with the update.</font>").arg(firmwareOnDevice).arg(latestFirmwareAvailable)
			wrapMode: Text.WrapAtWordBoundaryOrAnywhere
		}

		RowLayout {
			id: forceDownloadOption
			Layout.fillWidth: true
			Layout.topMargin: 0
			spacing: Kirigami.Units.smallSpacing
			TemplateCheckBox {
				id: forceAll
				checked: manager.DC_forceDownload
				enabled: comboVendor.currentIndex != -1 && comboProduct.currentIndex != -1 && comboConnection.currentIndex != -1 && !progressBar.visible && firmwareUpdateState == 0
				visible: enabled
				height: forceAllLabel.height - Kirigami.Units.smallSpacing;
				width: height
				onClicked: {
					manager.DC_forceDownload = !manager.DC_forceDownload;
				}
			}
			TemplateLabel {
				id: forceAllLabel
				text: qsTr("force downloading all dives")
				visible: forceAll.visible
				wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			}
		}

		RowLayout {
			id: syncTimeOption
			Layout.fillWidth: true
			Layout.topMargin: 0
			spacing: Kirigami.Units.smallSpacing
			TemplateCheckBox {
				id: syncTimeWithDiveComputer
				checked: Backend.sync_dc_time
				enabled: comboVendor.currentIndex != -1 && comboProduct.currentIndex != -1 && comboConnection.currentIndex != -1 && !progressBar.visible && firmwareUpdateState == 0
				visible: enabled
				height: syncTimeLabel.height - Kirigami.Units.smallSpacing;
				width: height
				onClicked: {
					Backend.sync_dc_time = checked
				}
			}
			TemplateLabel {
				id: syncTimeLabel
				text: qsTr("Sync dive computer time")
				visible: syncTimeWithDiveComputer.visible
				wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			}
		}

		ListView {
			id: dlList
			Layout.topMargin: Kirigami.Units.smallSpacing * 4
			Layout.bottomMargin: bottomButtons.height / 2
			Layout.fillWidth: true
			Layout.fillHeight: true

			model : importModel
			delegate : DownloadedDiveDelegate {
				id: delegate
				datetime: model.datetime ? model.datetime : ""
				duration: model.duration ? model.duration : ""
				depth: model.depth ? model.depth : ""
				selected: model.selected ? model.selected : false

				onClicked : {
					manager.appendTextToLog("Selecting index" + index);
					importModel.selectRow(index)
				}
			}
		}
		TemplateLabel {
			text: qsTr("Please wait while we record these dives...")
			Layout.fillWidth: true
			visible: acceptButton.busy
			leftPadding: Kirigami.Units.gridUnit * 3 // trust me - that looks better
		}
		RowLayout {
			id: bottomButtons
			TemplateLabel {
				text: ""  // Spacer on the left for hamburger menu
				width: Kirigami.Units.gridUnit * 2.5
			}
			TemplateButton {
				id: acceptButton
				property bool busy: false
				enabled: divesDownloaded
				text: qsTr("Accept")
				bottomPadding: Kirigami.Units.gridUnit / 2
				onClicked: {
					progressBar.visible = false
					manager.appendTextToLog("Save downloaded dives that were selected")
					busy = true
					rootItem.showBusy("Save selected dives")
					manager.appendTextToLog("Record dives")
					importModel.recordDives()
					// it's important to save the changes because the app could get killed once
					// it's in the background - and the freshly downloaded dives would get lost
					manager.changesNeedSaving()
					rootItem.hideBusy()
					busy = false
					divesDownloaded = false

					if (!checkFirmwareUpdateAvailable()) {
						closePage()
					}
				}
			}
			TemplateLabel {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			TemplateButton {
				id: select
				enabled: divesDownloaded
				text: qsTr("Select All")
				bottomPadding: Kirigami.Units.gridUnit / 2
				onClicked : {
					importModel.selectAll()
				}
			}
			TemplateButton {
				id: unselect
				enabled: divesDownloaded
				text: qsTr("Unselect All")
				bottomPadding: Kirigami.Units.gridUnit / 2
				onClicked : {
					importModel.selectNone()
				}
			}
		}

		onVisibleChanged: {
			if (!setupUSB) {
				// if we aren't called with a known USB connection, check if we can find
				// a known BT/BLE device
				manager.appendTextToLog("download page -- looking for known BT/BLE device")
				comboVendor.currentIndex = comboProduct.currentIndex = comboConnection.currentIndex = -1
				dc1.enabled = dc2.enabled = dc3.enabled = dc4.enabled = true
				if (visible) {
					// we started the BT/BLE scan when Subsurface-mobile started, let's see if
					// that found something
					comboVendor.currentIndex = manager.getDetectedVendorIndex()
					comboProduct.currentIndex = manager.getDetectedProductIndex(comboVendor.currentText)
					comboConnection.currentIndex = manager.getMatchingAddress(comboVendor.currentText, comboProduct.currentText)
					// also check if there are USB devices (this only has an effect on Android)
					manager.usbRescan()
				}
			}
		}
	}

	onProgressChanged: {
		progressBar.value = manager.progress

		if (progressBar.value >= progressBar.to) {
			if (firmwareUpdateState != 0) {
				firmwareUpdateState = 3
				progressBar.visible = false
			}
		}
	}
}
