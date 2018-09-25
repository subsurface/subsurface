// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.2 as Controls
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.2 as Kirigami

Kirigami.Page {
	id: diveComputerDownloadWindow
	anchors.top:parent.top
	width: parent.width
	height: parent.height
	Layout.fillWidth: true;
	title: qsTr("Dive Computer")
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	property alias dcImportModel: importModel
	property bool divesDownloaded: false
	property bool btEnabled: manager.btEnabled
	property string btMessage: manager.btEnabled ? "" : qsTr("Bluetooth is not enabled")
	property alias vendor: comboVendor.currentIndex
	property alias product: comboProduct.currentIndex
	property alias connection: comboConnection.currentIndex

	DCDownloadThread {
		id: downloadThread

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
			id: buttonGrid
			Layout.alignment: Qt.AlignTop
			Layout.topMargin: Kirigami.Units.smallSpacing * 4
			columns: 2
			Controls.Label {
				text: qsTr(" Vendor name: ")
				font.pointSize: subsurfaceTheme.regularPointSize
			}
			Controls.ComboBox {
				id: comboVendor
				Layout.fillWidth: true
				Layout.preferredHeight: fontMetrics.height * 2.5
				model: vendorList
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboVendor.width
					height: fontMetrics.height * 2.0
					contentItem: Text {
						text: modelData
						font.pointSize: subsurfaceTheme.regularPointSize
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}
					highlighted: comboVendor.highlightedIndex === index
				}
				contentItem: Text {
					text: comboVendor.displayText
					font.pointSize: subsurfaceTheme.regularPointSize
					leftPadding: Kirigami.Units.gridUnit * 0.5
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
				onCurrentTextChanged: {
					manager.DC_vendor = currentText
					comboProduct.model = manager.getProductListFromVendor(currentText)
					if (currentIndex == manager.getDetectedVendorIndex())
						comboProduct.currentIndex = manager.getDetectedProductIndex(currentText)
					if (currentText === "Atomic Aquatics") {
						comboConnection.model = [ qsTr("USB device") ]
						comboConnection.currentIndex = 0
					} else {
						comboConnection.model = connectionListModel
					}
				}
			}
			Controls.Label {
				text: qsTr(" Dive Computer:")
				font.pointSize: subsurfaceTheme.regularPointSize
			}
			Controls.ComboBox {
				id: comboProduct
				Layout.fillWidth: true
				Layout.preferredHeight: fontMetrics.height * 2.5
				model: null
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboProduct.width
					height: fontMetrics.height * 2.0
					contentItem: Text {
						text: modelData
						font.pointSize: subsurfaceTheme.regularPointSize
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}
					highlighted: comboProduct.highlightedIndex === index
				}
				contentItem: Text {
					text: comboProduct.displayText
					font.pointSize: subsurfaceTheme.regularPointSize
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
			Controls.Label {
				text: qsTr(" Connection:")
				font.pointSize: subsurfaceTheme.regularPointSize
			}
			Controls.ComboBox {
				id: comboConnection
				Layout.fillWidth: true
				Layout.preferredHeight: fontMetrics.height * 2.5
				model: connectionListModel
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboConnection.width
					height: fontMetrics.height * 2.0
					contentItem: Text {
						text: modelData
						font.pointSize: subsurfaceTheme.smallPointSize
						verticalAlignment: Text.AlignVCenter
						elide: Text.ElideRight
					}
					highlighted: comboConnection.highlightedIndex === index
				}
				contentItem: Text {
					text: comboConnection.displayText
					font.pointSize: subsurfaceTheme.smallPointSize
					leftPadding: Kirigami.Units.gridUnit * 0.5
					horizontalAlignment: Text.AlignLeft
					verticalAlignment: Text.AlignVCenter
					elide: Text.ElideRight
				}
				onCurrentTextChanged: {
					// pattern that matches BT addresses
					var btAddr = /[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]/ ;

					// On iOS we store UUID instead of device address.
					if (Qt.platform.os === 'ios')
						btAddr = /\{?[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\}/;

					if (btAddr.test(currentText))
						manager.DC_bluetoothMode = true
					else
						manager.DC_bluetoothMode = false
					manager.DC_devName = currentText
				}
			}
		}

		Controls.Label {
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
				comboVendor.currentIndex = comboVendor.find(vendor);
				comboProduct.currentIndex = comboProduct.find(product);
				comboConnection.currentIndex = manager.getConnectionIndex(device);
			}
			SsrfButton {
				id: dc1
				visible: PrefDiveComputer.vendor1 !== ""
				text: PrefDiveComputer.vendor1 + " - " + PrefDiveComputer.product1
				onClicked: {
					// update comboboxes
					dc1.enabled = false
					dc2.enabled = true
					dc3.enabled = true
					dc4.enabled = true
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor1, PrefDiveComputer.product1, PrefDiveComputer.device1)
				}
			}
			SsrfButton {
				id: dc2
				visible: PrefDiveComputer.vendor2 !== ""
				text: PrefDiveComputer.vendor2 + " - " + PrefDiveComputer.product2
				onClicked: {
					// update comboboxes
					dc1.enabled = true
					dc2.enabled = false
					dc3.enabled = true
					dc4.enabled = true
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor2, PrefDiveComputer.product2, PrefDiveComputer.device2)
				}
			}
			SsrfButton {
				id: dc3
				visible: PrefDiveComputer.vendor3 !== ""
				text: PrefDiveComputer.vendor3 + " - " + PrefDiveComputer.product3
				onClicked: {
					// update comboboxes
					dc1.enabled = true
					dc2.enabled = true
					dc3.enabled = false
					dc4.enabled = true
					rememberedDCsGrid.setDC(PrefDiveComputer.vendor3, PrefDiveComputer.product3, PrefDiveComputer.device3)
				}
			}
			SsrfButton {
				id: dc4
				visible: PrefDiveComputer.vendor4 !== ""
				text: PrefDiveComputer.vendor4 + " - " + PrefDiveComputer.product4
				onClicked: {
					// update comboboxes
					dc1.enabled = true
					dc2.enabled = true
					dc3.enabled = true
					dc4.enabled = false
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
			Layout.topMargin: Kirigami.Units.smallSpacing * 2
			spacing: Kirigami.Units.smallSpacing
			SsrfButton {
				id: download
				text: qsTr("Download")
				onClicked: {
					text = qsTr("Retry")
					// strip any BT Name from the address
					var devName = manager.DC_devName
					if (devName != qsTr("USB device"))
						manager.DC_devName = devName.replace(/^(.*) /, "")
					manager.appendTextToLog("DCDownloadThread started for " + manager.DC_vendor + " " + manager.DC_product + " on "+ manager.DC_devName)
					progressBar.visible = true
					downloadThread.start()
				}
			}
			SsrfButton {
				id:quitbutton
				text: progressBar.visible ? qsTr("Cancel") : qsTr("Quit")
				onClicked: {
					manager.cancelDownloadDC()
					if (!progressBar.visible) {
						stackView.pop();
						download.text = qsTr("Download")
						divesDownloaded = false
						manager.progressMessage = ""
					}
					manager.appendTextToLog("exit DCDownload screen")
				}
			}
			SsrfButton {
				id:rescanbutton
				text: qsTr("Rescan")
				onClicked: {
					manager.btRescan()
				}
			}

			Controls.Label {
				Layout.fillWidth: true
				text: divesDownloaded ? qsTr(" Downloaded dives") :
							(manager.progressMessage != "" ? qsTr("Info:") + " " + manager.progressMessage : btMessage)
				wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			}
		}

		ListView {
			id: dlList
			Layout.topMargin: Kirigami.Units.smallSpacing * 4
			Layout.bottomMargin: bottomButtons.height * 1.5
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
					console.log("Selecting index" + index);
					importModel.selectRow(index)
				}
			}
		}

		RowLayout {
			id: bottomButtons
			Layout.fillWidth: true
			Controls.Label {
				text: ""  // Spacer on the left for hamburger menu
				Layout.fillWidth: true
			}
			SsrfButton {
				id: acceptButton
				enabled: divesDownloaded
				text: qsTr("Accept")
				onClicked: {
					manager.appendTextToLog("Save downloaded dives that were selected")
					importModel.recordDives()
					manager.saveChangesLocal()
					diveModel.clear()
					diveModel.addAllDives()
					stackView.pop();
					download.text = qsTr("Download")
					divesDownloaded = false
				}
			}
			Controls.Label {
				text: ""  // Spacer between 2 button groups
				Layout.fillWidth: true
			}
			SsrfButton {
				id: select
				enabled: divesDownloaded
				text: qsTr("Select All")
				onClicked : {
					importModel.selectAll()
				}
			}
			SsrfButton {
				id: unselect
				enabled: divesDownloaded
				text: qsTr("Unselect All")
				onClicked : {
					importModel.selectNone()
				}
			}
		}

		onVisibleChanged: {
			comboVendor.currentIndex = comboProduct.currentIndex = comboConnection.currentIndex = -1
			if (visible && PrefDiveComputer.vendor !== "" ) {
				rememberedDCsGrid.setDC(PrefDiveComputer.vendor1, PrefDiveComputer.product1, PrefDiveComputer.device1)
				dc1.enabled = false
			}
		}
	}
}
