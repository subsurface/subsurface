// SPDX-License-Identifier: GPL-2.0
#include "testsurfacegf.h"
#include "core/deco.h"
#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/gas.h"
#include "core/po2.h"
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

/* Build a CCR square-profile dive directly on dcs[0]: a constant commanded
 * setpoint throughout, and (whenever tailSensorValue is non-null) explicit
 * per-sample O2 sensor readings tracking that setpoint, except at the final
 * (surfaced) sample, where the readings diverge to *tailSensorValue - the
 * way an upstream report showed a CCR loop's actually measured PO2 falling
 * well below the still-logged commanded setpoint once the loop was no
 * longer being actively driven. Sensor readings are stamped into the sample
 * stream regardless of noO2Sensors, so this can also build a fixture where
 * raw (and in reality: stray/unused) sensor data is present even though the
 * dive computer declared no_o2sensors == 0. */
static std::unique_ptr<dive> makeCcrSquareDive(int number, timestamp_t when, pressure_t surfacePressure,
						depth_t depth, int descentSeconds, int bottomEndSeconds, int totalSeconds,
						pressure_t setpoint, int noO2Sensors, const pressure_t *tailSensorValue)
{
	auto d = std::make_unique<dive>();
	d->number = number;
	d->when = when;

	cylinder_t *cyl0 = d->get_or_create_cylinder(0); // diluent
	cyl0->gasmix = gasmix_air;
	cyl0->type.size = 3_l;
	cyl0->type.workingpressure = 200_bar;
	cyl0->cylinder_use = DILUENT;
	reset_cylinders(d.get(), true);

	struct divecomputer &dc = d->dcs[0];
	dc.model = "subsurface test ccr dc";
	dc.when = when;
	dc.surface_pressure = surfacePressure;
	dc.divemode = CCR;
	dc.no_o2sensors = noO2Sensors;

	auto stampSensors = [](struct sample *s, pressure_t value) {
		for (int i = 0; i < 3; i++)
			s->o2sensor[i].mbar = value.mbar;
	};

	struct sample *s;
	s = prepare_sample(&dc);
	s->time.seconds = 0;
	s->depth.mm = 0;
	s->setpoint.mbar = setpoint.mbar;
	if (tailSensorValue)
		stampSensors(s, setpoint);
	s = prepare_sample(&dc);
	s->time.seconds = descentSeconds;
	s->depth.mm = depth.mm;
	s->setpoint.mbar = setpoint.mbar;
	if (tailSensorValue)
		stampSensors(s, setpoint);
	s = prepare_sample(&dc);
	s->time.seconds = bottomEndSeconds;
	s->depth.mm = depth.mm;
	s->setpoint.mbar = setpoint.mbar;
	if (tailSensorValue)
		stampSensors(s, setpoint);
	s = prepare_sample(&dc);
	s->time.seconds = totalSeconds;
	s->depth.mm = 0;
	s->setpoint.mbar = setpoint.mbar; // the logged setpoint never changes...
	if (tailSensorValue)
		stampSensors(s, *tailSensorValue); // ...but the measured PO2 has dropped
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

/* Regression test for a second, CCR/PSCR-specific defect behind the same
 * upstream report: add_dive_to_deco() (used to carry tissue loading over to
 * the next dive) always used the dive computer's logged, commanded setpoint,
 * while the live profile display (fill_o2_values()/calculate_ccr_po2()) uses
 * the actually measured, sensor-voted PO2 instead whenever the dive computer
 * reports O2 sensor data. For a dive whose measured PO2 diverges from its
 * commanded setpoint, the two computations disagreed about how much inert
 * gas the diver was exposed to during the dive, producing exactly the same
 * kind of Surface GF discontinuity across the surface interval as
 * testSurfaceGfCarryover() guards against - just caused by the PO2 used
 * rather than by the ambient pressure used.
 */
void TestSurfaceGf::testCcrPo2CarryoverSensorVsSetpoint()
{
	clear_dive_file_data();
	prefs = default_prefs;
	prefs.planner_deco_mode = BUEHLMANN;
	set_gf(30, 70);

	pressure_t setpoint = 1300_mbar;
	// The measured PO2 at the end of the dive has dropped well below both
	// the still-logged setpoint and ambient pressure, same as in the
	// upstream report's CCR dive.
	pressure_t tailSensorValue = 700_mbar;

	auto d1 = makeCcrSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660,
				     setpoint, 3, &tailSensorValue);
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
	QVERIFY(dive1EndSurfaceGf > 1.0);

	struct plot_info pi2 = create_plot_info_new(dive2, &dive2->dcs[0], nullptr);
	QVERIFY(pi2.nr > 1);

	// As in testSurfaceGfCarryover(): a 2 minute surface interval must not
	// raise the Surface GF above what it was at the end of the previous
	// dive, and it should stay in the same ballpark.
	QVERIFY(pi2.entry[0].surface_gf <= dive1EndSurfaceGf);
	QVERIFY(pi2.entry[0].surface_gf > dive1EndSurfaceGf * 0.5);
}

