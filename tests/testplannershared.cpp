// SPDX-License-Identifier: GPL-2.0
#include "testplannershared.h"
#include "backend-shared/plannershared.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefUnit.h"

#include <QTest>
#include <QSignalSpy>

void TestPlannerShared::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestPlannerShared");
	plannerShared::instance();
}


// test values have been researched with official subsurface 4.9.3
//
// research have been done by using the UI of subsurface, while
// checking the content of org.hohndel.subsurface.Subsurface.plist
// (securing UI value it not the default)
//
// All test have been done on a iMac Pro with Catalina 10.15.2, values in the plist
// might be different on other systems.
// if you experience such problems, please report these so a proper adjustment can
// be made.
//
// meters pr time unit and feet pr time unit
// UI (meters) - plist value       UI (feet) - plist value
//    16m      -    267               33f    -     168
//     7m      -    117               27f    -     137
//     8m      -    133               40f    -     203
//    10m      -    167               35f    -     178
//
// liter pr time unit and cuft pr time unit
// UI (liters) - plist value       UI (cuft) - plist value
//    30l      -    30000           0,9cuft  -   25485
//     5l      -     5000           0,01cuft -     283
//     8l      -     8000           0,4cuft  -   11327
//    10l      -    10000           0,7cuft  -   19822
//
// bar pr time unit and psi pr time unit
// UI (bar) - plist value       UI (psi) - plist value
//   1,5b      -   1500           1,5psi -   1500
//   1,6b      -   1600           1,3psi -   1300
//   1,2b      -   1200           1,1psi -   1100
//   1,0b      -   1000           1,0psi -   1000
//
// sac factor
// UI - plist value
//    4,2      -    420
//    3,5      -    350
//    2,8      -    280
//    2,0      -    200



void TestPlannerShared::test_rates()

{
	// Set system to use meters
	qPrefUnits::set_unit_system(QStringLiteral("metric"));

	plannerShared::set_ascratelast6m(16);
	QCOMPARE(qPrefDivePlanner::ascratelast6m(), 267);
	plannerShared::set_ascratelast6m(7);
	QCOMPARE(qPrefDivePlanner::ascratelast6m(), 117);
	qPrefDivePlanner::set_ascratelast6m(133);
	QCOMPARE(plannerShared::ascratelast6m(), 8);
	qPrefDivePlanner::set_ascratelast6m(167);
	QCOMPARE(plannerShared::ascratelast6m(), 10);

	plannerShared::set_ascratestops(16);
	QCOMPARE(qPrefDivePlanner::ascratestops(), 267);
	plannerShared::set_ascratestops(7);
	QCOMPARE(qPrefDivePlanner::ascratestops(), 117);
	qPrefDivePlanner::set_ascratestops(133);
	QCOMPARE(plannerShared::ascratestops(), 8);
	qPrefDivePlanner::set_ascratestops(167);
	QCOMPARE(plannerShared::ascratestops(), 10);

	plannerShared::set_ascrate50(16);
	QCOMPARE(qPrefDivePlanner::ascrate50(), 267);
	plannerShared::set_ascrate50(7);
	QCOMPARE(qPrefDivePlanner::ascrate50(), 117);
	qPrefDivePlanner::set_ascrate50(133);
	QCOMPARE(plannerShared::ascrate50(), 8);
	qPrefDivePlanner::set_ascrate50(167);
	QCOMPARE(plannerShared::ascrate50(), 10);

	plannerShared::set_ascrate75(16);
	QCOMPARE(qPrefDivePlanner::ascrate75(), 267);
	plannerShared::set_ascrate75(7);
	QCOMPARE(qPrefDivePlanner::ascrate75(), 117);
	qPrefDivePlanner::set_ascrate75(133);
	QCOMPARE(plannerShared::ascrate75(), 8);
	qPrefDivePlanner::set_ascrate75(167);
	QCOMPARE(plannerShared::ascrate75(), 10);

	plannerShared::set_descrate(16);
	QCOMPARE(qPrefDivePlanner::descrate(), 267);
	plannerShared::set_descrate(7);
	QCOMPARE(qPrefDivePlanner::descrate(), 117);
	qPrefDivePlanner::set_descrate(133);
	QCOMPARE(plannerShared::descrate(), 8);
	qPrefDivePlanner::set_descrate(167);
	QCOMPARE(plannerShared::descrate(), 10);

	// Set system to use feet
	qPrefUnits::set_unit_system(QStringLiteral("imperial"));

	plannerShared::set_ascratelast6m(33);
	QCOMPARE(qPrefDivePlanner::ascratelast6m(), 168);
	plannerShared::set_ascratelast6m(27);
	QCOMPARE(qPrefDivePlanner::ascratelast6m(), 137);
	qPrefDivePlanner::set_ascratelast6m(203);
	QCOMPARE(plannerShared::ascratelast6m(), 40);
	qPrefDivePlanner::set_ascratelast6m(178);
	QCOMPARE(plannerShared::ascratelast6m(), 35);

	plannerShared::set_ascratestops(33);
	QCOMPARE(qPrefDivePlanner::ascratestops(), 168);
	plannerShared::set_ascratestops(27);
	QCOMPARE(qPrefDivePlanner::ascratestops(), 137);
	qPrefDivePlanner::set_ascratestops(203);
	QCOMPARE(plannerShared::ascratestops(), 40);
	qPrefDivePlanner::set_ascratestops(178);
	QCOMPARE(plannerShared::ascratestops(), 35);

	plannerShared::set_ascrate50(33);
	QCOMPARE(qPrefDivePlanner::ascrate50(), 168);
	plannerShared::set_ascrate50(27);
	QCOMPARE(qPrefDivePlanner::ascrate50(), 137);
	qPrefDivePlanner::set_ascrate50(203);
	QCOMPARE(plannerShared::ascrate50(), 40);
	qPrefDivePlanner::set_ascrate50(178);
	QCOMPARE(plannerShared::ascrate50(), 35);

	plannerShared::set_ascrate75(33);
	QCOMPARE(qPrefDivePlanner::ascrate75(), 168);
	plannerShared::set_ascrate75(27);
	QCOMPARE(qPrefDivePlanner::ascrate75(), 137);
	qPrefDivePlanner::set_ascrate75(203);
	QCOMPARE(plannerShared::ascrate75(), 40);
	qPrefDivePlanner::set_ascrate75(178);
	QCOMPARE(plannerShared::ascrate75(), 35);

	plannerShared::set_descrate(33);
	QCOMPARE(qPrefDivePlanner::descrate(), 168);
	plannerShared::set_descrate(27);
	QCOMPARE(qPrefDivePlanner::descrate(), 137);
	qPrefDivePlanner::set_descrate(203);
	QCOMPARE(plannerShared::descrate(), 40);
	qPrefDivePlanner::set_descrate(178);
	QCOMPARE(plannerShared::descrate(), 35);
}

