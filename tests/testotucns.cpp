// SPDX-License-Identifier: GPL-2.0
#include "testotucns.h"
#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/gas.h"
#include "core/sample.h"
#include "core/units.h"

#include <vector>

void TestOtuCns::initTestCase()
{
	TestBase::initTestCase();
}

/* Build a simple square-profile OC dive directly on dcs[0], the way a
 * downloaded dive computer log looks. Mirrors makeSquareDive() in
 * testsurfacegf.cpp (defaulting to air, like that one); duplicated here so
 * this file has no dependency on that one. Passing descentSeconds ==
 * bottomEndSeconds collapses the plateau to zero duration, giving a plain
 * pyramid profile (immediate turnaround at depth) instead of a square one -
 * used by testOcThresholdCrossing() to cross the OTU/CNS threshold on both
 * the way down and the way up within a single dive.
 */
static std::unique_ptr<dive> makeSquareDive(int number, timestamp_t when, pressure_t surfacePressure,
					     depth_t depth, int descentSeconds, int bottomEndSeconds, int totalSeconds,
					     struct gasmix gasmix = gasmix_air)
{
	auto d = std::make_unique<dive>();
	d->number = number;
	d->when = when;

	cylinder_t *cyl0 = d->get_or_create_cylinder(0);
	cyl0->gasmix = gasmix;
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

/* One point of a synthetic CCR/PSCR sample stream: time, depth, commanded
 * setpoint and up to three O2 cell readings (0 mbar = "not reporting"). */
struct CcrPoint {
	int time;
	double depthM;
	pressure_t setpoint;
	pressure_t sensor[3];
};

/* Build a CCR/PSCR dive directly on dcs[0] from an explicit list of sample
 * points, giving full control over per-sample O2 cell readings (dropout,
 * drift, ...) that the fixtures in testsurfacegf.cpp don't need. */
static std::unique_ptr<dive> makeCcrDive(int number, timestamp_t when, pressure_t surfacePressure,
					  divemode_t divemode, int noO2Sensors,
					  const std::vector<CcrPoint> &points)
{
	auto d = std::make_unique<dive>();
	d->number = number;
	d->when = when;

	cylinder_t *cyl0 = d->get_or_create_cylinder(0); // diluent (CCR) / drive gas (PSCR)
	cyl0->gasmix = gasmix_air;
	cyl0->type.size = 3_l;
	cyl0->type.workingpressure = 200_bar;
	// Bug note: get_effective_divemode() (core/dive.cpp) silently demotes a
	// CCR dc's divemode back to OC whenever the active cylinder's
	// cylinder_use is OC_GAS/TRAVEL_OC - which is cylinder_t's default. A
	// CCR fixture that leaves cyl0 at that default therefore silently
	// falls through to the active_o2()/OC path (core/divelist.cpp) instead
	// of exercising the CCR setpoint/sensor branches it was built to test.
	// Marking the CCR cylinder DILUENT here sidesteps that trap; this is a
	// suspected pre-existing bug (not this harness's to fix), so every
	// fixture below depends on this line to actually test what it says it
	// tests.
	cyl0->cylinder_use = divemode == PSCR ? OC_GAS : DILUENT;
	reset_cylinders(d.get(), true);

	struct divecomputer &dc = d->dcs[0];
	dc.model = "subsurface test ccr dc";
	dc.when = when;
	dc.surface_pressure = surfacePressure;
	dc.divemode = divemode;
	dc.no_o2sensors = noO2Sensors;

	for (const CcrPoint &p: points) {
		struct sample *s = prepare_sample(&dc);
		s->time.seconds = p.time;
		s->depth.mm = lrint(p.depthM * 1000);
		s->setpoint.mbar = p.setpoint.mbar;
		for (int i = 0; i < 3; i++)
			s->o2sensor[i].mbar = p.sensor[i].mbar;
	}
	dc.duration.seconds = points.back().time;

	return d;
}

/* Build a CCR dive that bails out to OC mid-dive via a real cylinder switch
 * event, instead of makeCcrDive()'s single-diluent-cylinder setup. Cylinder
 * 0 is the diluent (as makeCcrDive() requires - see its comment); cylinder 1
 * is the bailout gas, left at its cylinder_use default (OC_GAS,
 * core/equipment.h). A real gas-switch event to cylinder 1 is added at
 * bailoutTime, so get_dive_status_at() (core/dive.cpp) genuinely resolves
 * this dc's per-sample divemode as OC (get_effective_divemode(), triggered
 * by the active cylinder being an OC_GAS/TRAVEL_OC one) for samples from
 * that point on, even though dc.divemode itself stays CCR for the whole dc
 * - the shape that exposed the bug where a CCR dc's silent cells during an
 * OC bailout kept carrying forward a stale pre-bailout PO2 reading instead
 * of switching to the bailout gas's own value (see
 * otu_cns_po2_resolver::segment_use_sensors() in core/divelist.cpp). */
static std::unique_ptr<dive> makeCcrBailoutDive(int number, timestamp_t when, pressure_t surfacePressure,
						 struct gasmix bailoutGas, int bailoutTime,
						 const std::vector<CcrPoint> &points)
{
	auto d = makeCcrDive(number, when, surfacePressure, CCR, 3, points);

	cylinder_t *cyl1 = d->get_or_create_cylinder(1); // bailout gas
	cyl1->gasmix = bailoutGas;
	cyl1->type.size = 11_l;
	cyl1->type.workingpressure = 200_bar;
	reset_cylinders(d.get(), true);

	add_gas_switch_event(d.get(), &d->dcs[0], bailoutTime, 1);

	return d;
}

/* Record a single dive in a fresh dive_table and return the OTU/CNS values
 * dive_table::update_cylinder_related_info() (called from record_dive() via
 * fixup_dive()) produces for it via calculate_otu()/calculate_cns_dive() in
 * core/divelist.cpp. With no other dive in the table, calculate_cns() has no
 * previous dive to carry CNS over from, so dive.maxcns is exactly
 * lrint(calculate_cns_dive(dive)). */
static void recordAndGet(std::unique_ptr<dive> d, int *otu, int *maxcns)
{
	clear_dive_file_data();
	divelog.dives.record_dive(std::move(d));
	*otu = divelog.dives[0]->otu;
	*maxcns = divelog.dives[0]->maxcns;
}

/* (a) Plain OC air square dive: calculate_otu()/calculate_cns_dive() never
 * touch ccr_po2_loop for a non-CCR/PSCR dc (the final else branch in both
 * functions is untouched by this change), so this must produce bit-for-bit
 * identical OTU/CNS before and after. 30m is deep enough on air to push PO2
 * past the 500 mbar OTU/CNS threshold. */
void TestOtuCns::testOcAirUnchanged()
{
	int otu, maxcns;
	recordAndGet(makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660), &otu, &maxcns);