/* Companion to testCcrPo2CarryoverSensorVsSetpoint(): a CCR dive computer
 * that does not report O2 sensor data at all (no_o2sensors == 0) must keep
 * carrying over tissue loading from its commanded setpoint only, exactly as
 * before this fix - even if, as can happen with real dive computers, some
 * raw O2 sensor fields are present in the sample stream without being
 * declared as valid channels. */
void TestSurfaceGf::testCcrPo2CarryoverSensorless()
{
	pressure_t setpoint = 1300_mbar;
	pressure_t strayReading = 700_mbar;

	auto run = [&](const pressure_t *sensorValue) {
		clear_dive_file_data();
		prefs = default_prefs;
		prefs.planner_deco_mode = BUEHLMANN;
		set_gf(30, 70);

		auto d1 = makeCcrSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660,
					     setpoint, 0, sensorValue);
		divelog.dives.record_dive(std::move(d1));
		// record_dive() -> fixup_dive() -> fixup_no_o2sensors() infers
		// no_o2sensors from non-zero sample data whenever it was recorded
		// as 0, which would defeat the point of this fixture. Force it back
		// to 0 to exercise the defensive case: raw o2sensor[] data present
		// (e.g. left over from an edit) without a declared sensor count.
		divelog.dives[0]->dcs[0].no_o2sensors = 0;
		timestamp_t dive2When = divelog.dives[0]->endtime() + 120;
		auto d2 = makeSquareDive(2, dive2When, 1013_mbar, 10_m, 30, 270, 300);
		divelog.dives.record_dive(std::move(d2));

		struct plot_info pi2 = create_plot_info_new(divelog.dives[1].get(), &divelog.dives[1]->dcs[0], nullptr);
		return pi2.entry[0].surface_gf;
	};

	double withoutStrayData = run(nullptr);
	double withStrayData = run(&strayReading);

	// Sanity check: the setpoint-only carry-over still has to produce
	// meaningful residual nitrogen loading.
	QVERIFY(withoutStrayData > 1.0);
	// The whole point: no_o2sensors == 0 must ignore whatever happens to be
	// in o2sensor[], exactly like before this fix.
	QCOMPARE(withStrayData, withoutStrayData);
}

/* Regression test for a sensor-index/sensor-number mix-up in the bubble sort
 * inside sort_o2_pressures() (core/po2.cpp): the swap used to store the raw
 * loop position into sensorn[i] instead of the sensor number that was
 * actually found at that position. For a sparse (non-identity) sensorn[],
 * such as a sensor that has not yet reported a reading, this could make the
 * sort hand the voting logic a sensor index that was never populated as a
 * valid reading in the first place.
 *
 * This drives ccr_po2_loop::next() - the only public entry point into the
 * buggy sort - directly, with four O2 sensors of which sensor 1 never
 * reports a value (stays 0 mbar, as a never-connected/never-read sensor
 * would). Sensors 0, 2 and 3 report 1000, 700 and 900 mbar: with at least
 * three valid readings, the Shearwater-style voting logic votes the
 * outlier (700 mbar) out and averages the remaining two (900 and 1000),
 * giving 950 mbar.
 *
 * With the index/sensor-number mix-up, the sort could instead splice the
 * never-reporting sensor 1 (0 mbar) into the sorted list in place of sensor
 * 2, which got "voted out" for free (subtracting 0 from the running sum)
 * while still shrinking the divisor - inflating the result to 1300 mbar,
 * higher than every sensor actually reported.
 */
void TestSurfaceGf::testCcrPo2VotingSparseSensors()
{
	struct divecomputer dc;
	dc.divemode = CCR;
	dc.no_o2sensors = 4;

	pressure_t o2sensor[MAX_O2_SENSORS] = {};
	o2sensor[0].mbar = 1000;
	o2sensor[1].mbar = 0; // never reports a reading
	o2sensor[2].mbar = 700;
	o2sensor[3].mbar = 900;

	ccr_po2_loop loop(dc);
	pressure_t po2 = loop.next(o2sensor, 0_mbar, 3000_mbar);

	// The never-reporting sensor must never be admitted into the vote: the
	// result has to stay within the range of the sensors that actually
	// reported (700..1000 mbar), not be inflated beyond all of them.
	QVERIFY(po2.mbar >= 700);
	QVERIFY(po2.mbar <= 1000);
	QCOMPARE(po2.mbar, 950);
}

QTEST_GUILESS_MAIN(TestSurfaceGf)
