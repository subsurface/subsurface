// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.Page {
	id: statisticsPage
	objectName: "StatisticsPage"
	title: qsTr("Statistics")
	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0
	width: rootItem.width
	implicitWidth: rootItem.width
	property bool wide: width > height
	StatsManager {
		id: statsManager
	}
	onVisibleChanged: {
		if (visible)
			statsManager.doit()
	}

	GridLayout {
		anchors.fill: parent
		ColumnLayout {
			id: i1
			Layout.column: 0
			Layout.row: 0
			Layout.margins: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Base variable")
			}
			TemplateComboBox  {
				id: var1
				model: statsManager.var1List
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var1Changed(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i2
			Layout.column: wide ? 0 : 1
			Layout.row: wide ? 1 : 0
			Layout.margins: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Binning")
			}
			TemplateComboBox {
				id: var1Binner
				model: statsManager.binner1List
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var1BinnerChanged(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i3
			Layout.column: wide ? 0 : 2
			Layout.row: wide ? 2 : 0
			Layout.margins: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Data")
			}
			TemplateComboBox {
				id: var2
				model: statsManager.var2List
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var2Changed(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i4
			Layout.column: wide ? 0 : 3
			Layout.row: wide ? 3 : 0
			Layout.margins: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Binning")
			}
			TemplateComboBox {
				id: var2Binner
				model: statsManager.binner2List
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var2BinnerChanged(currentIndex)
				}
			}
		}
		Item {
			Layout.column: wide ? 0 : 4
			Layout.row: wide ? 4 : 0
			Layout.preferredHeight: wide ? parent.height - Kirigami.Units.gridUnit * 16 : Kirigami.Units.gridUnit
			Layout.preferredWidth: wide ? parent.width - i1.implicitWidt - i2.implicitWidt - i3.implicitWidt - i4.implicitWidth : Kirigami.Units.gridUnit
			// just used for spacing
		}

		StatsView {
			Layout.row: wide ? 0 : 1
			Layout.column: wide ? 1 : 0
			Layout.rowSpan: wide ? 5 : 1
			Layout.columnSpan: wide ? 1 : 5
			id: statsView
			Layout.fillWidth: true
			Layout.fillHeight: true
		}
	}
	Component.onCompleted: {
		statsManager.init(statsView, var1)
		console.log("Statistics widget loaded")
	}
}