	QCOMPARE(otu, 6);
	QCOMPARE(maxcns, 2);
}

/* (a2) Shallow OC air square dive: PO2 never exceeds the 500 mbar OTU/CNS
 * threshold (core/divelist.cpp: the `po2i > 500 || po2f > 500` check for
 * OTU, `po2 <= 500` skip for CNS), so both must come out at exactly 0, not
 * an arbitrary pinned number: 10m on air keeps ambient PO2 at roughly 0.21 *
 * 2.0 bar =~ 420 mbar throughout, comfortably under the threshold even at
 * the bottom. */
void TestOtuCns::testOcAirShallowSubThreshold()
{
	int otu, maxcns;
	recordAndGet(makeSquareDive(1, 1000000000, 1013_mbar, 10_m, 60, 300, 360), &otu, &maxcns);

	QCOMPARE(otu, 0);
	QCOMPARE(maxcns, 0);
}

/* (a3) OC profile crossing the 500 mbar threshold mid-segment, in both
 * directions, on EAN30: a plain pyramid (makeSquareDive() with
 * descentSeconds == bottomEndSeconds, collapsing the plateau to zero
 * duration) descends through the threshold on the way down and back through
 * it on the way up, exercising the descent partial-segment proration
 * (core/divelist.cpp "descent segment with po2i <= 500 mbar") and the
 * ascent partial-segment proration ("ascent segment with po2f <= 500
 * mbar") within the same dive. */
