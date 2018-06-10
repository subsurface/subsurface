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
			anchors {
				top: parent.top
				topMargin: Kirigami.Units.smallSpacing * 4
			}
			columns: 2
			Controls.Label { text: qsTr(" Vendor name: ") }
			Controls.ComboBox {
				id: comboVendor
				Layout.fillWidth: true
				model: vendorList
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboVendor.width
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
					comboProduct.model = manager.getProductListFromVendor(currentText)
					if (currentIndex == manager.getDetectedVendorIndex())
						comboProduct.currentIndex = manager.getDetectedProductIndex(currentText)
				}
			}
			Controls.Label { text: qsTr(" Dive Computer:") }
			Controls.ComboBox {
				id: comboProduct
				Layout.fillWidth: true
				model: null
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboProduct.width
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
					var newIdx = manager.getMatchingAddress(comboVendor.currentText, currentText)
					if (newIdx != -1)
						comboConnection.currentIndex = newIdx
				}

				onModelChanged: {
					currentIndex = manager.getDetectedProductIndex(comboVendor.currentText)
				}
			}
			Controls.Label { text: qsTr(" Connection:") }
			Controls.ComboBox {
				id: comboConnection
				Layout.fillWidth: true
				model: connectionListModel
				currentIndex: -1
				delegate: Controls.ItemDelegate {
					width: comboConnection.width
					contentItem: Text {
						text: modelData
						// color: "#21be2b"
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
						downloadThread.manager.DC_bluetoothMode = true
					else
						downloadThread.manager.DC_bluetoothMode = false
					downloadThread.manager.DC_devName = comboConnection.currentText
				}
			}
		}

		Controls.ProgressBar {
			anchors {
				top: buttonGrid.bottom
				topMargin: Kirigami.Units.smallSpacing * 4
			}
			id: progressBar
			Layout.fillWidth: true
			indeterminate: true
			visible: false
		}

		RowLayout {
			id: buttonBar
			anchors {
				left: parent.left
				right: parent.right
				top: progressBar.bottom
				topMargin: Kirigami.Units.smallSpacing * 2
			}
			spacing: Kirigami.Units.smallSpacing
			SsrfButton {
				id: download
				text: qsTr("Download")
				onClicked: {
					text = qsTr("Retry")
					// strip any BT Name from the address
					var devName = downloadThread.manager.devName
					downloadThread.manager.devName = devName.replace(/^(.*) /, "")
					manager.appendTextToLog("DCDownloadThread started for " + downloadThread.manager.product + " on "+ downloadThread.manager.devName)
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
				Layout.maximumWidth: parent.width - download.width - quitbutton.width - rescanbutton.width
				text: divesDownloaded ? qsTr(" Downloaded dives") :
							(manager.progressMessage != "" ? qsTr("Info:") + " " + manager.progressMessage : btMessage)
				wrapMode: Text.WrapAtWordBoundaryOrAnywhere
			}
		}

		ListView {
			id: dlList
			anchors {
				top: buttonBar.bottom
				topMargin: Kirigami.Units.smallSpacing * 4
			}
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
			if (visible) {
				comboVendor.currentIndex = manager.getDetectedVendorIndex()
				comboProduct.currentIndex = manager.getDetectedProductIndex(comboVendor.currentText)
				comboConnection.currentIndex = manager.getMatchingAddress(comboVendor.currentText, comboProduct.currentText)
				manager.DC_vendor = comboVendor.currentText
				manager.DC_product = comboProduct.currentText
				manager.DC_devName = comboConnection.currentText
			}
		}
	}
}
