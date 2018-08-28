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

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false
		property bool spy4 : false
		property bool spy6 : false
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

		Connections {
			target: PrefDivePlanner
			onAscratelast6mChanged: {spyCatcher.spy1 = true }
			onAscratestopsChanged: {spyCatcher.spy2 = true }
			onAscrate50Changed: {spyCatcher.spy3 = true }
			onAscrate75Changed: {spyCatcher.spy4 = true }
			onBottompo2Changed: {spyCatcher.spy6 = true }
			onBottomsacChanged: {spyCatcher.spy7 = true }
			onDecopo2Changed: {spyCatcher.spy8 = true }
			onDecosacChanged: {spyCatcher.spy9 = true }
			onDescrateChanged: {spyCatcher.spy10 = true }
			onDisplay_durationChanged: {spyCatcher.spy11 = true }
			onDisplay_runtimeChanged: {spyCatcher.spy12 = true }
			onDisplay_transitionsChanged: {spyCatcher.spy13 = true }
			onDisplay_variationsChanged: {spyCatcher.spy14 = true }
			onDoo2breaksChanged: {spyCatcher.spy15 = true }
			onDrop_stone_modeChanged: {spyCatcher.spy16 = true }
			onLast_stopChanged: {spyCatcher.spy17 = true }
			onMin_switch_durationChanged: {spyCatcher.spy18 = true }
			onProblemsolvingtimeChanged: {spyCatcher.spy20 = true }
			onReserve_gasChanged: {spyCatcher.spy21 = true }
			onSacfactorChanged: {spyCatcher.spy22 = true }
			onSafetystopChanged: {spyCatcher.spy23 = true }
			onSwitch_at_req_stopChanged: {spyCatcher.spy24 = true }
			onVerbatim_planChanged: {spyCatcher.spy25 = true }
		}
	}

	function test_signals() {
		PrefDivePlanner.ascratelast6m = -11
		PrefDivePlanner.ascratestops = -11
		PrefDivePlanner.ascrate50 = -12
		PrefDivePlanner.ascrate75 = -13
		// 5 not emitting signals
		PrefDivePlanner.bottompo2 = -14
		PrefDivePlanner.bottomsac = -15
		PrefDivePlanner.decopo2 = -16
		PrefDivePlanner.decosac = -17
		PrefDivePlanner.descrate = -18
		PrefDivePlanner.display_duration = ! PrefDivePlanner.display_duration
		PrefDivePlanner.display_runtime = ! PrefDivePlanner.display_runtime
		PrefDivePlanner.display_transitions = ! PrefDivePlanner.display_transitions
		PrefDivePlanner.display_variations = ! PrefDivePlanner.display_variations
		PrefDivePlanner.doo2breaks = ! PrefDivePlanner.doo2breaks
		PrefDivePlanner.drop_stone_mode = ! PrefDivePlanner.drop_stone_mode
		PrefDivePlanner.last_stop = ! PrefDivePlanner.last_stop
		PrefDivePlanner.min_switch_duration = -19
		// 19 not emitting signals
		PrefDivePlanner.problemsolvingtime = -20
		PrefDivePlanner.reserve_gas = -21
		PrefDivePlanner.sacfactor = -22
		PrefDivePlanner.safetystop = ! PrefDivePlanner.safetystop
		PrefDivePlanner.switch_at_req_stop = ! PrefDivePlanner.switch_at_req_stop
		PrefDivePlanner.verbatim_plan = ! PrefDivePlanner.verbatim_plan

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy6, true)
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
		compare(spyCatcher.spy20, true)
		compare(spyCatcher.spy21, true)
		compare(spyCatcher.spy22, true)
		compare(spyCatcher.spy23, true)
		compare(spyCatcher.spy24, true)
		compare(spyCatcher.spy25, true)
	}
}