void TestOtuCns::testOcThresholdCrossing()
{
	int otu, maxcns;
	recordAndGet(makeSquareDive(1, 1000000000, 1013_mbar, 20_m, 60, 60, 120, { 30_percent, 0_percent }), &otu, &maxcns);

	QCOMPARE(otu, 1);
	QCOMPARE(maxcns, 0);
}

/* (b) CCR dive, no_o2sensors == 3, all three cells agreeing with each other
 * and with the commanded setpoint throughout, comfortably below ambient at
 * every sample (including the shallowest, 10m, so ccr_po2_loop's ambient
 * clamp never triggers anywhere in this fixture - see testCcrAmbientClamp()
 * for that). Since every sample's cell 0 reading already equals what
 * calculate_ccr_po2() resolves from all three cells (they agree) and the
 * setpoint never changes, this must produce bit-for-bit identical OTU/CNS
 * whether PO2 is read from o2sensor[0] directly (old code) or routed
 * through ccr_po2_loop (new code). The profile never surfaces, to keep
 * every ambient pressure comfortably above the 1.2 bar reading. */
void TestOtuCns::testCcrSensorsAgreeUnchanged()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t cell = 1200_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   10.0, setpoint, { cell, cell, cell } },
		{ 300, 30.0, setpoint, { cell, cell, cell } },
		{ 600, 10.0, setpoint, { cell, cell, cell } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 3, points), &otu, &maxcns);

	QCOMPARE(otu, 13);
	QCOMPARE(maxcns, 5);
}

/* (b2) CCR dive with a commanded setpoint and no cell data at all
 * (no_o2sensors == 0, every o2sensor[] reading left at 0): the setpoint
 * branch (core/divelist.cpp "dc.divemode == CCR" sub-branch of the
 * non-sensor case), clamped to ambient at each endpoint's own depth, with
 * no averaging, unlike the sensor/OC/PSCR branches. Depends on the DILUENT
 * cylinder_use set by makeCcrDive() (see the bug note there) to actually
 * reach the CCR setpoint branch instead of being silently demoted to OC. */
void TestOtuCns::testCcrSetpointNoCellData()
{
	pressure_t setpoint = 1300_mbar;
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   0.0,  setpoint, { zero, zero, zero } },
		{ 60,  30.0, setpoint, { zero, zero, zero } },
		{ 540, 30.0, setpoint, { zero, zero, zero } },
		{ 660, 0.0,  setpoint, { zero, zero, zero } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 0, points), &otu, &maxcns);

	QCOMPARE(otu, 16);
	QCOMPARE(maxcns, 6);
}

/* (b3) A CCR dc that declares a non-zero no_o2sensors (3) but whose sample
 * stream never actually carries any non-zero o2sensor[] reading on any
 * channel - the mirror image of testCcrNoO2SensorsStrayCell()'s "declares 0
 * but has stray data" shape. Before this fix, use_sensors was gated on
 * `dc.no_o2sensors > 0` directly, so this dc was routed into the
 * sensor/ccr_po2_loop branch despite having no actual cell data to vote on
 * or carry forward (calculate_ccr_po2() would just see three all-zero
 * channels). With the fix, use_sensors is gated on has_any_cell_data()
 * unconditionally, so this dc correctly falls through to the same
 * setpoint-only branch as testCcrSetpointNoCellData() (no_o2sensors == 0),
 * producing the identical OTU/CNS: the declared sensor count alone must not
 * change which branch a dc with no actual cell data takes. */
