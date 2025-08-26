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
	id: divePlannerSetupWindow

	property string speedUnit: (Backend.length === Enums.METERS) ? qsTr("m/min") : qsTr("ft/min")
	property string volumeUnit: (Backend.volume === Enums.LITER) ? qsTr("L/min") : qsTr("cuft/min")
	property string pressureUnit: (Backend.pressure === Enums.BAR) ? qsTr("bar") : qsTr("psi")
	property string depthUnit: (Backend.length === Enums.METERS) ? qsTr("m") : qsTr("ft")

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
					text: qsTr("below 75% avg. depth [%1]").arg(speedUnit)
				}
				TemplateSpinBox {
					id: spinAscrate75
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascrate75
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.ascrate75 = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("75% to 50% avg. depth [%1]").arg(speedUnit)
				}
				TemplateSpinBox {
					id: spinAscrate50
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascrate50
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.ascrate50 = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("50% avg. depth to 6m [%1]").arg(speedUnit)
				}
				TemplateSpinBox {
					id: spinAscratestops
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascratestops
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.ascratestops = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("6m to surface [%1]").arg(speedUnit)
				}
				TemplateSpinBox {
					id: spinAscratelast6m
					from: 1
					to: 99
					stepSize: 1
					value: Backend.ascratelast6m
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.ascratelast6m = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					Layout.columnSpan: 2
					text: qsTr("Descent")
					font.bold: true
				}
				TemplateLabel {
					text: qsTr("Surface to the bottom [%1]").arg(speedUnit)
				}
				TemplateSpinBox {
					id: spinDescrate
					from: 1
					to: 99
					stepSize: 1
					value: Backend.descrate
					enabled: Backend.drop_stone_mode
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.descrate = value
						rootItem.settingsChanged()
					}
				}
				TemplateCheckBox {
					text: qsTr("Drop to first depth")
					checked: Backend.drop_stone_mode
					onClicked: {
						Backend.drop_stone_mode = checked
						rootItem.settingsChanged()
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
				//Divemode moved to plan edit
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
						rootItem.settingsChanged()
					}
				}

				TemplateLabel {
					text: qsTr("Reserve gas [%1]").arg(pressureUnit)
					leftPadding: Kirigami.Units.smallSpacing * 2
					enabled: Backend.planner_deco_mode === Enums.RECREATIONAL
				}
				TemplateSpinBox {
					from: 1
					to: 99
					stepSize: 1
					value: Backend.reserve_gas
					onValueModified: {
						Backend.reserve_gas = value
						rootItem.settingsChanged()
					}
				}

				TemplateCheckBox {
					text: qsTr("Safety stop")
					Layout.columnSpan: 2
					leftPadding: Kirigami.Units.smallSpacing * 6
					checked: Backend.safetystop
					onClicked: {
						Backend.safetystop = checked
						rootItem.settingsChanged()
					}
				}

				TemplateRadioButton {
					text: qsTr("Bühlmann deco")
					Layout.columnSpan: 2
					checked: Backend.planner_deco_mode === Enums.BUEHLMANN
					onClicked: {
						Backend.planner_deco_mode = Enums.BUEHLMANN
						rootItem.settingsChanged()
					}
				}

				TemplateLabel {
					text: qsTr("GFLow [%]")
					leftPadding: Kirigami.Units.smallSpacing * 2
				}
				TemplateSpinBox {
					from: 1
					to: 150
					stepSize: 1
					value: Backend.planner_gflow || 30
					onValueModified: {
						Backend.planner_gflow = value
						rootItem.settingsChanged()
					}
				}

				TemplateLabel {
					text: qsTr("GFHigh [%]")
					leftPadding: Kirigami.Units.smallSpacing * 2
				}
				TemplateSpinBox {
					from: 1
					to: 150
					stepSize: 1
					value: Backend.planner_gfhigh || 70
					onValueModified: {
						Backend.planner_gfhigh = value
						rootItem.settingsChanged()
					}
				}

				TemplateRadioButton {
					text: qsTr("VPM-B deco")
					Layout.columnSpan: 2
					checked: Backend.planner_deco_mode === Enums.VPMB
					onClicked: {
						Backend.planner_deco_mode = Enums.VPMB
						rootItem.settingsChanged()
					}
				}

				TemplateLabel {
					text: qsTr("Conservatism level [+]")
					leftPadding: 20
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Backend.vpmb_conservatism
					onValueModified: {
						Backend.vpmb_conservatism = value
						rootItem.settingsChanged()
					}
				}

				TemplateCheckBox {
					text: qsTr("Last stop at 20'/6m")
					Layout.columnSpan: 2
					onClicked: {
						Backend.last_stop6m = checked
						rootItem.settingsChanged()
					}
				}

				TemplateCheckBox {
					text: qsTr("Plan backgas breaks")
					Layout.columnSpan: 2
					checked: Backend.doo2breaks
					onClicked: {
						Backend.doo2breaks = checked
					}
				}

				TemplateCheckBox {
					text: qsTr("Only switch at required stops")
					Layout.columnSpan: 2
					checked: Backend.switch_at_req_stop
					onClicked: {
						Backend.switch_at_req_stop = checked
						rootItem.settingsChanged()
					}
				}

				TemplateLabel {
					text: qsTr("Min switch time [min]")
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Backend.min_switch_duration
					onValueModified: {
						Backend.min_switch_duration = value
						rootItem.settingsChanged()
					}
				}

				TemplateLabel {
					text: qsTr("Surface segment [min]")
				}
				TemplateSpinBox {
					from: 0
					to: 4
					stepSize: 1
					value: Backend.surface_segment
					onValueModified: {
						Backend.surface_segment = value
						rootItem.settingsChanged()
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
					text: qsTr("Bottom SAC [%1]").arg(volumeUnit)
				}
				TemplateSpinBox {
					id: spinBottomsac
					from: 1
					to:  (Backend.volume === Enums.LITER) ? 85 : 300
					stepSize: 1
					value: Backend.bottomsac
					textFromValue: function (value) {
						return (Backend.volume === Enums.LITER) ?
								   value.toString() : (value / 100).toFixed(2)
					}
					valueFromText: function(text) {
						return (Backend.volume === Enums.LITER) ? parseInt(text) : Math.round(parseFloat(text) * 100);
					}
					onValueModified: {
						Backend.bottomsac = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("Deco SAC [%1]").arg(volumeUnit)
				}
				TemplateSpinBox {
					id: spinDecosac
					from: 1
					to: (Backend.volume === Enums.LITER) ? 85 : 300
					stepSize: 1
					value: Backend.decosac
					textFromValue: function (value) {
						return (Backend.volume === Enums.LITER) ?
								   value.toString() : (value / 100).toFixed(2)
					}
					valueFromText: function(text) {
						return (Backend.volume === Enums.LITER) ? parseInt(text) : Math.round(parseFloat(text) * 100);
					}
					onValueModified: {
						Backend.decosac = value
						rootItem.settingsChanged()
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
					textFromValue: function (value) {
						return (value / 10).toFixed(1)
					}
					valueFromText: function(text) {
						return Math.round(parseFloat(text) * 10);
					}
					onValueModified: {
						Backend.sacfactor = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("Problem solving time [min]")
				}
				TemplateSpinBox {
					from: 1
					to: 9
					stepSize: 1
					value: Backend.problemsolvingtime
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.problemsolvingtime = value
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("Bottom pO₂ [%1]").arg(pressureUnit)
				}
				TemplateSpinBox {
					from: 0
					to: 250
					stepSize: 1
					value: Backend.bottompo2
					textFromValue: function (value) {
						return (value / 100).toFixed(2)
					}
					valueFromText: function (text) {
						return Math.round(parseFloat(text) * 100)
					}
					onValueModified: {
						Backend.bottompo2 = value * .01
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("Deco pO₂ [%1]").arg(pressureUnit)
				}
				TemplateSpinBox {
					from: 0
					to: 250
					stepSize: 1
					value: Backend.decopo2
					textFromValue: function (value) {
						return (value / 100).toFixed(2)
					}
					valueFromText: function (text) {
						return Math.round(parseFloat(text) * 100)
					}
					onValueModified: {
						Backend.decopo2 = value * .01
						rootItem.settingsChanged()
					}
				}
				TemplateLabel {
					text: qsTr("Best mix END [%1]").arg(depthUnit)
				}
				TemplateSpinBox {
					id: spinBestmixend
					from: 1
					to: 99
					stepSize: 1
					value: Backend.bestmixend
					textFromValue: function (value) { return value.toString() }
					valueFromText: function(text) { return parseInt(text) }
					onValueModified: {
						Backend.bestmixend = value
						rootItem.settingsChanged()
					}
				}
				TemplateCheckBox {
					text: qsTr("O2 narcotic")
					checked: Backend.o2narcotic
					onClicked: {
						Backend.o2narcotic = checked
						rootItem.settingsChanged()
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
						rootItem.settingsChanged()
					}
				}
				TemplateCheckBox {
					text: qsTr("Display segment duration")
					checked: Backend.display_duration
					onClicked: {
						Backend.display_duration = checked
						rootItem.settingsChanged()
					}
				}
				TemplateCheckBox {
					text: qsTr("Display transitions in deco")
					checked: Backend.display_transitions
					onClicked: {
						Backend.display_transitions = checked
						rootItem.settingsChanged()
					}
				}
				TemplateCheckBox {
					text: qsTr("Verbatim dive plan")
					checked: Backend.verbatim_plan
					onClicked: {
						Backend.verbatim_plan = checked
						rootItem.settingsChanged()
					}
				}
				TemplateCheckBox {
					text: qsTr("Display plan variations")
					checked: Backend.display_variations
					onClicked: {
						Backend.display_variations = checked
						rootItem.settingsChanged()
					}
				}
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
