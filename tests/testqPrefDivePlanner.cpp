// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDivePlanner.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefDivePlanner::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDivePlanner");
	qPref::registerQML(NULL);
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
	QCOMPARE(tst->bestmixend(), prefs.bestmixend.mm);
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
	tst->set_bestmixend(21);
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
	tst->set_bestmixend(41);
	tst->set_bottompo2(24);
	tst->set_bottomsac(25);
	tst->set_decopo2(26);
	tst->set_decosac(27);
	tst->set_descrate(28);
	tst->set_display_duration(true);
	tst->set_display_runtime(true);
	tst->set_display_transitions(true);
	tst->set_display_variations(true);
	tst->set_doo2breaks(true);
	tst->set_drop_stone_mode(true);
	tst->set_last_stop(true);
	tst->set_min_switch_duration(29);
	tst->set_planner_deco_mode(VPMB);
	tst->set_problemsolvingtime(30);
	tst->set_reserve_gas(31);
	tst->set_sacfactor(32);
	tst->set_safetystop(true);
	tst->set_switch_at_req_stop(true);
	tst->set_verbatim_plan(true);

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
	prefs.display_duration = false;
	prefs.display_runtime = false;
	prefs.display_transitions = false;
	prefs.display_variations = false;
	prefs.doo2breaks = false;
	prefs.drop_stone_mode = false;
	prefs.last_stop = false;
	prefs.min_switch_duration = 19;
	prefs.planner_deco_mode = BUEHLMANN;
	prefs.problemsolvingtime = 20;
	prefs.reserve_gas = 21;
	prefs.sacfactor = 22;
	prefs.safetystop = false;
	prefs.switch_at_req_stop = false;
	prefs.verbatim_plan = false;

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
	QCOMPARE(prefs.display_duration, true);
	QCOMPARE(prefs.display_runtime, true);
	QCOMPARE(prefs.display_transitions, true);
	QCOMPARE(prefs.display_variations, true);
	QCOMPARE(prefs.doo2breaks, true);
	QCOMPARE(prefs.drop_stone_mode, true);
	QCOMPARE(prefs.last_stop, true);
	QCOMPARE(prefs.min_switch_duration, 29);
	QCOMPARE(prefs.planner_deco_mode, VPMB);
	QCOMPARE(prefs.problemsolvingtime, 30);
	QCOMPARE(prefs.reserve_gas, 31);
	QCOMPARE(prefs.sacfactor, 32);
	QCOMPARE(prefs.safetystop, true);
	QCOMPARE(prefs.switch_at_req_stop, true);
	QCOMPARE(prefs.verbatim_plan, true);
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

	prefs.safetystop = true;
	prefs.sacfactor = 32;
	auto tst = qPrefDivePlanner::instance();

	QCOMPARE(tst->sacfactor(), qPrefDivePlanner::sacfactor());
	QCOMPARE(tst->safetystop(), qPrefDivePlanner::safetystop());
	QCOMPARE(qPrefDivePlanner::sacfactor(), 32);
	QCOMPARE(qPrefDivePlanner::safetystop(), true);
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	planner->sync();           \
	planner->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefDivePlanner::test_oldPreferences()
{
	auto planner = qPrefDivePlanner::instance();

	planner->set_last_stop(true);
	planner->set_verbatim_plan(true);
	planner->set_display_runtime(true);
	planner->set_display_duration(true);
	planner->set_display_transitions(true);
	planner->set_doo2breaks(true);
	planner->set_drop_stone_mode(true);
	planner->set_safetystop(true);
	planner->set_switch_at_req_stop(true);

	planner->set_ascrate75(1);
	planner->set_ascrate50(2);
	planner->set_ascratestops(3);
	planner->set_ascratelast6m(4);
	planner->set_descrate(5);
	planner->set_bottompo2(6);
	planner->set_decopo2(7);
	planner->set_bestmixend(8);
	planner->set_reserve_gas(9);
	planner->set_min_switch_duration(10);
	planner->set_bottomsac(11);
	planner->set_decosac(12);

	planner->set_planner_deco_mode(BUEHLMANN);

	TEST(planner->last_stop(), true);
	TEST(planner->verbatim_plan(), true);
	TEST(planner->display_runtime(), true);
	TEST(planner->display_duration(), true);
	TEST(planner->display_transitions(), true);
	TEST(planner->doo2breaks(), true);
	TEST(planner->drop_stone_mode(), true);
	TEST(planner->safetystop(), true);
	TEST(planner->switch_at_req_stop(), true);

	TEST(planner->ascrate75(), 1);
	TEST(planner->ascrate50(), 2);
	TEST(planner->ascratestops(), 3);
	TEST(planner->ascratelast6m(), 4);
	TEST(planner->descrate(), 5);
	TEST(planner->bottompo2(), 6);
	TEST(planner->decopo2(), 7);
	TEST(planner->bestmixend(), 8);
	TEST(planner->reserve_gas(), 9);
	TEST(planner->min_switch_duration(), 10);
	TEST(planner->bottomsac(), 11);
	TEST(planner->decosac(), 12);

	TEST(planner->planner_deco_mode(), BUEHLMANN);

	planner->set_last_stop(false);
	planner->set_verbatim_plan(false);
	planner->set_display_runtime(false);
	planner->set_display_duration(false);
	planner->set_display_transitions(false);
	planner->set_doo2breaks(false);
	planner->set_drop_stone_mode(false);
	planner->set_safetystop(false);
	planner->set_switch_at_req_stop(false);

	planner->set_ascrate75(11);
	planner->set_ascrate50(12);
	planner->set_ascratestops(13);
	planner->set_ascratelast6m(14);
	planner->set_descrate(15);
	planner->set_bottompo2(16);
	planner->set_decopo2(17);
	planner->set_bestmixend(18);
	planner->set_reserve_gas(19);
	planner->set_min_switch_duration(110);
	planner->set_bottomsac(111);
	planner->set_decosac(112);

	planner->set_planner_deco_mode(RECREATIONAL);

	TEST(planner->last_stop(), false);
	TEST(planner->verbatim_plan(), false);
	TEST(planner->display_runtime(), false);
	TEST(planner->display_duration(), false);
	TEST(planner->display_transitions(), false);
	TEST(planner->doo2breaks(), false);
	TEST(planner->drop_stone_mode(), false);
	TEST(planner->safetystop(), false);
	TEST(planner->switch_at_req_stop(), false);

	TEST(planner->ascrate75(), 11);
	TEST(planner->ascrate50(), 12);
	TEST(planner->ascratestops(), 13);
	TEST(planner->ascratelast6m(), 14);
	TEST(planner->descrate(), 15);
	TEST(planner->bottompo2(), 16);
	TEST(planner->decopo2(), 17);
	TEST(planner->bestmixend(), 18);
	TEST(planner->reserve_gas(), 19);
	TEST(planner->min_switch_duration(), 110);
	TEST(planner->bottomsac(), 111);
	TEST(planner->decosac(), 112);

	TEST(planner->planner_deco_mode(), RECREATIONAL);

}

