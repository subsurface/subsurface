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
	background: Rectangle { color: subsurfaceTheme.backgroundColor }

	property string speedUnit: (PrefUnits.length === Enums.METERS) ? qsTr(" m/min") : qsTr(" ft/min")
	property string volumeUnit: (PrefUnits.volume === Enums.LITER) ? qsTr(" l/min") : qsTr(" cuft/min")
	property string pressureUnit: (PrefUnits.pressure === Enums.BAR) ? qsTr(" bar") : qsTr(" psi")
	Connections {
		target: PrefUnits
		onLengthChanged: {
			spinAscrate75.value = Planner.ascrate75
			spinAscrate50.value = Planner.ascrate50
			spinAscratestops.value = Planner.ascratestops
			spinAscratelast6m.value = Planner.ascratelast6m
			spinDescrate.value = Planner.descrate
			spinBestmixend.value = Planner.bestmixend
		}
		onVolumeChanged: {
			spinBottomsac.value = Planner.bottomsac * ((PrefUnits.volume === Enums.LITER) ? 1 : 100)
			spinDecosac.value = Planner.decosac * ((PrefUnits.volume === Enums.LITER) ? 1 : 100)
		}
		onPressureChanged: {
			spinReserveGas.value = Planner.reserve_gas
		}
	}
	Column {
		width: parent.width
		spacing: 1
		Layout.margins: 10

		TemplateSection {
			id: rates
			title: qsTr("Rates")
			width: parent.width

			GridLayout {
				columns: 2
				rowSpacing: 10
				columnSpacing: 20
				visible: rates.isExpanded
				width: parent.width

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
					value: Planner.ascrate75
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Planner.ascrate75 = value
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
					value: Planner.ascrate50
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Planner.ascrate50 = value
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
					value: Planner.ascratestops
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Planner.ascratestops = value
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
					value: Planner.ascratelast6m
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Planner.ascratelast6m = value
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
					value: Planner.descrate
					enabled: !Planner.drop_stone_mode
					textFromValue: function (value, locale) {
						return value + speedUnit
					}
					onValueModified: {
						Planner.descrate = value
					}
				}
				TemplateCheckBox {
					text: qsTr("Drop to first depth")
					checked: Planner.drop_stone_mode
					onClicked: {
						Planner.drop_stone_mode = checked
					}
				}

			}
		}
		TemplateSection {
			id: planning
			title: qsTr("Planning")

			GridLayout {
				columns: 2
				rowSpacing: 10
				columnSpacing: 20
				visible: planning.isExpanded

				TemplateLabel {
					text: qsTr("Dive mode")
				}
				TemplateComboBox {
					editable: false
					currentIndex: Planner.dive_mode === Enums.OC ? 0 :
								  Planner.dive_mode === Enums.CCR ? 1 : 2
					model: ListModel {
						ListElement {text: qsTr("Open circuit")}
						ListElement {text: qsTr("CCR")}
						ListElement {text: qsTr("pSCR")}
					}
					onActivated:  {
						Planner.dive_mode = currentIndex === 0 ? Enums.OC :
											currentIndex === 1 ? Enums.CCR : Enums.PSCR
					}
				}

				TemplateCheckBox {
					text: qsTr("Bailout: Deco on OC")
					enabled: Planner.dive_mode !== Enums.OC
					checked: Planner.dobailout
					Layout.columnSpan: 2
					onClicked: {
						Planner.dobailout = checked
					}
				}

				TemplateRadioButton {
					text: qsTr("Recreational mode")
					checked: Planner.planner_deco_mode === Enums.RECREATIONAL
					Layout.columnSpan: 2
					onClicked: {
						Planner.planner_deco_mode = Enums.RECREATIONAL
					}
				}

				TemplateLabel {
					text: qsTr("Reserve gas")
					enabled: Planner.planner_deco_mode === Enums.RECREATIONAL
					leftPadding: 20
				}
				TemplateSpinBox {
					id: spinReserveGas
					enabled: Planner.planner_deco_mode === Enums.RECREATIONAL
					from: 1
					to:  (PrefUnits.pressure === Enums.BAR) ? 300 : 5000
					stepSize: 1
					value: Planner.reserve_gas
					textFromValue: function (value, locale) {
						return value + pressureUnit
					}
					onValueModified: {
						Planner.reserve_gas = value
					}
				}

				TemplateCheckBox {
					text: qsTr("Safety stop")
					enabled: Planner.planner_deco_mode === Enums.RECREATIONAL
					checked: Planner.safetystop
					Layout.columnSpan: 2
					leftPadding: 60
					onClicked: {
						Planner.safetystop = checked
					}
				}

				TemplateRadioButton {
					text: qsTr("Bühlmannh deco")
					Layout.columnSpan: 2
					checked: Planner.planner_deco_mode === Enums.BUEHLMANN
					onClicked: {
						Planner.planner_deco_mode = Enums.BUEHLMANN
					}
				}

				TemplateLabel {
					text: qsTr("GFLow")
					leftPadding: 20
					enabled: Planner.planner_deco_mode === Enums.BUEHLMANN
				}
				TemplateSpinBox {
					from: 1
					to: 99
					stepSize: 1
					value: Planner.gflow
					enabled: Planner.planner_deco_mode === Enums.BUEHLMANN
					textFromValue: function (value, locale) {
						return value + " %"
					}
					onValueModified: {
						Planner.gflow = value
					}
				}

				TemplateLabel {
					text: qsTr("GFHigh")
					leftPadding: 20
					enabled: Planner.planner_deco_mode == Enums.BUEHLMANN
				}
				TemplateSpinBox {
					from: 1
					to: 99
					stepSize: 1
					value: Planner.gfhigh
					enabled: Planner.planner_deco_mode == Enums.BUEHLMANN
					textFromValue: function (value, locale) {
						return value + " %"
					}
					onValueModified: {
						Planner.gfhigh = value
					}
				}

				TemplateRadioButton {
					text: qsTr("VPM-B deco")
					Layout.columnSpan: 2
					checked: Planner.planner_deco_mode === Enums.VPMB
					onClicked: {
						Planner.planner_deco_mode = Enums.VPMB
					}
				}

				TemplateLabel {
					text: qsTr("Conservatism level")
					leftPadding: 20
					enabled: Planner.planner_deco_mode === Enums.VPMB
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Planner.vpmb_conservatism
					enabled: Planner.planner_deco_mode === Enums.VPMB
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						Planner.vpmb_conservatism = value
					}
				}

				TemplateCheckBox {
					text: qsTr("Last stop at") + (PrefUnits.length === Enums.METERS ? " 6m" : " 20ft")
					Layout.columnSpan: 2
					enabled: Planner.planner_deco_mode === Enums.RECREATIONAL
					checked: Planner.last_stop
					onClicked: {
						Planner.last_stop = checked
					}
				}

				TemplateCheckBox {
					text: qsTr("Plan backgas breaks")
					Layout.columnSpan: 2
					enabled: Planner.planner_deco_mode !== Enums.RECREATIONAL && Planner.last_stop
					checked: Planner.doo2breaks
					onClicked: {
						Planner.doo2breaks = checked
					}
				}

				TemplateCheckBox {
					text: qsTr("Only switch at required stops")
					Layout.columnSpan: 2
					enabled: Planner.planner_deco_mode !== Enums.RECREATIONAL
					checked: Planner.switch_at_req_stop
					onClicked: {
						Planner.switch_at_req_stop = checked
					}
				}

				TemplateLabel {
					text: qsTr("Min switch time")
					enabled: Planner.planner_deco_mode !== Enums.RECREATIONAL
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Planner.min_switch_duration
					enabled: Planner.planner_deco_mode !== Enums.RECREATIONAL
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						Planner.min_switch_duration = value
					}
				}

				TemplateLabel {
					text: qsTr("Surface segment")
					enabled: Planner.planner_deco_mode !== Enums.RECREATIONAL
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Planner.surface_segment
					enabled: Planner.planner_deco_mode !== Enums.RECREATIONAL
					textFromValue: function (value, locale) {
						return qsTr("+") + value
					}
					onValueModified: {
						Planner.surface_segment = value
					}
				}
			}
		}
		TemplateSection {
			id: gasoptions
			title: qsTr("Gas options")

			GridLayout {
				columns: 2
				rowSpacing: 10
				columnSpacing: 20
				visible: gasoptions.isExpanded

				TemplateLabel {
					text: qsTr("Bottom SAC")
				}
				TemplateSpinBox {
					id: spinBottomsac
					from: 1
					to:  (PrefUnits.volume === Enums.LITER) ? 60 : 300
					stepSize: 1
					value: Planner.bottomsac * ((PrefUnits.volume === Enums.LITER) ? 1 : 100)
					textFromValue: function (value, locale) {
						return (PrefUnits.volume === Enums.LITER) ?
									value + volumeUnit :
									(value / 100).toFixed(2) + volumeUnit
					}
					onValueModified: {
						Planner.bottomsac = (PrefUnits.volume === Enums.LITER) ?
									value : value / 100
					}
				}
				TemplateLabel {
					text: qsTr("Deco SAC")
				}
				TemplateSpinBox {
					id: spinDecosac
					from: 1
					to: (PrefUnits.volume === Enums.LITER) ? 60 : 300
					stepSize: 1
					value: Planner.decosac * ((PrefUnits.volume === Enums.LITER) ? 1 : 100)
					textFromValue: function (value, locale) {
						return (PrefUnits.volume === Enums.LITER) ?
									value + volumeUnit :
									(value / 100).toFixed(2) + volumeUnit
					}
					onValueModified: {
						Planner.decosac = (PrefUnits.volume === Enums.LITER) ?
									value : value / 100
					}
				}
				TemplateLabel {
					text: qsTr("SAC factor")
				}
				TemplateSpinBox {
					from: 1
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
					value: Planner.problemsolvingtime
					textFromValue: function (value, locale) {
						return value + qsTr(" min")
					}
					onValueModified: {
						Planner.problemsolvingtime = value
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
					id: spinBestmixend
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
					checked: Planner.display_runtime
					onClicked: {
						Planner.display_runtime = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Display segment duration")
					checked: Planner.display_duration
					onClicked: {
						Planner.display_duration = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Display transitions in deco")
					checked: Planner.display_transitions
					onClicked: {
						Planner.display_transitions = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Verbatim dive plan")
					checked: Planner.verbatim_plan
					onClicked: {
						Planner.verbatim_plan = checked
					}
				}
				TemplateCheckBox {
					text: qsTr("Display plan variations")
					checked: Planner.display_variations
					onClicked: {
						Planner.display_variations = checked
					}
				}
			}
		}
	}
}
