// SPDX-License-Identifier: GPL-2.0
#include "testsurfacegf.h"
#include "core/deco.h"
#include "core/dive.h"
#include "core/divecomputer.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/gas.h"
#include "core/po2.h"
#include "core/pref.h"
#include "core/profile.h"
#include "core/sample.h"
#include "core/trip.h"
#include "core/units.h"
#include <cmath>

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

void TestSurfaceGf::testEndSurfaceGfSquareDive()
{
	clear_dive_file_data();
	prefs = default_prefs;
	prefs.planner_deco_mode = BUEHLMANN;
	auto d = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(d));
	const dive &logged = *divelog.dives[0];
	std::optional<double> endGf = divelog.dives.end_surface_gf(logged);
	QVERIFY(endGf.has_value());
	QVERIFY(*endGf > 0.0);

	plot_info pi = create_plot_info_new(&logged, &logged.dcs[0], nullptr);
	int last = -1;
	for (int i = 0; i < pi.nr; i++) {
		if (pi.entry[i].sec <= logged.totaltime().seconds)
			last = i;
	}
	QVERIFY(last >= 0);
	QVERIFY(std::abs(*endGf - pi.entry[last].surface_gf) <= 1.0);
}

void TestSurfaceGf::testEndSurfaceGfRepetitiveDive()
{
	clear_dive_file_data();
	prefs = default_prefs;
	prefs.planner_deco_mode = BUEHLMANN;
	const timestamp_t start = 1000000000;
	auto predecessor = makeSquareDive(1, start, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(predecessor));
	const timestamp_t secondStart = divelog.dives[0]->endtime() + 120;
	auto second = makeSquareDive(2, secondStart, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(second));
	dive_trip *trip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	trip->add_dive(divelog.dives[0].get());
	trip->add_dive(divelog.dives[1].get());
	std::optional<double> withPredecessor = divelog.dives.end_surface_gf(*divelog.dives[1]);
	QVERIFY(withPredecessor.has_value());

	clear_dive_file_data();
	auto alone = makeSquareDive(2, secondStart, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(alone));
	std::optional<double> withoutPredecessor = divelog.dives.end_surface_gf(*divelog.dives[0]);
	QVERIFY(withoutPredecessor.has_value());
	QVERIFY(*withPredecessor > *withoutPredecessor);
}

void TestSurfaceGf::testEndSurfaceGfNoSamples()
{
	clear_dive_file_data();
	auto d = std::make_unique<dive>();
	divelog.dives.record_dive(std::move(d));
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[0]).has_value());
}

void TestSurfaceGf::testEndSurfaceGfOverlappingDives()
{
	clear_dive_file_data();
	auto first = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(first));
	auto second = makeSquareDive(2, 1000000600, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(second));
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[1]).has_value());
}

/* Regression test for a review finding: the overlap check only looked at
 * whether an adjacent dive started at exactly the same instant, so it
 * covered the *later* of two staggered, overlapping dives (which
 * init_decompression() already flags via a negative computed surface
 * interval) and the case of two exactly simultaneous dives, but not the
 * *earlier* dive of a staggered, overlapping pair: init_decompression()'s
 * backward walk only ever considers dives starting strictly before this
 * one, so querying the earlier dive never reaches that negative-interval
 * check and used to report a value as if the later, overlapping dive did
 * not exist.
 */
void TestSurfaceGf::testEndSurfaceGfOverlappingDivesEarlierMember()
{
	clear_dive_file_data();
	auto first = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(first));
	auto second = makeSquareDive(2, 1000000600, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(second));
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[0]).has_value());
}

/* Regression test for a review finding: the overlap check only looked at
 * the immediately adjacent table entry on the earlier side, so a
 * long-running predecessor that is still "open" when the queried dive
 * starts went undetected whenever a short dive sat between them in the
 * table and had already ended by then. Using a different trip for the long
 * predecessor also rules out init_decompression()'s own machinery
 * incidentally catching the overlap, since it skips predecessors outside
 * the queried dive's trip entirely.
 */
