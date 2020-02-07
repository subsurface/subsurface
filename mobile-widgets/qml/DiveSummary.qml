// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	id: summary
	DiveSummaryModel { id: summaryModel }
	property string firstDive: ""
	property string lastDive: ""
	property int headerColumnWidth: Math.floor(width / 3)

	background: Rectangle { color: subsurfaceTheme.backgroundColor }
	title: qsTr("Dive summary")

	function reload() {
		summaryModel.setNumData(2)
		summaryModel.calc(0, selectionPrimary.currentIndex)
		summaryModel.calc(1, selectionSecondary.currentIndex)
		firstDive = Backend.firstDiveDate()
		lastDive = Backend.lastDiveDate()
	}

	onVisibleChanged: {
		if (visible) {
			reload()
		}
	}
	Connections {
		target: Backend
		onLengthChanged: {
			reload()
		}
		onVolumeChanged: {
			reload()
		}
	}

	GridLayout {
		columns: 3
		width: parent.width
		columnSpacing: Kirigami.Units.smallSpacing
		rowSpacing: Kirigami.Units.smallSpacing

		ListModel {
			id: monthModel
			ListElement {text: qsTr("Total")}
			ListElement {text: qsTr(" 1 month [ 30 days]")}
			ListElement {text: qsTr(" 2 month [ 60 days]")}
			ListElement {text: qsTr(" 3 month [ 90 days]")}
			ListElement {text: qsTr(" 4 month [120 days]")}
			ListElement {text: qsTr(" 5 month [150 days]")}
			ListElement {text: qsTr(" 6 month [180 days]")}
			ListElement {text: qsTr(" 7 month [210 days]")}
			ListElement {text: qsTr(" 8 month [240 days]")}
			ListElement {text: qsTr(" 9 month [270 days]")}
			ListElement {text: qsTr("10 month [300 days]")}
			ListElement {text: qsTr("11 month [330 days]")}
			ListElement {text: qsTr("12 month [360 days]")}
		}

		TemplateLabel {
			text: qsTr("last dive:")
			font.bold: true
			width: headerColumnWidth
		}
		TemplateLabel {
			Layout.alignment: Qt.AlignCenter
			text: summary.lastDive
		}
		TemplateLabel { /* just filling the third column with nothing */ }
		TemplateLabel {
			text: qsTr("first dive:")
			font.bold: true
			width: headerColumnWidth
		}
		TemplateLabel {
			Layout.alignment: Qt.AlignCenter
			text: summary.firstDive
		}
		TemplateLabel { /* just filling the third column with nothing */ }

		TemplateLabel {
			Layout.columnSpan: 3
		}

		TemplateButton {
			/* Replace by signals from the core in due course. */
			text: "Refresh"
			width: headerColumnWidth
			onClicked: reload()
		}
		TemplateComboBox {
			id: selectionPrimary
			editable: false
			currentIndex: 0
			model: monthModel
			onActivated:  {
				summaryModel.calc(0, currentIndex)
			}
		}
		TemplateComboBox {
			id: selectionSecondary
			editable: false
			currentIndex: 3
			model: monthModel
			onActivated:  {
				summaryModel.calc(1, currentIndex)
			}
		}

		Component {
			id: rowDelegate
			Row {
				height: headerLabel.height + Kirigami.Units.largeSpacing
				Rectangle {
					width: Kirigami.Units.gridUnit * 2
					height: parent.height
					color: "transparent"
				}
				Rectangle {
					color: index & 1 ? subsurfaceTheme.backgroundColor : subsurfaceTheme.lightPrimaryColor
					width: headerColumnWidth
					height: headerLabel.height + Kirigami.Units.largeSpacing
					Label {
						id: headerLabel
						color: subsurfaceTheme.textColor
						anchors.verticalCenter: parent.verticalCenter
						leftPadding: Kirigami.Units.largeSpacing
						text: header !== undefined ? header : ""
						font.bold: true
					}
				}
				Rectangle {
					color: index & 1 ? subsurfaceTheme.backgroundColor : subsurfaceTheme.lightPrimaryColor
					width: headerColumnWidth - 2 * Kirigami.Units.gridUnit
					height: headerLabel.height + Kirigami.Units.largeSpacing
					Label {
						color: subsurfaceTheme.textColor
						anchors.verticalCenter: parent.verticalCenter
						text: col0 !== undefined ? col0 : ""
					}
				}
				Rectangle {
					color: index & 1 ? subsurfaceTheme.backgroundColor : subsurfaceTheme.lightPrimaryColor
					width: headerColumnWidth - 2 * Kirigami.Units.gridUnit
					height: headerLabel.height + Kirigami.Units.largeSpacing
					Label {
						color: subsurfaceTheme.textColor
						anchors.verticalCenter: parent.verticalCenter
						text: col1 !== undefined ? col1 : ""
					}
				}
			}
		}

		Component {
			id: sectionDelegate
			Rectangle {
				width: headerColumnWidth * 3 - Kirigami.Units.gridUnit * 2
				height: sectionLabel.height + Kirigami.Units.largeSpacing
				Label {
					id: sectionLabel
					anchors.verticalCenter: parent.verticalCenter
					leftPadding: Kirigami.Units.largeSpacing
					color: subsurfaceTheme.textColor
					text: section
					font.bold: true
				}
			}
		}
		ListView {
			id: resultsTable
			model: summaryModel
			Layout.columnSpan: 3
			width: summary.width
			height: summaryModel.rowCount() * 2 * Kirigami.Units.gridUnit
			delegate: rowDelegate
			section.property: "section"
			section.delegate: sectionDelegate
			Component.onCompleted: {
				manager.appendTextToLog("SUMMARY: width: " + width + " height: " + height + " rows: " + model.rowCount())
			}
			onModelChanged: {
				manager.appendTextToLog("SUMMARY model changed; now width: " + width + " height: " + height + " rows: " + model.rowCount())
			}
		}
	}
}
