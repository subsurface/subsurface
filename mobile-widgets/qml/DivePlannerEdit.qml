// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

TemplatePage {
	id: divePlannerEditWindow
	title: qsTr("New Dive Plan")

	property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr("bar") : qsTr("psi")
	property string depthUnit: (Backend.length === Enums.METERS) ? qsTr("m") : qsTr("ft")

	property string planNotes: ""
	property var profileData: []

	// --- Data Models ---
	ListModel { id: cylinderListModel }
	ListModel { id: segmentListModel }
	property var gasNumberModel: []

	property var cylinderTypesModel: []

	// --- Functions ---
	function updateGasNumberList() {
		var newList = [];
		for (var i = 0; i < cylinderListModel.count; i++) {
			newList.push(qsTr("Gas %1").arg(i + 1));
		}
		gasNumberModel = newList;
	}

	function generatePlan(savePlan = false) {
		if (visible && segmentListModel.count > 0 && cylinderListModel.count > 0) {
			var cylinderData = []
			for (var i = 0; i < cylinderListModel.count; i++) {
				var item = cylinderListModel.get(i)
				cylinderData.push({
					"type": item.type,
					"mix": item.mix,
					"pressure": item.pressure,
					"use": overallDivemode.currentIndex == 1 ? item.use : 0,
				})
			}

			var segmentData = []
			var start_index = 0
			if (Backend.drop_stone_mode && segmentListModel.count > 0) {
				start_index = 1
				var descentRate = Backend.descrate;
				var firstSegment = segmentListModel.get(0)
				var descentDuration = 1
				if (descentRate > 0) {
					descentDuration = Math.ceil(firstSegment.depth / descentRate);
				}
				segmentData.push({
					"depth": firstSegment.depth,
					"duration": descentDuration,
					"gas": firstSegment.gas,
					"setpoint": firstSegment.setpoint,
					"divemode": firstSegment.divemode,
				})
				if (descentDuration < firstSegment.duration) {
					segmentData.push({
						"depth": firstSegment.depth, // Stays at the same depth
						"duration": firstSegment.duration - descentDuration,
						"gas": firstSegment.gas,
						"setpoint": firstSegment.setpoint,
						"divemode": firstSegment.divemode,
					});
				}
			}
			for (var j = start_index; j < segmentListModel.count; j++) {
				var item = segmentListModel.get(j)
				segmentData.push({
					"depth": item.depth,
					"duration": item.duration,
					"gas": item.gas,
					"setpoint": item.setpoint,
					"divemode": item.divemode,
				})
			}
			var salinity = 0
			if (waterTypeBox.currentIndex == 0) {
				salinity = 10300;
			}
			if (waterTypeBox.currentIndex == 1) {
				salinity = 10000;
			}
			if (waterTypeBox.currentIndex == 2) {
				salinity = 10200;
			}

			var planResult = Backend.divePlannerPointsModel.calculatePlan(
				cylinderData, segmentData, planDate.text, planTime.text,
				overallDivemode.currentIndex, salinity, savePlan
			)
			if (savePlan) {
				var newDiveId = planResult.newDiveId
				if (newDiveId !== -1) {
					manager.selectDive(newDiveId)
					showPage(diveList)
				}
			} else {
				// Handle planResult
				planNotes = planResult.notes
				profileData = planResult.profile
			}
		}


	}

	onVisibleChanged: {
		// This code runs every time the page becomes visible
		if (visible) {
			cylinderTypesModel = manager.cylinderListInit;
			generatePlan()
		}
	}

	onProfileDataChanged: {
		profileCanvas.requestPaint();
	}

	Component.onCompleted: {
		Backend.planner_gflow = PrefTechnicalDetails.gflow;
		Backend.planner_gfhigh = PrefTechnicalDetails.gfhigh;
		cylinderListModel.append({
			"type": PrefEquipment.default_cylinder ? PrefEquipment.default_cylinder : "AL80",
			"mix": "21/0",
			"pressure": (Backend.pressure === Enums.BAR) ? 200 : 3000,
			"use": 0 // Default to OC_GAS
		});

		segmentListModel.append({
			"depth": (Backend.length === Enums.METERS) ? 14 : 45,
			"duration": 20,
			"gas": 0, // Default to the first cylinder (index 0)
			"setpoint": Backend.default_setpoint,
			"divemode": 0,
		});

		updateGasNumberList();
		generatePlan();
	}

	Connections {
		target: cylinderListModel
		onRowsInserted: {
			updateGasNumberList();
			generatePlan();
		}
		onRowsRemoved: {
			updateGasNumberList();
			generatePlan();
		}
	}

	Connections {
		target: rootItem
		function onSettingsChanged() {
			generatePlan();
		}
	}

	ColumnLayout {
		width: parent.width
		spacing: Kirigami.Units.gridUnit
		Layout.margins: Kirigami.Units.gridUnit

		TemplateLabel {
			text: qsTr("Plan Details")
			font.bold: true
			font.pixelSize: Kirigami.Units.gridUnit * 1.2
		}

		GridLayout {
			Layout.fillWidth: true
			columns: 2

			TemplateTextField {
				id: planDate
				Layout.fillWidth: true
				placeholderText: qsTr("Date")
				// Use a standard, unambiguous format
				text: Qt.formatDate(new Date(), "yyyy-MM-dd")
			}
			TemplateTextField {
				id: planTime
				Layout.fillWidth: true
				placeholderText: qsTr("Time")
				// Use a standard, unambiguous format
				text: Qt.formatTime(new Date(), "hh:mm:ss")
			}
			TemplateLabel {
				text: qsTr("Dive Mode")
				verticalAlignment: Text.AlignVCenter
			}
			TemplateComboBox {
				id: overallDivemode
				Layout.fillWidth: true
				model: [ qsTr("Open circuit"), qsTr("CCR"), qsTr("pSCR") ]
				currentIndex: 0 // Default to OC
				onCurrentIndexChanged: {
					generatePlan();
				}
			}
			TemplateCheckBox {
				text: qsTr("Deco on OC bailout")
				Layout.columnSpan: 2
				checked: Backend.dobailout
				visible: overallDivemode.currentIndex !== 0
				onClicked: {
					Backend.dobailout = checked;
					generatePlan();
				}
			}
			TemplateLabel {
				text: qsTr("Water Type")
				verticalAlignment: Text.AlignVCenter
			}
			TemplateComboBox {
				id: waterTypeBox
				Layout.fillWidth: true
				model: [ qsTr("Sea Water"), qsTr("Fresh Water"), qsTr("EN13319") ]
				currentIndex: 0 // Default to Sea water
				onCurrentIndexChanged: {
					generatePlan();
				}
			}
		}

		// --- 1. Cylinders Section ---
		RowLayout {
			TemplateLabel {
				text: qsTr("Cylinders")
				font.bold: true
				font.pixelSize: Kirigami.Units.gridUnit * 1.2
			}
			TemplateButton {
					contentItem: Text {
						text: "+"
						horizontalAlignment: Text.AlignHCenter
						color: subsurfaceTheme.primaryTextColor
					}
					font.bold: true
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					onClicked: {
						cylinderListModel.append({
							"type": PrefEquipment.default_cylinder ? PrefEquipment.default_cylinder : "AL80",
							"mix": "21/0",
							"pressure": (Backend.pressure === Enums.BAR) ? 200 : 3000,
							"use": 0 // Default to OC_GAS
						});
						generatePlan();
					}
			}
		}
		RowLayout {
			spacing: Kirigami.Units.smallSpacing // Reduced spacing

			// Header labels updated for new layout
			TemplateLabel { text: qsTr("#"); Layout.preferredWidth: Kirigami.Units.gridUnit * 2; font.bold: true }
			TemplateLabel { text: qsTr("Type"); Layout.minimumWidth: Kirigami.Units.gridUnit * 8; Layout.maximumWidth: Kirigami.Units.gridUnit * 8; font.bold: true }
			TemplateLabel { text: qsTr("Mix"); Layout.preferredWidth: Kirigami.Units.gridUnit * 3; font.bold: true }
			TemplateLabel {
				text: qsTr("Dil");
				Layout.preferredWidth: Kirigami.Units.gridUnit * 3;
				visible: overallDivemode.currentIndex == 1
				font.bold: true
			}
			TemplateLabel { text: qsTr("[%1]").arg(pressureUnit); Layout.preferredWidth: Kirigami.Units.gridUnit * 4; font.bold: true }
			Item { Layout.preferredWidth: Kirigami.Units.iconSizes.medium }
		}

		ListView {
			id: cylinderListView
			Layout.fillWidth: true
			Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 8)
			clip: true
			model: cylinderListModel

			delegate: RowLayout {
				spacing: Kirigami.Units.smallSpacing // Reduced spacing

				TemplateLabel {
					text: index + 1
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
				TemplateComboBox {
					id: typeBox
					Layout.minimumWidth: Kirigami.Units.gridUnit * 8
					Layout.maximumWidth: Kirigami.Units.gridUnit * 8
					model: cylinderTypesModel
					currentIndex: model.indexOf(type)
					onActivated: {
						if (currentIndex !== -1) {
							// Update the 'type' property in the model with the selected cylinder text
							cylinderListModel.setProperty(parent.index, "type", currentText);

							// This updates the dive plan summary in real-time
							generatePlan();
						}
					}
				}
				TemplateTextField {
					id: mixField
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					text: mix
					onTextChanged: {
						cylinderListModel.setProperty(index, "mix", text);
						generatePlan();
					}
					onEditingFinished: {
						var parts = text.split('/');

						if (parts.length === 2) {
							var o2 = parseInt(parts[0], 10);
							var he = parseInt(parts[1], 10);

							if (!isNaN(o2) && !isNaN(he) && (o2 + he > 100)) {

								var correctedHe = 100 - o2;
								if (correctedHe < 0) correctedHe = 0; // Sanity check

								var correctedMix = o2 + "/" + correctedHe;

								text = correctedMix;
							}
						}
					}
					validator: RegularExpressionValidator { regularExpression: /(EAN100|EAN\d\d|AIR|100|\d{0,2}|\d{0,2}\/\d{0,2})/i }
					onActiveFocusChanged: cylinderListView.interactive = !activeFocus
				}
				TemplateCheckBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					// Map the model's 'use' property (0 or 1) to the checkbox state (false or true)
					checked: use === 1
					visible: overallDivemode.currentIndex == 1
					onClicked: {
						// Update 'use': if checked is true, set 'use' to 1 (Diluent); otherwise, set to 0 (OC-gas)
						cylinderListModel.setProperty(index, "use", checked ? 1 : 0);
						generatePlan();
					}
				}
				TemplateTextField {
					id: pressureField
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					text: pressure.toString()
					validator: IntValidator { bottom: 0; top: 10000 }
					onTextChanged: {
						cylinderListModel.setProperty(index, "pressure", Number(text));
						generatePlan();
					}
					onActiveFocusChanged: cylinderListView.interactive = !activeFocus
				}
				TemplateButton {
					text: "X"
					font.bold: true
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					enabled: cylinderListModel.count > 1
					onClicked: {
						cylinderListModel.remove(index);
						generatePlan();
					}
				}
			}
		}

		// --- 2. Dive Segments Section ---
		RowLayout {
			TemplateLabel {
				text: qsTr("Dive Segments")
				font.bold: true
				font.pixelSize: Kirigami.Units.gridUnit * 1.2
			}
			TemplateButton {
				contentItem: Text {
					text: "+"
					horizontalAlignment: Text.AlignHCenter
					color: subsurfaceTheme.primaryTextColor
				}
				font.bold: true
				Layout.preferredWidth: Kirigami.Units.gridUnit * 2
				onClicked: {
					if (segmentListModel.count > 0) {
						var lastSegment = segmentListModel.get(segmentListModel.count - 1);
						segmentListModel.append({
							"depth": lastSegment.depth,
							"duration": 10,
							"gas": lastSegment.gas,
							"setpoint": lastSegment.setpoint,
							"divemode": lastSegment.divemode,
						});
						generatePlan();
					}
				}
			}
		}

		RowLayout {
			spacing: Kirigami.Units.gridUnit

			TemplateLabel {
				text: qsTr("Depth [%1]").arg(depthUnit);
				Layout.preferredWidth: Kirigami.Units.gridUnit * 4
				font.bold: true
			}
			TemplateLabel {
				text: qsTr("Time [min]");
				Layout.preferredWidth: Kirigami.Units.gridUnit * 4
				font.bold: true
			}
			TemplateLabel {
				text: qsTr("Gas");
				Layout.preferredWidth: Kirigami.Units.gridUnit * 5
				font.bold: true;
			}
			TemplateLabel {
				text: qsTr("Setpoint [bar]");
				Layout.preferredWidth: Kirigami.Units.gridUnit * 5
				font.bold: true;
				visible: overallDivemode.currentIndex == 1;
			}
			TemplateLabel {
				text: qsTr("Dive Mode");
				Layout.preferredWidth: Kirigami.Units.gridUnit * 6
				font.bold: true;
				visible: overallDivemode.currentIndex == 2;
			}
			Item { Layout.preferredWidth: Kirigami.Units.iconSizes.medium } // Spacer
		}

		ListView {
			id: segmentListView
			Layout.fillWidth: true
			Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 8)
			clip: true

			model: segmentListModel
			delegate: RowLayout {
				spacing: Kirigami.Units.gridUnit

				TemplateTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					text: depth.toString()
					validator: IntValidator { bottom: 0; top: 900 }
					onTextChanged: {
						segmentListModel.setProperty(index, "depth", Number(text));
						generatePlan();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 4
					text: duration.toString()
					validator: IntValidator { bottom: 1; top: 999 }
					onTextChanged: {
						segmentListModel.setProperty(index, "duration", Number(text));
						generatePlan();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateComboBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 5
					model: gasNumberModel
					currentIndex: gas
					onCurrentIndexChanged: {
						segmentListModel.setProperty(index, "gas", currentIndex)
						generatePlan();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 5
					text: cylinderListModel.get(gas) && cylinderListModel.get(gas).use === 1 ? (setpoint / 1000.0).toFixed(2) : ""
					validator: DoubleValidator {
						bottom: 0.16;
						top: 2.0;
						decimals: 2;
						notation: DoubleValidator.StandardNotation
					}
					visible: overallDivemode.currentIndex == 1
					enabled: cylinderListModel.get(gas) && cylinderListModel.get(gas).use == 1
					onTextChanged: {
						if (cylinderListModel.get(gas) && cylinderListModel.get(gas).use === 1) {
							segmentListModel.setProperty(index, "setpoint", Number(text) * 1000);
							generatePlan();
						}
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateComboBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 6
					model: [ qsTr("OC"), qsTr("pSCR") ]
					// Skip CCR (value === 1) as it's not applicable for pSCR mode
					currentIndex: divemode === 2 ? 1 : divemode
					visible: overallDivemode.currentIndex == 2
					onCurrentIndexChanged: {
						segmentListModel.setProperty(index, "divemode", currentIndex === 1 ? 2 : currentIndex);
						generatePlan();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateButton {
					text: "X"
					font.bold: true
					enabled: segmentListModel.count > 1
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					onClicked: {
						segmentListModel.remove(index);
						generatePlan();
					}

				}
			}
		}
		Canvas {
			id: profileCanvas
			Layout.fillWidth: true
			Layout.preferredHeight: Kirigami.Units.gridUnit * 10

			onPaint: {
				if (profileData.length < 2) return;

				var ctx = getContext("2d");
				ctx.reset();

				// --- Find max values and define geometry ---
				var maxTime = 0;
				var maxDepthMm = 0;
				for (var i = 0; i < profileData.length; i++) {
					if (profileData[i].time > maxTime) maxTime = profileData[i].time;
					if (profileData[i].depth > maxDepthMm) maxDepthMm = profileData[i].depth * 1.1;
				}
				if (maxTime === 0 || maxDepthMm === 0) return;

				var padding = 30; // Increased padding for labels
				var drawWidth = width - (padding * 1.5);
				var drawHeight = height - (padding * 1.5);
				var xScale = drawWidth / maxTime;
				var yScale = drawHeight / maxDepthMm;

				ctx.strokeStyle = "lightgray";
				ctx.lineWidth = 0.5;
				ctx.fillStyle = "gray";
				ctx.font = "10px sans-serif";

				var numDepthLines = 4;
				ctx.textAlign = "right";
				ctx.textBaseline = "middle";
				for (var i = 1; i <= numDepthLines; i++) {
					var y = padding + (i * drawHeight / numDepthLines);
					ctx.beginPath();
					ctx.moveTo(padding, y);
					ctx.lineTo(width - padding / 2, y);
					ctx.stroke();

					var depthValMm = (i / numDepthLines) * maxDepthMm;
					var depthInUserUnits = (Backend.length === Enums.METERS) ? depthValMm / 1000 : depthValMm / 304.8;
					ctx.fillText(depthInUserUnits.toFixed(0), padding - 5, y);
				}

				var numTimeLines = Math.floor(maxTime / 60 / 5); // A line every 5 minutes
				ctx.textAlign = "center";
				ctx.textBaseline = "top";
				for (var j = 1; j <= numTimeLines; j++) {
					var timeVal = j * 5 * 60; // Every 5 minutes
					if (timeVal > maxTime) continue;
					var x = padding + (timeVal * xScale);
					ctx.beginPath();
					ctx.moveTo(x, padding);
					ctx.lineTo(x, height - padding);
					ctx.stroke();
					ctx.fillText((timeVal / 60).toFixed(0), x, height - padding + 5);
				}

				ctx.strokeStyle = Kirigami.Theme.highlightColor;
				ctx.lineWidth = 2;
				ctx.beginPath();
				var firstPoint = profileData[0];
				var startX = padding + (firstPoint.time * xScale);
				var startY = padding + (firstPoint.depth * yScale);
				ctx.moveTo(startX, startY);

				for (var j = 1; j < profileData.length; j++) {
					var point = profileData[j];
					var x = padding + (point.time * xScale);
					var y = padding + (point.depth * yScale);
					ctx.lineTo(x, y);
				}
				ctx.stroke();
			}
		}

		TemplateLabel {
			text: qsTr("Dive Plan Summary")
			font.bold: true
			font.pixelSize: Kirigami.Units.gridUnit * 1.2
		}

		Controls.TextArea {
			Layout.fillWidth: true
			readOnly: true
			wrapMode: Text.Wrap
			text: planNotes
			textFormat: Text.RichText
			color: subsurfaceTheme.textColor
		}

		TemplateButton {
			contentItem: Text {
				text: "Save plan"
				horizontalAlignment: Text.AlignHCenter
				color: subsurfaceTheme.primaryTextColor
			}
			font.bold: true
			Layout.fillWidth: true
			onClicked: {
				generatePlan(true);
			}
		}
	}
	actions: [
		Kirigami.Action {
			icon {
				name: state = ":/icons/ic_settings.svg"
				color: subsurfaceTheme.primaryColor
			}
			text: "Settings"
			onTriggered: {
				showPage(divePlannerSetupWindow)
			}
		},
		Kirigami.Action {
			icon {
				name: state = ":icons/media-playlist-repeat.svg"
				color: subsurfaceTheme.primaryColor
			}
			text: "Refresh"
			onTriggered: {
				generatePlan()
			}
		},
		Kirigami.Action {
			icon {
				name: state = ":/icons/undo.svg"
				color: subsurfaceTheme.primaryColor
			}
			text: "Back"
			onTriggered: {
				pageStack.pop()
			}
		}
	]
}
