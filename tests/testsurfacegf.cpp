// SPDX-License-Identifier: GPL-2.0
#include "testsurfacegf.h"
#include "core/deco.h"
#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/gas.h"
#include "core/pref.h"
#include "core/profile.h"
#include "core/sample.h"
#include "core/units.h"

void TestSurfaceGf::initTestCase()
{
	TestBase::initTestCase();
}

/* Build a simple square-profile OC air dive directly on dcs[0], the way a
 * downloaded dive computer log looks: dive-level fields that mirror dc
 * fields (surface_pressure, salinity, ...) are left at zero so that
 * dive::fixup_dive() derives them from the dc, exactly as it does on import.
 */
static std::unique_ptr<dive> makeSquareDive(int number, timestamp_t when, pressure_t surfacePressure,
					     depth_t depth, int descentSeconds, int bottomEndSeconds, int totalSeconds)
{
	auto d = std::make_unique<dive>();
	d->number = number;
	d->when = when;

	cylinder_t *cyl0 = d->get_or_create_cylinder(0);
	cyl0->gasmix = gasmix_air;
	cyl0->type.size = 15_l;
	cyl0->type.workingpressure = 200_bar;
	reset_cylinders(d.get(), true);

	struct divecomputer &dc = d->dcs[0];
	dc.model = "subsurface test dc";
	dc.when = when;
	dc.surface_pressure = surfacePressure;
	dc.divemode = OC;

	struct sample *s;
	s = prepare_sample(&dc);
	s->time.seconds = 0;
	s->depth.mm = 0;
	s = prepare_sample(&dc);
	s->time.seconds = descentSeconds;
	s->depth.mm = depth.mm;
	s = prepare_sample(&dc);
	s->time.seconds = bottomEndSeconds;
	s->depth.mm = depth.mm;
	s = prepare_sample(&dc);
	s->time.seconds = totalSeconds;
	s->depth.mm = 0;
	dc.duration.seconds = totalSeconds;

	return d;
}

/* Reproduces the exact scenario from the upstream bug report: the dive is not
 * manually added (it was downloaded, or built in the Planner), so ambient
 * pressure during the dive is, and always was, based on dcs[0]'s own recorded
 * surface pressure (dive::depth_to_bar()/depth_to_mbar() have preferred that
 * over the dive-level field for non-manual dives all along). But the
 * dive-level "Atm. pressure" field - used by dive::get_surface_pressure(),
 * and through it by the tissue carry-over and Surface GF baseline math - can
 * be edited by the user independently of dcs[0]'s recorded value (see
 * TabDiveInformation::updateTextBox()/EditAtmPress), so the two fields can
 * diverge, which used to make the Surface GF baseline inconsistent with the
 * ambient pressure the dive itself was actually computed against.
 */
void TestSurfaceGf::testSurfacePressureConsistency()
{
	clear_dive_file_data();
	prefs = default_prefs;

	auto d = makeSquareDive(1, 1000000000, 1013_mbar, 20_m, 60, 600, 660);
	// Simulate a user having edited the "Atm. pressure" field in the dive
	// info tab to a value that no longer matches dcs[0]'s recorded pressure.
	d->surface_pressure = 990_mbar;
	divelog.dives.record_dive(std::move(d));
	dive &dive1 = *divelog.dives[0];

	// fixup_dive() must not have overwritten the user-edited value.
	QCOMPARE(dive1.surface_pressure.mbar, 990);
	QCOMPARE(dive1.dcs[0].surface_pressure.mbar, 1013);

	// get_surface_pressure() has to agree with the pressure actually used
	// for ambient-pressure math during the dive (depth_to_mbar(0) is the
	// pressure at the surface, i.e. the dive's resolved surface pressure).
	QCOMPARE(dive1.get_surface_pressure().mbar, dive1.depth_to_mbar(0_m));
	// For this non-manually-added dive, that has to be dcs[0]'s value, not
	// the stale, diverged dive-level one.
	QCOMPARE(dive1.get_surface_pressure().mbar, 1013);

	// Manually added dives have no dc-recorded pressure, so they must keep
	// using the user-entered dive-level value, unaffected by this fix.
	auto m = std::make_unique<dive>();
	m->number = 2;
	m->when = 1000010000;
	m->dcs[0].model = "manually added dive";
	m->surface_pressure = 990_mbar;
	cylinder_t *cyl0 = m->get_or_create_cylinder(0);
	cyl0->gasmix = gasmix_air;
	cyl0->type.size = 15_l;
	cyl0->type.workingpressure = 200_bar;
	reset_cylinders(m.get(), true);
	divelog.dives.record_dive(std::move(m));
	dive &manualDive = *divelog.dives[1];
	QCOMPARE(manualDive.get_surface_pressure().mbar, 990);
	QCOMPARE(manualDive.get_surface_pressure().mbar, manualDive.depth_to_mbar(0_m));
}

