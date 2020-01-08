// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	title: qsTr("Dive planner setup")

	property string speedUnit: (Backend.length === Enums.METERS) ? qsTr(" m/min") : qsTr(" ft/min")
	property string volumeUnit: (Backend.volume === Enums.LITER) ? qsTr(" l/min") : qsTr(" cuft/min")
	Connections {
		target: Backend
		onLengthChanged: {
			spinAscrate75.value = Backend.ascrate75
			spinAscrate50.value = Backend.ascrate50
			spinAscratestops.value = Backend.ascratestops
			spinAscratelast6m.value = Backend.ascratelast6m
			spinDescrate.value = Backend.descrate
		}
		onVolumeChanged: {
			spinBottomsac.value = Backend.bottomsac
			spinDecosac.value = Backend.decosac
		}
	}
	Column {
		width: parent.width
		spacing: 1
		Layout.margins: Kirigami.Units.gridUnit

		TemplateSection {
			id: rates
			title: qsTr("Rates")

			GridLayout {
				columns: 2
				rowSpacing: Kirigami.Units.smallSpacing * 2
				columnSpacing: Kirigami.Units.smallSpacing
				visible: rates.isExpanded

				TemplateLabel {
					Layout.columnSpan: 2
					text: qsTr("Ascent")
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("below 75% avg. depth")
				}
				TemplateSpinBox {
					id: spinAscrate75
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascrate75
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Backend.ascrate75 = value
					}
				}
				TemplateLabel {
					text: qsTr("75% to 50% avg. depth")
				}
				TemplateSpinBox {
					id: spinAscrate50
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascrate50
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Backend.ascrate50 = value
					}
				}
				TemplateLabel {
					text: qsTr("50% avg. depth to 6m")
				}
				TemplateSpinBox {
					id: spinAscratestops
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascratestops
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Backend.ascratestops = value
					}
				}
				TemplateLabel {
					text: qsTr("6m to surface")
				}
				TemplateSpinBox {
					id: spinAscratelast6m
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascratelast6m
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Backend.ascratelast6m = value
					}
				}
				TemplateLabel {
					Layout.columnSpan: 2
					text: qsTr("Descent")
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("Surface to the bottom")
				}
				TemplateSpinBox {
					id: spinDescrate
					from: 1
					to: 99
					stepSize: 1
					value: Backend.descrate
					enabled: !Backend.drop_stone_mode
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Backend.descrate = value
					}
				}
				TemplateCheckBox {
					text: qsTr("Drop to first depth")
					checked: Backend.drop_stone_mode
					onClicked: {
						Backend.drop_stone_mode = checked
					}
				}
			}
		}
		TemplateSection {
			id: planning
			title: qsTr("Planning")

			GridLayout {
				columns: 2
				rowSpacing: Kirigami.Units.smallSpacing * 2
				columnSpacing: Kirigami.Units.smallSpacing
				visible: planning.isExpanded

				// Only support "Open circuit"
				TemplateLabel {
					text: "WORK in progress"
				}

				TemplateRadioButton {
					text: qsTr("Recreational NO deco")
					Layout.columnSpan: 2
				}
				// Reserve gas is 50bar (PADI/SSI rules)
				TemplateRadioButton {
					text: qsTr("Bühlmann, GFLow/GFHigh:")
				}
				Row {
					spacing: 0

					TemplateSpinBox {
						width: planning.width / 2 -30
						from: 1
						to: 99
						stepSize: 1
						value: 15
						textFromValue: function (value, locale) {
							return value + qsTr(" %")
						}
						onValueModified: {
							console.log("got value: " + value)
						}
					}
					TemplateSpinBox {
						width: planning.width / 2 -30
						from: 1
						to: 99
						stepSize: 1
						value: 15
						textFromValue: function (value, locale) {
							return value + qsTr(" %")
						}
						onValueModified: {
							console.log("got value: " + value)
						}
					}
				}
				TemplateRadioButton {
					text: qsTr("VPM-B, Conservatism:")
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: 2
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						console.log("got value: " + value)
					}
				}
			}
		}
		TemplateSection {
			id: gasoptions
			title: qsTr("Gas options")

			GridLayout {
				columns: 2
				rowSpacing: Kirigami.Units.smallSpacing * 2
				columnSpacing: Kirigami.Units.smallSpacing
				visible: gasoptions.isExpanded

				TemplateLabel {
					text: qsTr("Bottom SAC")
				}
				TemplateSpinBox {
					id: spinBottomsac
					from: 1
					to:  (Backend.volume === Enums.LITER) ? 85 : 300
					stepSize: 1
					value: Backend.bottomsac
					textFromValue: function (value, locale) {
						return (Backend.volume === Enums.LITER) ?
									value + volumeUnit :
									(value / 100).toFixed(2) + volumeUnit
					}
					onValueModified: {
						Backend.bottomsac = value
					}
				}
				TemplateLabel {
					text: qsTr("Deco SAC")
				}
				TemplateSpinBox {
					id: spinDecosac
					from: 1
					to: (Backend.volume === Enums.LITER) ? 85 : 300
					stepSize: 1
					value: Backend.decosac
					textFromValue: function (value, locale) {
						return (Backend.volume === Enums.LITER) ?
									value + volumeUnit :
									(value / 100).toFixed(2) + volumeUnit
					}
					onValueModified: {
						Backend.decosac = value
					}
				}
				TemplateLabel {
					text: qsTr("SAC factor")
				}
				TemplateSpinBox {
					from: 20
					to: 99
					stepSize: 1
					value: Planner.sacfactor
					textFromValue: function (value, locale) {
						return (value / 10).toFixed(1)
					}
					onValueModified: {
						Planner.sacfactor = value
					}
				}
				TemplateLabel {
					text: qsTr("Problem solving time")
				}
				TemplateSpinBox {
					from: 1
					to: 9
					stepSize: 1
					value: Backend.problemsolvingtime
					textFromValue: function (value, locale) {
						return value + qsTr(" min")
					}
					onValueModified: {
						Backend.problemsolvingtime = value
					}
				}
				TemplateLabel {
					text: qsTr("Bottom pO2")
				}
				TemplateSpinBox {
					from: 0
					to: 200
					stepSize: 1
					value: Planner.bottompo2
					textFromValue: function (value, locale) {
						return (value / 100).toFixed(2) + "bar"
					}
					onValueModified: {
						Planner.bottompo2 = value
					}
				}
				TemplateLabel {
					text: qsTr("Deco pO2")
				}
				TemplateSpinBox {
					from: 0
					to: 200
					stepSize: 1
					value: Planner.decopo2
					textFromValue: function (value, locale) {
						return (value / 100).toFixed(2) + "bar"
					}
					onValueModified: {
						Planner.decopo2 = value
					}
				}
				TemplateLabel {
					text: qsTr("Best mix END")
				}
				TemplateSpinBox {
					from: 1
					to: 99
					stepSize: 1
					value: Planner.bestmixend
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Planner.bestmixend = value
					}
				}
				TemplateCheckBox {
					text: qsTr("O2 narcotic")
					checked: Planner.o2narcotic
					onClicked: {
						Planner.o2narcotic = checked
					}
				}
			}
		}
		TemplateSection {
			id: notes
			title: qsTr("Notes")

			ColumnLayout {
				visible: notes.isExpanded

				TemplateCheckBox {
					text: qsTr("Display runtime")
					checked: Backend.display_runtime
					onClicked: {
						Backend.display_runtime = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Display segment duration")
					checked: Backend.display_duration
					onClicked: {
						Backend.display_duration = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Display transitions in deco")
					checked: Backend.display_transitions
					onClicked: {
						Backend.display_transitions = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Verbatim dive plan")
					checked: Backend.verbatim_plan
					onClicked: {
						Backend.verbatim_plan = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Display plan variations")
					checked: Backend.display_variations
					onClicked: {
						Backend.display_variations = checked
					}
				}
			}
		}
	}
}
