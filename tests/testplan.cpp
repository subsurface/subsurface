// SPDX-License-Identifier: GPL-2.0
#include "testplan.h"
#include "core/deco.h"
#include "core/dive.h"
#include "core/event.h"
#include "core/planner.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/units.h"
#include <QDebug>

#define DEBUG 1

// testing the dive plan algorithm
struct decostop stoptable[60];
struct deco_state test_deco_state;
extern bool plan(struct deco_state *ds, struct diveplan *diveplan, struct dive *dive, int timestep, struct decostop *decostoptable, struct deco_state **cached_datap, bool is_planner, bool show_disclaimer);
void setupPrefs()
{
	copy_prefs(&default_prefs, &prefs);
	prefs.ascrate50 = feet_to_mm(30) / 60;
	prefs.ascrate75 = prefs.ascrate50;
	prefs.ascratestops = prefs.ascrate50;
	prefs.ascratelast6m = feet_to_mm(10) / 60;
	prefs.last_stop = true;
}

void setupPrefsVpmb()
{
	copy_prefs(&default_prefs, &prefs);
	prefs.ascrate50 = 10000 / 60;
	prefs.ascrate75 = prefs.ascrate50;
	prefs.ascratestops = prefs.ascrate50;
	prefs.ascratelast6m = prefs.ascrate50;
	prefs.descrate = 99000 / 60;
	prefs.last_stop = false;
	prefs.planner_deco_mode = VPMB;
	prefs.vpmb_conservatism = 0;
}