/* Find the first plot point at or after the given time. */
static const struct plot_data &entryAt(const struct plot_info &pi, int sec)
{
	for (int i = 0; i < pi.nr; i++) {
		if (pi.entry[i].sec >= sec)
			return pi.entry[i];
	}
	return pi.entry[pi.nr - 1];
}

/* Regression test for the upstream report: a short (2 minute) surface
 * interval between two dives must only let tissue loading relax towards
 * equilibrium, so the Surface GF at the start of the second dive must never
 * be higher than the Surface GF at the end of the first dive. This also
 * exercises plot index 0 (the point at the very start of the dive, before
 * any of the dive's own samples have been processed), which used to be left
 * at its stale 0.0 default because the main loop in
 * calculate_deco_information() starts at index 1.
 */
void TestSurfaceGf::testSurfaceGfCarryover()
{
	clear_dive_file_data();
	prefs = default_prefs;
	prefs.planner_deco_mode = BUEHLMANN;
	set_gf(30, 70);

	// A 30m/11min square dive on air is enough to pick up a meaningful
	// amount of nitrogen loading without requiring mandatory deco stops.
	auto d1 = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(d1));

	// Dive 2 starts 2 minutes after dive 1 ends.
	timestamp_t dive2When = divelog.dives[0]->endtime() + 120;
	auto d2 = makeSquareDive(2, dive2When, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(d2));

	dive *dive1 = divelog.dives[0].get();
	dive *dive2 = divelog.dives[1].get();

	struct plot_info pi1 = create_plot_info_new(dive1, &dive1->dcs[0], nullptr);
	QVERIFY(pi1.nr > 1);
	double dive1EndSurfaceGf = pi1.entry[pi1.nr - 1].surface_gf;
	// Sanity check: this profile has to produce meaningful residual nitrogen
	// loading, or the regression below would pass trivially.
	QVERIFY(dive1EndSurfaceGf > 1.0);

	struct plot_info pi2 = create_plot_info_new(dive2, &dive2->dcs[0], nullptr);
	QVERIFY(pi2.nr > 1);
	const struct plot_data &dive2Start = entryAt(pi2, 0);

	// Plot index 0 must no longer be stuck at the stale 0.0 default: it has
	// to reflect the tissue state carried over from dive 1.
	QVERIFY(pi2.entry[0].surface_gf > 1.0);
	QCOMPARE(pi2.entry[0].surface_gf, dive2Start.surface_gf);

	// The actual regression: a 2 minute surface interval must not raise the
	// Surface GF above what it was at the end of the previous dive.
	QVERIFY(dive2Start.surface_gf <= dive1EndSurfaceGf);
	// ... and, since only 2 minutes have passed, it should still be in the
	// same ballpark, not an unrelated, arbitrarily lower value either.
	QVERIFY(dive2Start.surface_gf > dive1EndSurfaceGf * 0.5);
}

QTEST_GUILESS_MAIN(TestSurfaceGf)
