// GasCalculator.qml
// SPDX-License-Identifier: GPL-2.0
import QtQuick 6.0
import QtQuick.Controls 2.12 as Controls
import QtQuick.Layouts 1.12
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

TemplatePage {
	id: gasCalculatorPage
	title: qsTr("Gas Calculator")

	ListModel {
		id: resultsModel
	}

	ColumnLayout {
		id: mainLayout 
		width: parent.width
		Layout.margins: Kirigami.Units.gridUnit
		spacing: Kirigami.Units.largeSpacing

		GridLayout {
			id: inputsGrid
			columns: 2
			Layout.fillWidth: true
			
			//TemplateLabel { text: qsTr("Cylinder Type") }
			TemplateComboBox {
				id: typeBox
				visible: false //Not used yet, but will still be left in for future use
				Layout.fillWidth: true
				model: manager.cylinderListInit
				currentIndex: model.indexOf("AL80")
			}

			TemplateLabel { text: qsTr("Oxygen %") }
			TemplateSpinBox {
				id: o2Box
				Layout.fillWidth: true
				from: 0
				to: 100
				stepSize: 1
				value: 21
				onActiveFocusChanged: gasCalculatorPage.interactive = !activeFocus
			}

			TemplateLabel { text: qsTr("Helium %") }
			TemplateSpinBox {
				id: heBox
				Layout.fillWidth: true
				from: 0
				to: 100
				stepSize: 1
				value: 0
				onActiveFocusChanged: gasCalculatorPage.interactive = !activeFocus
			}
		}
		
		TemplateButton {
			text: qsTr("Calculate")
			Layout.alignment: Qt.AlignHCenter
			onClicked: {
				var cylinderType = typeBox.currentText;
				if (heBox.value + o2Box.value > 100) {
					heBox.value = 100 - o2Box.value;
				}
				var o2_permille = o2Box.value * 10;
				var he_permille = heBox.value * 10;
				
				var results = Backend.divePlannerPointsModel.calculateGasInfo(cylinderType, o2_permille, he_permille);
				resultsModel.clear();
				for (var i = 0; i < results.length; i++) {
					resultsModel.append(results[i]);
				}
			}
		}

		// This ColumnLayout holds our header and the repeater-generated rows.
		ColumnLayout {
			Layout.fillWidth: true
			spacing: Kirigami.Units.smallSpacing

			// 1. Header Row (as a single GridLayout)
			GridLayout {
				Layout.fillWidth: true
				columns: 3

				TemplateLabel {
					text: qsTr("pO₂")
					font.bold: true
					horizontalAlignment: Text.AlignHCenter
					// Assign 25% of the width to this column
					Layout.preferredWidth: mainLayout.width * 0.25
				}
				TemplateLabel {
					text: qsTr("MOD")
					font.bold: true
					horizontalAlignment: Text.AlignHCenter
					// Assign 35% of the width to this column
					Layout.preferredWidth: mainLayout.width * 0.35
				}
				TemplateLabel {
					text: Backend.o2narcotic ? qsTr("END @ MOD") : qsTr("EAD @ MOD")
					font.bold: true
					horizontalAlignment: Text.AlignHCenter
					// Assign 40% of the width to this column
					Layout.preferredWidth: mainLayout.width * 0.40
				}
			}

			// 2. Data Rows created by a Repeater
			Repeater {
				id: resultsRepeater
				model: resultsModel
				
				delegate: GridLayout {
					Layout.fillWidth: true
					columns: 3

					TemplateLabel {
						text: po2
						horizontalAlignment: Text.AlignHCenter
						// Use the SAME proportion as the header
						Layout.preferredWidth: mainLayout.width * 0.25
					}
					TemplateLabel {
						text: mod
						horizontalAlignment: Text.AlignHCenter
						// Use the SAME proportion as the header
						Layout.preferredWidth: mainLayout.width * 0.35
					}
					TemplateLabel {
						text: ead
						horizontalAlignment: Text.AlignHCenter
						// Use the SAME proportion as the header
						Layout.preferredWidth: mainLayout.width * 0.40
					}
				}
			}
		}
	}
	
	actions: [
		Kirigami.Action {
			icon {
				name: state = ":/icons/undo.svg"
				color: subsurfaceTheme.primaryColor
			}
			text: "Return"
			onTriggered: {
				pageStack.pop()
			}
		}
	]
}