void TestPlannerShared::test_planning()
{
	// Variables currently not tested
	// dive_mode: OC, CCR, PSCR, FREEDIVE, NUM_DIVEMODE, UNDEF_COMP_TYPE
	// planner_deco_mode
	// dobailout
	// reserve_gas
	// safetystop
	// gflow
	// gfhigh
	// vpmb_conservatism
	// drop_stone_mode
	// last_stop
	// switch_at_req_stop
	// doo2breaks
	// min_switch_duration
}

void TestPlannerShared::test_gas()
{
	// test independent of metric/imperial
	plannerShared::set_sacfactor(4.2);
	QCOMPARE(qPrefDivePlanner::sacfactor(), 420);
	plannerShared::set_sacfactor(3.5);
	QCOMPARE(qPrefDivePlanner::sacfactor(), 350);
	qPrefDivePlanner::set_sacfactor(280);
	QCOMPARE(plannerShared::sacfactor(), 2.8);
	qPrefDivePlanner::set_sacfactor(200);
	QCOMPARE(plannerShared::sacfactor(), 2.0);

	plannerShared::set_problemsolvingtime(4);
	QCOMPARE(qPrefDivePlanner::problemsolvingtime(), 4);
	plannerShared::set_problemsolvingtime(5);
	QCOMPARE(qPrefDivePlanner::problemsolvingtime(), 5);
	qPrefDivePlanner::set_problemsolvingtime(2);
	QCOMPARE(plannerShared::problemsolvingtime(), 2);
	qPrefDivePlanner::set_problemsolvingtime(6);
	QCOMPARE(plannerShared::problemsolvingtime(), 6);

	// Set system to use meters
	qPrefUnits::set_unit_system(QStringLiteral("metric"));

	plannerShared::set_bottomsac(30);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 30000);
	plannerShared::set_bottomsac(5);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 5000);
	qPrefDivePlanner::set_bottomsac(8000);
	QCOMPARE(plannerShared::bottomsac(), 8);
	qPrefDivePlanner::set_bottomsac(10000);
	QCOMPARE(plannerShared::bottomsac(), 10);

	plannerShared::set_decosac(30);
	QCOMPARE(qPrefDivePlanner::decosac(), 30000);
	plannerShared::set_decosac(5);
	QCOMPARE(qPrefDivePlanner::decosac(), 5000);
	qPrefDivePlanner::set_decosac(8000);
	QCOMPARE(plannerShared::decosac(), 8);
	qPrefDivePlanner::set_decosac(10000);
	QCOMPARE(plannerShared::decosac(), 10);

	plannerShared::set_bottompo2(1.5);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1500);
	plannerShared::set_bottompo2(1.6);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1600);
	qPrefDivePlanner::set_bottompo2(1200);
	QCOMPARE(plannerShared::bottompo2(), 1.2);
	qPrefDivePlanner::set_bottompo2(1000);
	QCOMPARE(plannerShared::bottompo2(), 1.0);

	plannerShared::set_decopo2(1.5);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1500);
	plannerShared::set_decopo2(1.6);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1600);
	qPrefDivePlanner::set_decopo2(1100);
	QCOMPARE(plannerShared::decopo2(), 1.1);
	qPrefDivePlanner::set_decopo2(1000);
	QCOMPARE(plannerShared::decopo2(), 1.0);

	plannerShared::set_bestmixend(16);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 16000);
	plannerShared::set_bestmixend(7);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 7000);
	qPrefDivePlanner::set_bestmixend(8000);
	QCOMPARE(plannerShared::bestmixend(), 8);
	qPrefDivePlanner::set_bestmixend(10000);
	QCOMPARE(plannerShared::bestmixend(), 10);

	// Set system to use feet
	qPrefUnits::set_unit_system(QStringLiteral("imperial"));

	plannerShared::set_bottomsac(0.9);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 25485);
	plannerShared::set_bottomsac(0.01);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 283);
	qPrefDivePlanner::set_bottomsac(11327);