void TestOtuCns::testCcrDeclaredSensorsNoCellData()
{
	pressure_t setpoint = 1300_mbar;
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   0.0,  setpoint, { zero, zero, zero } },
		{ 60,  30.0, setpoint, { zero, zero, zero } },
		{ 540, 30.0, setpoint, { zero, zero, zero } },
		{ 660, 0.0,  setpoint, { zero, zero, zero } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 3, points), &otu, &maxcns);

	QCOMPARE(otu, 16);
	QCOMPARE(maxcns, 6);
}

/* (b4) A CCR dc that declares no_o2sensors == 3 whose cell 0 never reports
 * (reads 0 mbar) for the entire dive, while cells 1 and 2 keep agreeing
 * with each other at a real, non-setpoint reading throughout. The dc never
 * surfaces, so this isolates the gate from the ambient clamp.
 *
 * Before this fix, has_cell_data (and therefore use_sensors) was decided by
 * has_stray_sensor0_data(), which only ever scanned o2sensor[0]: with that
 * channel silent for the whole dive, has_cell_data was false regardless of
 * cells 1/2, so this dc fell through to the non-sensor, setpoint-only CCR
 * branch for the whole dive (min(setpoint, ambient), unclamped against the
 * real cell data), diverging from add_dive_to_deco() and the live profile,
 * which both vote across all three declared channels via ccr_po2_loop and
 * would use cells 1/2's real reading instead.
 *
 * With the fix, has_cell_data (has_any_cell_data()) scans every channel up
 * to MAX_O2_SENSORS, so cells 1/2's non-zero readings are seen, use_sensors
 * is true, and segment_use_sensors() (already per-channel) agrees -
 * matching the deco/profile paths for the same dive. calculate_ccr_po2()
 * then ignores the always-zero channel 0 reading (its "if (o2sensor[i].mbar)"
 * check) and resolves PO2 from the two agreeing cells.
 */
void TestOtuCns::testCcrSensor0SilentOtherSensorsAgree()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t cell = 1000_mbar;
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   20.0, setpoint, { zero, cell, cell } },
		{ 600, 20.0, setpoint, { zero, cell, cell } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 3, points), &otu, &maxcns);

	// Old (pre-fix) values, captured against the has_stray_sensor0_data()
	// gate this change replaces: otu == 13, maxcns == 5 (the setpoint-only
	// branch, min(1200, ambient) for the whole dive - see
	// testCcrSensorsAgreeUnchanged()/testCcrSetpointNoCellData() for the
	// same numbers from the same unclamped setpoint).
	QCOMPARE(otu, 10);
	QCOMPARE(maxcns, 3);
}

/* (c) Sensor-0 dropout: cell 0 stops reporting (reads 0 mbar) for two
 * consecutive samples while cells 1 and 2 keep reporting correctly (and no
 * longer agree with cell 0's last known value). The dc never surfaces, so
 * this isolates the dropout/carry-forward behaviour from the ambient
 * clamp.
 *
 * Old code (direct o2sensor[0] read, core/divelist.cpp before this change):
 * the per-pair gate is `psample.o2sensor[0].mbar`, checked only at each
 * segment's START sample.
 *   - segment 0->300s: psample (t=0) cell 0 = 1200 (nonzero) -> po2i = 1200;
 *     sample (t=300) cell 0 = 0 -> po2f = po2i = 1200 (frozen, the real
 *     cells 1/2 = 1000 are never consulted).
 *   - segment 300->600s: psample (t=300) cell 0 = 0 -> gate fails entirely
 *     -> falls through to the non-sensor CCR setpoint branch for the WHOLE
 *     segment: po2i = po2f = min(setpoint, ambient) = 1200 ("drops to
 *     setpoint" exactly as described in the problem statement).
 *   - segment 600->900s: psample (t=600) cell 0 = 0 -> same as above, 1200.
 *   So the old code effectively computes a flat PO2 of 1200 mbar for the
 *   whole dive, as if the dropout and the cells 1/2 readings did not exist.
 *   This produced OTU 20 / CNS 7 before this change (captured against
 *   unmodified core/divelist.cpp).
 *
 * New code (ccr_po2_loop): cell 0's carry-forward is per-sensor, inside the
 * helper's own last_sensor[] state, not gated on the pair's start sample:
 *   - at t=300 and t=600, cell 0 is carried forward as its last reported
 *     value (1200), and voted alongside cells 1/2 (1000, 1000): sorted
 *     [1000, 1000, 1200], spread 200 <= sum/3/5 (213), so all three stay in
 *     the average: (1000+1000+1200)/3 = 1066.
 *   - at t=0 and t=900 all three cells agree at 1200, so the result is 1200.
 *   So the corrected PO2 dips to 1066 mbar during the dropout instead of
 *   staying frozen/falling back to the flat setpoint-only 1200.
 */
