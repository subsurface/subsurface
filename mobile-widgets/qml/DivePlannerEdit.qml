// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami as Kirigami

TemplatePage {
	id: divePlannerEditWindow
	title: qsTr("New Dive Plan")
	bottomPadding: Kirigami.Units.gridUnit * 4

	property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr("bar") : qsTr("psi")
	property string depthUnit: (Backend.length === Enums.METERS) ? qsTr("m") : qsTr("ft")

	property string planNotes: ""
	property var profileData: []
	property date planDateTime: new Date()
	// AI-generated (Claude)
	property bool exceedsNDL: false

	// AI-generated (Claude): iOS does not map the date and time hints to a
	// specialised keyboard. Keep Android's specialised hints while requesting
	// iOS's number-and-punctuation keyboard for fields with separators.
	function punctuationInputMethodHints(platformName, androidHints) {
		return platformName === "ios" ? Qt.ImhPreferNumbers : androidHints
	}

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

	// AI-generated (Claude)
	function dateInputFormat(format) {
		var components = format.match(/d+|M+|y+/g);
		if (!components)
			return format;
		for (var i = 0; i < components.length; ++i) {
			if (components[i].indexOf("d") === 0 && components[i].length > 2)
				components[i] = "d";
			if (components[i].indexOf("M") === 0 && components[i].length > 2)
				components[i] = "M";
		}
		return components.join("/");
	}

	function updateDateTimeDisplay() {
		if (!planDate.activeFocus)
			planDate.text = Qt.formatDate(planDateTime, PrefLanguage.effectiveDateFormatShort);
		if (!planTime.activeFocus)
			planTime.text = Qt.formatTime(planDateTime, PrefLanguage.effectiveTimeFormat);
	}

	function applyDateInput(text, format) {
		var loc = Qt.locale(PrefLanguage.preferenceLocaleName());
		var parsed = Date.fromLocaleDateString(loc, text, dateInputFormat(format));
		if (isNaN(parsed.getTime()))
			return false;
		var yearToken = format.match(/y+/);
		if (yearToken && yearToken[0].length <= 2) {
			var century = Math.floor(planDateTime.getFullYear() / 100) * 100;
			parsed.setFullYear(century + parsed.getFullYear() % 100);
		}
		var updated = new Date(planDateTime.getTime());
		updated.setFullYear(parsed.getFullYear(), parsed.getMonth(), parsed.getDate());
		if (updated.getFullYear() !== parsed.getFullYear() || updated.getMonth() !== parsed.getMonth() ||
		    updated.getDate() !== parsed.getDate())
			return false;
		planDateTime = updated;
		return true;
	}

	function applyTimeInput(text, format) {
		var displayText = PrefLanguage.timeDisplayText(text);
		if (displayText === "")
			return false;
		var loc = Qt.locale(PrefLanguage.preferenceLocaleName());
		var parsed = Date.fromLocaleTimeString(loc, displayText, format);
		if (isNaN(parsed.getTime()))
			return false;
		var updated = new Date(planDateTime.getTime());
		updated.setHours(parsed.getHours(), parsed.getMinutes(), parsed.getSeconds(), 0);
		planDateTime = updated;
		return true;
	}

	function gasEditText(text) {
		if (/^AIR$/i.test(text))
			return "21";
		var enrichedAir = /^EAN(\d{1,3})$/i.exec(text);
		return enrichedAir ? enrichedAir[1] : text;
	}

	function gasDisplayText(text, originalDisplay, originalEdit) {
		if (text === originalEdit)
			return originalDisplay;
		if (text.indexOf("/") !== -1 || text === "")
			return text;
		if (/^AIR$/i.test(text))
			return "AIR";
		var enrichedAir = /^EAN(\d{1,3})$/i.exec(text);
		if (enrichedAir)
			return "EAN" + Number(enrichedAir[1]);
		var percent = Number(text);
		// Guard against non-numeric input; never emit an "EANNaN" string.
		if (isNaN(percent))
			return text;
		return percent === 21 ? "AIR" : "EAN" + percent;
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
				cylinderData, segmentData,
				Qt.formatDate(planDateTime, "yyyy-MM-dd"), Qt.formatTime(planDateTime, "hh:mm:ss"),
				overallDivemode.currentIndex, salinity, savePlan
			)
			if (planResult.dateTimeValid === false)
				return;
			// AI-generated (Claude)
			// Always update the preview so that a refused save still explains why
			// the recreational plan is invalid.
			planNotes = planResult.notes
			profileData = planResult.profile
			exceedsNDL = planResult.exceedsNDL === true
			if (savePlan) {
				var newDiveId = planResult.newDiveId
				if (newDiveId !== -1) {
					manager.selectDive(newDiveId)
					showPage(diveList)
				}
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

	// AI-generated (Claude)
	onExceedsNDLChanged: {
		profileCanvas.requestPaint();
	}

	Component.onCompleted: {
		Backend.planner_gflow = PrefTechnicalDetails.gflow;
		Backend.planner_gfhigh = PrefTechnicalDetails.gfhigh;
		cylinderListModel.append({
			"type": PrefEquipment.default_cylinder ? PrefEquipment.default_cylinder : "AL80",
			"mix": "AIR",
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
		function onRowsInserted() {
			updateGasNumberList();
			generatePlan();
		}
		function onRowsRemoved() {
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

	Connections {
		target: PrefLanguage
		function onDateTimeFormatsChanged() { updateDateTimeDisplay(); }
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

			TemplateLabel {
				text: qsTr("Date")
				verticalAlignment: Text.AlignVCenter
			}
			TemplateLabel {
				text: qsTr("Time")
				verticalAlignment: Text.AlignVCenter
			}
			SsrfTextField {
				id: planDate
				property string initialEditText: ""
				property string editFormat: ""
				Layout.fillWidth: true
				sampleText: "0000-00-00"
				inputMethodHints: punctuationInputMethodHints(Qt.platform.os, Qt.ImhDate)
				text: Qt.formatDate(planDateTime, PrefLanguage.effectiveDateFormatShort)
				onActiveFocusChanged: {
					if (activeFocus) {
						editFormat = PrefLanguage.effectiveDateFormatShort;
						initialEditText = Qt.formatDate(planDateTime, dateInputFormat(editFormat));
						text = initialEditText;
					} else {
						if (text !== initialEditText)
							applyDateInput(text, editFormat);
						updateDateTimeDisplay();
					}
				}
			}
			// AI-generated (Claude): Keep the time keypad while exposing meridiem selection.
			RowLayout {
				Layout.fillWidth: true
				SsrfTextField {
					id: planTime
					property string initialEditText: ""
					property string editFormat: ""
					Layout.fillWidth: true
					sampleText: "00:00 PM"
					inputMethodHints: punctuationInputMethodHints(Qt.platform.os, Qt.ImhTime)
					text: Qt.formatTime(planDateTime, PrefLanguage.effectiveTimeFormat)
					onActiveFocusChanged: {
						if (activeFocus) {
							editFormat = PrefLanguage.effectiveTimeFormat;
							var editText = PrefLanguage.timeEditText(text);
							initialEditText = editText !== "" ? editText : text;
							text = initialEditText;
						} else {
							if (text !== initialEditText)
								applyTimeInput(text, editFormat);
							updateDateTimeDisplay();
						}
					}
				}
				TemplateButton {
					text: qsTr("A/P")
					visible: planTime.activeFocus && /AP|ap/.test(PrefLanguage.effectiveTimeFormat)
					fontSize: subsurfaceTheme.smallPointSize
					padding: 0
					Layout.alignment: Qt.AlignVCenter
					focusPolicy: Qt.NoFocus
					onClicked: planTime.text = PrefLanguage.toggleMeridiem(planTime.text, false)
				}
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
				onActivated: {
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
				onActivated: {
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
					text: "+"
					font.bold: true
					onClicked: {
						cylinderListModel.append({
							"type": PrefEquipment.default_cylinder ? PrefEquipment.default_cylinder : "AL80",
							"mix": "AIR",
							"pressure": (Backend.pressure === Enums.BAR) ? 200 : 3000,
							"use": 0 // Default to OC_GAS
						});
						generatePlan();
					}
			}
		}
		ListView {
			id: cylinderListView
			Layout.fillWidth: true
			Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 10)
			clip: true
			model: cylinderListModel

			header: RowLayout {
				width: cylinderListView.width
				spacing: Kirigami.Units.smallSpacing

				TemplateLabel { text: qsTr("#"); Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5; font.bold: true }
				TemplateLabel { text: qsTr("Type"); Layout.fillWidth: true; font.bold: true }
				TemplateLabel {
					text: qsTr("Mix");
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5;
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("Dil");
					Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5;
					visible: overallDivemode.currentIndex == 1
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("[%1]").arg(pressureUnit);
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5;
					font.bold: true
				}
				TemplateButton { text: "X"; font.bold: true; opacity: 0 }
			}

			delegate: RowLayout {
				width: cylinderListView.width
				spacing: Kirigami.Units.smallSpacing
				// give the row index a name that signal arguments cannot shadow
				readonly property int rowIndex: index

				TemplateLabel {
					text: rowIndex + 1
					Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
				TemplateComboBox {
					id: typeBox
					Layout.fillWidth: true
					model: cylinderTypesModel
					currentIndex: model.indexOf(type)
					onActivated: {
						if (currentIndex !== -1) {
							// Update the 'type' property in the model with the selected cylinder text
							cylinderListModel.setProperty(rowIndex, "type", currentText);

							// This updates the dive plan summary in real-time
							generatePlan();
						}
					}
				}
				SsrfTextField {
					id: mixField
					property string displayText: ""
					property string initialEditText: ""
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5
					sampleText: "18/45"
					inputMethodHints: punctuationInputMethodHints(Qt.platform.os, Qt.ImhDate)
					text: mix
					onTextChanged: {
						if (text !== mix) {
							cylinderListModel.setProperty(rowIndex, "mix", text);
							generatePlan();
						}
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
					onActiveFocusChanged: {
						cylinderListView.interactive = !activeFocus;
						if (activeFocus) {
							displayText = text;
							initialEditText = gasEditText(text);
							text = initialEditText;
						} else {
							text = gasDisplayText(text, displayText, initialEditText);
						}
					}
				}
				TemplateCheckBox {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
					// Map the model's 'use' property (0 or 1) to the checkbox state (false or true)
					checked: use === 1
					visible: overallDivemode.currentIndex == 1
					onClicked: {
						// Update 'use': if checked is true, set 'use' to 1 (Diluent); otherwise, set to 0 (OC-gas)
						cylinderListModel.setProperty(rowIndex, "use", checked ? 1 : 0);
						generatePlan();
					}
				}
				SsrfTextField {
					id: pressureField
					Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5
					sampleText: "3000"
					text: pressure.toString()
					// request a numeric keyboard on mobile for this integer-only field
					inputMethodHints: Qt.ImhDigitsOnly
					validator: IntValidator { bottom: 0; top: 10000 }
					onTextChanged: {
						if (Number(text) !== pressure) {
							cylinderListModel.setProperty(rowIndex, "pressure", Number(text));
							generatePlan();
						}
					}
					onActiveFocusChanged: cylinderListView.interactive = !activeFocus
				}
				TemplateButton {
					text: "X"
					font.bold: true
					enabled: cylinderListModel.count > 1
					onClicked: {
						cylinderListModel.remove(rowIndex);
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
				text: "+"
				font.bold: true
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

		ListView {
			id: segmentListView
			Layout.fillWidth: true
			Layout.preferredHeight: Math.min(contentHeight, Kirigami.Units.gridUnit * 10)
			clip: true

			model: segmentListModel

			header: RowLayout {
				width: segmentListView.width
				spacing: Kirigami.Units.smallSpacing

				TemplateLabel {
					text: qsTr("Depth [%1]").arg(depthUnit);
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("Time [min]");
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("Gas");
					Layout.fillWidth: true
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
				TemplateButton { text: "X"; font.bold: true; opacity: 0 }
			}

			delegate: RowLayout {
				width: segmentListView.width
				spacing: Kirigami.Units.smallSpacing
				// give the row index a name that signal arguments cannot shadow
				readonly property int rowIndex: index

				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					sampleText: "900"
					text: depth.toString()
					// request a numeric keyboard on mobile for this integer-only field
					inputMethodHints: Qt.ImhDigitsOnly
					validator: IntValidator { bottom: 0; top: 900 }
					onTextChanged: {
						if (Number(text) !== depth) {
							segmentListModel.setProperty(rowIndex, "depth", Number(text));
							generatePlan();
						}
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 3
					sampleText: "999"
					text: duration.toString()
					// request a numeric keyboard on mobile for this integer-only field
					inputMethodHints: Qt.ImhDigitsOnly
					validator: IntValidator { bottom: 1; top: 999 }
					onTextChanged: {
						if (Number(text) !== duration) {
							segmentListModel.setProperty(rowIndex, "duration", Number(text));
							generatePlan();
						}
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateComboBox {
					Layout.fillWidth: true
					model: gasNumberModel
					currentIndex: gas
					onActivated: {
						segmentListModel.setProperty(rowIndex, "gas", currentIndex)
						generatePlan();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				SsrfTextField {
					Layout.preferredWidth: Kirigami.Units.gridUnit * 5
					sampleText: "00.00"
					text: cylinderListModel.get(gas) && cylinderListModel.get(gas).use === 1 ? (setpoint / 1000.0).toFixed(2) : ""
					// request a numeric keyboard on mobile; ImhFormattedNumbersOnly allows the decimal separator
					inputMethodHints: punctuationInputMethodHints(Qt.platform.os, Qt.ImhFormattedNumbersOnly)
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
							if (Math.round(Number(text) * 1000) !== setpoint) {
								segmentListModel.setProperty(rowIndex, "setpoint", Math.round(Number(text) * 1000));
								generatePlan();
							}
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
					onActivated: {
						segmentListModel.setProperty(rowIndex, "divemode", currentIndex === 1 ? 2 : currentIndex);
						generatePlan();
					}
					onActiveFocusChanged: segmentListView.interactive = !activeFocus
				}

				TemplateButton {
					text: "X"
					font.bold: true
					enabled: segmentListModel.count > 1
					onClicked: {
						segmentListModel.remove(rowIndex);
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

				// AI-generated (Claude)
				// Draw the dive profile. When a recreational dive breaches the
				// decompression ceiling, the profile is still shown, but filled in
				// red to flag the problem.
				var profileColor = exceedsNDL ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.highlightColor;
				var firstPoint = profileData[0];
				var startX = padding + (firstPoint.time * xScale);
				var startY = padding + (firstPoint.depth * yScale);
				var lastX = startX;

				// Build the filled area under the profile line.
				ctx.fillStyle = exceedsNDL ? Kirigami.Theme.negativeBackgroundColor : Kirigami.Theme.highlightColor;
				ctx.globalAlpha = exceedsNDL ? 1.0 : 0.25;
				ctx.beginPath();
				ctx.moveTo(startX, padding);
				ctx.lineTo(startX, startY);
				for (var j = 1; j < profileData.length; j++) {
					var point = profileData[j];
					var x = padding + (point.time * xScale);
					var y = padding + (point.depth * yScale);
					ctx.lineTo(x, y);
					lastX = x;
				}
				ctx.lineTo(lastX, padding);
				ctx.closePath();
				ctx.fill();
				ctx.globalAlpha = 1.0;

				// Draw the profile line on top.
				ctx.strokeStyle = profileColor;
				ctx.lineWidth = 2;
				ctx.beginPath();
				ctx.moveTo(startX, startY);
				for (var k = 1; k < profileData.length; k++) {
					var linePoint = profileData[k];
					var lx = padding + (linePoint.time * xScale);
					var ly = padding + (linePoint.depth * yScale);
					ctx.lineTo(lx, ly);
				}
				ctx.stroke();
			}
		}

		// AI-generated (Claude)
		Kirigami.InlineMessage {
			Layout.fillWidth: true
			visible: exceedsNDL
			type: Kirigami.MessageType.Error
			text: qsTr("This dive exceeds the no-decompression limit for recreational mode. See the dive plan summary below for details.")
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
			text: qsTr("Save plan")
			font.bold: true
			Layout.fillWidth: true
			enabled: !exceedsNDL
			onClicked: {
				generatePlan(true);
			}
		}
	}
	Item {
		parent: divePlannerEditWindow
		z: 999
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		anchors.right: parent.right
		height: Kirigami.Units.gridUnit * 3 + Kirigami.Units.smallSpacing * 2
		Row {
			anchors.centerIn: parent
			spacing: Kirigami.Units.gridUnit
			SsrfToolButton {
				iconSource: "qrc:/icons/undo.svg"
				onClicked: pageStack.pop()
			}
			SsrfToolButton {
				iconSource: "qrc:/icons/media-playlist-repeat.svg"
				highlighted: true
				onClicked: generatePlan()
			}
			SsrfToolButton {
				iconSource: "qrc:/icons/ic_settings.svg"
				onClicked: showPage(divePlannerSetupWindow)
			}
		}
	}
}
