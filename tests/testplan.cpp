#include "dive.h"
#include "testplan.h"
#include "planner.h"
#include "units.h"
#include <QDebug>

// testing the dive plan algorithm
extern bool plan(struct diveplan *diveplan, char **cached_datap, bool is_planner, bool show_disclaimer);

void setupPrefs()
{
	prefs = default_prefs;
	prefs.ascrate50 = feet_to_mm(30) / 60;
	prefs.ascrate75 = prefs.ascrate50;
	prefs.ascratestops = prefs.ascrate50;
	prefs.ascratelast6m = feet_to_mm(10) / 60;
	prefs.last_stop = true;

}

void setupPlan(struct diveplan *dp)
{
	dp->salinity = 10030;
	dp->surface_pressure = 1013;
	dp->gfhigh = 100;
	dp->gflow = 100;
	dp->bottomsac = 0;
	dp->decosac = 0;

	struct gasmix bottomgas = { {150}, {450} };
	struct gasmix ean36 = { {360}, {0} };
	struct gasmix oxygen = { {1000}, {0} };
	pressure_t po2 = { 1600 };
	displayed_dive.cylinder[0].gasmix = bottomgas;
	displayed_dive.cylinder[1].gasmix = ean36;
	displayed_dive.cylinder[2].gasmix = oxygen;
	reset_cylinders(&displayed_dive, true);
	free_dps(dp);

	int droptime = M_OR_FT(79, 260) * 60 / M_OR_FT(23, 75);
	plan_add_segment(dp, droptime, M_OR_FT(79, 260), bottomgas, 0, 1);
	plan_add_segment(dp, 30*60 - droptime, M_OR_FT(79, 260), bottomgas, 0, 1);
	plan_add_segment(dp, 0, gas_mod(&ean36, po2, &displayed_dive, M_OR_FT(3,10)).mm, ean36, 0, 1);
	plan_add_segment(dp, 0, gas_mod(&oxygen, po2, &displayed_dive, M_OR_FT(3,10)).mm, oxygen, 0, 1);
}

void TestPlan::testMetric()
{
	char *cache = NULL;

	setupPrefs();
	prefs.unit_system = METRIC;
	prefs.units.length = units::METERS;
	prefs.deco_mode = BUEHLMANN;

	struct diveplan testPlan = { 0 };
	setupPlan(&testPlan);

	plan(&testPlan, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive);
#endif

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
	// check expected run time of 105 minutes
	QCOMPARE(displayed_dive.dc.duration.seconds, 104u * 60u);
}

void TestPlan::testImperial()
{
	char *cache = NULL;

	setupPrefs();
	prefs.unit_system = IMPERIAL;
	prefs.units.length = units::FEET;
	prefs.deco_mode = BUEHLMANN;

	struct diveplan testPlan = { 0 };
	setupPlan(&testPlan);

	plan(&testPlan, &cache, 1, 0);

#if DEBUG
	free(displayed_dive.notes);
	displayed_dive.notes = NULL;
	save_dive(stdout, &displayed_dive);
#endif

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
	// check expected run time of 105 minutes
	QCOMPARE(displayed_dive.dc.duration.seconds, 105u * 60u);
}

QTEST_MAIN(TestPlan)