void TestOtuCns::testCcrSensor0Dropout()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t full = 1200_mbar;
	pressure_t driftedLow = 1000_mbar;
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   20.0, setpoint, { full, full, full } },
		{ 300, 20.0, setpoint, { zero, driftedLow, driftedLow } },
		{ 600, 20.0, setpoint, { zero, driftedLow, driftedLow } },
		{ 900, 20.0, setpoint, { full, full, full } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 3, points), &otu, &maxcns);

	// Old (pre-change) values, captured against unmodified core/divelist.cpp
	// before this change: otu == 20, maxcns == 7.
	QCOMPARE(otu, 18);
	QCOMPARE(maxcns, 6);
}

/* (d) Sensor-0 voted-out drift: cell 0 has drifted high (1.6 bar) while
 * cells 1 and 2 keep agreeing at the commanded setpoint (1.2 bar), for the
 * whole (non-surfacing) profile.
 *
 * Old code always read o2sensor[0] directly, so it used the drifted 1600
 * mbar reading for the entire dive, unclamped (ambient at 20m is far above
 * 1600 mbar, so no clamp would have applied here even if the old code had
 * one). This produced OTU 19 / CNS 21 before this change.
 *
 * New code resolves PO2 via calculate_ccr_po2()'s Shearwater-style voting:
 * sorted [1200, 1200, 1600], spread 400 > sum/3/5 (266), so the 1600 mbar
 * outlier is voted out and the result is the average of the remaining two,
 * 1200 mbar, i.e. exactly the agreeing cells/setpoint.
 */
void TestOtuCns::testCcrSensor0VotedOutDrift()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t driftedHigh = 1600_mbar;
	pressure_t agreeing = 1200_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   20.0, setpoint, { driftedHigh, agreeing, agreeing } },
		{ 600, 20.0, setpoint, { driftedHigh, agreeing, agreeing } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 3, points), &otu, &maxcns);

	// Old (pre-change) values, captured against unmodified core/divelist.cpp
	// before this change: otu == 19, maxcns == 21.
	QCOMPARE(otu, 13);
	QCOMPARE(maxcns, 5);
}

