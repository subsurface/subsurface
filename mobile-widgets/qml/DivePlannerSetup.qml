// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

TemplatePage {
	title: qsTr("Dive planner setup")

	property string speedUnit: (Backend.length === Enums.METERS) ? qsTr(" m/min") : qsTr(" ft/min")
	property string volumeUnit: (Backend.volume === Enums.LITER) ? qsTr(" l/min") : qsTr(" cuft/min")
	property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr(" BAR") : qsTr(" PSI")
	Connections {
		target: Backend
		onLengthChanged: {
			spinAscrate75.value = Backend.ascrate75
			spinAscrate50.value = Backend.ascrate50
			spinAscratestops.value = Backend.ascratestops
			spinAscratelast6m.value = Backend.ascratelast6m
			spinDescrate.value = Backend.descrate
			spinBestmixend.value = Backend.bestmixend
		}
		onVolumeChanged: {
			spinBottomsac.value = Backend.bottomsac
			spinDecosac.value = Backend.decosac
		}
		onPressureChanged: {
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

				TemplateLabel {
					text: qsTr("Dive mode")
				}
				TemplateComboBox {
					editable: false
					currentIndex: Backend.dive_mode
					model: ListModel {
						ListElement {text: qsTr("Open circuit")}
						ListElement {text: qsTr("CCR")}
						ListElement {text: qsTr("pSCR")}
					}
					onActivated:  {
						Backend.dive_mode = currentIndex
					}
				}
				TemplateCheckBox {
					text: qsTr("Bailout: Deco on OC")
					Layout.columnSpan: 2
					checked: Backend.dobailout
				}

				TemplateRadioButton {
					text: qsTr("Recreational mode")
					Layout.columnSpan: 2
					checked: Backend.planner_deco_mode === Enums.RECREATIONAL
					onClicked: {
						Backend.planner_deco_mode = Enums.RECREATIONAL
					}
				}

				TemplateLabel {
					text: qsTr("Reserve gas")
					leftPadding: Kirigami.Units.smallSpacing * 2
					enabled: Backend.planner_deco_mode === Enums.RECREATIONAL
				}
				TemplateSpinBox {
					from: 1
					to: 99
					stepSize: 1
					value: Backend.reserve_gas
					textFromValue: function (value, locale) {
						return value + volumeUnit
					}
					onValueModified: {
						Backend.reserve_gas = value
					}
				}

				TemplateCheckBox {
					text: qsTr("Safety stop")
					Layout.columnSpan: 2
					leftPadding: Kirigami.Units.smallSpacing * 6
					checked: Backend.safetystop
					onClicked: {
						Backend.safetystop = checked
					}
				}

				TemplateRadioButton {
					text: qsTr("Bühlmann deco")
					Layout.columnSpan: 2
					checked: Backend.planner_deco_mode === Enums.BUEHLMANN
					onClicked: {
						Backend.planner_deco_mode = Enums.BUEHLMANN
					}
				}

				TemplateLabel {
					text: qsTr("GFLow")
					leftPadding: Kirigami.Units.smallSpacing * 2
				}
				TemplateSpinBox {
					from: 10
					to: 150
					stepSize: 1
					value: Backend.planner_gflow
					textFromValue: function (value, locale) {
						return value + volumeUnit
					}
					onValueModified: {
						Backend.planner_gflow = value
					}
				}

				TemplateLabel {
					text: qsTr("GFHigh")
					leftPadding: Kirigami.Units.smallSpacing * 2
				}
				TemplateSpinBox {
					from: 10
					to: 150
					stepSize: 1
					value: Backend.planner_gfhigh
					textFromValue: function (value, locale) {
						return value + volumeUnit
					}
					onValueModified: {
						Backend.planner_gfhigh = value
					}
				}

				TemplateRadioButton {
					text: qsTr("VPM-B deco")
					Layout.columnSpan: 2
					checked: Backend.planner_deco_mode === Enums.VPMB
					onClicked: {
						Backend.planner_deco_mode = Enums.VPMB
					}
				}

				TemplateLabel {
					text: qsTr("Conservatism level")
					leftPadding: 20
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Backend.vpmb_conservatism
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						Backend.vpmb_conservatism = value
					}
				}

				TemplateCheckBox {
					text: qsTr("Last stop at ??")
					Layout.columnSpan: 2
				}

				TemplateCheckBox {
					text: qsTr("Plan backgas breaks")
					Layout.columnSpan: 2
					checked: Backend.last_stop6m
					onClicked: {
						Backend.last_stop6m = checked
					}
				}

				TemplateCheckBox {
					text: qsTr("Only switch at required stops")
					Layout.columnSpan: 2
					checked: Backend.switch_at_req_stop
					onClicked: {
						Backend.switch_at_req_stop = checked
					}
				}

				TemplateLabel {
					text: qsTr("Min switch time")
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Backend.min_switch_duration
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						Backend.min_switch_duration = value
					}
				}

				TemplateLabel {
					text: qsTr("Surface segment")
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Backend.surface_segment
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						Backend.surface_segment = value
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
					from: 10
					to: 99
					stepSize: 1
					value: Backend.sacfactor
					textFromValue: function (value, locale) {
						return (value / 10).toFixed(1)
					}
					onValueModified: {
						Backend.sacfactor = value
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
					to: 250
					stepSize: 1
					value: Backend.bottompo2
					textFromValue: function (value, locale) {
						return (value / 100).toFixed(2) + "bar"
					}
					onValueModified: {
						Backend.bottompo2 = value
					}
				}
				TemplateLabel {
					text: qsTr("Deco pO2")
				}
				TemplateSpinBox {
					from: 0
					to: 250
					stepSize: 1
					value: Backend.decopo2
					textFromValue: function (value, locale) {
						return (value / 100).toFixed(2) + "bar"
					}
					onValueModified: {
						Backend.decopo2 = value
					}
				}
				TemplateLabel {
					text: qsTr("Best mix END")
				}
				TemplateSpinBox {
					id: spinBestmixend
					from: 1
					to: 99
					stepSize: 1
					value: Backend.bestmixend
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Backend.bestmixend = value
					}
				}
				TemplateCheckBox {
					text: qsTr("O2 narcotic")
					checked: Backend.o2narcotic
					onClicked: {
						Backend.o2narcotic = checked
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
