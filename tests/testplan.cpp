// SPDX-License-Identifier: GPL-2.0
#include "testplan.h"
#include "core/deco.h"
#include "core/dive.h"
#include "core/event.h"
#include "core/errorhelper.h"
#include "core/planner.h"
#include "core/pref.h"
#include "core/profile.h"
#include "core/sample.h"
#include "core/subsurfacestartup.h"
#include "core/tanksensormapping.h"
#include "core/units.h"

#define DEBUG 1

// testing the dive plan algorithm
static struct dive dive;
static struct deco_state test_deco_state;
void setupPrefs()
{
	prefs = default_prefs;
	prefs.ascrate50 = feet_to_mm(30) / 60;
	prefs.ascrate75 = prefs.ascrate50;
	prefs.ascratestops = prefs.ascrate50;
	prefs.ascratelast6m = feet_to_mm(10) / 60;
	prefs.last_stop = true;
}

void setupPrefsVpmb()
{
	prefs = default_prefs;
	prefs.ascrate50 = 10000 / 60;
	prefs.ascrate75 = prefs.ascrate50;
	prefs.ascratestops = prefs.ascrate50;
	prefs.ascratelast6m = prefs.ascrate50;
	prefs.descrate = 99000 / 60;
	prefs.last_stop = false;
	prefs.planner_deco_mode = VPMB;
	prefs.vpmb_conservatism = 0;
}

diveplan setupPlan()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.gfhigh = 100;
	dp.gflow = 100;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 15_percent, 45_percent};
	struct gasmix ean36 = { 36_percent, 0_percent};
	struct gasmix oxygen = { 100_percent, 0_percent};
	pressure_t po2 = 1600_mbar;;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 36_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean36;
	cyl2->gasmix = oxygen;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(79, 260).mm * 60 / m_or_ft(23, 75).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean36, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 2, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(79, 260), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, m_or_ft(79, 260), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb45m30mTx()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.gfhigh = 100;
	dp.gflow = 100;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 21_percent, 35_percent};
	struct gasmix ean50 = { 50_percent, 0_percent};
	struct gasmix oxygen = { 100_percent, 0_percent};
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 24_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean50;
	cyl2->gasmix = oxygen;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(45, 150).mm * 60 / m_or_ft(23, 75).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 2, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(45, 150), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, m_or_ft(45, 150), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb60m10mTx()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.gfhigh = 100;
	dp.gflow = 100;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 18_percent, 45_percent };
	struct gasmix tx50_15 = { 50_percent, 15_percent };
	struct gasmix oxygen = { 100_percent, 0_percent };
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 24_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = tx50_15;
	cyl2->gasmix = oxygen;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(60, 200).mm * 60 / m_or_ft(23, 75).mm;
	plan_add_segment(dp, 0, dive.gas_mod(tx50_15, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 2, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 10 * 60 - droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb60m30minAir()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 21_percent, 0_percent};
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 100_l;
	cyl0->type.workingpressure = 232_bar;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(60, 200).mm * 60 / m_or_ft(99, 330).mm;
	plan_add_segment(dp, droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb60m30minEan50()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 21_percent, 0_percent };
	struct gasmix ean50 = { 50_percent, 0_percent };
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 36_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean50;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(60, 200).mm * 60 / m_or_ft(99, 330).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb60m30minTx()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 18_percent, 45_percent };
	struct gasmix ean50 = { 50_percent, 0_percent };
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 36_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean50;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(60, 200).mm * 60 / m_or_ft(99, 330).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, m_or_ft(60, 200), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmbMultiLevelAir()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 21_percent, 0_percent };
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 200_l;
	cyl0->type.workingpressure = 232_bar;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(20, 66).mm * 60 / m_or_ft(99, 330).mm;
	plan_add_segment(dp, droptime, m_or_ft(20, 66), 0, 0, 1, OC);
	plan_add_segment(dp, 10 * 60 - droptime, m_or_ft(20, 66), 0, 0, 1, OC);
	plan_add_segment(dp, 1 * 60, m_or_ft(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 29 * 60, m_or_ft(60, 200), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb100m60min()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 18_percent, 45_percent };
	struct gasmix ean50 = { 50_percent, 0_percent };
	struct gasmix oxygen = { 100_percent, 0_percent };
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 200_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean50;
	cyl2->gasmix = oxygen;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(100, 330).mm * 60 / m_or_ft(99, 330).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 2, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 60 * 60 - droptime, m_or_ft(100, 330), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb100m10min()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 18_percent, 45_percent };
	struct gasmix ean50 = { 50_percent, 0_percent};
	struct gasmix oxygen = { 100_percent, 0_percent};
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 60_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean50;
	cyl2->gasmix = oxygen;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(100, 330).mm * 60 / m_or_ft(99, 330).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 2, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 10 * 60 - droptime, m_or_ft(100, 330), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb30m20min()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 21_percent, 0_percent };
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 36_l;
	cyl0->type.workingpressure = 232_bar;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(30, 100).mm * 60 / m_or_ft(18, 60).mm;
	plan_add_segment(dp, droptime, m_or_ft(30, 100), 0, 0, 1, OC);
	plan_add_segment(dp, 20 * 60 - droptime, m_or_ft(30, 100), 0, 0, 1, OC);
	return dp;
}

