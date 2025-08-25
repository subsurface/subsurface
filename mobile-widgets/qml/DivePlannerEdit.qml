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

	property string planNotes: ""
	property string maxDepth: ""
	property string duration: ""
	property var profileData: []

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

	function updateLivePlanInfo() {
		if (visible && segmentListModel.count > 0 && cylinderListModel.count > 0) {
			var cylinderData = []
			for (var i = 0; i < cylinderListModel.count; i++) {
				var item = cylinderListModel.get(i)
				cylinderData.push({
										"type": item.type, "mix": item.mix,
										"pressure": item.pressure, "use": item.use
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
										"gas": firstSegment.gas
									})
				if (descentDuration < firstSegment.duration) {
					segmentData.push({
						"depth": firstSegment.depth, // Stays at the same depth
						"duration": firstSegment.duration - descentDuration,
						"gas": firstSegment.gas
					});
				}
			}
			for (var j = start_index; j < segmentListModel.count; j++) {
				var item = segmentListModel.get(j)
				segmentData.push({
									"depth": item.depth,
									"duration": item.duration,
									"gas": item.gas
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
				diveModeBox.currentIndex, salinity, false
			)
			// Handle planResult
			planNotes = planResult.notes
			maxDepth = planResult.maxDepth
			duration = planResult.duration
			profileData = planResult.profile
		}


	}

	onVisibleChanged: {
		// This code runs every time the page becomes visible
		if (visible) {
			updateLivePlanInfo()
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
			"gas": 0 // Default to the first cylinder (index 0)
		});

		updateGasNumberList();
		updateLivePlanInfo();
	}

	Connections {
		target: cylinderListModel
		onRowsInserted: {
			updateGasNumberList();
			updateLivePlanInfo();
		}
		onRowsRemoved: {
			updateGasNumberList();
			updateLivePlanInfo();
		}
	}

	ColumnLayout {
		width: parent.width
		spacing: Kirigami.Units.gridUnit
		Layout.margins: Kirigami.Units.gridUnit

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
				enabled: false
				model: [qsTr("Open circuit"), qsTr("CCR"), qsTr("pSCR")]
				currentIndex: 0 // Default to OC
			}
			Controls.Label {
				text: qsTr("Water Type")
				verticalAlignment: Text.AlignVCenter
			}
			TemplateComboBox {
				id: waterTypeBox
				Layout.fillWidth: true
				enabled: true
				model: [qsTr("Sea Water"), qsTr("Fresh Water"), qsTr("EN13319")]
				currentIndex: 0 // Default to Sea water
				onCurrentIndexChanged: {
					updateLivePlanInfo();
				}
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
						updateLivePlanInfo();
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
			Controls.Label { text: qsTr("[%1]").arg(pressureUnit); Layout.fillWidth: true; font.bold: true }
			Item { Layout.preferredWidth: Kirigami.Units.iconSizes.medium }
		}

				ListView {
			id: cylinderListView
			Layout.fillWidth: true
			Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 8)
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
					onTextChanged: {
						cylinderListModel.setProperty(index, "mix", text);
						updateLivePlanInfo();
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
					validator: RegExpValidator { regExp: /(EAN100|EAN\d\d|AIR|100|\d{0,2}|\d{0,2}\/\d{0,2})/i }
					onActiveFocusChanged: cylinderListView.interactive = !activeFocus
				}
				TemplateComboBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 5
					model: [qsTr("OC-gas"), qsTr("diluent"), qsTr("oxygen")]
					enabled: false
					currentIndex: use
					onCurrentIndexChanged: if (currentIndex !== -1) cylinderListModel.setProperty(index, "use", currentIndex)
				}
				TemplateTextField {
					id: pressureField
					Layout.fillWidth: true
					text: pressure.toString()
					validator: IntValidator { bottom: 0; top: 10000 }
					onTextChanged: {
						cylinderListModel.setProperty(index, "pressure", Number(text));
						updateLivePlanInfo();
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
						updateLivePlanInfo();
					}
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
							"gas": lastSegment.gas
						});
						updateLivePlanInfo();
					}
				}
			}
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
			Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 8)
			clip: true

			model: segmentListModel
			delegate: RowLayout {
				width: segmentListView.width
				spacing: Kirigami.Units.gridUnit

				TemplateTextField {
					Layout.fillWidth: true
					text: depth.toString()
					validator: IntValidator { bottom: 0; top: 900 }
					onTextChanged: {
						segmentListModel.setProperty(index, "depth", Number(text));
						updateLivePlanInfo();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateTextField {
					Layout.fillWidth: true
					text: duration.toString()
					validator: IntValidator { bottom: 1; top: 999 }
					onTextChanged: {
						segmentListModel.setProperty(index, "duration", Number(text));
						updateLivePlanInfo();
					}
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
						updateLivePlanInfo();
					}
				}

				TemplateButton {
					text: "X"
					font.bold: true
					enabled: segmentListModel.count > 1
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2
					onClicked: {
						segmentListModel.remove(index);
						updateLivePlanInfo();
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

		Controls.Label {
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
				var cylinderData = []
				for (var i = 0; i < cylinderListModel.count; i++) {
					var item = cylinderListModel.get(i)
					cylinderData.push({
											"type": item.type, "mix": item.mix,
											"pressure": item.pressure, "use": item.use
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
											"gas": firstSegment.gas
										})
					if (descentDuration < firstSegment.duration) {
						segmentData.push({
							"depth": firstSegment.depth, // Stays at the same depth
							"duration": firstSegment.duration - descentDuration,
							"gas": firstSegment.gas
						});
					}
				}
				for (var j = 0; j < segmentListModel.count; j++) {
					var item = segmentListModel.get(j)
					segmentData.push({
										"depth": item.depth,
										"duration": item.duration,
										"gas": item.gas
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
					diveModeBox.currentIndex, salinity, true // shouldSave is true
				)
				var newDiveId = planResult.newDiveId
				if (newDiveId !== -1) {
					manager.selectDive(newDiveId)
					showPage(diveList)
				}
			}
		}
	}
	actions.right: Kirigami.Action {
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
			name: state = ":icons/media-playlist-repeat.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: "Refresh"
		onTriggered: {
			updateLivePlanInfo()
		}
	}
	actions.left: Kirigami.Action {
		icon {
			name: state = ":/icons/undo.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: "Back"
		onTriggered: {
			pageStack.pop()
		}
	}

}
