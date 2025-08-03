// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.12
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	id: divePlannerEditWindow
	title: qsTr("New Dive Plan")

	property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr("bar") : qsTr("psi")
	property string depthUnit: (Backend.length === Enums.METERS) ? qsTr("m") : qsTr("ft")

	// --- Data Models ---
	ListModel { id: cylinderListModel }
	ListModel { id: segmentListModel }
	property var gasNumberModel: []

	// --- Functions ---
		function updateGasNumberList() {
		var newList = [];
		for (var i = 0; i < cylinderListModel.count; i++) {
			newList.push(qsTr("Gas %1").arg(i + 1));
		}
		gasNumberModel = newList;
	}

	Component.onCompleted: {
		cylinderListModel.append({
			"type": PrefEquipment.default_cylinder ? PrefEquipment.default_cylinder : "AL80",
			"mix": "21/0",
			"pressure": (Backend.pressure === Enums.BAR) ? 200 : 3000,
			"use": 0 // Default to OC_GAS
		});

		segmentListModel.append({
			"depth": (Backend.length === Enums.METERS) ? 20 : 60,
			"duration": 1,
			"gas": 0 // Default to the first cylinder (index 0)
		});

		updateGasNumberList();
	}

	Connections {
		target: cylinderListModel
		onRowsInserted: updateGasNumberList()
		onRowsRemoved: updateGasNumberList()
	}

	ColumnLayout {
		width: parent.width
		spacing: Kirigami.Units.gridUnit
		Layout.margins: Kirigami.Units.gridUnit

// --- NEW: Plan Details Section ---
		Controls.Label {
			text: qsTr("Plan Details")
			font.bold: true
			font.pixelSize: Kirigami.Units.gridUnit * 1.2
		}

		GridLayout {
			Layout.fillWidth: true
			columns: 2

			Controls.TextField {
				id: planDate
				Layout.fillWidth: true
				placeholderText: qsTr("Date")
				// Use a standard, unambiguous format
				text: Qt.formatDate(new Date(), "yyyy-MM-dd")
			}
			Controls.TextField {
				id: planTime
				Layout.fillWidth: true
				placeholderText: qsTr("Time")
				// Use a standard, unambiguous format
				text: Qt.formatTime(new Date(), "hh:mm:ss")
			}
			Controls.Label {
				text: qsTr("Dive Mode")
				verticalAlignment: Text.AlignVCenter
			}
			TemplateComboBox {
				id: diveModeBox
				Layout.fillWidth: true
				model: [qsTr("Open circuit"), qsTr("CCR"), qsTr("pSCR")]
				currentIndex: 0 // Default to OC
			}
		}

		// --- 1. Cylinders Section ---
		RowLayout {
			Controls.Label {
				text: qsTr("Cylinders")
				font.bold: true
				font.pixelSize: Kirigami.Units.gridUnit * 1.2
			}
			TemplateButton {
					text: "+"
					font.bold: true
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					onClicked: {
						cylinderListModel.append({
							"type": PrefEquipment.default_cylinder ? PrefEquipment.default_cylinder : "AL80",
							"mix": "21/0",
							"pressure": (Backend.pressure === Enums.BAR) ? 200 : 3000,
							"use": 0 // Default to OC_GAS
						});
					}
			}
		}
		RowLayout {
			Layout.fillWidth: true
			spacing: Kirigami.Units.smallSpacing // Reduced spacing

			// Header labels updated for new layout
			Controls.Label { text: qsTr("#"); Layout.preferredWidth: Kirigami.Units.gridUnit * 2; font.bold: true }
			Controls.Label { text: qsTr("Type"); Layout.preferredWidth: Kirigami.Units.gridUnit * 6; font.bold: true }
			Controls.Label { text: qsTr("Mix"); Layout.preferredWidth: Kirigami.Units.gridUnit * 4; font.bold: true }
			Controls.Label { text: qsTr("Use"); Layout.preferredWidth: Kirigami.Units.gridUnit * 5; font.bold: true }
			Controls.Label { text: qsTr("(%1)").arg(pressureUnit); Layout.fillWidth: true; font.bold: true }
			Item { Layout.preferredWidth: Kirigami.Units.iconSizes.medium }
		}

				ListView {
			id: cylinderListView
			Layout.fillWidth: true
			Layout.preferredHeight: Kirigami.Units.gridUnit * 8
			clip: true
			model: cylinderListModel

			delegate: RowLayout {
				width: cylinderListView.width
				spacing: Kirigami.Units.smallSpacing // Reduced spacing

				Controls.Label {
					text: index + 1
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
				TemplateComboBox {
					id: typeBox
					Layout.preferredWidth: Kirigami.Units.gridUnit * 6
					model: manager.cylinderListInit
					currentIndex: model.indexOf(type)
					
				}
				TemplateTextField {
					id: mixField
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					text: mix
					onEditingFinished: cylinderListModel.setProperty(index, "mix", text)
					validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{1,2}|\d{1,2}\/\d{1,2})/i }
					onActiveFocusChanged: cylinderListView.interactive = !activeFocus
				}
				TemplateComboBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 5
					model: [qsTr("OC-gas"), qsTr("diluent"), qsTr("oxygen")]
					currentIndex: use
					onCurrentIndexChanged: if (currentIndex !== -1) cylinderListModel.setProperty(index, "use", currentIndex)
				}
				TemplateTextField {
					id: pressureField
					Layout.fillWidth: true
					text: pressure.toString()
					validator: IntValidator { bottom: 0; top: 10000 }
					onEditingFinished: cylinderListModel.setProperty(index, "pressure", Number(text))
					onActiveFocusChanged: cylinderListView.interactive = !activeFocus
				}
				TemplateButton {
					text: "X"
					font.bold: true
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					enabled: cylinderListModel.count > 1
					onClicked: cylinderListModel.remove(index)
				}
			}
		}

		// --- 2. Dive Segments Section ---
		RowLayout {
			Controls.Label {
				text: qsTr("Dive Segments")
				font.bold: true
				font.pixelSize: Kirigami.Units.gridUnit * 1.2
			}
			TemplateButton {
				text: "+"
				font.bold: true
				Layout.preferredWidth: Kirigami.Units.gridUnit * 2
				onClicked: {
					if (segmentListModel.count > 0) {
						var lastSegment = segmentListModel.get(segmentListModel.count - 1);
						segmentListModel.append({
							"depth": lastSegment.depth,
							"duration": 10,
							"gas": lastSegment.gas
						});
					}
				}
			}
		}
		Controls.Label {
			text: qsTr("Dive Segments")
			font.bold: true
			font.pixelSize: Kirigami.Units.gridUnit * 1.2
			Layout.topMargin: Kirigami.Units.largeSpacing
		}

		RowLayout {
			Layout.fillWidth: true
			spacing: Kirigami.Units.gridUnit

			Controls.Label { text: qsTr("Depth (%1)").arg(depthUnit); Layout.fillWidth: true; font.bold: true }
			Controls.Label { text: qsTr("Duration (min)"); Layout.fillWidth: true; font.bold: true }
			Controls.Label { text: qsTr("Gas"); Layout.fillWidth: true; font.bold: true }
			Item { Layout.preferredWidth: Kirigami.Units.iconSizes.medium } // Spacer
		}

		ListView {
			id: segmentListView
			Layout.fillWidth: true
			Layout.preferredHeight: Kirigami.Units.gridUnit * 12
			clip: true

			model: segmentListModel
			delegate: RowLayout {
				width: segmentListView.width
				spacing: Kirigami.Units.gridUnit

				TemplateTextField {
					Layout.fillWidth: true
					text: depth.toString()
					validator: IntValidator { bottom: 0; top: 300 }
					onEditingFinished: segmentListModel.setProperty(index, "depth", Number(text))
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateTextField {
					Layout.fillWidth: true
					text: duration.toString()
					validator: IntValidator { bottom: 1; top: 999 }
					onEditingFinished: segmentListModel.setProperty(index, "duration", Number(text))
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateComboBox {
					Layout.fillWidth: true
					Layout.minimumWidth: Kirigami.Units.gridUnit * 6
					model: gasNumberModel

					currentIndex: gas
					onCurrentIndexChanged: {
						if (currentIndex !== -1)
						   segmentListModel.setProperty(index, "gas", currentValue)
					}
				}

				TemplateButton {
					text: "X"
					font.bold: true
					enabled: segmentListModel.count > 1
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					onClicked: segmentListModel.remove(index)
				}
			}
		}
	}
	actions.left: Kirigami.Action {
		icon {
			name: state = ":/icons/ic_settings.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: "Settings"
		onTriggered: {
			showPage(divePlannerSetupWindow)			
		}
	}
	actions.main: Kirigami.Action {
		icon {
			name: state = ":/icons/document-save.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: "Save"
		onTriggered: {
			var cylinderData = []
			for (var i = 0; i < cylinderListModel.count; i++) {
				var item = cylinderListModel.get(i)
				cylinderData.push({
										"type": item.type, "mix": item.mix,
										"pressure": item.pressure, "use": item.use
									})
			}

			var segmentData = []
			for (var j = 0; j < segmentListModel.count; j++) {
				var item = segmentListModel.get(j)
				segmentData.push({
									"depth": item.depth,
									"duration": item.duration,
									"gas": item.gas
								})
			}
			var planResult = Backend.divePlannerPointsModel.calculatePlan(
				cylinderData, segmentData, planDate.text, planTime.text,
				diveModeBox.currentIndex, true // shouldSave is true
			)
			var newDiveId = planResult.newDiveId
			if (newDiveId !== -1) {
				manager.selectDive(newDiveId)
				showPage(diveList)
			}
		}
	}
	actions.right: Kirigami.Action {
		icon {
			name: state = ":/icons/preview.svg" 
			color: subsurfaceTheme.primaryColor
		}
		text: "Plan"
		onTriggered: {
			var cylinderData = []
			for (var i = 0; i < cylinderListModel.count; i++) {
				var item = cylinderListModel.get(i)
				cylinderData.push({
										"type": item.type, "mix": item.mix,
										"pressure": item.pressure, "use": item.use
									})
			}

			var segmentData = []
			for (var j = 0; j < segmentListModel.count; j++) {
				var item = segmentListModel.get(j)
				segmentData.push({
										"depth": item.depth,
										"duration": item.duration,
										"gas": item.gas
									})
			}

			// 1. Populate the preview page's INPUT properties
			divePlannerViewWindow.cylindersData = cylinderData
			divePlannerViewWindow.segmentsData = segmentData
			divePlannerViewWindow.planDate = planDate.text
			divePlannerViewWindow.planTime = planTime.text
			divePlannerViewWindow.diveMode = diveModeBox.currentIndex

			// 2. Navigate to the preview page
			showPage(divePlannerViewWindow)
		}
	}

}