diveplan setupPlanVpmb100mTo70m30min()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix bottomgas = { 12_percent, 65_percent };
	struct gasmix tx21_35 = { 21_percent, 35_percent };
	struct gasmix ean50 = { 50_percent, 0_percent };
	struct gasmix oxygen = { 100_percent, 0_percent };
	pressure_t po2 = 1600_mbar;
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl3 = dive.get_or_create_cylinder(3);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cyl0->gasmix = bottomgas;
	cyl0->type.size = 36_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = tx21_35;
	cyl2->gasmix = ean50;
	cyl3->gasmix = oxygen;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(100, 330).mm * 60 / m_or_ft(18, 60).mm;
	plan_add_segment(dp, 0, dive.gas_mod(tx21_35, po2, m_or_ft(3, 10)), 1, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 2, 0, 1, OC);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 3, 0, 1, OC);
	plan_add_segment(dp, droptime, m_or_ft(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 20 * 60 - droptime, m_or_ft(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 3 * 60, m_or_ft(70, 230), 0, 0, 1, OC);
	plan_add_segment(dp, (30 - 20 - 3) * 60, m_or_ft(70, 230), 0, 0, 1, OC);
	return dp;
}

/* This tests handling different gases in the manually entered part of the dive */

diveplan setupPlanSeveralGases()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix ean36 = { 36_percent, 0_percent };
	struct gasmix tx11_50 = { 11_percent, 50_percent };

	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = ean36;
	cyl0->type.size = 36_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = tx11_50;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	plan_add_segment(dp, 120, 40_m, 0, 0, true, OC);
	plan_add_segment(dp, 18 * 60, 40_m, 0, 0, true, OC);
	plan_add_segment(dp, 10 * 60, 10_m, 1, 0, true, OC);
	plan_add_segment(dp, 5 * 60, 10_m, 0, 0, true, OC);
	return dp;
}

diveplan setupPlanCcr()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.gflow = 50;
	dp.gfhigh = 70;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	pressure_t po2 = 1600_mbar;
	struct gasmix diluent = { 20_percent, 21_percent};
	struct gasmix ean53 = { 53_percent, 0_percent};
	struct gasmix tx19_33 = { 19_percent, 33_percent};
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = diluent;
	cyl0->depth = dive.gas_mod(diluent, po2, m_or_ft(3, 10));
	cyl0->type.size = 3_l;
	cyl0->type.workingpressure = 200_bar;
	cyl0->cylinder_use = DILUENT;
	cyl1->gasmix = ean53;
	cyl1->depth = dive.gas_mod(ean53, po2, m_or_ft(3, 10));
	cyl2->gasmix = tx19_33;
	cyl2->depth = dive.gas_mod(tx19_33, po2, m_or_ft(3, 10));
	reset_cylinders(&dive, true);

	plan_add_segment(dp, 0, cyl1->depth, 1, 0, false, OC);
	plan_add_segment(dp, 0, cyl2->depth, 2, 0, false, OC);
	plan_add_segment(dp, 20 * 60, m_or_ft(60, 197), 0, 1300, true, CCR);

	return dp;
}

/* We compare the calculated runtimes against two values:
 * - Known runtime calculated by Subsurface previously (to detect if anything has changed)
 * - Benchmark runtime (we should be close, but not always exactly the same)
 */
