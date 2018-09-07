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

		var x07 = PrefTechnicalDetails.display_unused_tanks
		PrefTechnicalDetails.display_unused_tanks = true
		compare(PrefTechnicalDetails.display_unused_tanks, true)

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
	}
}
