// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefTechnicalDetails"

	SsrfTechnicalDetailsPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x01 = tst.calcalltissues
		tst.calcalltissues = true
		compare(tst.calcalltissues, true)

		var x02 = tst.calcceiling
		tst.calcceiling = true
		compare(tst.calcceiling, true)

		var x03 = tst.calcceiling3m
		tst.calcceiling3m = true
		compare(tst.calcceiling3m, true)

		var x04 = tst.calcndltts
		tst.calcndltts = true
		compare(tst.calcndltts, true)

		var x05 = tst.dcceiling
		tst.dcceiling = true
		compare(tst.dcceiling, true)

//TBD	var x06 = tst.display_deco_mode
//TBD	tst.display_deco_mode = BUEHLMANN
//TBD	compare(tst.display_deco_mode, BUEHLMANN)

		var x07 = tst.display_unused_tanks
		tst.display_unused_tanks = true
		compare(tst.display_unused_tanks, true)

		var x08 = tst.ead
		tst.ead = true
		compare(tst.ead, true)

		var x09 = tst.gfhigh
		tst.gfhigh = 27
		compare(tst.gfhigh, 27)

		var x10 = tst.gflow
		tst.gflow = 25
		compare(tst.gflow, 25)

		var x11 = tst.gf_low_at_maxdepth
		tst.gf_low_at_maxdepth = true
		compare(tst.gf_low_at_maxdepth, true)

		var x12 = tst.hrgraph
		tst.hrgraph = true
		compare(tst.hrgraph, true)

		var x13 = tst.mod
		tst.mod = true
		compare(tst.mod, true)

		var x14 = tst.modpO2 = 1.02;
		tst.modpO2 = 1.02
		compare(tst.modpO2, 1.02)

		var x15 = tst.percentagegraph
		tst.percentagegraph = true
		compare(tst.percentagegraph, true)

		var x16 = tst.redceiling
		tst.redceiling = true
		compare(tst.redceiling, true)

		var x17 = tst.rulergraph
		tst.rulergraph = true
		compare(tst.rulergraph, true)

		var x18 = tst.show_average_depth
		tst.show_average_depth = true
		compare(tst.show_average_depth, true)

		var x19 = tst.show_ccr_sensors
		tst.show_ccr_sensors = true
		compare(tst.show_ccr_sensors, true)

		var x20 = tst.show_ccr_setpoint
		tst.show_ccr_setpoint = true
		compare(tst.show_ccr_setpoint, true)

		var x21 = tst.show_icd
		tst.show_icd = true
		compare(tst.show_icd, true)

		var x22 = tst.show_pictures_in_profile
		tst.show_pictures_in_profile = true
		compare(tst.show_pictures_in_profile, true)

		var x23 = tst.show_sac
		tst.show_sac = true
		compare(tst.show_sac, true)

		var x24 = tst.show_scr_ocpo2
		tst.show_scr_ocpo2 = true
		compare(tst.show_scr_ocpo2, true)

		var x25 = tst.tankbar
		tst.tankbar = true
		compare(tst.tankbar, true)

		var x26 = tst.vpmb_conservatism
		tst.vpmb_conservatism = 127
		compare(tst.vpmb_conservatism, 127)

		var x27 = tst.zoomed_plot
		tst.zoomed_plot = true
		compare(tst.zoomed_plot, true)
	}
}