bool compareDecoTime(int actualRunTimeSeconds, int benchmarkRunTimeSeconds, int knownSsrfRunTimeSeconds)
{
	bool result;

	// If the calculated run time equals the expected run time, do a simple comparison
	if (actualRunTimeSeconds == benchmarkRunTimeSeconds) {
		result = true;
	} else {
		/* We want the difference between the expected and calculated total run time to be not more than
		* 1% of total run time + 1 minute */
		int permilDifferenceAllowed = 1 * 10;
		int absoluteDifferenceAllowedSeconds = 60;
		int totalDifferenceAllowed = lrint(0.001 * permilDifferenceAllowed * benchmarkRunTimeSeconds + absoluteDifferenceAllowedSeconds);
		int totalDifference = abs(actualRunTimeSeconds - benchmarkRunTimeSeconds);

		report_info("Calculated run time = %d seconds", actualRunTimeSeconds);
		report_info("Expected run time = %d seconds", benchmarkRunTimeSeconds);
		report_info("Allowed time difference is %g percent plus %d seconds = %d seconds",
		       permilDifferenceAllowed * 0.1, absoluteDifferenceAllowedSeconds, totalDifferenceAllowed);
		report_info("total difference = %d seconds", totalDifference);

		result = (totalDifference <= totalDifferenceAllowed);
	}
	if ((knownSsrfRunTimeSeconds > 0) && (actualRunTimeSeconds != knownSsrfRunTimeSeconds)) {
		report_error("Calculated run time does not match known Subsurface runtime");
		report_error("Calculated runtime: %d", actualRunTimeSeconds);
		report_error("Known Subsurface runtime: %d", knownSsrfRunTimeSeconds);
	}
	return result;
}

void TestPlan::testMetric()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = BUEHLMANN;

	auto testPlan = setupPlan();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 148l);
	QVERIFY(dive.dcs[0].events.size() >= 2);
	// check first gas change to EAN36 at 33m
	struct event *ev = &dive.dcs[0].events[0];
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 36);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 33000);
	// check second gas change to Oxygen at 6m
	ev = &dive.dcs[0].events[1];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 6000);
	// check expected run time of 109 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 109u * 60u, 109u * 60u));
}

void TestPlan::testImperial()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = IMPERIAL;
	prefs.units.length = units::FEET;
	prefs.planner_deco_mode = BUEHLMANN;

	auto testPlan = setupPlan();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 155l);
	QVERIFY(dive.dcs[0].events.size() >= 2);
	// check first gas change to EAN36 at 33m
	struct event *ev = &dive.dcs[0].events[0];
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 36);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 33528);
	// check second gas change to Oxygen at 6m
	ev = &dive.dcs[0].events[1];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 6096);
	// check expected run time of 111 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 111u * 60u - 2u, 111u * 60u - 2u));
}

void TestPlan::testVpmbMetric45m30minTx()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb45m30mTx();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 108l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	// check benchmark run time of 141 minutes, and known Subsurface runtime of 139 minutes
	//QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 141u * 60u + 20u, 139u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m10minTx()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb60m10mTx();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 162l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	// check benchmark run time of 141 minutes, and known Subsurface runtime of 139 minutes
	//QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 141u * 60u + 20u, 139u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m30minAir()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb60m30minAir();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 180l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	// check benchmark run time of 141 minutes, and known Subsurface runtime of 139 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 141u * 60u + 20u, 139u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m30minEan50()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb60m30minEan50();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 155l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	QVERIFY(dive.dcs[0].events.size() >= 1);
	// check first gas change to EAN50 at 21m
	struct event *ev = &dive.dcs[0].events[0];
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 21000);
	// check benchmark run time of 95 minutes, and known Subsurface runtime of 96 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 95u * 60u + 20u, 96u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m30minTx()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb60m30minTx();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 159l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	// check first gas change to EAN50 at 21m
	QVERIFY(dive.dcs[0].events.size() >= 1);
	struct event *ev = &dive.dcs[0].events[0];
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 21000);
	// check benchmark run time of 89 minutes, and known Subsurface runtime of 89 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 89u * 60u + 20u, 89u * 60u + 20u));
}

void TestPlan::testVpmbMetric100m60min()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb100m60min();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 157l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	QVERIFY(dive.dcs[0].events.size() >= 2);
	// check first gas change to EAN50 at 21m
	struct event *ev = &dive.dcs[0].events[0];
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 21000);
	// check second gas change to Oxygen at 6m
	ev = &dive.dcs[0].events[1];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 6000);
	// check benchmark run time of 311 minutes, and known Subsurface runtime of 314 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 311u * 60u + 20u, 315u * 60u + 20u));
}

