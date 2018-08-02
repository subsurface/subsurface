// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDivePlanner.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

void TestQPrefDivePlanner::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDivePlanner");
}

void TestQPrefDivePlanner::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefDivePlanner::instance();

	prefs.ascratelast6m = 10;
	prefs.ascratestops = 11;
	prefs.ascrate50 = 12;
	prefs.ascrate75 = 13;
	prefs.bestmixend.mm  = 11;
	prefs.bottompo2 = 14;
	prefs.bottomsac = 15;
	prefs.decopo2 = 16;
	prefs.decosac = 17;
	prefs.descrate = 18;
	prefs.display_duration = true;
	prefs.display_runtime = true;
	prefs.display_transitions = true;
	prefs.display_variations = true;
	prefs.doo2breaks = true;
	prefs.drop_stone_mode = true;
	prefs.last_stop = true;
	prefs.min_switch_duration = 19;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.problemsolvingtime = 20;
	prefs.reserve_gas = 21;
	prefs.sacfactor = 22;
	prefs.safetystop = true;
	prefs.switch_at_req_stop = true;
	prefs.verbatim_plan = true;

	QCOMPARE(tst->ascratelast6m(), prefs.ascratelast6m);
	QCOMPARE(tst->ascratestops(), prefs.ascratestops);
	QCOMPARE(tst->ascrate50(), prefs.ascrate50);
	QCOMPARE(tst->ascrate75(), prefs.ascrate75);
	QCOMPARE(tst->bestmixend().mm, prefs.bestmixend.mm);
	QCOMPARE(tst->bottompo2(), prefs.bottompo2);
	QCOMPARE(tst->bottomsac(), prefs.bottomsac);
	QCOMPARE(tst->decopo2(), prefs.decopo2);
	QCOMPARE(tst->decosac(), prefs.decosac);
	QCOMPARE(tst->descrate(), prefs.descrate);
	QCOMPARE(tst->display_duration(), prefs.display_duration);
	QCOMPARE(tst->display_runtime(), prefs.display_runtime);
	QCOMPARE(tst->display_transitions(), prefs.display_transitions);
	QCOMPARE(tst->display_variations(), prefs.display_variations);
	QCOMPARE(tst->doo2breaks(), prefs.doo2breaks);
	QCOMPARE(tst->drop_stone_mode(), prefs.drop_stone_mode);
	QCOMPARE(tst->last_stop(), prefs.last_stop);
	QCOMPARE(tst->min_switch_duration(), prefs.min_switch_duration);
	QCOMPARE(tst->planner_deco_mode(), prefs.planner_deco_mode);
	QCOMPARE(tst->problemsolvingtime(), prefs.problemsolvingtime);
	QCOMPARE(tst->reserve_gas(), prefs.reserve_gas);
	QCOMPARE(tst->sacfactor(), prefs.sacfactor);
	QCOMPARE(tst->safetystop(), prefs.safetystop);
	QCOMPARE(tst->switch_at_req_stop(), prefs.switch_at_req_stop);
	QCOMPARE(tst->verbatim_plan(), prefs.verbatim_plan);
}

void TestQPrefDivePlanner::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefDivePlanner::instance();

	tst->set_ascratelast6m(20);
	tst->set_ascratestops(21);
	tst->set_ascrate50(22);
	tst->set_ascrate75(23);
	depth_t x;
	x.mm = 21;
	tst->set_bestmixend(x);
	tst->set_bottompo2(24);
	tst->set_bottomsac(25);
	tst->set_decopo2(26);
	tst->set_decosac(27);
	tst->set_descrate(28);
	tst->set_display_duration(false);
	tst->set_display_runtime(false);
	tst->set_display_transitions(false);
	tst->set_display_variations(false);
	tst->set_doo2breaks(false);
	tst->set_drop_stone_mode(false);
	tst->set_last_stop(false);
	tst->set_min_switch_duration(29);
	tst->set_planner_deco_mode(VPMB);
	tst->set_problemsolvingtime(30);
	tst->set_reserve_gas(31);
	tst->set_sacfactor(32);
	tst->set_safetystop(false);
	tst->set_switch_at_req_stop(false);
	tst->set_verbatim_plan(false);

	QCOMPARE(prefs.ascratelast6m, 20);
	QCOMPARE(prefs.ascratestops, 21);
	QCOMPARE(prefs.ascrate50, 22);
	QCOMPARE(prefs.ascrate75, 23);
	QCOMPARE(prefs.bestmixend.mm , 21);
	QCOMPARE(prefs.bottompo2, 24);
	QCOMPARE(prefs.bottomsac, 25);
	QCOMPARE(prefs.decopo2, 26);
	QCOMPARE(prefs.decosac, 27);
	QCOMPARE(prefs.descrate, 28);
	QCOMPARE(prefs.display_duration, false);
	QCOMPARE(prefs.display_runtime, false);
	QCOMPARE(prefs.display_transitions, false);
	QCOMPARE(prefs.display_variations, false);
	QCOMPARE(prefs.doo2breaks, false);
	QCOMPARE(prefs.drop_stone_mode, false);
	QCOMPARE(prefs.last_stop, false);
	QCOMPARE(prefs.min_switch_duration, 29);
	QCOMPARE(prefs.planner_deco_mode, VPMB);
	QCOMPARE(prefs.problemsolvingtime, 30);
	QCOMPARE(prefs.reserve_gas, 31);
	QCOMPARE(prefs.sacfactor, 32);
	QCOMPARE(prefs.safetystop, false);
	QCOMPARE(prefs.switch_at_req_stop, false);
	QCOMPARE(prefs.verbatim_plan, false);
}