void TestSurfaceGf::testEndSurfaceGfNonAdjacentOverlap()
{
	clear_dive_file_data();
	// A (long, no trip): 1000000000 .. 1000007200 (2h)
	auto a = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 7000, 7200);
	divelog.dives.record_dive(std::move(a));
	// B (short, intervening): starts inside A's interval, ends well before C.
	auto b = makeSquareDive(2, 1000000500, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(b));
	// C: starts after B ends, but still inside A's interval. B and C share a
	// trip; A does not belong to it.
	auto c = makeSquareDive(3, 1000001000, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(c));
	dive_trip *trip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	trip->add_dive(divelog.dives[1].get());
	trip->add_dive(divelog.dives[2].get());

	// Sanity check: B (the adjacent predecessor) must not itself overlap C,
	// or the existing adjacent-neighbour check alone would already catch
	// this and the test wouldn't be exercising the new code path.
	QVERIFY(divelog.dives[1]->endtime() <= divelog.dives[2]->when);
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[2]).has_value());
}

/* Pinning test for the overlaps_other_dive()-based refactor: one dive fully
 * contained inside another overlaps it just as much as a partial, staggered
 * overlap does - querying either dive of the pair must return no value.
 */
void TestSurfaceGf::testEndSurfaceGfOneDiveInsideAnother()
{
	clear_dive_file_data();
	// Outer (long): 1000000000 .. 1000007200 (2h)
	auto outer = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 7000, 7200);
	divelog.dives.record_dive(std::move(outer));
	// Inner: fully contained within the outer dive's interval.
	auto inner = makeSquareDive(2, 1000001000, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(inner));

	QVERIFY(divelog.dives[1]->when > divelog.dives[0]->when);
	QVERIFY(divelog.dives[1]->endtime() < divelog.dives[0]->endtime());
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[0]).has_value());
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[1]).has_value());
}

/* Pinning test: two earlier dives sharing the queried dive's own trip
 * overlap each other, but neither overlaps the queried dive itself, so
 * overlaps_other_dive() does not reject it directly. The rejection instead
 * has to come from init_decompression()'s own negative-surface-interval
 * detection while it walks the same-trip deco chain back through both
 * overlapping predecessors.
 */
void TestSurfaceGf::testEndSurfaceGfPredecessorsOverlapSameTrip()
{
	clear_dive_file_data();
	auto a = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 7000, 7200);
	divelog.dives.record_dive(std::move(a));
	auto b = makeSquareDive(2, 1000000500, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(b));
	auto d = makeSquareDive(3, 1000008000, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(d));
	dive_trip *trip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	trip->add_dive(divelog.dives[0].get());
	trip->add_dive(divelog.dives[1].get());
	trip->add_dive(divelog.dives[2].get());

	// Sanity: A and B really do overlap each other...
	QVERIFY(divelog.dives[1]->when < divelog.dives[0]->endtime());
	// ...but neither overlaps D itself, so the new check alone wouldn't
	// reject it.
	QVERIFY(divelog.dives[0]->endtime() <= divelog.dives[2]->when);
	QVERIFY(divelog.dives[1]->endtime() <= divelog.dives[2]->when);
	QVERIFY(!divelog.dives.overlaps_other_dive(*divelog.dives[2]));

	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[2]).has_value());
}

/* Pinning test: two dives that overlap each other sit in a *different* trip
 * from the queried dive. Overlap is checked trip-independently, but
 * init_decompression()'s own deco-chain walk is not, so the queried dive -
 * which does not itself overlap anything - must still get a value: the
 * other trip's internal overlap is none of its concern.
 */
