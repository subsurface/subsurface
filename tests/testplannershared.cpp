// SPDX-License-Identifier: GPL-2.0
#include "testplannershared.h"
#include "backend-shared/plannershared.h"

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

	// Variables to test
	// ascratelast6m
	// ascratestops
	// ascrate50
	// ascrate75
	// descrate
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
