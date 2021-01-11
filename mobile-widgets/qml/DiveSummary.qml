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
	property int headerColumnWidth: Math.floor((width - Kirigami.Units.gridUnit) / 3)

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
			ListElement {text: qsTr("All")}
			ListElement {text: qsTr("1 month")}
			ListElement {text: qsTr("3 months")}
			ListElement {text: qsTr("6 months")}
			ListElement {text: qsTr("1 year")}
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
			text: qsTr("Refresh")
			implicitWidth: headerColumnWidth - Kirigami.Units.largeSpacing
			onClicked: reload()
		}
		TemplateSlimComboBox {
			id: selectionPrimary
			Layout.maximumWidth: headerColumnWidth - Kirigami.Units.largeSpacing
			Layout.preferredWidth: headerColumnWidth - Kirigami.Units.largeSpacing
			currentIndex: 0
			model: monthModel
			font.pointSize: subsurfaceTheme.smallPointSize
			onActivated:  {
				summaryModel.calc(0, currentIndex)
			}
		}
		TemplateSlimComboBox {
			id: selectionSecondary
			Layout.maximumWidth: headerColumnWidth - Kirigami.Units.largeSpacing
			Layout.preferredWidth: headerColumnWidth - Kirigami.Units.largeSpacing
			currentIndex: 3
			model: monthModel
			font.pointSize: subsurfaceTheme.smallPointSize
			onActivated:  {
				summaryModel.calc(1, currentIndex)
			}
		}

		Component {
			id: rowDelegate
			Row {
				Layout.leftMargin: 0
				height: headerLabel.height + Kirigami.Units.largeSpacing
				Rectangle {
					width: Kirigami.Units.gridUnit
					height: parent.height
					color: "transparent"
				}
				Rectangle {
					color: index & 1 ? subsurfaceTheme.backgroundColor : subsurfaceTheme.lightPrimaryColor
					width: headerColumnWidth + Kirigami.Units.gridUnit
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
					width: headerColumnWidth - 1.5 * Kirigami.Units.gridUnit
					height: headerLabel.height + Kirigami.Units.largeSpacing
					Label {
						color: subsurfaceTheme.textColor
						anchors.verticalCenter: parent.verticalCenter
						text: col0 !== undefined ? col0 : ""
					}
				}
				Rectangle {
					color: index & 1 ? subsurfaceTheme.backgroundColor : subsurfaceTheme.lightPrimaryColor
					width: headerColumnWidth - 1.5 * Kirigami.Units.gridUnit
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
			Column {
				Rectangle {
					width: headerColumnWidth * 3 - Kirigami.Units.gridUnit * 2
					height: Kirigami.Units.largeSpacing
					color: subsurfaceTheme.backgroundColor
				}
				Rectangle {
					width: headerColumnWidth * 3 - Kirigami.Units.gridUnit * 2
					height: sectionLabel.height + Kirigami.Units.largeSpacing
					color: subsurfaceTheme.backgroundColor
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
		}
	}
}
