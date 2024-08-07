// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefTechnicalDetails"

	function test_variables() {
		var x01 = PrefTechnicalDetails.calcalltissues
		PrefTechnicalDetails.calcalltissues = true
		compare(PrefTechnicalDetails.calcalltissues, true)

		var x02 = PrefTechnicalDetails.calcceiling
		PrefTechnicalDetails.calcceiling = true
		compare(PrefTechnicalDetails.calcceiling, true)

		var x03 = PrefTechnicalDetails.calcceiling3m
		PrefTechnicalDetails.calcceiling3m = true
		compare(PrefTechnicalDetails.calcceiling3m, true)

		var x04 = PrefTechnicalDetails.calcndltts
		PrefTechnicalDetails.calcndltts = true
		compare(PrefTechnicalDetails.calcndltts, true)

		var x05 = PrefTechnicalDetails.dcceiling
		PrefTechnicalDetails.dcceiling = true
		compare(PrefTechnicalDetails.dcceiling, true)

//TBD	var x06 = PrefTechnicalDetails.display_deco_mode
//TBD	PrefTechnicalDetails.display_deco_mode = BUEHLMANN
//TBD	compare(PrefTechnicalDetails.display_deco_mode, BUEHLMANN)

		var x07 = PrefTechnicalDetails.include_unused_tanks
		PrefTechnicalDetails.include_unused_tanks = true
		compare(PrefTechnicalDetails.include_unused_tanks, true)

		var x08 = PrefTechnicalDetails.ead
		PrefTechnicalDetails.ead = true
		compare(PrefTechnicalDetails.ead, true)

		var x09 = PrefTechnicalDetails.gfhigh
		PrefTechnicalDetails.gfhigh = 27
		compare(PrefTechnicalDetails.gfhigh, 27)

		var x10 = PrefTechnicalDetails.gflow
		PrefTechnicalDetails.gflow = 25
		compare(PrefTechnicalDetails.gflow, 25)

		var x11 = PrefTechnicalDetails.gf_low_at_maxdepth
		PrefTechnicalDetails.gf_low_at_maxdepth = true
		compare(PrefTechnicalDetails.gf_low_at_maxdepth, true)

		var x12 = PrefTechnicalDetails.hrgraph
		PrefTechnicalDetails.hrgraph = true
		compare(PrefTechnicalDetails.hrgraph, true)

		var x13 = PrefTechnicalDetails.mod
		PrefTechnicalDetails.mod = true
		compare(PrefTechnicalDetails.mod, true)

		var x14 = PrefTechnicalDetails.modpO2 = 1.02;
		PrefTechnicalDetails.modpO2 = 1.02
		compare(PrefTechnicalDetails.modpO2, 1.02)

		var x15 = PrefTechnicalDetails.percentagegraph
		PrefTechnicalDetails.percentagegraph = true
		compare(PrefTechnicalDetails.percentagegraph, true)

		var x16 = PrefTechnicalDetails.redceiling
		PrefTechnicalDetails.redceiling = true
		compare(PrefTechnicalDetails.redceiling, true)

		var x17 = PrefTechnicalDetails.rulergraph
		PrefTechnicalDetails.rulergraph = true
		compare(PrefTechnicalDetails.rulergraph, true)

		var x18 = PrefTechnicalDetails.show_average_depth
		PrefTechnicalDetails.show_average_depth = true
		compare(PrefTechnicalDetails.show_average_depth, true)

		var x19 = PrefTechnicalDetails.show_ccr_sensors
		PrefTechnicalDetails.show_ccr_sensors = true
		compare(PrefTechnicalDetails.show_ccr_sensors, true)

		var x20 = PrefTechnicalDetails.show_ccr_setpoint
		PrefTechnicalDetails.show_ccr_setpoint = true
		compare(PrefTechnicalDetails.show_ccr_setpoint, true)

		var x21 = PrefTechnicalDetails.show_icd
		PrefTechnicalDetails.show_icd = true
		compare(PrefTechnicalDetails.show_icd, true)

		var x22 = PrefTechnicalDetails.show_pictures_in_profile
		PrefTechnicalDetails.show_pictures_in_profile = true
		compare(PrefTechnicalDetails.show_pictures_in_profile, true)

		var x23 = PrefTechnicalDetails.show_sac
		PrefTechnicalDetails.show_sac = true
		compare(PrefTechnicalDetails.show_sac, true)

		var x24 = PrefTechnicalDetails.show_scr_ocpo2
		PrefTechnicalDetails.show_scr_ocpo2 = true
		compare(PrefTechnicalDetails.show_scr_ocpo2, true)

		var x25 = PrefTechnicalDetails.tankbar
		PrefTechnicalDetails.tankbar = true
		compare(PrefTechnicalDetails.tankbar, true)

		var x26 = PrefTechnicalDetails.vpmb_conservatism
		PrefTechnicalDetails.vpmb_conservatism = 127
		compare(PrefTechnicalDetails.vpmb_conservatism, 127)

		var x27 = PrefTechnicalDetails.zoomed_plot
		PrefTechnicalDetails.zoomed_plot = true
		compare(PrefTechnicalDetails.zoomed_plot, true)

		var x28 = PrefTechnicalDetails.allowOcGasAsDiluent
		PrefTechnicalDetails.allowOcGasAsDiluent = true
		compare(PrefTechnicalDetails.allowOcGasAsDiluent, true)

	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false
		property bool spy4 : false
		property bool spy5 : false
		property bool spy7 : false
		property bool spy8 : false
		property bool spy9 : false
		property bool spy10 : false
		property bool spy11 : false
		property bool spy12 : false
		property bool spy13 : false
		property bool spy14 : false
		property bool spy15 : false
		property bool spy16 : false
		property bool spy17 : false
		property bool spy18 : false
		property bool spy19 : false
		property bool spy20 : false
		property bool spy21 : false
		property bool spy22 : false
		property bool spy23 : false
		property bool spy24 : false
		property bool spy25 : false
		property bool spy26 : false
		property bool spy27 : false

		Connections {
			target: PrefTechnicalDetails
			onCalcalltissuesChanged: {spyCatcher.spy1 = true }
			onCalcceilingChanged: {spyCatcher.spy2 = true }
			onCalcceiling3mChanged: {spyCatcher.spy3 = true }
			onCalcndlttsChanged: {spyCatcher.spy4 = true }
			onDcceilingChanged: {spyCatcher.spy5 = true }
			onDisplay_unused_tanksChanged: {spyCatcher.spy7 = true }
			onEadChanged: {spyCatcher.spy8 = true }
			onGfhighChanged: {spyCatcher.spy9 = true }
			onGflowChanged: {spyCatcher.spy10 = true }
			onGf_low_at_maxdepthChanged: {spyCatcher.spy11 = true }
			onHrgraphChanged: {spyCatcher.spy12 = true }
			onModChanged: {spyCatcher.spy13 = true }
			onModpO2Changed: {spyCatcher.spy14 = true }
			onPercentagegraphChanged: {spyCatcher.spy15 = true }
			onRedceilingChanged: {spyCatcher.spy16 = true }
			onRulergraphChanged: {spyCatcher.spy17 = true }
			onShow_average_depthChanged: {spyCatcher.spy18 = true }
			onShow_ccr_sensorsChanged: {spyCatcher.spy19 = true }
			onShow_ccr_setpointChanged: {spyCatcher.spy20 = true }
			onShow_icdChanged: {spyCatcher.spy21 = true }
			onShow_pictures_in_profileChanged: {spyCatcher.spy22 = true }
			onShow_sacChanged: {spyCatcher.spy23 = true }
			onShow_scr_ocpo2Changed: {spyCatcher.spy24 = true }
			onTankbarChanged: {spyCatcher.spy25 = true }
			onVpmb_conservatismChanged: {spyCatcher.spy26 = true }
			onZoomed_plotChanged: {spyCatcher.spy27 = true }
		}
	}

	function test_signals() {
		PrefTechnicalDetails.calcalltissues = ! PrefTechnicalDetails.calcalltissues
		PrefTechnicalDetails.calcceiling = ! PrefTechnicalDetails.calcceiling
		PrefTechnicalDetails.calcceiling3m = ! PrefTechnicalDetails.calcceiling3m
		PrefTechnicalDetails.calcndltts = ! PrefTechnicalDetails.calcndltts
		PrefTechnicalDetails.dcceiling = ! PrefTechnicalDetails.dcceiling
		// 6 does not emit signal
		PrefTechnicalDetails.include_unused_tanks = ! PrefTechnicalDetails.include_unused_tanks
		PrefTechnicalDetails.ead = ! PrefTechnicalDetails.ead
		PrefTechnicalDetails.gfhigh = -27
		PrefTechnicalDetails.gflow = -25
		PrefTechnicalDetails.gf_low_at_maxdepth = ! PrefTechnicalDetails.gf_low_at_maxdepth
		PrefTechnicalDetails.hrgraph = ! PrefTechnicalDetails.hrgraph
		PrefTechnicalDetails.mod = ! PrefTechnicalDetails.mod
		PrefTechnicalDetails.modpO2 = -1.02
		PrefTechnicalDetails.percentagegraph = ! PrefTechnicalDetails.percentagegraph
		PrefTechnicalDetails.redceiling = ! PrefTechnicalDetails.redceiling
		PrefTechnicalDetails.rulergraph = ! PrefTechnicalDetails.rulergraph
		PrefTechnicalDetails.show_average_depth = ! PrefTechnicalDetails.show_average_depth
		PrefTechnicalDetails.show_ccr_sensors = ! PrefTechnicalDetails.show_ccr_sensors
		PrefTechnicalDetails.show_ccr_setpoint = ! PrefTechnicalDetails.show_ccr_setpoint
		PrefTechnicalDetails.show_icd = ! PrefTechnicalDetails.show_icd
		PrefTechnicalDetails.show_pictures_in_profile = ! PrefTechnicalDetails.show_pictures_in_profile
		PrefTechnicalDetails.show_sac = ! PrefTechnicalDetails.show_sac
		PrefTechnicalDetails.show_scr_ocpo2 = ! PrefTechnicalDetails.show_scr_ocpo2
		PrefTechnicalDetails.tankbar = ! PrefTechnicalDetails.tankbar
		PrefTechnicalDetails.vpmb_conservatism = -127
		PrefTechnicalDetails.zoomed_plot = ! PrefTechnicalDetails.zoomed_plot
		PrefTechnicalDetails.allowOcGasAsDiluent = ! PrefTechnicalDetails.allowOcGasAsDiluent

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy7, true)
		compare(spyCatcher.spy8, true)
		compare(spyCatcher.spy9, true)
		compare(spyCatcher.spy10, true)
		compare(spyCatcher.spy11, true)
		compare(spyCatcher.spy12, true)
		compare(spyCatcher.spy13, true)
		compare(spyCatcher.spy14, true)
		compare(spyCatcher.spy15, true)
		compare(spyCatcher.spy16, true)
		compare(spyCatcher.spy17, true)
		compare(spyCatcher.spy18, true)
		compare(spyCatcher.spy19, true)
		compare(spyCatcher.spy20, true)
		compare(spyCatcher.spy21, true)
		compare(spyCatcher.spy22, true)
		compare(spyCatcher.spy23, true)
		compare(spyCatcher.spy24, true)
		compare(spyCatcher.spy25, true)
		compare(spyCatcher.spy26, true)
		compare(spyCatcher.spy27, true)
	}
}
