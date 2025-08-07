// GasCalculator.qml
// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12 as Controls
import QtQuick.Controls 1.4 // Import for TableView
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
				var o2_permille = o2Box.value * 10;
				var he_permille = heBox.value * 10;
				
				var results = Backend.divePlannerPointsModel.calculateGasInfo(cylinderType, o2_permille, he_permille);
				resultsModel.clear();
				for (var i = 0; i < results.length; i++) {
					resultsModel.append(results[i]);
				}
			}
		}

		TableView {
			Layout.fillWidth: true
			height: rowHeight * (resultsModel.count + 1) 
			
			model: resultsModel

			TableViewColumn {
				role: "po2"
				title: qsTr("pO₂")
				width: gasCalculatorPage.width * 0.20
				horizontalAlignment: Text.AlignHCenter
			}
			TableViewColumn {
				role: "mod"
				title: qsTr("MOD")
				width: gasCalculatorPage.width * 0.25
				horizontalAlignment: Text.AlignHCenter
			}
			TableViewColumn {
				role: "ead"
				title: qsTr("EAD @ MOD")
				width: gasCalculatorPage.width * 0.25
				horizontalAlignment: Text.AlignHCenter
			}
		}
	}
		actions.left: Kirigami.Action {
		icon {
			name: state = ":/icons/undo.svg"
			color: subsurfaceTheme.primaryColor
		}
		text: "Return"
		onTriggered: {
			pageStack.pop()			
		}
	}
}