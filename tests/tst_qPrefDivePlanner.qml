// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefDivePlanner"

	SsrfDivePlannerPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {

		var x1 = tst.ascratelast6m
		tst.ascratelast6m = 11
		compare(tst.ascratelast6m, 11)

		var x2 = tst.ascratestops
		tst.ascratestops = 11
		compare(tst.ascratestops, 11)

		var x3 = tst.ascrate50
		tst.ascrate50 = 12
		compare(tst.ascrate50, 12)

		var x4 = tst.ascrate75
		tst.ascrate75 = 13
		compare(tst.ascrate75, 13)

//TBD		var x5 = tst.bestmixend
//TBD		tst.bestmixend = 51
//TBD		compare(tst.bestmixend, 51)

		var x6 = tst.bottompo2
		tst.bottompo2 = 14
		compare(tst.bottompo2,14)

		var x7 = tst.bottomsac
		tst.bottomsac = 15
		compare(tst.bottomsac, 15)

		var x8 = tst.decopo2
		tst.decopo2 = 16
		compare(tst.decopo2, 16)

		var x9 = tst.decosac
		tst.decosac = 17
		compare(tst.decosac, 17)

		var x10 = tst.descrate
		tst.descrate = 18
		compare(tst.descrate, 18)

		var x11 = tst.display_duration
		tst.display_duration = true
		compare(tst.display_duration, true)

		var x12 = tst.display_runtime
		tst.display_runtime = true
		compare(tst.display_runtime, true)

		var x13 = tst.display_transitions
		tst.display_transitions = true
		compare(tst.display_transitions, true)

		var x14 = tst.display_variations
		tst.display_variations = true
		compare(tst.display_variations, true)

		var x15 = tst.doo2breaks
		tst.doo2breaks = true
		compare(tst.doo2breaks, true)

		var x16 = tst.drop_stone_mode
		tst.drop_stone_mode = true
		compare(tst.drop_stone_mode, true)

		var x17 = tst.last_stop
		tst.last_stop = true
		compare(tst.last_stop, true)

		var x18 = tst.min_switch_duration
		tst.min_switch_duration = 19
		compare(tst.min_switch_duration, 19)

//TBD		var x19 = tst.planner_deco_mode
//TBD		tst.planner_deco_mode = BUEHLMANN
//TBD		compare(tst.planner_deco_mode, BUEHLMANN)

		var x20 = tst.problemsolvingtime
		tst.problemsolvingtime = 20
		compare(tst.problemsolvingtime, 20)

		var x21 = tst.reserve_gas
		tst.reserve_gas = 21
		compare(tst.reserve_gas, 21)

		var x22 = tst.sacfactor
		tst.sacfactor = 22
		compare(tst.sacfactor, 22)

		var x23 = tst.safetystop
		tst.safetystop = true
		compare(tst.safetystop, true)

		var x24 = tst.switch_at_req_stop
		tst.switch_at_req_stop = true
		compare(tst.switch_at_req_stop, true)

		var x25 = tst.verbatim_plan
		tst.verbatim_plan = true
		compare(tst.verbatim_plan, true)
	}
}