void TestQPrefDivePlanner::test_signals()
{
	QSignalSpy spy1(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascratelast6mChanged);
	QSignalSpy spy2(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascratestopsChanged);
	QSignalSpy spy3(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascrate50Changed);
	QSignalSpy spy4(qPrefDivePlanner::instance(), &qPrefDivePlanner::ascrate75Changed);
	QSignalSpy spy5(qPrefDivePlanner::instance(), &qPrefDivePlanner::bestmixendChanged);
	QSignalSpy spy6(qPrefDivePlanner::instance(), &qPrefDivePlanner::bottompo2Changed);
	QSignalSpy spy7(qPrefDivePlanner::instance(), &qPrefDivePlanner::bottomsacChanged);
	QSignalSpy spy8(qPrefDivePlanner::instance(), &qPrefDivePlanner::decopo2Changed);
	QSignalSpy spy9(qPrefDivePlanner::instance(), &qPrefDivePlanner::decosacChanged);
	QSignalSpy spy10(qPrefDivePlanner::instance(), &qPrefDivePlanner::descrateChanged);
	QSignalSpy spy11(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_durationChanged);
	QSignalSpy spy12(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_runtimeChanged);
	QSignalSpy spy13(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_transitionsChanged);
	QSignalSpy spy14(qPrefDivePlanner::instance(), &qPrefDivePlanner::display_variationsChanged);
	QSignalSpy spy15(qPrefDivePlanner::instance(), &qPrefDivePlanner::doo2breaksChanged);
	QSignalSpy spy16(qPrefDivePlanner::instance(), &qPrefDivePlanner::drop_stone_modeChanged);
	QSignalSpy spy17(qPrefDivePlanner::instance(), &qPrefDivePlanner::last_stopChanged);
	QSignalSpy spy18(qPrefDivePlanner::instance(), &qPrefDivePlanner::min_switch_durationChanged);
	QSignalSpy spy19(qPrefDivePlanner::instance(), &qPrefDivePlanner::planner_deco_modeChanged);
	QSignalSpy spy20(qPrefDivePlanner::instance(), &qPrefDivePlanner::problemsolvingtimeChanged);
	QSignalSpy spy21(qPrefDivePlanner::instance(), &qPrefDivePlanner::reserve_gasChanged);
	QSignalSpy spy22(qPrefDivePlanner::instance(), &qPrefDivePlanner::sacfactorChanged);
	QSignalSpy spy23(qPrefDivePlanner::instance(), &qPrefDivePlanner::safetystopChanged);
	QSignalSpy spy24(qPrefDivePlanner::instance(), &qPrefDivePlanner::switch_at_req_stopChanged);
	QSignalSpy spy25(qPrefDivePlanner::instance(), &qPrefDivePlanner::verbatim_planChanged);

	qPrefDivePlanner::set_ascratelast6m(-20);
	qPrefDivePlanner::set_ascratestops(-21);
	qPrefDivePlanner::set_ascrate50(-22);
	qPrefDivePlanner::set_ascrate75(-23);
	qPrefDivePlanner::set_bestmixend(-21);
	qPrefDivePlanner::set_bottompo2(-24);
	qPrefDivePlanner::set_bottomsac(-25);
	qPrefDivePlanner::set_decopo2(-26);
	qPrefDivePlanner::set_decosac(-27);
	qPrefDivePlanner::set_descrate(-28);
	prefs.display_duration = true;
	qPrefDivePlanner::set_display_duration(false);
	prefs.display_runtime = true;
	qPrefDivePlanner::set_display_runtime(false);
	prefs.display_transitions = true;
	qPrefDivePlanner::set_display_transitions(false);
	prefs.display_variations = true;
	qPrefDivePlanner::set_display_variations(false);
	prefs.doo2breaks = true;
	qPrefDivePlanner::set_doo2breaks(false);
	prefs.drop_stone_mode = true;
	qPrefDivePlanner::set_drop_stone_mode(false);
	prefs.last_stop = true;
	qPrefDivePlanner::set_last_stop(false);
	qPrefDivePlanner::set_min_switch_duration(-29);
	qPrefDivePlanner::set_planner_deco_mode(VPMB);
	qPrefDivePlanner::set_problemsolvingtime(-30);
	qPrefDivePlanner::set_reserve_gas(-31);
	qPrefDivePlanner::set_sacfactor(-32);
	prefs.safetystop = true;
	qPrefDivePlanner::set_safetystop(false);
	prefs.switch_at_req_stop = true;
	qPrefDivePlanner::set_switch_at_req_stop(false);
	prefs.verbatim_plan = true;
	qPrefDivePlanner::set_verbatim_plan(false);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);
	QCOMPARE(spy6.count(), 1);
	QCOMPARE(spy7.count(), 1);
	QCOMPARE(spy8.count(), 1);
	QCOMPARE(spy9.count(), 1);
	QCOMPARE(spy10.count(), 1);
	QCOMPARE(spy11.count(), 1);
	QCOMPARE(spy12.count(), 1);
	QCOMPARE(spy13.count(), 1);
	QCOMPARE(spy14.count(), 1);
	QCOMPARE(spy15.count(), 1);
	QCOMPARE(spy16.count(), 1);
	QCOMPARE(spy17.count(), 1);
	QCOMPARE(spy18.count(), 1);
	QCOMPARE(spy19.count(), 1);
	QCOMPARE(spy20.count(), 1);
	QCOMPARE(spy21.count(), 1);
	QCOMPARE(spy22.count(), 1);
	QCOMPARE(spy23.count(), 1);
	QCOMPARE(spy24.count(), 1);
	QCOMPARE(spy25.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toInt() == -20);
	QVERIFY(spy2.takeFirst().at(0).toInt() == -21);
	QVERIFY(spy3.takeFirst().at(0).toInt() == -22);
	QVERIFY(spy4.takeFirst().at(0).toInt() == -23);
	QVERIFY(spy5.takeFirst().at(0).toInt() == -21);
	QVERIFY(spy6.takeFirst().at(0).toInt() == -24);
	QVERIFY(spy7.takeFirst().at(0).toInt() == -25);
	QVERIFY(spy8.takeFirst().at(0).toInt() == -26);
	QVERIFY(spy9.takeFirst().at(0).toInt() == -27);
	QVERIFY(spy10.takeFirst().at(0).toInt() == -28);
	QVERIFY(spy11.takeFirst().at(0).toBool() == false);
	QVERIFY(spy12.takeFirst().at(0).toBool() == false);
	QVERIFY(spy13.takeFirst().at(0).toBool() == false);
	QVERIFY(spy14.takeFirst().at(0).toBool() == false);
	QVERIFY(spy15.takeFirst().at(0).toBool() == false);
	QVERIFY(spy16.takeFirst().at(0).toBool() == false);
	QVERIFY(spy17.takeFirst().at(0).toBool() == false);
	QVERIFY(spy18.takeFirst().at(0).toInt() == -29);
	QVERIFY(spy19.takeFirst().at(0).toInt() == VPMB);
	QVERIFY(spy20.takeFirst().at(0).toInt() == -30);
	QVERIFY(spy21.takeFirst().at(0).toInt() == -31);
	QVERIFY(spy22.takeFirst().at(0).toInt() == -32);
	QVERIFY(spy23.takeFirst().at(0).toBool() == false);
	QVERIFY(spy24.takeFirst().at(0).toBool() == false);
	QVERIFY(spy25.takeFirst().at(0).toBool() == false);
}


QTEST_MAIN(TestQPrefDivePlanner)