void TestQPrefDivePlanner::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefDivePlanner::instance();

	tst->set_ascratelast6m(20);
	tst->set_ascratestops(21);
	tst->set_ascrate50(22);
	tst->set_ascrate75(23);
	depth_t x;
	x.mm = 41;
	tst->set_bestmixend(x);
	tst->set_bottompo2(24);
	tst->set_bottomsac(25);
	tst->set_decopo2(26);
	tst->set_decosac(27);
	tst->set_descrate(28);
	tst->set_display_duration(false);
	tst->set_display_runtime(false);
	tst->set_display_transitions(false);
	tst->set_display_variations(false);
	tst->set_doo2breaks(false);
	tst->set_drop_stone_mode(false);
	tst->set_last_stop(false);
	tst->set_min_switch_duration(29);
	tst->set_planner_deco_mode(VPMB);
	tst->set_problemsolvingtime(30);
	tst->set_reserve_gas(31);
	tst->set_sacfactor(32);
	tst->set_safetystop(false);
	tst->set_switch_at_req_stop(false);
	tst->set_verbatim_plan(false);

	prefs.ascratelast6m = 10;
	prefs.ascratestops = 11;
	prefs.ascrate50 = 12;
	prefs.ascrate75 = 13;
	prefs.bestmixend.mm  = 11;
	prefs.bottompo2 = 14;
	prefs.bottomsac = 15;
	prefs.decopo2 = 16;
	prefs.decosac = 17;
	prefs.descrate = 18;
	prefs.display_duration = true;
	prefs.display_runtime = true;
	prefs.display_transitions = true;
	prefs.display_variations = true;
	prefs.doo2breaks = true;
	prefs.drop_stone_mode = true;
	prefs.last_stop = true;
	prefs.min_switch_duration = 19;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.problemsolvingtime = 20;
	prefs.reserve_gas = 21;
	prefs.sacfactor = 22;
	prefs.safetystop = true;
	prefs.switch_at_req_stop = true;
	prefs.verbatim_plan = true;

	tst->load();
	QCOMPARE(prefs.ascratelast6m, 20);
	QCOMPARE(prefs.ascratestops, 21);
	QCOMPARE(prefs.ascrate50, 22);
	QCOMPARE(prefs.ascrate75, 23);
	QCOMPARE(prefs.bestmixend.mm , 41);
	QCOMPARE(prefs.bottompo2, 24);
	QCOMPARE(prefs.bottomsac, 25);
	QCOMPARE(prefs.decopo2, 26);
	QCOMPARE(prefs.decosac, 27);
	QCOMPARE(prefs.descrate, 28);
	QCOMPARE(prefs.display_duration, false);
	QCOMPARE(prefs.display_runtime, false);
	QCOMPARE(prefs.display_transitions, false);
	QCOMPARE(prefs.display_variations, false);
	QCOMPARE(prefs.doo2breaks, false);
	QCOMPARE(prefs.drop_stone_mode, false);
	QCOMPARE(prefs.last_stop, false);
	QCOMPARE(prefs.min_switch_duration, 29);
	QCOMPARE(prefs.planner_deco_mode, VPMB);
	QCOMPARE(prefs.problemsolvingtime, 30);
	QCOMPARE(prefs.reserve_gas, 31);
	QCOMPARE(prefs.sacfactor, 32);
	QCOMPARE(prefs.safetystop, false);
	QCOMPARE(prefs.switch_at_req_stop, false);
	QCOMPARE(prefs.verbatim_plan, false);
}

