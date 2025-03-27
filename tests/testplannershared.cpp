// SPDX-License-Identifier: GPL-2.0
#include "testplannershared.h"
#include "backend-shared/plannershared.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefUnit.h"

#include <QTest>
#include <QSignalSpy>

void TestPlannerShared::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestPlannerShared");
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
	PlannerShared::set_sacfactor(4.2);
	QCOMPARE(qPrefDivePlanner::sacfactor(), 42);
	PlannerShared::set_sacfactor(3.5);
	QCOMPARE(qPrefDivePlanner::sacfactor(), 35);
	qPrefDivePlanner::set_sacfactor(280);
	QCOMPARE(PlannerShared::sacfactor(), 28);
	qPrefDivePlanner::set_sacfactor(200);
	QCOMPARE(PlannerShared::sacfactor(), 20);

	// Set system to use meters
	qPrefUnits::set_unit_system(METRIC);

	PlannerShared::set_bottomsac(30);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 30000);
	PlannerShared::set_bottomsac(5);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 5000);
	qPrefDivePlanner::set_bottomsac(8000);
	QCOMPARE(PlannerShared::bottomsac(), 8);
	qPrefDivePlanner::set_bottomsac(10000);
	QCOMPARE(PlannerShared::bottomsac(), 10);

	PlannerShared::set_decosac(30);
	QCOMPARE(qPrefDivePlanner::decosac(), 30000);
	PlannerShared::set_decosac(5);
	QCOMPARE(qPrefDivePlanner::decosac(), 5000);
	qPrefDivePlanner::set_decosac(8000);
	QCOMPARE(PlannerShared::decosac(), 8);
	qPrefDivePlanner::set_decosac(10000);
	QCOMPARE(PlannerShared::decosac(), 10);

	PlannerShared::set_bottompo2(1.5);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1500);
	PlannerShared::set_bottompo2(1.6);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1600);
	qPrefDivePlanner::set_bottompo2(1200);
	QCOMPARE(PlannerShared::bottompo2(), 1.2);
	qPrefDivePlanner::set_bottompo2(1000);
	QCOMPARE(PlannerShared::bottompo2(), 1.0);

	PlannerShared::set_decopo2(1.5);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1500);
	PlannerShared::set_decopo2(1.6);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1600);
	qPrefDivePlanner::set_decopo2(1100);
	QCOMPARE(PlannerShared::decopo2(), 1.1);
	qPrefDivePlanner::set_decopo2(1000);
	QCOMPARE(PlannerShared::decopo2(), 1.0);

	PlannerShared::set_bestmixend(16);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 16000);
	PlannerShared::set_bestmixend(7);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 7000);
	qPrefDivePlanner::set_bestmixend(8000);
	QCOMPARE(PlannerShared::bestmixend(), 8);
	qPrefDivePlanner::set_bestmixend(10000);
	QCOMPARE(PlannerShared::bestmixend(), 10);

	// Set system to use feet
	qPrefUnits::set_unit_system(IMPERIAL);

	PlannerShared::set_bottomsac(0.9);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 255);
	PlannerShared::set_bottomsac(0.01);
	QCOMPARE(qPrefDivePlanner::bottomsac(), 3);

	// Remark return will from qPref is in m / 1000.
	qPrefDivePlanner::set_bottomsac(2830);
	QCOMPARE(int(PlannerShared::bottomsac()), 9);
	qPrefDivePlanner::set_bottomsac(16000);
	QCOMPARE(int(PlannerShared::bottomsac()), 56);

	PlannerShared::set_decosac(0.9);
	QCOMPARE(qPrefDivePlanner::decosac(), 255);
	PlannerShared::set_decosac(0.01);
	QCOMPARE(qPrefDivePlanner::decosac(), 3);

	// Remark return will from qPref is in m / 1000.
	qPrefDivePlanner::set_decosac(11500);
	QCOMPARE(int(PlannerShared::decosac()), 40);
	qPrefDivePlanner::set_decosac(19800);
	QCOMPARE(int(PlannerShared::decosac()), 69);

	// Remark bottompo2 is in BAR, even though unit system is
	// Imperial, the desktop version is like that.
	PlannerShared::set_bottompo2(1.5);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1500);
	PlannerShared::set_bottompo2(1.6);
	QCOMPARE(qPrefDivePlanner::bottompo2(), 1600);
	qPrefDivePlanner::set_bottompo2(1200);
	QCOMPARE(PlannerShared::bottompo2(), 1.2);
	qPrefDivePlanner::set_bottompo2(1000);
	QCOMPARE(PlannerShared::bottompo2(), 1.0);

	// Remark decopo2 is in BAR, even though unit system is
	// Imperial, the desktop version is like that.
	PlannerShared::set_decopo2(1.5);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1500);
	PlannerShared::set_decopo2(1.6);
	QCOMPARE(qPrefDivePlanner::decopo2(), 1600);
	qPrefDivePlanner::set_decopo2(1200);
//Not implemented	QCOMPARE(PlannerShared::decopo2(), 1.2);
	qPrefDivePlanner::set_decopo2(1000);
	QCOMPARE(PlannerShared::decopo2(), 1.0);

	PlannerShared::set_bestmixend(33);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 10058);
	PlannerShared::set_bestmixend(27);
	QCOMPARE(qPrefDivePlanner::bestmixend(), 8230);
	qPrefDivePlanner::set_bestmixend(11000);
	QCOMPARE(PlannerShared::bestmixend(), 36);
	qPrefDivePlanner::set_bestmixend(7000);
	QCOMPARE(PlannerShared::bestmixend(), 23);

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