//Not implemented	QCOMPARE(plannerShared::bottomsac(), 0.4);
	qPrefDivePlanner::set_bottomsac(19822);
//Not implemented	QCOMPARE(plannerShared::bottomsac(), 0.7);

	plannerShared::set_decosac(0.9);
	QCOMPARE(qPrefDivePlanner::decosac(), 25485);
	plannerShared::set_decosac(0.01);
	QCOMPARE(qPrefDivePlanner::decosac(), 283);
	qPrefDivePlanner::set_decosac(11327);
//Not implemented	QCOMPARE(plannerShared::decosac(), 0.4);
	qPrefDivePlanner::set_decosac(19822);
//Not implemented	QCOMPARE(plannerShared::decosac(), 0.7);

	// Remark bottompo2 is in BAR, even though unit system is
	// Imperial, the desktop version is like that.
	plannerShared::set_bottompo2(1.5);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1500);
	plannerShared::set_bottompo2(1.6);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1600);
	qPrefDivePlanner::set_bottompo2(1200);
	QCOMPARE(plannerShared::bottompo2(), 1.2);
	qPrefDivePlanner::set_bottompo2(1000);
	QCOMPARE(plannerShared::bottompo2(), 1.0);

	// Remark decopo2 is in BAR, even though unit system is
	// Imperial, the desktop version is like that.
	plannerShared::set_decopo2(1.5);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1500);
	plannerShared::set_decopo2(1.6);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1600);
	qPrefDivePlanner::set_decopo2(1200);
//Not implemented	QCOMPARE(plannerShared::decopo2(), 1.2);
	qPrefDivePlanner::set_decopo2(1000);
	QCOMPARE(plannerShared::decopo2(), 1.0);

	plannerShared::set_bestmixend(33);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 10058);
	plannerShared::set_bestmixend(27);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 8230);
	qPrefDivePlanner::set_bestmixend(11000);
	QCOMPARE(plannerShared::bestmixend(), 36);
	qPrefDivePlanner::set_bestmixend(7000);
	QCOMPARE(plannerShared::bestmixend(), 23);

	// Variables currently not tested
	// o2narcotic
}

void TestPlannerShared::test_notes()
{
	// Variables currently not tested
	// display_runtime
	// display_duration
	// display_transitions
	// verbatim_plan
	// display_variations
}

QTEST_MAIN(TestPlannerShared)
