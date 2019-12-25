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

void TestPlannerShared::test_rates()

{
	// Rates all use meters pr time unit
	// test values have been researched with official subsurface 4.9.3

	// UI (meters) - plist value       UI (feet) - plist value
	//    16m      -    267               33f    -     168
	//     7m      -    117               27f    -     137
	//     8m      -    133               40f    -     203
	//    10m      -    167               35f    -     178

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
	// Variables to test
	// dive_mode
	//OC, CCR, PSCR, FREEDIVE, NUM_DIVEMODE, UNDEF_COMP_TYPE

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
	// Variables to test
	// bottomsac
	// decosac
	// problemsolvingtime
	// sacfactor
	// o2narcotic
	// bottompo2
	// decopo2
	// bestmixend
}

void TestPlannerShared::test_notes()
{
	// Variables to test
	// display_runtime
	// display_duration
	// display_transitions
	// verbatim_plan
	// display_variations
}

QTEST_MAIN(TestPlannerShared)