void TestPlan::testMultipleGases()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanSeveralGases();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	gasmix gas = dive.get_gasmix_at_time(dive.dcs[0], 20_min + 1_sec);
	QCOMPARE(get_o2(gas), 110);
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 2480u, 2480u));
}

void TestPlan::testVpmbMetricMultiLevelAir()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmbMultiLevelAir();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 101l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	// check benchmark run time of 167 minutes, and known Subsurface runtime of 169 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 167u * 60u + 20u, 169u * 60u + 20u));
}

void TestPlan::testVpmbMetric100m10min()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb100m10min();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 175l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	QVERIFY(dive.dcs[0].events.size() >= 2);
	// check first gas change to EAN50 at 21m
	struct event *ev = &dive.dcs[0].events[0];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 21000);
	// check second gas change to Oxygen at 6m
	ev = &dive.dcs[0].events[1];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 6000);
	// check benchmark run time of 58 minutes, and known Subsurface runtime of 57 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 58u * 60u + 20u, 57u * 60u + 20u));
}

/* This tests that a previously calculated plan isn't affecting the calculations of the next plan.
 * It is NOT a 'repetitive dive' test (i.e. with a surface interval and considering tissue
 * saturation from the previous dive).
 */
void TestPlan::testVpmbMetricRepeat()
{
	deco_state_cache cache;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	auto testPlan = setupPlanVpmb30m20min();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	auto dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 61l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	// check benchmark run time of 27 minutes, and known Subsurface runtime of 28 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 27u * 60u + 20u, 27u * 60u + 20u));

	int firstDiveRunTimeSeconds = dive.dcs[0].duration.seconds;

	testPlan = setupPlanVpmb100mTo70m30min();
	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 85l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);
	QVERIFY(dive.dcs[0].events.size() >= 3);
	// check first gas change to 21/35 at 63m
	struct event *ev = &dive.dcs[0].events[0];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->gas.mix.o2.permille, 210);
	QCOMPARE(ev->gas.mix.he.permille, 350);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 63000);
	// check second gas change to EAN50 at 21m
	ev = &dive.dcs[0].events[1];
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 21000);
	// check third gas change to Oxygen at 6m
	ev = &dive.dcs[0].events[2];
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 3);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&dive.dcs[0], ev->time.seconds), 6000);
	// we don't have a benchmark, known Subsurface runtime is 126 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 127u * 60u + 20u, 127u * 60u + 20u));

	testPlan = setupPlanVpmb30m20min();
	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, 1, 0, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check minimum gas result
	dp = std::find_if(testPlan.dp.begin(), testPlan.dp.end(), [](auto &dp) { return dp.minimum_gas.mbar != 0; });
	QCOMPARE(lrint(dp == testPlan.dp.end() ? 0.0 : dp->minimum_gas.mbar / 1000.0), 61l);
	// print first ceiling
	printf("First ceiling %.1f m\n", dive.mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar).mm * 0.001);

	// check runtime is exactly the same as the first time
	int finalDiveRunTimeSeconds = dive.dcs[0].duration.seconds;
	QCOMPARE(finalDiveRunTimeSeconds, firstDiveRunTimeSeconds);
}


// Test that the correct gases are selected during a CCR dive with bailout ascent
// Includes a regression test for https://groups.google.com/g/subsurface-divelog/c/8N3cTz2Zv5E

void TestPlan::testCcrBailoutGasSelection()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = BUEHLMANN;
	dive.dcs[0].divemode = CCR;
	prefs.dobailout = true;

	auto testPlan = setupPlanCcr();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, true, false, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// check diluent used
	cylinder_t *cylinder = dive.get_cylinder(get_cylinderid_at_time(&dive, &dive.dcs[0], 20_min - 1_sec));
	QCOMPARE(cylinder->cylinder_use, DILUENT);
	QCOMPARE(get_o2(cylinder->gasmix), 200);

	// check deep bailout used
	cylinder = dive.get_cylinder(get_cylinderid_at_time(&dive, &dive.dcs[0], 20_min + 1_sec));
	QCOMPARE(cylinder->cylinder_use, OC_GAS);
	QCOMPARE(get_o2(cylinder->gasmix), 190);

	// check shallow bailout used
	cylinder = dive.get_cylinder(get_cylinderid_at_time(&dive, &dive.dcs[0], 30_min));
	QCOMPARE(cylinder->cylinder_use, OC_GAS);
	QCOMPARE(get_o2(cylinder->gasmix), 530);

	// check expected run time of 51 minutes
	QVERIFY(compareDecoTime(dive.dcs[0].duration.seconds, 51 * 60, 51 * 60));

}