/* (e) A CCR dc that declares no_o2sensors == 0 (no valid O2 sensor
 * channels) while o2sensor[0] still carries stray, non-zero data - a real
 * data shape, see testsurfacegf.cpp's testCcrPo2CarryoverSensorless(). The
 * dc never surfaces, isolating this from the ambient clamp.
 *
 * The previous, direct o2sensor[0] read (`psample.o2sensor[0].mbar`) never
 * looked at no_o2sensors at all, so it used the stray 1600 mbar reading as
 * if it were valid sensor data for the whole dive: OTU 19 / CNS 21 (same
 * numbers as testCcrSensor0VotedOutDrift(), since both drive that code down
 * the same raw-sensor0 path with the same value).
 *
 * Gating the sensor branch on `dc.no_o2sensors > 0` in addition to
 * loop_po2.uses_sensors(), matching add_dive_to_deco(), would make this dc
 * fall through to the setpoint branch instead, giving min(setpoint,
 * ambient) = 1200 mbar (OTU 13 / CNS 5, the same as the
 * agreeing-cells/setpoint fixtures) - silently changing OTU/CNS for a real,
 * live data shape with no way to tell how many stored logs look like this.
 * otu_cns_po2_resolver (core/divelist.cpp) deliberately does not add that
 * guard.
 *
 * Instead, otu_cns_po2_resolver decides whether to use the sensor branch
 * from whether cell data is actually present (the same effect as the old
 * direct-read gate), then resolves it through a no_o2sensors-widened
 * ccr_po2_loop stand-in so the stray 1600 mbar reading is not discarded
 * again from the inside. With only cell 0 ever reporting,
 * calculate_ccr_po2() has a single valid reading (no voting to do) and
 * returns it directly, clamped to ambient at 20m (comfortably above 1600
 * mbar, so the clamp does not bite here) - i.e. the same OTU 19 / CNS 21
 * the old direct-read code produced for this shape, now reached through the
 * shared resolver instead of a direct o2sensor[0] read.
 */
void TestOtuCns::testCcrNoO2SensorsStrayCell()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t strayReading = 1600_mbar;
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   20.0, setpoint, { strayReading, zero, zero } },
		{ 600, 20.0, setpoint, { strayReading, zero, zero } },
	};
	auto d = makeCcrDive(1, 1000000000, 1013_mbar, CCR, 3, points);
	// Build the fixture with no_o2sensors temporarily non-zero so
	// prepare_sample()/record_dive() don't discard the stray cell data,
	// then force no_o2sensors back to 0, the same way
	// testsurfacegf.cpp's testCcrPo2CarryoverSensorless() does: record_dive()
	// -> fixup_dive() -> fixup_no_o2sensors() infers no_o2sensors from
	// non-zero sample data whenever it was recorded as 0, which would
	// defeat the point of this fixture. Re-running the full fixup_dive()
	// afterwards would re-trigger fixup_no_o2sensors() and re-infer
	// no_o2sensors from the still-present stray data, so recompute only
	// update_cylinder_related_info() (sac/otu/maxcns), not the whole
	// fixup, with the overridden no_o2sensors == 0 in place.
	clear_dive_file_data();
	divelog.dives.record_dive(std::move(d));
	divelog.dives[0]->dcs[0].no_o2sensors = 0;
	divelog.dives[0]->otu = 0;
	divelog.dives[0]->maxcns = 0;
	// calculate_cns()'s shortcut (`if (dive.cns) return dive.cns;`) would
	// otherwise just hand back the stale value cached by the record_dive()
	// call above (computed before no_o2sensors was overridden), without
	// recomputing against the new value at all.
	divelog.dives[0]->cns = 0;
	divelog.dives.update_cylinder_related_info(*divelog.dives[0]);

	int otu = divelog.dives[0]->otu;
	int maxcns = divelog.dives[0]->maxcns;

	// See the comment above this test: this is deliberately the same
	// numbers (19, 21) the old direct-read code produced for this shape,
	// not the (13, 5) an additional no_o2sensors > 0 guard would give.
	QCOMPARE(otu, 19);
	QCOMPARE(maxcns, 21);
}

/* (f) A single O2 cell (no_o2sensors == 1) reading above ambient at a
 * shallow (1m) sample.
 *
 * The old, direct o2sensor[0] read never clamped at all: it used the raw
 * 1600 mbar cell reading for the whole dive, completely independent of
 * depth, which produced OTU 19 / CNS 21 - the same numbers
 * testCcrSensor0VotedOutDrift() and testCcrNoO2SensorsStrayCell() pin, at
 * 20m instead of 1m: same reading, same duration, same (lack of) clamp.
 *
 * ccr_po2_loop::next() clamps unconditionally to ambient, so the resolved
 * PO2 here is only about 1113 mbar (ambient at 1m) instead of 1600 - lower,
 * not higher, so the corrected OTU/CNS must be smaller than the old,
 * unclamped ones, and than the other two 1600 mbar fixtures above that
 * resolve at the deeper, unclamped 20m.
 */
