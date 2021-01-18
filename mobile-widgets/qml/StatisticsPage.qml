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
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
	property bool wide: width > rootItem.height
	StatsManager {
		id: statsManager
	}
	ChartListModel {
		id: chartListModel
	}
	onVisibleChanged: {
		if (visible)
			statsManager.doit()
	}
	onWidthChanged: {
		if (visible) {
			statsManager.doit()
		}
	}
	onWideChanged: {
		// so this means we rotated the device - and sometimes after rotation
		// the stats widget is empty.
		rotationRedrawTrigger.start()
	}
	Timer {
		// wait .5 seconds (so the OS rotation animation has a chance to run) and then set var1 again
		// to its current value, which appears to be enough to ensure that the chart is drawn again
		id: rotationRedrawTrigger
		interval: 500
		onTriggered: statsManager.var1Changed(i1.var1currentIndex)
	}

	Component {
		id: chartListDelegate
		Kirigami.AbstractListItem {
			id: chartListDelegateItem
			height: isHeader ? 1 + 8 * Kirigami.Units.smallSpacing : 11 * Kirigami.Units.smallSpacing // delegateInnerItem.height
			onClicked: {
				if (!isHeader) {
					chartTypePopup.close()
					statsManager.setChart(id)
				}
			}
			Item {
				id: chartListDelegateInnerItem
				Row {
					height: childrenRect.height
					spacing: Kirigami.Units.smallSpacing
					Kirigami.Icon {
						id: chartIcon
						source: icon
						width: iconSize !== undefined ? iconSize.width : 0
					}
					Label {
						text: chartName
						font.bold: isHeader
						color: subsurfaceTheme.textColor
					}
				}
			}
		}
	}
	Popup {
		id: chartTypePopup
		x: Kirigami.Units.gridUnit * 2
		y: Kirigami.Units.gridUnit
		width: Math.min(Kirigami.Units.gridUnit * 12, statisticsPage.width * 0.6)
		height: Math.min(statisticsPage.height - 3 * Kirigami.Units.gridUnit, chartListModel.count * Kirigami.Units.gridUnit * 2.75)
		modal: true
		focus: true
		clip: true
		closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
		ListView {
			id: chartTypes
			model: chartListModel
			anchors.fill: parent
			delegate: chartListDelegate
		}
	}

	GridLayout {
		anchors.fill: parent
		ColumnLayout {
			id: i1
			Layout.column: 0
			Layout.row: 0
			Layout.leftMargin: Kirigami.Units.smallSpacing
			Layout.topMargin: Kirigami.Units.smallSpacing
			property alias var1currentIndex: var1.currentIndex
			TemplateLabelSmall {
				text: qsTr("Base variable")
			}
			TemplateSlimComboBox  {
				id: var1
				model: statsManager.var1List
				currentIndex: statsManager.var1Index;
				onCurrentIndexChanged: {
					statsManager.var1Changed(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i2
			Layout.column: wide ? 0 : 1
			Layout.row: wide ? 1 : 0
			Layout.leftMargin: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Binning")
			}
			TemplateSlimComboBox {
				id: var1Binner
				model: statsManager.binner1List
				currentIndex: statsManager.binner1Index;
				onCurrentIndexChanged: {
					statsManager.var1BinnerChanged(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i3
			Layout.column: wide ? 0 : 0
			Layout.row: wide ? 2 : 1
			Layout.leftMargin: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Data")
			}
			TemplateSlimComboBox {
				id: var2
				model: statsManager.var2List
				currentIndex: statsManager.var2Index;
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var2Changed(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i4
			Layout.column: wide ? 0 : 1
			Layout.row: wide ? 3 : 1
			Layout.leftMargin: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Binning")
			}
			TemplateSlimComboBox {
				id: var2Binner
				model: statsManager.binner2List
				currentIndex: statsManager.binner2Index;
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var2BinnerChanged(currentIndex)
				}
			}
		}
		ColumnLayout {
			id: i5
			Layout.column: wide ? 0 : 0
			Layout.row: wide ? 4 : 2
			Layout.leftMargin: Kirigami.Units.smallSpacing
			TemplateLabelSmall {
				text: qsTr("Operation")
			}
			TemplateSlimComboBox {
				id: var2Operation
				model: statsManager.operation2List
				currentIndex: statsManager.operation2Index;
				Layout.fillWidth: false
				onCurrentIndexChanged: {
					statsManager.var2OperationChanged(currentIndex)
				}
			}
		}
		Button {
			id: chartTypeButton
			Layout.column: wide ? 0 : 1
			Layout.row: wide ? 5 : 2
			Layout.leftMargin: Kirigami.Units.smallSpacing
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.preferredHeight: Kirigami.Units.gridUnit * 2
			Layout.preferredWidth: Kirigami.Units.gridUnit * 8
			background: Rectangle {
				color: chartTypeButton.pressed ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor
				antialiasing: true
				radius: Kirigami.Units.smallSpacing * 2
				height: Kirigami.Units.gridUnit * 2
			}
			contentItem: Text {
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignHCenter
				color: subsurfaceTheme.primaryTextColor
				text: qsTr("Chart type")
			}
			onClicked: chartTypePopup.open()
		}
		Item {
			Layout.column: wide ? 0 : 2
			Layout.row: wide ? 6 : 2
			Layout.preferredHeight: wide ? parent.height - Kirigami.Units.gridUnit * 16 : Kirigami.Units.gridUnit
			Layout.fillWidth: wide ? false : true
			// just used for spacing
		}
		StatsView {
			Layout.column: wide ? 1 : 0
			Layout.row: wide ? 0 : 3
			Layout.columnSpan: wide ? 1 : 3
			Layout.rowSpan: wide ? 7 : 1
			id: statsView
			Layout.margins: Kirigami.Units.smallSpacing
			Layout.fillWidth: true
			Layout.fillHeight: true
			Layout.maximumHeight: wide ? statisticsPage.height - 2 * Kirigami.Units.gridUnit :
						     statisticsPage.height - 2 * Kirigami.Units.gridUnit - i4.height
			Layout.maximumWidth: wide ? statisticsPage.width - 2 * Kirigami.Units.gridUnit - i4.width :
						     statisticsPage.width - 2 * Kirigami.Units.smallSpacing
		}
	}
	Component.onCompleted: {
		statsManager.init(statsView, chartListModel)
	}
}