// AI-generated (Claude)
// Set up a simple single-cylinder air dive for recreational-mode planning.
static diveplan setupPlanRecreational(depth_t depth, int minutes)
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.gfhigh = 100;
	dp.gflow = 100;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix air = { 21_percent, 0_percent };
	// Start from a clean set of cylinders so that leftovers from a previous
	// test (e.g. CCR diluent / bailout gases) do not affect this OC dive.
	dive.cylinders.clear();
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cyl0->gasmix = air;
	cyl0->type.size = 24_l;
	cyl0->type.workingpressure = 232_bar;
	cyl0->cylinder_use = OC_GAS;
	reset_cylinders(&dive, true);

	int droptime = depth.mm * 60 / m_or_ft(23, 75).mm;
	Q_ASSERT(minutes * 60 >= droptime);
	plan_add_segment(dp, droptime, depth, 0, 0, true, OC);
	plan_add_segment(dp, minutes * 60 - droptime, depth, 0, 0, true, OC);
	return dp;
}

// AI-generated (Claude)
// A conservative recreational dive that stays within the no-decompression
// limit must produce a valid plan.
void TestPlan::testRecreationalWithinNDL()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = RECREATIONAL;
	prefs.reserve_gas = 40000;
	dive.dcs[0].divemode = OC;

	auto testPlan = setupPlanRecreational(m_or_ft(20, 66), 20);

	planner_error_t error = plan(&test_deco_state, testPlan, &dive, 0, 60, cache, true, false, nullptr);

	QCOMPARE(error, PLAN_OK);
}

// AI-generated (Claude)
// Set up a pSCR technical plan to 30m on EAN32, with deco gases EAN50 and
// 100% O2, planned with Buehlmann/GF. This is the reported real-world
// configuration in which the ascent switch to 100% O2 needs no further stop
// at the switch depth, so the switch waypoint ends up with zero duration
// (regression test for a backwards/non-monotonic sample timestamp).
diveplan setupPlanPscr30mEan32Ean50O2()
{
	diveplan dp;
	dp.salinity = 10300;
	dp.surface_pressure = 1_atm;
	dp.gflow = 35;
	dp.gfhigh = 75;
	dp.bottomsac = prefs.bottomsac;
	dp.decosac = prefs.decosac;

	struct gasmix ean32 = { 32_percent, 0_percent };
	struct gasmix ean50 = { 50_percent, 0_percent };
	struct gasmix oxygen = { 100_percent, 0_percent };
	pressure_t po2 = 1600_mbar;
	// Start from a clean set of cylinders so that leftovers from a
	// previous test (e.g. CCR diluent / bailout gases) do not affect the
	// gas selection for this dive.
	dive.cylinders.clear();
	// Note: we add the highest-index cylinder first, because
	// pointers to cylinders are not stable when reallocating.
	// For testing OK - don't do this in actual code!
	cylinder_t *cyl2 = dive.get_or_create_cylinder(2);
	cylinder_t *cyl0 = dive.get_or_create_cylinder(0);
	cylinder_t *cyl1 = dive.get_or_create_cylinder(1);
	cyl0->gasmix = ean32;
	cyl0->type.size = 18_l;
	cyl0->type.workingpressure = 232_bar;
	cyl1->gasmix = ean50;
	cyl1->type.size = 11_l;
	cyl1->type.workingpressure = 232_bar;
	cyl2->gasmix = oxygen;
	cyl2->type.size = 7_l;
	cyl2->type.workingpressure = 232_bar;
	dive.surface_pressure = 1_atm;
	reset_cylinders(&dive, true);

	int droptime = m_or_ft(30, 100).mm * 60 / m_or_ft(18, 60).mm;
	plan_add_segment(dp, 0, dive.gas_mod(ean50, po2, m_or_ft(3, 10)), 1, 0, 1, PSCR);
	plan_add_segment(dp, 0, dive.gas_mod(oxygen, po2, m_or_ft(3, 10)), 2, 0, 1, PSCR);
	plan_add_segment(dp, droptime, m_or_ft(30, 100), 0, 0, 1, PSCR);
	plan_add_segment(dp, 20 * 60 - droptime, m_or_ft(30, 100), 0, 0, 1, PSCR);
	return dp;
}