void TestOtuCns::testCcrAmbientClamp()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t cell = 1600_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   1.0, setpoint, { cell, 0_mbar, 0_mbar } },
		{ 600, 1.0, setpoint, { cell, 0_mbar, 0_mbar } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, CCR, 1, points), &otu, &maxcns);

	// Old (pre-change) values, captured against unmodified core/divelist.cpp
	// before this change: otu == 19, maxcns == 21. The old code's sensor
	// branch does not clamp to ambient at all, so it is also unaffected
	// by depth: the same 1600 mbar reading for the same 600s duration
	// gives the same result as testCcrSensor0VotedOutDrift() (20m) and
	// testCcrNoO2SensorsStrayCell() (also 20m), even though this fixture
	// is at only 1m.
	QCOMPARE(otu, 12);
	QCOMPARE(maxcns, 4);
}

/* Optional PSCR sanity case: a PSCR dc with no_o2sensors > 0 whose two
 * cells agree with each other and with the setpoint throughout, never
 * surfacing. ccr_po2_loop::uses_sensors() already requires
 * `dc.divemode == PSCR && dc.no_o2sensors` for PSCR, so this exercises that
 * this code path (not just CCR) is wired through the helper, and that an
 * agreeing-cells PSCR dive is unaffected by this change. */
void TestOtuCns::testPscrSensorsAgreeUnchanged()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t cell = 1200_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   10.0, setpoint, { cell, cell, 0_mbar } },
		{ 300, 25.0, setpoint, { cell, cell, 0_mbar } },
		{ 600, 10.0, setpoint, { cell, cell, 0_mbar } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, PSCR, 2, points), &otu, &maxcns);

	QCOMPARE(otu, 13);
	QCOMPARE(maxcns, 5);
}

/* PSCR with no O2 sensors at all (no_o2sensors == 0, no cell data):
 * ccr_po2_loop::uses_sensors() is false for this combination (PSCR only
 * uses_sensors() when dc.no_o2sensors is set), so this must take the
 * pscr_o2() branch (core/divelist.cpp "dc.divemode == PSCR" sub-branch of
 * the non-sensor case) for the whole dive, unaffected by the
 * no_o2sensors-widening behaviour in otu_cns_po2_resolver (that only ever
 * widens the CCR sensor branch, never PSCR's). */
void TestOtuCns::testPscrNoO2Sensors()
{
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   0.0,  zero, { zero, zero, zero } },
		{ 60,  30.0, zero, { zero, zero, zero } },
		{ 300, 30.0, zero, { zero, zero, zero } },
		{ 360, 0.0,  zero, { zero, zero, zero } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, PSCR, 0, points), &otu, &maxcns);

	QCOMPARE(otu, 1);
	QCOMPARE(maxcns, 1);
}

/* PSCR with a declared no_o2sensors (2) but no actual cell data in any
 * sample: ccr_po2_loop::uses_sensors() is true for this combination (PSCR
 * uses_sensors() only checks dc.no_o2sensors is set, not whether the
 * samples carry real data), so before this fix use_sensors was true here
 * purely because of the declared count, routing this dc into the sensor
 * branch with calculate_ccr_po2() seeing three all-zero channels. With the
 * fix, use_sensors additionally requires has_any_cell_data(), so this
 * dc falls through to the same pscr_o2() branch as testPscrNoO2Sensors()
 * (no_o2sensors == 0), producing the identical OTU/CNS. */