void TestSurfaceGf::testEndSurfaceGfOtherTripOverlapDoesNotAffectDive()
{
	clear_dive_file_data();
	auto a = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 7000, 7200);
	divelog.dives.record_dive(std::move(a));
	auto b = makeSquareDive(2, 1000000500, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(b));
	auto d = makeSquareDive(3, 1000008000, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(d));
	dive_trip *otherTrip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	otherTrip->add_dive(divelog.dives[0].get());
	otherTrip->add_dive(divelog.dives[1].get());
	dive_trip *ownTrip = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	ownTrip->add_dive(divelog.dives[2].get());

	QVERIFY(!divelog.dives.overlaps_other_dive(*divelog.dives[2]));
	QVERIFY(divelog.dives.end_surface_gf(*divelog.dives[2]).has_value());
}

/* Pinning test: a dive in another trip, ending less than 48h before the
 * queried dive and not overlapping it, must not change the queried dive's
 * value at all. This is not new behaviour - init_decompression() only ever
 * carries tissue loading over from predecessors in the *same* trip, never
 * merely because they are close in time, matching the profile's pre-existing
 * semantics - it is pinned here to confirm the overlaps_other_dive()-based
 * refactor does not accidentally change it.
 */
void TestSurfaceGf::testEndSurfaceGfCrossTripWithin48hUnaffected()
{
	clear_dive_file_data();
	prefs = default_prefs;
	prefs.planner_deco_mode = BUEHLMANN;
	auto a = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(a));
	timestamp_t dWhen = divelog.dives[0]->endtime() + 120; // 2 minutes later, well within 48h
	auto d = makeSquareDive(2, dWhen, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(d));
	dive_trip *tripA = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	tripA->add_dive(divelog.dives[0].get());
	dive_trip *tripD = divelog.trips.put(std::make_unique<dive_trip>()).ptr;
	tripD->add_dive(divelog.dives[1].get());
	std::optional<double> withA = divelog.dives.end_surface_gf(*divelog.dives[1]);
	QVERIFY(withA.has_value());

	clear_dive_file_data();
	auto alone = makeSquareDive(2, dWhen, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(alone));
	std::optional<double> withoutA = divelog.dives.end_surface_gf(*divelog.dives[0]);
	QVERIFY(withoutA.has_value());

	QCOMPARE(*withA, *withoutA);
}

/* Pinning test for the overlaps_other_dive()-based refactor: a dive that is
 * not (yet) in the table is now actually checked for overlap against the
 * dives that are, instead of being silently skipped, as the old,
 * get_idx()-based neighbour check used to do whenever get_idx() returned
 * "not found".
 */
void TestSurfaceGf::testEndSurfaceGfNotInTableOverlapsTableEntry()
{
	clear_dive_file_data();
	auto inTable = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(inTable));

	// Not registered in divelog.dives at all.
	auto notInTable = makeSquareDive(2, 1000000300, 1013_mbar, 10_m, 30, 270, 300);
	QVERIFY(divelog.dives[0]->endtime() > notInTable->when); // really overlaps

	QVERIFY(!divelog.dives.end_surface_gf(*notInTable).has_value());
}

/* Regression test for a review finding on the overlap check above: two
 * dives starting at exactly the same instant overlap just as much as the
 * already-covered case of staggered, overlapping start times, but
 * init_decompression()'s backward and forward walks both stop on a
 * "when >= this dive's when" comparison, which treats an exactly
 * simultaneous dive as neither a predecessor nor a successor. Without an
 * explicit check, that silently skips the other dive and returns success
 * instead of reporting the overlap.
 */
void TestSurfaceGf::testEndSurfaceGfSimultaneousDives()
{
	clear_dive_file_data();
	auto first = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);
	divelog.dives.record_dive(std::move(first));
	auto second = makeSquareDive(2, 1000000000, 1013_mbar, 10_m, 30, 270, 300);
	divelog.dives.record_dive(std::move(second));
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[0]).has_value());
	QVERIFY(!divelog.dives.end_surface_gf(*divelog.dives[1]).has_value());
}

