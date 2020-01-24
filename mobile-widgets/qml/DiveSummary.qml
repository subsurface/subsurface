// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
	title: qsTr("Dive summary")

	ListModel {
		id: monthModel
		ListElement {text: qsTr("Total")}
		ListElement {text: qsTr(" 1 month")}
		ListElement {text: qsTr(" 2 month")}
		ListElement {text: qsTr(" 3 month")}
		ListElement {text: qsTr(" 4 month")}
		ListElement {text: qsTr(" 5 month")}
		ListElement {text: qsTr(" 6 month")}
		ListElement {text: qsTr(" 7 month")}
		ListElement {text: qsTr(" 8 month")}
		ListElement {text: qsTr(" 9 month")}
		ListElement {text: qsTr("10 month")}
		ListElement {text: qsTr("11 month")}
		ListElement {text: qsTr("12 month")}
	}

	onVisibleChanged: {
		if (visible)
			Backend.summaryCalculation(selectionPrimary.currentIndex, selectionSecondary.currentIndex)
	}

	GridLayout {
		columns: 3
		width: parent.width
		columnSpacing: Kirigami.Units.smallSpacing
		rowSpacing: Kirigami.Units.smallSpacing

		TemplateLabel {
			text: qsTr("oldest/newest dive")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[0]
		}
		TemplateLabel {
			text: Backend.resultCalculation[1]
		}

		TemplateLabel {
			Layout.columnSpan: 3
		}

		TemplateLabel {
			text: ""
		}
		TemplateComboBox {
			id: selectionPrimary
			editable: false
			currentIndex: 0
			model: monthModel
			onActivated:  {
				Backend.summaryCalculation(selectionPrimary.currentIndex, selectionSecondary.currentIndex)
			}
		}
		TemplateComboBox {
			id: selectionSecondary
			editable: false
			currentIndex: 3
			model: monthModel
			onActivated:  {
				Backend.summaryCalculation(selectionPrimary.currentIndex, selectionSecondary.currentIndex)
			}
		}
		TemplateLabel {
			text: qsTr("dives")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[2]
		}
		TemplateLabel {
			text: Backend.resultCalculation[3]
		}
		TemplateLabel {
			text: qsTr("- EANx")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[4]
		}
		TemplateLabel {
			text: Backend.resultCalculation[5]
		}
		TemplateLabel {
			text: qsTr("- depth > 39m")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[6]
		}
		TemplateLabel {
			text: Backend.resultCalculation[7]
		}

		TemplateLabel {
			Layout.columnSpan: 3
		}

		TemplateLabel {
			text: qsTr("dive time")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[8]
		}
		TemplateLabel {
			text: Backend.resultCalculation[9]
		}
		TemplateLabel {
			text: qsTr("max dive time")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[10]
		}
		TemplateLabel {
			text: Backend.resultCalculation[11]
		}
		TemplateLabel {
			text: qsTr("avg. dive time")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[12]
		}
		TemplateLabel {
			text: Backend.resultCalculation[13]
		}

		TemplateLabel {
			Layout.columnSpan: 3
		}

		TemplateLabel {
			text: qsTr("max depth")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[14]
		}
		TemplateLabel {
			text: Backend.resultCalculation[15]
		}
		TemplateLabel {
			text: qsTr("avg. max depth")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[16]
		}
		TemplateLabel {
			text: Backend.resultCalculation[17]
		}

		TemplateLabel {
			Layout.columnSpan: 3
		}

		TemplateLabel {
			text: qsTr("min. SAC")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[18]
		}
		TemplateLabel {
			text: Backend.resultCalculation[19]
		}
		TemplateLabel {
			text: qsTr("avg. SAC")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[20]
		}
		TemplateLabel {
			text: Backend.resultCalculation[21]
		}

		TemplateLabel {
			Layout.columnSpan: 3
		}

		TemplateLabel {
			text: qsTr("dive plan(s)")
			font.bold: true
		}
		TemplateLabel {
			text: Backend.resultCalculation[22]
		}
		TemplateLabel {
			text: Backend.resultCalculation[23]
		}

	}
}