void setupPlan(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->gfhigh = 100;
	dp->gflow = 100;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{150}, {450}};
	struct gasmix ean36 = {{360}, {0}};
	struct gasmix oxygen = {{1000}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cylinder_t *cyl2 = get_or_create_cylinder(&displayed_dive, 2);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 36000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = ean36;
	cyl2->gasmix = oxygen;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(79, 260) * 60 / M_OR_FT(23, 75);
	plan_add_segment(dp, 0, gas_mod(ean36, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(oxygen, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 2, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(79, 260), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, M_OR_FT(79, 260), 0, 0, 1, OC);
}

void setupPlanVpmb45m30mTx(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->gfhigh = 100;
	dp->gflow = 100;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{210}, {350}};
	struct gasmix ean50 = {{500}, {0}};
	struct gasmix oxygen = {{1000}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cylinder_t *cyl2 = get_or_create_cylinder(&displayed_dive, 2);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 24000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = ean50;
	cyl2->gasmix = oxygen;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(45, 150) * 60 / M_OR_FT(23, 75);
	plan_add_segment(dp, 0, gas_mod(ean50, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(oxygen, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 2, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(45, 150), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, M_OR_FT(45, 150), 0, 0, 1, OC);
}

void setupPlanVpmb60m10mTx(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->gfhigh = 100;
	dp->gflow = 100;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{180}, {450}};
	struct gasmix tx50_15 = {{500}, {150}};
	struct gasmix oxygen = {{1000}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cylinder_t *cyl2 = get_or_create_cylinder(&displayed_dive, 2);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 24000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = tx50_15;
	cyl2->gasmix = oxygen;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(60, 200) * 60 / M_OR_FT(23, 75);
	plan_add_segment(dp, 0, gas_mod(tx50_15, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(oxygen, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 2, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 10 * 60 - droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
}

void setupPlanVpmb60m30minAir(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{210}, {0}};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 100000;
	cyl0->type.workingpressure.mbar = 232000;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(60, 200) * 60 / M_OR_FT(99, 330);
	plan_add_segment(dp, droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
}

void setupPlanVpmb60m30minEan50(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{210}, {0}};
	struct gasmix ean50 = {{500}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 36000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = ean50;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(60, 200) * 60 / M_OR_FT(99, 330);
	plan_add_segment(dp, 0, gas_mod(ean50, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
}

void setupPlanVpmb60m30minTx(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{180}, {450}};
	struct gasmix ean50 = {{500}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 36000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = ean50;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(60, 200) * 60 / M_OR_FT(99, 330);
	plan_add_segment(dp, 0, gas_mod(ean50, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 30 * 60 - droptime, M_OR_FT(60, 200), 0, 0, 1, OC);
}

void setupPlanVpmbMultiLevelAir(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{210}, {0}};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 200000;
	cyl0->type.workingpressure.mbar = 232000;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(20, 66) * 60 / M_OR_FT(99, 330);
	plan_add_segment(dp, droptime, M_OR_FT(20, 66), 0, 0, 1, OC);
	plan_add_segment(dp, 10 * 60 - droptime, M_OR_FT(20, 66), 0, 0, 1, OC);
	plan_add_segment(dp, 1 * 60, M_OR_FT(60, 200), 0, 0, 1, OC);
	plan_add_segment(dp, 29 * 60, M_OR_FT(60, 200), 0, 0, 1, OC);
}

void setupPlanVpmb100m60min(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{180}, {450}};
	struct gasmix ean50 = {{500}, {0}};
	struct gasmix oxygen = {{1000}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cylinder_t *cyl2 = get_or_create_cylinder(&displayed_dive, 2);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 200000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = ean50;
	cyl2->gasmix = oxygen;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(100, 330) * 60 / M_OR_FT(99, 330);
	plan_add_segment(dp, 0, gas_mod(ean50, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(oxygen, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 2, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 60 * 60 - droptime, M_OR_FT(100, 330), 0, 0, 1, OC);
}

void setupPlanVpmb100m10min(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{180}, {450}};
	struct gasmix ean50 = {{500}, {0}};
	struct gasmix oxygen = {{1000}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cylinder_t *cyl2 = get_or_create_cylinder(&displayed_dive, 2);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 60000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = ean50;
	cyl2->gasmix = oxygen;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(100, 330) * 60 / M_OR_FT(99, 330);
	plan_add_segment(dp, 0, gas_mod(ean50, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(oxygen, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 2, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 10 * 60 - droptime, M_OR_FT(100, 330), 0, 0, 1, OC);
}

void setupPlanVpmb30m20min(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{210}, {0}};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 36000;
	cyl0->type.workingpressure.mbar = 232000;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(30, 100) * 60 / M_OR_FT(18, 60);
	plan_add_segment(dp, droptime, M_OR_FT(30, 100), 0, 0, 1, OC);
	plan_add_segment(dp, 20 * 60 - droptime, M_OR_FT(30, 100), 0, 0, 1, OC);
}

void setupPlanVpmb100mTo70m30min(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix bottomgas = {{120}, {650}};
	struct gasmix tx21_35 = {{210}, {350}};
	struct gasmix ean50 = {{500}, {0}};
	struct gasmix oxygen = {{1000}, {0}};
	pressure_t po2 = {1600};
	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cylinder_t *cyl2 = get_or_create_cylinder(&displayed_dive, 2);
	cylinder_t *cyl3 = get_or_create_cylinder(&displayed_dive, 3);
	cyl0->gasmix = bottomgas;
	cyl0->type.size.mliter = 36000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = tx21_35;
	cyl2->gasmix = ean50;
	cyl3->gasmix = oxygen;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(100, 330) * 60 / M_OR_FT(18, 60);
	plan_add_segment(dp, 0, gas_mod(tx21_35, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 1, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(ean50, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 2, 0, 1, OC);
	plan_add_segment(dp, 0, gas_mod(oxygen, po2, &displayed_dive, M_OR_FT(3, 10)).mm, 3, 0, 1, OC);
	plan_add_segment(dp, droptime, M_OR_FT(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 20 * 60 - droptime, M_OR_FT(100, 330), 0, 0, 1, OC);
	plan_add_segment(dp, 3 * 60, M_OR_FT(70, 230), 0, 0, 1, OC);
	plan_add_segment(dp, (30 - 20 - 3) * 60, M_OR_FT(70, 230), 0, 0, 1, OC);
}

/* This tests handling different gases in the manually entered part of the dive */

void setupPlanSeveralGases(struct diveplan *dp)
{
	dp->salinity = 10300;
	dp->surface_pressure = 1013;
	dp->bottomsac = prefs.bottomsac;
	dp->decosac = prefs.decosac;

	struct gasmix ean36 = {{360}, {0}};
	struct gasmix tx11_50 = {{110}, {500}};

	cylinder_t *cyl0 = get_or_create_cylinder(&displayed_dive, 0);
	cylinder_t *cyl1 = get_or_create_cylinder(&displayed_dive, 1);
	cyl0->gasmix = ean36;
	cyl0->type.size.mliter = 36000;
	cyl0->type.workingpressure.mbar = 232000;
	cyl1->gasmix = tx11_50;
	displayed_dive.surface_pressure.mbar = 1013;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	plan_add_segment(dp, 120, 40000, 0, 0, true, OC);
	plan_add_segment(dp, 18 * 60, 40000, 0, 0, true, OC);
	plan_add_segment(dp, 10 * 60, 10000, 1, 0, true, OC);
	plan_add_segment(dp, 5 * 60, 10000, 0, 0, true, OC);
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

		qDebug("Calculated run time = %d seconds", actualRunTimeSeconds);
		qDebug("Expected run time = %d seconds", benchmarkRunTimeSeconds);
		qDebug("Allowed time difference is %g percent plus %d seconds = %d seconds",
		       permilDifferenceAllowed * 0.1, absoluteDifferenceAllowedSeconds, totalDifferenceAllowed);
		qDebug("total difference = %d seconds", totalDifference);

		result = (totalDifference <= totalDifferenceAllowed);
	}
	if ((knownSsrfRunTimeSeconds > 0) && (actualRunTimeSeconds != knownSsrfRunTimeSeconds)) {
		QWARN("Calculated run time does not match known Subsurface runtime");
		qWarning("Calculated runtime: %d", actualRunTimeSeconds);
		qWarning("Known Subsurface runtime: %d", knownSsrfRunTimeSeconds);
	}
	return result;
}

void TestPlan::testMetric()
{
	struct deco_state *cache = NULL;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.planner_deco_mode = BUEHLMANN;

	struct diveplan testPlan = {};
	setupPlan(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 148l);
	// check first gas change to EAN36 at 33m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 36);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 33000);
	// check second gas change to Oxygen at 6m
	ev = ev->next;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 6000);
	// check expected run time of 109 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 109u * 60u, 109u * 60u));
}

void TestPlan::testImperial()
{
	struct deco_state *cache = NULL;

	setupPrefs();
	prefs.unit_system = IMPERIAL;
	prefs.units.length = units::FEET;
	prefs.planner_deco_mode = BUEHLMANN;

	struct diveplan testPlan = {};
	setupPlan(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 154l);
	// check first gas change to EAN36 at 33m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 36);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 33528);
	// check second gas change to Oxygen at 6m
	ev = ev->next;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 6096);
	// check expected run time of 111 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 111u * 60u - 2u, 111u * 60u - 2u));
}

void TestPlan::testVpmbMetric45m30minTx()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb45m30mTx(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 108l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check benchmark run time of 141 minutes, and known Subsurface runtime of 139 minutes
	//QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 141u * 60u + 20u, 139u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m10minTx()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb60m10mTx(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 162l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check benchmark run time of 141 minutes, and known Subsurface runtime of 139 minutes
	//QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 141u * 60u + 20u, 139u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m30minAir()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb60m30minAir(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 180l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check benchmark run time of 141 minutes, and known Subsurface runtime of 139 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 141u * 60u + 20u, 139u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m30minEan50()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb60m30minEan50(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 155l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check first gas change to EAN50 at 21m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 21000);
	// check benchmark run time of 95 minutes, and known Subsurface runtime of 96 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 95u * 60u + 20u, 96u * 60u + 20u));
}

void TestPlan::testVpmbMetric60m30minTx()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb60m30minTx(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 159l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check first gas change to EAN50 at 21m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 21000);
	// check benchmark run time of 89 minutes, and known Subsurface runtime of 89 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 89u * 60u + 20u, 89u * 60u + 20u));
}

void TestPlan::testVpmbMetric100m60min()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb100m60min(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 157l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check first gas change to EAN50 at 21m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 21000);
	// check second gas change to Oxygen at 6m
	ev = ev->next;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 6000);
	// check benchmark run time of 311 minutes, and known Subsurface runtime of 314 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 311u * 60u + 20u, 315u * 60u + 20u));
}

void TestPlan::testMultipleGases()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};

	setupPlanSeveralGases(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	gasmix gas;
	gas = get_gasmix_at_time(&displayed_dive, &displayed_dive.dc, {20 * 60 + 1});
	QCOMPARE(get_o2(gas), 110);
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 2480u, 2480u));
}

void TestPlan::testVpmbMetricMultiLevelAir()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmbMultiLevelAir(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 101l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check benchmark run time of 167 minutes, and known Subsurface runtime of 169 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 167u * 60u + 20u, 169u * 60u + 20u));
}

void TestPlan::testVpmbMetric100m10min()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb100m10min(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 175l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check first gas change to EAN50 at 21m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 21000);
	// check second gas change to Oxygen at 6m
	ev = ev->next;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 6000);
	// check benchmark run time of 58 minutes, and known Subsurface runtime of 57 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 58u * 60u + 20u, 57u * 60u + 20u));
}

/* This tests that a previously calculated plan isn't affecting the calculations of the next plan.
 * It is NOT a 'repetitive dive' test (i.e. with a surface interval and considering tissue
 * saturation from the previous dive).
 */
void TestPlan::testVpmbMetricRepeat()
{
	struct deco_state *cache = NULL;

	setupPrefsVpmb();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;

	struct diveplan testPlan = {};
	setupPlanVpmb30m20min(&testPlan);

	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	struct divedatapoint *dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 61l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check benchmark run time of 27 minutes, and known Subsurface runtime of 28 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 27u * 60u + 20u, 27u * 60u + 20u));

	int firstDiveRunTimeSeconds = displayed_dive.dc.duration.seconds;

	setupPlanVpmb100mTo70m30min(&testPlan);
	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 80l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));
	// check first gas change to 21/35 at 66m
	struct event *ev = displayed_dive.dc.events;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 1);
	QCOMPARE(ev->gas.mix.o2.permille, 210);
	QCOMPARE(ev->gas.mix.he.permille, 350);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 66000);
	// check second gas change to EAN50 at 21m
	ev = ev->next;
	QCOMPARE(ev->gas.index, 2);
	QCOMPARE(ev->value, 50);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 21000);
	// check third gas change to Oxygen at 6m
	ev = ev->next;
	QVERIFY(ev != NULL);
	QCOMPARE(ev->gas.index, 3);
	QCOMPARE(ev->value, 100);
	QCOMPARE(get_depth_at_time(&displayed_dive.dc, ev->time.seconds), 6000);
	// we don't have a benchmark, known Subsurface runtime is 126 minutes
	QVERIFY(compareDecoTime(displayed_dive.dc.duration.seconds, 127u * 60u + 20u, 127u * 60u + 20u));

	setupPlanVpmb30m20min(&testPlan);
	plan(&test_deco_state, &testPlan, &displayed_dive, 60, stoptable, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive, false);
#endif

	// check minimum gas result
	dp = testPlan.dp;
	while (!dp->minimum_gas.mbar && dp->next)
		dp = dp->next;
	QCOMPARE(lrint(dp->minimum_gas.mbar / 1000.0), 61l);
	// print first ceiling
	printf("First ceiling %.1f m\n", (mbar_to_depth(test_deco_state.first_ceiling_pressure.mbar, &displayed_dive) * 0.001));

	// check runtime is exactly the same as the first time
	int finalDiveRunTimeSeconds = displayed_dive.dc.duration.seconds;
	QCOMPARE(finalDiveRunTimeSeconds, firstDiveRunTimeSeconds);
}

QTEST_GUILESS_MAIN(TestPlan)