// AI-generated (Claude)
// Regression test: an ascent gas switch to 100% O2 that needs no further
// stop at the switch depth must never result in dc samples whose time runs
// backwards (or even just repeats out of order). create_dive_from_plan()
// must tolerate a zero-duration switch waypoint.
void TestPlan::testZeroDurationO2SwitchSampleOrder()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = BUEHLMANN;
	dive.dcs[0].divemode = PSCR;

	auto testPlan = setupPlanPscr30mEan32Ean50O2();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, true, false, nullptr);

#if DEBUG
	dive.notes.clear();
	save_dive(stdout, dive, false);
#endif

	// Make sure the plan actually exercises the regression it is meant to
	// guard against: it must contain a genuine zero-duration switch to the
	// 100% O2 cylinder (id 2), i.e. a waypoint on cylinder 2 at the same
	// positive time as the preceding waypoint on the previously active
	// cylinder. Without this check the test would also pass for a plan
	// that never reaches the O2 switch at all.
	bool foundZeroDurationO2Switch = false;
	for (size_t i = 1; i < testPlan.dp.size(); i++) {
		const auto &prev = testPlan.dp[i - 1];
		const auto &cur = testPlan.dp[i];
		if (cur.cylinderid == 2 && prev.cylinderid != 2 &&
		    cur.time > 0 && cur.time == prev.time) {
			foundZeroDurationO2Switch = true;
			break;
		}
	}
	QVERIFY2(foundZeroDurationO2Switch,
		 "plan did not contain a zero-duration switch to cylinder 2 (100% O2)");

	const auto &samples = dive.dcs[0].samples;
	QVERIFY(samples.size() > 1);
	for (size_t i = 1; i < samples.size(); i++) {
		if (samples[i].time.seconds < samples[i - 1].time.seconds)
			report_error("Sample %zu time %d is earlier than previous sample time %d",
				     i, samples[i].time.seconds, samples[i - 1].time.seconds);
		QVERIFY(samples[i].time.seconds >= samples[i - 1].time.seconds);
	}
}