/* Regression test for a review finding: add_dive_to_deco() only ever walks
 * dcs[0]'s own samples, so it cannot know about an interval between dcs[0]'s
 * last sample and dive.totaltime() (the maximum over *all* dive computers).
 * That gap is normally zero or negative, because dc_totaltime() deliberately
 * discards dcs[0]'s own trailing surface samples - but a second dive
 * computer that recorded a longer dive (disagreeing dive computers being the
 * most plausible real-world cause) can make it positive. Simulating only up
 * to dcs[0]'s last sample would then report a Surface GF frozen at that
 * point, silently ignoring whatever happened - in or out of the water -
 * during the unaccounted interval. end_surface_gf() must decline to answer
 * instead.
 */
void TestSurfaceGf::testEndSurfaceGfUnaccountedMultiDcTail()
{
	clear_dive_file_data();
	prefs = default_prefs;
	prefs.planner_deco_mode = BUEHLMANN;
	auto d = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 540, 660);

	// A second dive computer claims the diver was still at depth well past
	// dcs[0]'s own last sample (660), stretching dive.totaltime() out to 900.
	struct divecomputer dc2;
	dc2.model = "subsurface test dc 2";
	dc2.when = 1000000000;
	dc2.surface_pressure = 1013_mbar;
	dc2.divemode = OC;
	struct sample *s;
	s = prepare_sample(&dc2);
	s->time.seconds = 0;
	s->depth.mm = 0;
	s = prepare_sample(&dc2);
	s->time.seconds = 120;
	s->depth.mm = 30000;
	s = prepare_sample(&dc2);
	s->time.seconds = 540;
	s->depth.mm = 30000;
	s = prepare_sample(&dc2);
	s->time.seconds = 870;
	s->depth.mm = 30000;
	s = prepare_sample(&dc2);
	s->time.seconds = 900;
	s->depth.mm = 0;
	dc2.duration.seconds = 900;
	d->dcs.push_back(dc2);
	divelog.dives.record_dive(std::move(d));
	const dive &logged = *divelog.dives[0];

	// Sanity check: this fixture has to actually reproduce the gap this
	// test is about, or the assertion below would pass trivially.
	QVERIFY(logged.dcs[0].samples.back().time.seconds < logged.totaltime().seconds);
	QVERIFY(!divelog.dives.end_surface_gf(logged).has_value());
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

/* Append extraTailSamples additional trailing surface (depth 0) samples,
 * tailInterval seconds apart, after a dive's existing last sample (which
 * must already be a surface sample, e.g. the final sample produced by
 * makeSquareDive()/makeCcrSquareDive()). Real dive computers routinely keep
 * logging like this for a while after the diver has actually surfaced;
 * dc_totaltime()/dive::totaltime() deliberately discard all but the first
 * of these, so varying extraTailSamples alone must never change anything
 * derived from this dive's own end time - including what gets carried over
 * to the next dive.
 */
static void appendTrailingSurfaceTail(struct dive &d, int extraTailSamples, int tailInterval)
{
	struct divecomputer &dc = d.dcs[0];
	int lastTime = dc.samples.back().time.seconds;
	for (int i = 1; i <= extraTailSamples; i++) {
		struct sample *s = prepare_sample(&dc);
		s->time.seconds = lastTime + i * tailInterval;
		s->depth.mm = 0;
	}
}

/* Regression test for the upstream report's underlying time-accounting bug:
 * add_dive_to_deco() used to walk every raw sample of a dive, with no
 * notion of where the dive "really" ends, while init_decompression() -
 * right afterwards, for the very same dive - separately works out the
 * surface interval before the next dive from dive::endtime() (which, via
 * dc_totaltime(), deliberately discards all but the first of a dive
 * computer's trailing surface samples). Whenever a dive computer kept
 * logging for a while after the diver surfaced, that trailing tail was
 * simulated twice: once (in full) by add_dive_to_deco(), and again as part
 * of the surface segment, double-counting it as extra off-gassing applied
 * right after surfacing - where it has the largest, and understating,
 * effect on the Surface GF carried into the next dive.
 *
 * This drives the invariant directly: with a fixed real surface interval
 * (dive 2 always starts the same fixed time after dive 1 starts), growing
 * dive 1's trailing surface tail must not change dive 2's starting Surface
 * GF at all, since neither dive::endtime() nor the true elapsed time
 * between the dives depends on that tail's length.
 */