void TestOtuCns::testPscrDeclaredSensorsNoCellData()
{
	pressure_t zero = 0_mbar;
	std::vector<CcrPoint> points = {
		{ 0,   0.0,  zero, { zero, zero, zero } },
		{ 60,  30.0, zero, { zero, zero, zero } },
		{ 300, 30.0, zero, { zero, zero, zero } },
		{ 360, 0.0,  zero, { zero, zero, zero } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrDive(1, 1000000000, 1013_mbar, PSCR, 2, points), &otu, &maxcns);

	QCOMPARE(otu, 1);
	QCOMPARE(maxcns, 1);
}

/* (g) CCR dive with a real OC bailout (a gas-switch event to an OC_GAS
 * cylinder) whose cells also go silent (0 mbar) from the bailout sample
 * onward, for the rest of the dive - the regression fixed by
 * otu_cns_po2_resolver::segment_use_sensors() in core/divelist.cpp. The dc
 * never surfaces, so this isolates the bailout behaviour from the ambient
 * clamp.
 *
 * Phase 1 (0-600s): CCR at a commanded 1200 mbar setpoint, 3 cells
 * agreeing, exactly like testCcrSensorsAgreeUnchanged(); well above the
 * OTU/CNS 500 mbar threshold at 20m. At 600s, a real gas-switch event moves
 * the active cylinder to the bailout gas (EAN50) and the cells fall silent
 * (0 mbar) for the rest of the dive.
 *
 * Segment (0-600s) is the single transition segment: its own divemode
 * (resolved at its start sample, 0s) is still CCR (the switch has not
 * happened yet), so it stays on the sensor branch regardless of the bailout
 * fix; its end sample (600s) resolves through ccr_po2_loop's carry-forward
 * from the last real pre-bailout reading (1200 mbar), exactly the
 * single-segment staleness testCcrSensor0Dropout() already accepts -
 * unaffected by, and explicitly out of scope for, this fix.
 *
 * Phase 2 (600-1200s): OC on the bailout gas (EAN50) at the same 20m depth,
 * which is also comfortably above the threshold, but at a different PO2
 * (EAN50 at ~3034 mbar ambient =~ 1517 mbar) than the frozen pre-bailout
 * 1200 mbar, so the test is sensitive to which PO2 source segments
 * (600-900s) and (900-1200s) actually use.
 *
 * Without the bailout fix, use_sensors is a dc-level decision only, so
 * segments (600-900s) and (900-1200s) also stay on the sensor branch even
 * though their own divemode resolves to OC: ccr_po2_loop keeps carrying
 * forward the last known pre-bailout cell reading (1200 mbar, clamped to
 * ambient), so the whole dive after reaching depth computes as a flat 1200
 * mbar PO2, as if the bailout and the gas switch had never happened.
 * Captured against that code on this branch: otu == 26, maxcns == 9.
 *
 * With the bailout fix, segments (600-900s) and (900-1200s) resolve
 * divemode == OC with no raw cell data on their own end sample, so they
 * fall through to the active_o2()-based OC branch instead, using the
 * bailout gas's own, higher PO2 (=~1517 mbar) rather than the frozen 1200
 * mbar for those two segments, raising both OTU and CNS relative to the
 * frozen-PO2 result: otu == 31, maxcns == 14.
 */
void TestOtuCns::testCcrBailoutSilentCells()
{
	pressure_t setpoint = 1200_mbar;
	pressure_t cell = 1200_mbar;
	pressure_t zero = 0_mbar;
	struct gasmix bailoutGas = { 50_percent, 0_percent };
	std::vector<CcrPoint> points = {
		{ 0,    20.0, setpoint, { cell, cell, cell } },
		{ 600,  20.0, setpoint, { zero, zero, zero } },
		{ 900,  20.0, setpoint, { zero, zero, zero } },
		{ 1200, 20.0, setpoint, { zero, zero, zero } },
	};
	int otu, maxcns;
	recordAndGet(makeCcrBailoutDive(1, 1000000000, 1013_mbar, bailoutGas, 600, points), &otu, &maxcns);

	QCOMPARE(otu, 31);
	QCOMPARE(maxcns, 14);
}

QTEST_GUILESS_MAIN(TestOtuCns)