// AI-generated (Claude)
// Regression test for a CodeRabbit review point on the zero-duration gas
// switch fix: at a switch waypoint with dp.time == lasttime, the outgoing
// cylinder's last plot_info entry and the gas-change event share one second,
// and gasmix_loop::cylinder_index_at() advances past any gas-change event
// with time <= the query time. populate_pressure_information() therefore
// zeroes the outgoing cylinder's raw SENSOR_PR at that shared entry.
//
// Measurement (ctest TestPlan, this branch, 14 Oct 2026) confirmed that this
// zeroing is real, but harmless and not new:
//  - It also happens, identically, at the pre-existing EAN32->EAN50 switch
//    in this same plan, which takes real time and predates this fix.
//  - populate_pressure_information() (core/gaspressures.cpp:368-373) reads
//    the outgoing cylinder's pressure into its pr_track end value BEFORE
//    zeroing the live entry, and fill_missing_tank_pressures() then
//    recomputes the identical value into INTERPOLATED_PR at that very
//    entry. get_plot_pressure(), the accessor the profile actually draws
//    with (profile-widget/diveprofileitem.cpp), falls back from SENSOR_PR to
//    INTERPOLATED_PR, so the plotted curve is unaffected either way.
//  - This planner-generated dive has an empty dc.tank_sensor_mappings, so
//    per-sample pressure[] values never reach plot_info at all
//    (core/profile.cpp:420-426); the planner's pressure curve comes solely
//    from the cylinder start/end anchors plus interpolation.
// This test pins that recovery: the outgoing cylinder (1) must still report
// its correct final pressure via get_plot_pressure() at the switch entry,
// and the incoming cylinder (2) must report its correct starting pressure.
void TestPlan::testZeroDurationO2SwitchPressureAttribution()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = BUEHLMANN;
	dive.dcs[0].divemode = PSCR;

	auto testPlan = setupPlanPscr30mEan32Ean50O2();

	plan(&test_deco_state, testPlan, &dive, 0, 60, cache, true, false, nullptr);

	// Design-constraint precondition (core/profile.cpp:724-728):
	// setup_gas_sensor_pressure() skips any cylinder with !start || !end ||
	// start == end, so each cylinder must be genuinely consumed here or its
	// pressure would never be plotted at all, for an unrelated reason.
	const cylinder_t *cyl0 = dive.get_cylinder(0);
	const cylinder_t *cyl1 = dive.get_cylinder(1);
	const cylinder_t *cyl2 = dive.get_cylinder(2);
	for (const cylinder_t *cyl: { cyl0, cyl1, cyl2 }) {
		QVERIFY2(cyl->start.mbar != 0, "cylinder start pressure must be nonzero");
		QVERIFY2(cyl->end.mbar != 0, "cylinder end pressure must be nonzero");
		QVERIFY2(cyl->start.mbar != cyl->end.mbar, "cylinder must be genuinely consumed (start == end otherwise)");
	}

	// This is a planner-generated dive: per-sample pressures must not reach
	// plot_info via dc->tank_sensor_mappings (core/profile.cpp:420-426), so
	// the pressure curve below can only come from the cylinder start/end
	// anchors plus interpolation.
	QVERIFY(dive.dcs[0].tank_sensor_mappings.empty());

	// Locate the zero-duration switch waypoint to cylinder 2 (same
	// detection logic as testZeroDurationO2SwitchSampleOrder()).
	int switchSecond = -1;
	for (size_t i = 1; i < testPlan.dp.size(); i++) {
		const auto &prev = testPlan.dp[i - 1];
		const auto &cur = testPlan.dp[i];
		if (cur.cylinderid == 2 && prev.cylinderid != 2 && cur.time > 0 && cur.time == prev.time) {
			switchSecond = cur.time;
			break;
		}
	}
	QVERIFY2(switchSecond > 0, "plan did not contain a zero-duration switch to cylinder 2 (100% O2)");

	auto pi = create_plot_info_new(&dive, &dive.dcs[0], &test_deco_state);

	// Find the first and last plot_info entry at the switch second: the
	// outgoing cylinder's last entry and the incoming cylinder's first
	// entry share this second (there may be more than one duplicate entry).
	int firstAtS = -1, lastAtS = -1;
	for (int i = 0; i < pi.nr; i++) {
		if (pi.entry[i].sec == switchSecond) {
			if (firstAtS < 0)
				firstAtS = i;
			lastAtS = i;
		}
	}
	QVERIFY2(firstAtS >= 0, "no plot_info entry found at the switch second");

	// The outgoing cylinder (1, EAN50) must still report its correct final
	// pressure via the accessor the profile actually draws with, even
	// though its raw SENSOR_PR is zeroed at this entry.
	QCOMPARE(get_plot_sensor_pressure(pi, firstAtS, 1), 0);
	QCOMPARE(get_plot_pressure(pi, firstAtS, 1), cyl1->end.mbar);

	// The incoming cylinder (2, 100% O2) must report its correct starting
	// pressure (as a raw, unzeroed sensor anchor) at the same entry.
	QCOMPARE(get_plot_sensor_pressure(pi, firstAtS, 2), cyl2->start.mbar);

	// Sanity-check the duplicate entries at the same second, if any: they
	// must not regress cylinder 2's pressure above its starting pressure.
	for (int i = firstAtS; i <= lastAtS; i++)
		QVERIFY(get_plot_pressure(pi, i, 2) <= cyl2->start.mbar);
}

// AI-generated (Claude)
// A recreational dive that exceeds the no-decompression limit (deep and long)
// must be flagged as an error rather than silently producing a plan that
// ascends through the decompression ceiling (issue #4892).
void TestPlan::testRecreationalExceedingNDL()
{
	deco_state_cache cache;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = RECREATIONAL;
	prefs.reserve_gas = 40000;
	dive.dcs[0].divemode = OC;

	auto testPlan = setupPlanRecreational(m_or_ft(50, 165), 50);

	planner_error_t error = plan(&test_deco_state, testPlan, &dive, 0, 60, cache, true, false, nullptr);

	QCOMPARE(error, PLAN_ERROR_RECREATIONAL_EXCEEDS_NDL);
}

QTEST_GUILESS_MAIN(TestPlan)