void TestSurfaceGf::testSurfaceTailLengthInvarianceOc()
{
	auto run = [&](int extraTailSamples) {
		clear_dive_file_data();
		prefs = default_prefs;
		prefs.planner_deco_mode = BUEHLMANN;
		set_gf(30, 70);

		// Last in-water sample at 5390s, first (bridging) surface sample at
		// 5400s - mirroring the shape of the upstream report's file, just
		// with round numbers.
		auto d1 = makeSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 5390, 5400);
		appendTrailingSurfaceTail(*d1, extraTailSamples, 10);
		divelog.dives.record_dive(std::move(d1));

		// Dive 2 starts a fixed 400s after dive 1 ends (per dive 1's own,
		// tail-independent end time), regardless of how long dive 1's
		// trailing tail is.
		timestamp_t dive2When = divelog.dives[0]->endtime() + 400;
		auto d2 = makeSquareDive(2, dive2When, 1013_mbar, 10_m, 30, 270, 300);
		divelog.dives.record_dive(std::move(d2));

		struct plot_info pi2 = create_plot_info_new(divelog.dives[1].get(), &divelog.dives[1]->dcs[0], nullptr);
		return pi2.entry[0].surface_gf;
	};

	double shortTail = run(0);
	double longTail = run(29); // adds 290s of trailing surface samples

	// Sanity check: this profile has to produce meaningful residual
	// nitrogen loading, or the regression below would pass trivially.
	QVERIFY(shortTail > 1.0);
	// The actual regression: the length of dive 1's trailing surface tail
	// must not affect dive 2's starting Surface GF at all.
	QCOMPARE(longTail, shortTail);
}

/* CCR companion to testSurfaceTailLengthInvarianceOc(): the same invariant,
 * to pin that the fix is not divemode-specific. */
void TestSurfaceGf::testSurfaceTailLengthInvarianceCcr()
{
	pressure_t setpoint = 1300_mbar;

	auto run = [&](int extraTailSamples) {
		clear_dive_file_data();
		prefs = default_prefs;
		prefs.planner_deco_mode = BUEHLMANN;
		set_gf(30, 70);

		auto d1 = makeCcrSquareDive(1, 1000000000, 1013_mbar, 30_m, 120, 5390, 5400,
					     setpoint, 0, nullptr);
		appendTrailingSurfaceTail(*d1, extraTailSamples, 10);
		divelog.dives.record_dive(std::move(d1));

		timestamp_t dive2When = divelog.dives[0]->endtime() + 400;
		auto d2 = makeSquareDive(2, dive2When, 1013_mbar, 10_m, 30, 270, 300);
		divelog.dives.record_dive(std::move(d2));

		struct plot_info pi2 = create_plot_info_new(divelog.dives[1].get(), &divelog.dives[1]->dcs[0], nullptr);
		return pi2.entry[0].surface_gf;
	};

	double shortTail = run(0);
	double longTail = run(29);

	QVERIFY(shortTail > 1.0);
	QCOMPARE(longTail, shortTail);
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
	QVERIFY(!dive2->dcs[0].samples.empty());
	QVERIFY(pi2.nr > 2);
	// Entries 0 and 1 precede the sample stream; entry 2 is its first sample.
	const struct plot_data &dive2Start = pi2.entry[2];

	// Plot index 0 must no longer be stuck at the stale 0.0 default: it has
	// to reflect the tissue state carried over from dive 1.
	QVERIFY(pi2.entry[0].surface_gf > 1.0);
	QVERIFY(dive2Start.surface_gf > 1.0);
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
