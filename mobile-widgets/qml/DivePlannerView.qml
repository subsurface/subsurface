// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.12
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	id: divePlannerViewWindow
	title: qsTr("Dive Plan Preview")

	property var cylindersData: []
	property var segmentsData: []
	property string planDate: ""
	property string planTime: ""
	property int diveMode: 0

	property string planNotes: ""
	property string maxDepth: ""
	property string duration: ""
	property var profileData: []

	onVisibleChanged: {
		if (visible && cylindersData.length > 0) {
			var planResult = Backend.divePlannerPointsModel.calculatePlan(
				cylindersData,
				segmentsData,
				planDate,
				planTime,
				diveMode,
				false 
			)

			// Populate the display properties with the results
			planNotes = planResult.notes
			maxDepth = planResult.maxDepth
			duration = planResult.duration
			profileData = planResult.profile
		}
	}

	onProfileDataChanged: {
		profileCanvas.requestPaint();
	}

	ColumnLayout {
		width: parent.width
		Layout.margins: Kirigami.Units.gridUnit
		spacing: Kirigami.Units.largeSpacing

		Canvas {
			id: profileCanvas
			Layout.fillWidth: true
			Layout.preferredHeight: Kirigami.Units.gridUnit * 20

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
	}
	actions.main: Kirigami.Action {
		icon {
			name: state = ":/icons/document-save.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: "Replan"
		onTriggered: {
			var planResult = Backend.divePlannerPointsModel.calculatePlan(
				cylindersData, segmentsData, planDate, planTime,
				diveMode, true // shouldSave is true
			)
			var newDiveId = planResult.newDiveId
			if (newDiveId !== -1) {
				manager.selectDive(newDiveId)
				showPage(diveList)
			}
		
			
		}
	}
	actions.left: Kirigami.Action {
		icon {
			name: state = ":/icons/undo.svg" 
			color: subsurfaceTheme.primaryColor
		}
		text: "Replan"
		onTriggered: {
			console.log("replan button triggered")
			pageStack.pop()
		}
	}
}