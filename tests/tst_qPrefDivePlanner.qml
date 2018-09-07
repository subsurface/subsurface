// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefDivePlanner"

	function test_variables() {

		var x1 = PrefDivePlanner.ascratelast6m
		PrefDivePlanner.ascratelast6m = 11
		compare(PrefDivePlanner.ascratelast6m, 11)

		var x2 = PrefDivePlanner.ascratestops
		PrefDivePlanner.ascratestops = 11
		compare(PrefDivePlanner.ascratestops, 11)

		var x3 = PrefDivePlanner.ascrate50
		PrefDivePlanner.ascrate50 = 12
		compare(PrefDivePlanner.ascrate50, 12)

		var x4 = PrefDivePlanner.ascrate75
		PrefDivePlanner.ascrate75 = 13
		compare(PrefDivePlanner.ascrate75, 13)

//TBD		var x5 = PrefDivePlanner.bestmixend
//TBD		PrefDivePlanner.bestmixend = 51
//TBD		compare(PrefDivePlanner.bestmixend, 51)

		var x6 = PrefDivePlanner.bottompo2
		PrefDivePlanner.bottompo2 = 14
		compare(PrefDivePlanner.bottompo2,14)

		var x7 = PrefDivePlanner.bottomsac
		PrefDivePlanner.bottomsac = 15
		compare(PrefDivePlanner.bottomsac, 15)

		var x8 = PrefDivePlanner.decopo2
		PrefDivePlanner.decopo2 = 16
		compare(PrefDivePlanner.decopo2, 16)

		var x9 = PrefDivePlanner.decosac
		PrefDivePlanner.decosac = 17
		compare(PrefDivePlanner.decosac, 17)

		var x10 = PrefDivePlanner.descrate
		PrefDivePlanner.descrate = 18
		compare(PrefDivePlanner.descrate, 18)

		var x11 = PrefDivePlanner.display_duration
		PrefDivePlanner.display_duration = true
		compare(PrefDivePlanner.display_duration, true)

		var x12 = PrefDivePlanner.display_runtime
		PrefDivePlanner.display_runtime = true
		compare(PrefDivePlanner.display_runtime, true)

		var x13 = PrefDivePlanner.display_transitions
		PrefDivePlanner.display_transitions = true
		compare(PrefDivePlanner.display_transitions, true)

		var x14 = PrefDivePlanner.display_variations
		PrefDivePlanner.display_variations = true
		compare(PrefDivePlanner.display_variations, true)

		var x15 = PrefDivePlanner.doo2breaks
		PrefDivePlanner.doo2breaks = true
		compare(PrefDivePlanner.doo2breaks, true)

		var x16 = PrefDivePlanner.drop_stone_mode
		PrefDivePlanner.drop_stone_mode = true
		compare(PrefDivePlanner.drop_stone_mode, true)

		var x17 = PrefDivePlanner.last_stop
		PrefDivePlanner.last_stop = true
		compare(PrefDivePlanner.last_stop, true)

		var x18 = PrefDivePlanner.min_switch_duration
		PrefDivePlanner.min_switch_duration = 19
		compare(PrefDivePlanner.min_switch_duration, 19)

//TBD		var x19 = PrefDivePlanner.planner_deco_mode
//TBD		PrefDivePlanner.planner_deco_mode = BUEHLMANN
//TBD		compare(PrefDivePlanner.planner_deco_mode, BUEHLMANN)

		var x20 = PrefDivePlanner.problemsolvingtime
		PrefDivePlanner.problemsolvingtime = 20
		compare(PrefDivePlanner.problemsolvingtime, 20)

		var x21 = PrefDivePlanner.reserve_gas
		PrefDivePlanner.reserve_gas = 21
		compare(PrefDivePlanner.reserve_gas, 21)

		var x22 = PrefDivePlanner.sacfactor
		PrefDivePlanner.sacfactor = 22
		compare(PrefDivePlanner.sacfactor, 22)

		var x23 = PrefDivePlanner.safetystop
		PrefDivePlanner.safetystop = true
		compare(PrefDivePlanner.safetystop, true)

		var x24 = PrefDivePlanner.switch_at_req_stop
		PrefDivePlanner.switch_at_req_stop = true
		compare(PrefDivePlanner.switch_at_req_stop, true)

		var x25 = PrefDivePlanner.verbatim_plan
		PrefDivePlanner.verbatim_plan = true
		compare(PrefDivePlanner.verbatim_plan, true)
	}
}