void TestQPrefDivePlanner::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefDivePlanner::instance();

	prefs.ascratelast6m = 20;
	prefs.ascratestops = 21;
	prefs.ascrate50 = 22;
	prefs.ascrate75 = 23;
	prefs.bestmixend.mm  = 51;
	prefs.bottompo2 = 24;
	prefs.bottomsac = 25;
	prefs.decopo2 = 26;
	prefs.decosac = 27;
	prefs.descrate = 28;
	prefs.display_duration = false;
	prefs.display_runtime = false;
	prefs.display_transitions = false;
	prefs.display_variations = false;
	prefs.doo2breaks = false;
	prefs.drop_stone_mode = false;
	prefs.last_stop = false;
	prefs.min_switch_duration = 29;
	prefs.planner_deco_mode = VPMB;
	prefs.problemsolvingtime = 30;
	prefs.reserve_gas = 31;
	prefs.sacfactor = 32;
	prefs.safetystop = false;
	prefs.switch_at_req_stop = false;
	prefs.verbatim_plan = false;

	tst->sync();
	prefs.ascratelast6m = 10;
	prefs.ascratestops = 11;
	prefs.ascrate50 = 12;
	prefs.ascrate75 = 13;
	prefs.bestmixend.mm  = 11;
	prefs.bottompo2 = 14;
	prefs.bottomsac = 15;
	prefs.decopo2 = 16;
	prefs.decosac = 17;
	prefs.descrate = 18;
	prefs.display_duration = true;
	prefs.display_runtime = true;
	prefs.display_transitions = true;
	prefs.display_variations = true;
	prefs.doo2breaks = true;
	prefs.drop_stone_mode = true;
	prefs.last_stop = true;
	prefs.min_switch_duration = 19;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.problemsolvingtime = 20;
	prefs.reserve_gas = 21;
	prefs.sacfactor = 22;
	prefs.safetystop = true;
	prefs.switch_at_req_stop = true;
	prefs.verbatim_plan = true;

	tst->load();
	QCOMPARE(prefs.ascratelast6m, 20);
	QCOMPARE(prefs.ascratestops, 21);
	QCOMPARE(prefs.ascrate50, 22);
	QCOMPARE(prefs.ascrate75, 23);
	QCOMPARE(prefs.bestmixend.mm , 51);
	QCOMPARE(prefs.bottompo2, 24);
	QCOMPARE(prefs.bottomsac, 25);
	QCOMPARE(prefs.decopo2, 26);
	QCOMPARE(prefs.decosac, 27);
	QCOMPARE(prefs.descrate, 28);
	QCOMPARE(prefs.display_duration, false);
	QCOMPARE(prefs.display_runtime, false);
	QCOMPARE(prefs.display_transitions, false);
	QCOMPARE(prefs.display_variations, false);
	QCOMPARE(prefs.doo2breaks, false);
	QCOMPARE(prefs.drop_stone_mode, false);
	QCOMPARE(prefs.last_stop, false);
	QCOMPARE(prefs.min_switch_duration, 29);
	QCOMPARE(prefs.planner_deco_mode, VPMB);
	QCOMPARE(prefs.problemsolvingtime, 30);
	QCOMPARE(prefs.reserve_gas, 31);
	QCOMPARE(prefs.sacfactor, 32);
	QCOMPARE(prefs.safetystop, false);
	QCOMPARE(prefs.switch_at_req_stop, false);
	QCOMPARE(prefs.verbatim_plan, false);
}

void TestQPrefDivePlanner::test_multiple()
{
	// test multiple instances have the same information

	prefs.sacfactor = 22;
	prefs.safetystop = true;
	auto tst_direct = new qPrefDivePlanner;

	prefs.sacfactor = 32;
	auto tst = qPrefDivePlanner::instance();

	QCOMPARE(tst->sacfactor(), tst_direct->sacfactor());
	QCOMPARE(tst->safetystop(), tst_direct->safetystop());
	QCOMPARE(tst_direct->sacfactor(), 32);
	QCOMPARE(tst_direct->safetystop(), true);
}

QTEST_MAIN(TestQPrefDivePlanner)
