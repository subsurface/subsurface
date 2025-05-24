// SPDX-License-Identifier: GPL-2.0
/* planner.cpp
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "dive.h"
#include "divelist.h" // for init_decompression()
#include "divelog.h"
#include "sample.h"
#include "subsurface-string.h"
#include "deco.h"
#include "errorhelper.h"
#include "event.h"
#include "interpolate.h"
#include "planner.h"
#include "range.h"
#include "subsurface-time.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"
#include "qthelper.h"
#include "version.h"

//#define DEBUG_PLAN 255

static constexpr int base_timestep = 2; // seconds

static std::vector<depth_t> decostoplevels_metric = { 0_m, 3_m, 6_m, 9_m, 12_m, 15_m, 18_m, 21_m, 24_m, 27_m,
					30_m, 33_m, 36_m, 39_m, 42_m, 45_m, 48_m, 51_m, 54_m, 57_m,
					60_m, 63_m, 66_m, 69_m, 72_m, 75_m, 78_m, 81_m, 84_m, 87_m,
					90_m, 100_m, 110_m, 120_m, 130_m, 140_m, 150_m, 160_m, 170_m,
					180_m, 190_m, 200_m, 220_m, 240_m, 260_m, 280_m, 300_m,
					320_m, 340_m, 360_m, 380_m };
// Note: for fractional feet (e.g. 333.33 ft), we use mm, since currently our custom depth literals only support integers.
static std::vector<depth_t> decostoplevels_imperial = { 0_ft, 10_ft, 20_ft, 30_ft, 40_ft, 50_ft, 60_ft, 70_ft, 80_ft, 90_ft,
					100_ft, 110_ft, 120_ft, 130_ft, 140_ft, 150_ft, 160_ft, 170_ft, 180_ft, 190_ft,
					200_ft, 210_ft, 220_ft, 230_ft, 240_ft, 250_ft, 260_ft, 270_ft, 280_ft, 290_ft,
					300_ft, 333_ft, 367_ft, 400_ft, 433_ft, 467_ft, 500_ft, 533_ft, 567_ft,
					600_ft, 633_ft, 667_ft, 733_ft, 800_ft, 867_ft, 933_ft, 1000_ft,
					1067_ft, 1133_ft, 1200_ft, 1267_ft };

#if DEBUG_PLAN
void dump_plan(const struct diveplan &diveplan)
{
	struct tm tm;
	utc_mkdate(diveplan.when, &tm);

	printf("\nDiveplan @ %04d-%02d-%02d %02d:%02d:%02d (surfpres %dmbar):\n",
	       tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec,
	       diveplan.surface_pressure.mbar);
	for (auto &dp: diveplan.dp) {
		printf("\t%3u:%02u: %6dmm cylid: %2d setpoint: %d\n", FRACTION_TUPLE(dp.time, 60), dp.depth.mm, dp.cylinderid, dp.setpoint);
	}
}
#endif

diveplan::diveplan()
{
}

diveplan::~diveplan()
{
}

bool diveplan::is_empty() const
{
	return std::none_of(dp.begin(), dp.end(), [](const divedatapoint &dp) { return dp.time != 0; });
}

/* get the cylinder index at a certain time during the dive */
int get_cylinderid_at_time(struct dive *dive, struct divecomputer *dc, duration_t time)
{
	// we start with the first cylinder unless an event tells us otherwise
	int cylinder_idx = 0;
	for (const auto &event: dc->events) {
		if (event.time.seconds > time.seconds)
			break;
		if (event.name == "gaschange")
			cylinder_idx = dive->get_cylinder_index(event, *dc);
	}
	return cylinder_idx;
}

static int get_gasidx(struct dive *dive, struct gasmix mix)
{
	return find_best_gasmix_match(mix, dive->cylinders, nullptr);
}

static void interpolate_transition(struct deco_state *ds, struct dive *dive, duration_t t0, duration_t t1, depth_t d0, depth_t d1, struct gasmix gasmix, o2pressure_t po2, enum divemode_t divemode)
{
	int32_t j;

	for (j = t0.seconds; j < t1.seconds; j++) {
		depth_t depth = interpolate(d0, d1, j - t0.seconds, t1.seconds - t0.seconds);
		add_segment(ds, dive->depth_to_bar(depth), gasmix, 1, po2.mbar, divemode, prefs.bottomsac, true);
	}
	if (d1.mm > d0.mm)
		calc_crushing_pressure(ds, dive->depth_to_bar(d1));
}

/* returns the tissue tolerance at the end of this (partial) dive */
static int tissue_at_end(struct deco_state *ds, struct dive *dive, const struct divecomputer *dc, deco_state_cache &cache)
{
	depth_t lastdepth;
	duration_t t0;
	int surface_interval = 0;

	if (!dive)
		return 0;
	if (cache) {
		cache.restore(ds, true);
	} else {
		surface_interval = divelog.dives.init_decompression(ds, dive, true);
		cache.cache(ds);
	}
	if (dc->samples.empty())
		return 0;

	const struct sample *psample = nullptr;
	gasmix_loop loop_gas(*dive, *dc);
	divemode_loop loop_mode(*dc);
	for (auto &sample: dc->samples) {
		o2pressure_t setpoint = psample ? psample->setpoint
						: sample.setpoint;

		duration_t t1 = sample.time;
		struct gasmix gas = dive->get_gasmix_at_time(*dc, t0);
		if (psample)
			lastdepth = psample->depth;

		/* The ceiling in the deeper portion of a multilevel dive is sometimes critical for the VPM-B
		 * Boyle's law compensation.  We should check the ceiling prior to ascending during the bottom
		 * portion of the dive.  The maximum ceiling might be reached while ascending, but testing indicates
		 * that it is only marginally deeper than the ceiling at the start of ascent.
		 * Do not set the first_ceiling_pressure variable (used for the Boyle's law compensation calculation)
		 * at this stage, because it would interfere with calculating the ceiling at the end of the bottom
		 * portion of the dive.
		 * Remember the value for later.
		 */
		if ((decoMode(true) == VPMB) && (lastdepth.mm > sample.depth.mm)) {
			pressure_t ceiling_pressure;
			nuclear_regeneration(ds, t0.seconds);
			vpmb_start_gradient(ds);
			ceiling_pressure.mbar = dive->depth_to_mbar(deco_allowed_depth(tissue_tolerance_calc(ds, dive,
													dive->depth_to_bar(lastdepth), true),
										dive->surface_pressure.mbar / 1000.0,
										dive,
										1));
			if (ceiling_pressure.mbar > ds->max_bottom_ceiling_pressure.mbar)
				ds->max_bottom_ceiling_pressure.mbar = ceiling_pressure.mbar;
		}

		[[maybe_unused]] auto [divemode, _cylinder_index, _gasmix] = get_dive_status_at(*dive, *dc, t0.seconds + 1, &loop_mode, &loop_gas);

		interpolate_transition(ds, dive, t0, t1, lastdepth, sample.depth, gas, setpoint, divemode);
		psample = &sample;
		t0 = t1;
	}
	return surface_interval;
}

/* calculate the new end pressure of the cylinder, based on its current end pressure and the
 * latest segment. */
static void update_cylinder_pressure(struct dive *d, int old_depth, int new_depth, int duration, int sac, cylinder_t *cyl, bool in_deco, enum divemode_t divemode)
{
	volume_t gas_used;
	pressure_t delta_p;
	depth_t mean_depth;
	int factor = 1000;

	if (divemode == PSCR)
		factor = prefs.pscr_ratio;

	if (!cyl)
		return;
	mean_depth.mm = (old_depth + new_depth) / 2;
	gas_used.mliter = lrint(d->depth_to_atm(mean_depth) * sac / 60 * duration * factor / 1000);
	cyl->gas_used += gas_used;
	if (in_deco)
		cyl->deco_gas_used += gas_used;
	if (cyl->type.size.mliter) {
		delta_p.mbar = lrint(gas_used.mliter * 1000.0 / cyl->type.size.mliter * gas_compressibility_factor(cyl->gasmix, cyl->end.mbar / 1000.0));
		cyl->end -= delta_p;
	}
}

static struct sample *create_sample(struct divecomputer &dc, int time, depth_t depth, bool entered)
{
	struct sample *sample = prepare_sample(&dc);
	sample->time.seconds = time;
	sample->depth = depth;
	sample->manually_entered = entered;
	sample->sac.mliter = entered ? prefs.bottomsac : prefs.decosac;

	return sample;
}

/* overwrite the data in dive
 * return false if something goes wrong */
static void create_dive_from_plan(struct diveplan &diveplan, struct dive *dive, struct divecomputer *dc, bool track_gas)
{
	cylinder_t *cyl;
	int oldpo2 = 0;
	int lasttime = 0, last_manual_point = 0;
	depth_t lastdepth;
	int lastcylid;
	enum divemode_t type = dc->divemode;

	if (diveplan.dp.empty())
		return;
#if DEBUG_PLAN & 4
	printf("in create_dive_from_plan\n");
	dump_plan(diveplan);
#endif
	dive->salinity = diveplan.salinity;
	// reset the cylinders and clear out the samples and events of the
	// dive-to-be-planned so we can restart
	reset_cylinders(dive, track_gas);
	dc->when = dive->when = diveplan.when;
	dc->surface_pressure = diveplan.surface_pressure;
	dc->salinity = diveplan.salinity;
	dc->samples.clear();
	dc->events.clear();
	/* Create first sample at time = 0, not based on dp because
	 * there is no real dp for time = 0, set first cylinder to 0
	 * O2 setpoint for this sample will be filled later from next dp */
	cyl = dive->get_or_create_cylinder(0);
	struct sample *sample = prepare_sample(dc);
	sample->sac.mliter = prefs.bottomsac;
	if (track_gas && cyl->type.workingpressure.mbar)
		sample->pressure[0].mbar = cyl->end.mbar;
	sample->manually_entered = true;
	lastcylid = 0;
	for (auto &dp: diveplan.dp) {
		int po2 = dp.setpoint;
		int time = dp.time;
		depth_t depth = dp.depth;

		if (time == 0) {
			/* special entries that just inform the algorithm about
			 * additional gases that are available */
			continue;
		}

		/* Check for SetPoint change */
		if (oldpo2 != po2) {
			/* this is a bad idea - we should get a different SAMPLE_EVENT type
			 * reserved for this in libdivecomputer... overloading SMAPLE_EVENT_PO2
			 * with a different meaning will only cause confusion elsewhere in the code */
			if (dp.divemode == type)
				add_event(dc, lasttime, SAMPLE_EVENT_PO2, 0, po2, QT_TRANSLATE_NOOP("gettextFromC", "SP change"));
			oldpo2 = po2;
		}

		/* Make sure we have the new gas, and create a gas change event */
		if (dp.cylinderid != lastcylid) {
			/* need to insert a first sample for the new gas */
			add_gas_switch_event(dive, dc, lasttime + 1, dp.cylinderid);
			cyl = dive->get_cylinder(dp.cylinderid); // FIXME: This actually may get one past the last cylinder for "surface air".
			if (!cyl) {
				report_error("Invalid cylinder in create_dive_from_plan(): %d", dp.cylinderid);
				continue;
			}

			sample->setpoint.mbar = po2;
			if (po2)
				sample->o2sensor[0].mbar = po2;
			type = get_effective_divemode(*dc, *cyl);

			sample = create_sample(*dc, lasttime + 1, lastdepth, dp.entered);

			lastcylid = dp.cylinderid;
		}
		if (dp.divemode != type) {
			type = dp.divemode;
			if ((dc->divemode == CCR && prefs.allowOcGasAsDiluent && cyl->cylinder_use == OC_GAS) || dc->divemode == PSCR)
				add_event(dc, lasttime, SAMPLE_EVENT_BOOKMARK, 0, type, "modechange");
		}

		/* set po2 at beginning of this segment */
		/* and keep it valid for last sample - where it likely doesn't matter */
		sample->setpoint.mbar = po2;

		sample = create_sample(*dc, time, depth, dp.entered);
		sample->setpoint.mbar = po2;
		if (dp.entered)
			last_manual_point = time;
		lastdepth = depth;
		lasttime = time;

		if (track_gas) {
			if (!sample[-1].setpoint.mbar)    /* Don't track gas usage for CCR legs of dive */
				update_cylinder_pressure(dive, sample[-1].depth.mm, depth.mm, time - sample[-1].time.seconds,
					dp.entered ? diveplan.bottomsac : diveplan.decosac, cyl, !dp.entered, type);
			if (cyl->type.workingpressure.mbar)
				sample->pressure[0].mbar = cyl->end.mbar;
		}
	}
	dc->last_manual_time.seconds = last_manual_point;

#if DEBUG_PLAN & 32
	save_dive(stdout, *dive, false);
#endif
	return;
}

divedatapoint::divedatapoint(int time_incr, depth_t depth, int cylinderid, int po2, bool entered) :
	time(time_incr),
	depth(depth),
	cylinderid(cylinderid),
	minimum_gas(0_bar),
	setpoint(po2),
	entered(entered),
	divemode(OC)
{
}

static void add_to_end_of_diveplan(struct diveplan &diveplan, const struct divedatapoint &dp)
{
	auto maxtime = std::max_element(diveplan.dp.begin(), diveplan.dp.end(),
		[] (const divedatapoint &p1, const divedatapoint &p2)
		{ return p1.time < p2.time; });
	int lasttime = maxtime != diveplan.dp.end() ? maxtime->time : 0;
	diveplan.dp.push_back(dp);
	diveplan.dp.back().time += lasttime;
}

void plan_add_segment(struct diveplan &diveplan, int duration, depth_t depth, int cylinderid, int po2, bool entered, enum divemode_t divemode)
{
	struct divedatapoint dp(duration, depth, cylinderid, divemode == CCR ? po2 : 0, entered);
	dp.divemode = divemode;
	add_to_end_of_diveplan(diveplan, dp);
}

struct gaschanges {
	int depth;
	int gasidx;
};

// Return new setpoint if cylinderi is a setpoint change an 0 if not

static int setpoint_change(struct dive *dive, int cylinderid)
{
	cylinder_t *cylinder = dive->get_cylinder(cylinderid);
	if (cylinder->type.description.empty())
		return 0;
	if (starts_with(cylinder->type.description, "SP ")) {
		float sp;
		sscanf(cylinder->type.description.c_str() + 3, "%f", &sp);
		return (int) (sp * 1000.0);
	} else {
		return 0;
	}
}

static std::vector<gaschanges> analyze_gaslist(const struct diveplan &diveplan, struct dive *dive, int dcNr,
		int depth, int *asc_cylinder, bool ccr, bool &inappropriate_cylinder_use)
{
	size_t nr = 0;
	std::vector<gaschanges> gaschanges;
	const struct divedatapoint *best_ascent_dp = nullptr;
	bool total_time_zero = true;
	const divecomputer *dc = dive->get_dc(dcNr);
	for (auto &dp: diveplan.dp) {
		inappropriate_cylinder_use = inappropriate_cylinder_use || !is_cylinder_use_appropriate(*dc, *dive->get_cylinder(dp.cylinderid), false);

		if (dp.time == 0 && total_time_zero && (ccr == (bool) setpoint_change(dive, dp.cylinderid))) {
			if (dp.depth.mm <= depth) {
				int i = 0;
				nr++;
				gaschanges.resize(nr);
				while (i < static_cast<int>(nr) - 1) {
					if (dp.depth.mm < gaschanges[i].depth) {
						for (int j = static_cast<int>(nr) - 2; j >= i; j--)
							gaschanges[j + 1] = gaschanges[j];
						break;
					}
					i++;
				}
				gaschanges[i].depth = dp.depth.mm;
				gaschanges[i].gasidx = dp.cylinderid;
				assert(gaschanges[i].gasidx != -1);
			} else {
				/* is there a better mix to start deco? */
				if (!best_ascent_dp || dp.depth.mm < best_ascent_dp->depth.mm) {
					best_ascent_dp = &dp;
				}
			}
		} else {
			total_time_zero = false;
		}
	}
	if (best_ascent_dp)
		*asc_cylinder = best_ascent_dp->cylinderid;
#if DEBUG_PLAN & 16
	for (size_t nr = 0; nr < gaschanges.size(); nr++) {
		int idx = gaschanges[nr].gasidx;
		printf("gaschange nr %zu: @ %5.2lfm gasidx %d (%s)\n", nr, gaschanges[nr].depth / 1000.0,
		       idx, dive->get_cylinder(idx)->gasmix.name().c_str());
	}
#endif
	return gaschanges;
}

/* sort all the stops into one ordered list */
static std::vector<depth_t> sort_stops(const std::vector<depth_t> &dstops, size_t dnr, const std::vector<gaschanges> &gstops)
{
	int total = static_cast<int>(dnr + gstops.size());
	std::vector<depth_t> stoplevels(total);

	/* Can't happen. */
	if (dnr == 0)
		return {};

	/* no gaschanges */
	if (gstops.empty()) {
		std::copy(dstops.begin(), dstops.begin() + dnr, stoplevels.begin());
		return stoplevels;
	}
	int i = static_cast<int>(total) - 1;
	int gi = static_cast<int>(gstops.size()) - 1;
	int di = static_cast<int>(dnr) - 1;
	while (i >= 0) {
		if (dstops[di].mm > gstops[gi].depth) {
			stoplevels[i] = dstops[di];
			di--;
		} else if (dstops[di].mm == gstops[gi].depth) {
			stoplevels[i] = dstops[di];
			di--;
			gi--;
		} else {
			stoplevels[i].mm = gstops[gi].depth;
			gi--;
		}
		i--;
		if (di < 0) {
			while (gi >= 0)
				stoplevels[i--].mm = gstops[gi--].depth;
			break;
		}
		if (gi < 0) {
			while (di >= 0)
				stoplevels[i--] = dstops[di--];
			break;
		}
	}
	while (i >= 0)
		stoplevels[i--].mm = 0;

#if DEBUG_PLAN & 16
	int k;
	for (k = static_cast<int>(gstops.size()) + dnr - 1; k >= 0; k--) {
		printf("stoplevel[%d]: %5.2lfm\n", k, stoplevels[k].mm / 1000.0);
		if (stoplevels[k].mm == 0)
			break;
	}
#endif
	return stoplevels;
}

int ascent_velocity(depth_t depth, depth_t avg_depth, int)
{
	/* We need to make this configurable */

	/* As an example (and possibly reasonable default) this is the Tech 1 provedure according
	 * to http://www.globalunderwaterexplorers.org/files/Standards_and_Procedures/SOP_Manual_Ver2.0.2.pdf */

	if (depth.mm * 4 > avg_depth.mm * 3) {
		return prefs.ascrate75;
	} else {
		if (depth.mm * 2 > avg_depth.mm) {
			return prefs.ascrate50;
		} else {
			if (depth.mm > 6000)
				return prefs.ascratestops;
			else
				return prefs.ascratelast6m;
		}
	}
}

static void track_ascent_gas(depth_t depth, struct dive *dive, int cylinder_id, depth_t avg_depth, int bottom_time, bool safety_stop, enum divemode_t divemode)
{
	cylinder_t *cylinder = dive->get_cylinder(cylinder_id);
	while (depth.mm > 0) {
		int deltad = ascent_velocity(depth, avg_depth, bottom_time) * base_timestep;
		if (deltad > depth.mm)
			deltad = depth.mm;
		update_cylinder_pressure(dive, depth.mm, depth.mm - deltad, base_timestep, prefs.decosac, cylinder, true, divemode);
		if (depth.mm <= 5000 && depth.mm >= (5000 - deltad) && safety_stop) {
			update_cylinder_pressure(dive, 5000, 5000, 180, prefs.decosac, cylinder, true, divemode);
			safety_stop = false;
		}
		depth.mm -= deltad;
	}
}

// Determine whether ascending to the next stop will break the ceiling.  Return true if the ascent is ok, false if it isn't.
static bool trial_ascent(struct deco_state *ds, int wait_time, depth_t trial_depth, depth_t stoplevel, depth_t avg_depth, int bottom_time, struct gasmix gasmix, int po2, double surface_pressure, struct dive *dive, enum divemode_t divemode)
{

	bool clear_to_ascend = true;
	deco_state_cache trial_cache;

	// For consistency with other VPM-B implementations, we should not start the ascent while the ceiling is
	// deeper than the next stop (thus the offgasing during the ascent is ignored).
	// However, we still need to make sure we don't break the ceiling due to on-gassing during ascent.
	trial_cache.cache(ds);
	if (wait_time)
		add_segment(ds, dive->depth_to_bar(trial_depth),
			    gasmix,
			    wait_time, po2, divemode, prefs.decosac, true);
	if (decoMode(true) == VPMB) {
		double tolerance_limit = tissue_tolerance_calc(ds, dive, dive->depth_to_bar(stoplevel), true);
		update_regression(ds, dive);
		if (deco_allowed_depth(tolerance_limit, surface_pressure, dive, 1).mm > stoplevel.mm) {
			trial_cache.restore(ds, false);
			return false;
		}
	}

	while (trial_depth.mm > stoplevel.mm) {
		double tolerance_limit;
		int deltad = ascent_velocity(trial_depth, avg_depth, bottom_time) * base_timestep;
		if (deltad > trial_depth.mm) /* don't test against depth above surface */
			deltad = trial_depth.mm;
		add_segment(ds, dive->depth_to_bar(trial_depth),
			    gasmix,
			    base_timestep, po2, divemode, prefs.decosac, true);
		tolerance_limit = tissue_tolerance_calc(ds, dive, dive->depth_to_bar(trial_depth), true);
		if (decoMode(true) == VPMB)
			update_regression(ds, dive);
		if (deco_allowed_depth(tolerance_limit, surface_pressure, dive, 1).mm > trial_depth.mm - deltad) {
			/* We should have stopped */
			clear_to_ascend = false;
			break;
		}
		trial_depth.mm -= deltad;
	}
	trial_cache.restore(ds, false);
	return clear_to_ascend;
}

/* Determine if there is enough gas for the dive.  Return true if there is enough.
 * Also return true if this cannot be calculated because the cylinder doesn't have
 * size or a starting pressure.
 */
static bool enough_gas(const struct dive *dive, int current_cylinder)
{
	if (current_cylinder < 0 || static_cast<size_t>(current_cylinder) >= dive->cylinders.size())
		return false;
	const cylinder_t *cyl = dive->get_cylinder(current_cylinder);

	if (!cyl->start.mbar)
		return true;
	if (cyl->type.size.mliter)
		return (cyl->end.mbar - prefs.reserve_gas) / 1000.0 * cyl->type.size.mliter > cyl->deco_gas_used.mliter;
	else
		return true;
}

/* Do a binary search for the time the ceiling is clear to ascent to target_depth.
 * Minimal solution is min + 1, and the solution should be an integer multiple of stepsize.
 * leap is a guess for the maximum but there is no guarantee that leap is an upper limit.
 * So we always test at the upper bundary, not in the middle!
 */
static int wait_until(struct deco_state *ds, struct dive *dive, int clock, int min, int leap, int stepsize, depth_t depth, depth_t target_depth, depth_t avg_depth, int bottom_time, struct gasmix gasmix, int po2, double surface_pressure, enum divemode_t divemode)
{
	// When a deco stop exceeds two days, there is something wrong...
	if (min >= 48 * 3600)
		return 50 * 3600;
	// Round min + leap up to the next multiple of stepsize
	int upper = min + leap + stepsize - 1 - (min + leap - 1) % stepsize;
	// Is the upper boundary too small?
	if (!trial_ascent(ds, upper - clock, depth, target_depth, avg_depth, bottom_time, gasmix, po2, surface_pressure, dive, divemode))
		return wait_until(ds, dive, clock, upper, leap, stepsize, depth, target_depth, avg_depth, bottom_time, gasmix, po2, surface_pressure, divemode);

	if (upper - min <= stepsize)
		return upper;

	return wait_until(ds, dive, clock, min, leap / 2, stepsize, depth, target_depth, avg_depth, bottom_time, gasmix, po2, surface_pressure, divemode);
}

// returns an (average_depth, maximum_depth) pair
static std::pair<depth_t, depth_t> average_max_depth(const struct diveplan &dive)
{
	depth_t integral; // Strictly speaking not a depth, but depth × time. Might want to define a custom time for that.
	depth_t last_depth, max_depth;
	int last_time = 0;

	for (auto &dp: dive.dp) {
		if (dp.time) {
			/* Ignore gas indication samples */
			integral += (dp.depth + last_depth) * (dp.time - last_time) / 2;
			last_time = dp.time;
			last_depth = dp.depth;
			if (dp.depth.mm > max_depth.mm)
				max_depth = dp.depth;
		}
	}
	if (last_time)
		return { integral / last_time, max_depth };
	return { 0_m, 0_m };
}

std::vector<decostop> plan(struct deco_state *ds, struct diveplan &diveplan, struct dive *dive, int dcNr, int timestep, deco_state_cache &cache, bool is_planner, bool show_disclaimer)
{

	int bottom_gi;
	int bottom_stopidx;
	bool is_final_plan = true;
	int bottom_time;
	int previous_deco_time;
	deco_state_cache bottom_cache;
	int po2;
	int transitiontime, gi;
	int stop_cylinder;
	size_t stopidx;
	bool stopping = false;
	bool pendinggaschange = false;
	int clock, previous_point_time;
	int last_ascend_rate;
	int best_first_ascend_cylinder = -1;
	struct gasmix gas, bottom_gas;
	bool o2break_next = false;
	int break_cylinder = -1, breakfrom_cylinder = 0;
	bool last_segment_min_switch = false;
	planner_error_t error = PLAN_OK;
	depth_t first_stop_depth;
	int laststoptime = timestep;
	bool o2breaking = false;
	struct divecomputer *dc = dive->get_dc(dcNr);

	set_gf(diveplan.gflow, diveplan.gfhigh);
	set_vpmb_conservatism(diveplan.vpmb_conservatism);

	if (diveplan.surface_pressure.mbar == 0) {
		// Lets use dive's surface pressure in planner, if have one...
		if (dc->surface_pressure.mbar != 0) { // First from DC...
			diveplan.surface_pressure = dc->surface_pressure;
		} else if (dive->surface_pressure.mbar != 0) { // After from user...
			diveplan.surface_pressure = dive->surface_pressure;
		} else {
			diveplan.surface_pressure = 1_atm;
		}
	}

	clear_deco(ds, dive->surface_pressure.mbar / 1000.0, true);
	ds->max_bottom_ceiling_pressure = ds->first_ceiling_pressure = 0_bar;
	create_dive_from_plan(diveplan, dive, dc, is_planner);

	// Do we want deco stop array in metres or feet?
	auto &decostoplevels = prefs.units.length == units::METERS ?
		decostoplevels_metric : decostoplevels_imperial;

	/* If the user has selected last stop to be at 6m/20', we need to get rid of the 3m/10' stop.
	 * Otherwise reinstate the last stop 3m/10' stop.
	 * Remark: not reentrant, but the user probably won't change preferences while this is running.
	 */
	if (prefs.last_stop)
		decostoplevels[1] = 0_m;
	else
		decostoplevels[1] = m_or_ft(3, 10);

	/* Let's start at the last 'sample', i.e. the last manually entered waypoint. */
	const struct sample &sample = dc->samples.back();

	/* Keep time during the ascend */
	bottom_time = clock = previous_point_time = sample.time.seconds;

	// Find the divemode at the end of the dive
	[[maybe_unused]] auto [divemode, current_cylinder, gasmix] = get_dive_status_at(*dive, *dc, bottom_time);
	gas = *gasmix;

	po2 = sample.setpoint.mbar;
	depth_t depth = sample.depth;
	auto [avg_depth, max_depth] = average_max_depth(diveplan);
	last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);

	/* if all we wanted was the dive just get us back to the surface */
	if (!is_planner) {
		/* Attn: for manually entered dives, we depend on the last segment having the
		 * same ascent rate as in fake_dc(). If you change it here, also change it there.
		 */
		transitiontime = lrint(depth.mm / (double)prefs.ascratelast6m);
		plan_add_segment(diveplan, transitiontime, 0_m, current_cylinder, po2, false, divemode);
		create_dive_from_plan(diveplan, dive, dc, is_planner);
		return {};
	}

#if DEBUG_PLAN & 4
	printf("gas %s\n", gas.name().c_str());
	printf("depth %5.2lfm \n", depth.mm / 1000.0);
	printf("current_cylinder %i\n", current_cylinder);
#endif

	/* Find the gases available for deco */

	bool inappropriate_cylinder_use = false;
	std::vector<gaschanges> gaschanges = analyze_gaslist(diveplan, dive, dcNr, depth.mm, &best_first_ascend_cylinder, divemode == CCR && !prefs.dobailout, inappropriate_cylinder_use);
	if (inappropriate_cylinder_use) {
		error = PLAN_ERROR_INAPPROPRIATE_GAS;
	}

	/* Find the first potential decostopdepth above current depth */
	for (stopidx = 0; stopidx < decostoplevels.size(); stopidx++)
		if (decostoplevels[stopidx].mm > depth.mm)
			break;
	if (stopidx > 0)
		stopidx--;
	/* Stoplevels are either depths of gas changes or potential deco stop depths. */
	std::vector<depth_t> stoplevels = sort_stops(decostoplevels, stopidx + 1, gaschanges);
	stopidx += gaschanges.size();

	gi = static_cast<int>(gaschanges.size()) - 1;

	/* Set tissue tolerance and initial vpmb gradient at start of ascent phase */
	diveplan.surface_interval = tissue_at_end(ds, dive, dc, cache);
	nuclear_regeneration(ds, clock);
	vpmb_start_gradient(ds);
	if (decoMode(true) == RECREATIONAL) {
		bool safety_stop = prefs.safetystop && max_depth.mm >= 10000;
		track_ascent_gas(depth, dive, current_cylinder, avg_depth, bottom_time, safety_stop, divemode);
		// How long can we stay at the current depth and still directly ascent to the surface?
		do {
			add_segment(ds, dive->depth_to_bar(depth),
				    dive->get_cylinder(current_cylinder)->gasmix,
				    timestep, po2, divemode, prefs.bottomsac, true);
			update_cylinder_pressure(dive, depth.mm, depth.mm, timestep, prefs.bottomsac, dive->get_cylinder(current_cylinder), false, divemode);
			clock += timestep;
		} while (trial_ascent(ds, 0, depth, 0_m, avg_depth, bottom_time, dive->get_cylinder(current_cylinder)->gasmix,
				      po2, diveplan.surface_pressure.mbar / 1000.0, dive, divemode) &&
			 enough_gas(dive, current_cylinder) && clock < 6 * 3600);

		// We did stay one timestep too many.
		// In the best of all worlds, we would roll back also the last add_segment in terms of caching deco state, but
		// let's ignore that since for the eventual ascent in recreational mode, nobody looks at the ceiling anymore,
		// so we don't really have to compute the deco state.
		update_cylinder_pressure(dive, depth.mm, depth.mm, -timestep, prefs.bottomsac, dive->get_cylinder(current_cylinder), false, divemode);
		clock -= timestep;
		plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, true, divemode);
		previous_point_time = clock;
		do {
			/* Ascend to surface */
			int deltad = ascent_velocity(depth, avg_depth, bottom_time) * base_timestep;
			if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
				plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
			}
			if (depth.mm - deltad < 0)
				deltad = depth.mm;

			clock += base_timestep;
			depth.mm -= deltad;
			if (depth.mm <= 5000 && depth.mm >= (5000 - deltad) && safety_stop) {
				plan_add_segment(diveplan, clock - previous_point_time, 5_m, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				clock += 180;
				plan_add_segment(diveplan, clock - previous_point_time, 5_m, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				safety_stop = false;
			}
		} while (depth.mm > 0);
		plan_add_segment(diveplan, clock - previous_point_time, 0_m, current_cylinder, po2, false, divemode);
		create_dive_from_plan(diveplan, dive, dc, is_planner);
		diveplan.add_plan_to_notes(*dive, show_disclaimer, error);
		fixup_dc_duration(*dc);

		return {};
	}

	if (best_first_ascend_cylinder != -1 && best_first_ascend_cylinder != current_cylinder) {
		current_cylinder = best_first_ascend_cylinder;
		gas = dive->get_cylinder(current_cylinder)->gasmix;

#if DEBUG_PLAN & 16
		printf("switch to gas %d (%d/%d) @ %5.2lfm\n", best_first_ascend_cylinder,
		       (get_o2(gas) + 5) / 10, (get_he(gas) + 5) / 10, gaschanges[best_first_ascend_cylinder].depth / 1000.0);
#endif
	}

	// VPM-B or Buehlmann Deco
	tissue_at_end(ds, dive, dc, cache);
	if ((divemode == CCR || divemode == PSCR) && prefs.dobailout) {
		divemode = OC;
		po2 = 0;
		int bailoutsegment = std::max(prefs.min_switch_duration, 60 * prefs.problemsolvingtime);
		add_segment(ds, dive->depth_to_bar(depth),
			dive->get_cylinder(current_cylinder)->gasmix,
			bailoutsegment, po2, divemode, prefs.bottomsac, true);
		plan_add_segment(diveplan, bailoutsegment, depth, current_cylinder, po2, false, divemode);
		bottom_time += bailoutsegment;
	}
	previous_deco_time = 100000000;
	ds->deco_time = 10000000;
	bottom_cache.cache(ds);  // Lets us make several iterations
	depth_t bottom_depth = depth;
	bottom_gi = gi;
	bottom_gas = gas;
	bottom_stopidx = stopidx;

	//CVA
	std::vector<decostop> decostoptable;
	do {
		decostoptable.clear();
		is_final_plan = (decoMode(true) == BUEHLMANN) || (previous_deco_time - ds->deco_time < 10);  // CVA time converges
		if (ds->deco_time != 10000000)
			vpmb_next_gradient(ds, ds->deco_time, diveplan.surface_pressure.mbar / 1000.0, true);

		previous_deco_time = ds->deco_time;
		bottom_cache.restore(ds, true);

		depth = bottom_depth;
		gi = bottom_gi;
		clock = previous_point_time = bottom_time;
		gas = bottom_gas;
		stopping = false;
		bool decodive = false;
		first_stop_depth = 0_m;
		stopidx = bottom_stopidx;
		ds->first_ceiling_pressure.mbar = dive->depth_to_mbar(
					deco_allowed_depth(tissue_tolerance_calc(ds, dive, dive->depth_to_bar(depth), true),
							   diveplan.surface_pressure.mbar / 1000.0, dive, 1));
		if (ds->max_bottom_ceiling_pressure.mbar > ds->first_ceiling_pressure.mbar)
			ds->first_ceiling_pressure.mbar = ds->max_bottom_ceiling_pressure.mbar;

		last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
		/* Always prefer the best_first_ascend_cylinder if it has the right gasmix.
		 * Otherwise take first cylinder from list with rightgasmix  */
		if (best_first_ascend_cylinder != -1 && same_gasmix(gas, dive->get_cylinder(best_first_ascend_cylinder)->gasmix))
			current_cylinder = best_first_ascend_cylinder;
		else
			current_cylinder = get_gasidx(dive, gas);
		if (current_cylinder == -1) {
			report_error(translate("gettextFromC", "Can't find gas %s"), gas.name().c_str());
			current_cylinder = 0;
		}
		reset_regression(ds);
		while (error == PLAN_OK) {
			/* We will break out when we hit the surface */
			do {
				/* Ascend to next stop depth */
				depth_t deltad { .mm = ascent_velocity(depth, avg_depth, bottom_time) * base_timestep };
				if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
					if (is_final_plan)
						plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
					previous_point_time = clock;
					stopping = false;
					last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
				}
				if (depth.mm - deltad.mm < stoplevels[stopidx].mm)
					deltad = depth - stoplevels[stopidx];

				add_segment(ds, dive->depth_to_bar(depth),
								dive->get_cylinder(current_cylinder)->gasmix,
								base_timestep, po2, divemode, prefs.decosac, true);
				last_segment_min_switch = false;
				clock += base_timestep;
				depth -= deltad;
				/* Print VPM-Gradient as gradient factor, this has to be done from within deco.cpp */
				if (decodive)
					ds->plot_depth = depth.mm;
			} while (depth.mm > 0 && depth.mm > stoplevels[stopidx].mm);

			if (depth.mm <= 0)
				break; /* We are at the surface */

			if (gi >= 0 && stoplevels[stopidx].mm <= gaschanges[gi].depth) {
				/* We have reached a gas change.
				 * Record this in the dive plan */

				/* Check we need to change cylinder.
				 * We might not if the cylinder was chosen by the user
				 * or user has selected only to switch only at required stops.
				 * If current gas is hypoxic, we want to switch asap */

				if (current_cylinder != gaschanges[gi].gasidx) {
					if (!prefs.switch_at_req_stop ||
							!trial_ascent(ds, 0, depth, stoplevels[stopidx - 1], avg_depth, bottom_time,
							dive->get_cylinder(current_cylinder)->gasmix, po2, diveplan.surface_pressure.mbar / 1000.0, dive, divemode) || get_o2(dive->get_cylinder(current_cylinder)->gasmix) < 160) {
						if (is_final_plan)
							plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
						stopping = true;
						previous_point_time = clock;
						current_cylinder = gaschanges[gi].gasidx;
						if (divemode == CCR)
							po2 = setpoint_change(dive, current_cylinder);
#if DEBUG_PLAN & 16
						gas = dive->get_cylinder(current_cylinder)->gasmix;
						printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
							(get_o2(gas) + 5) / 10, (get_he(gas) + 5) / 10, gaschanges[gi].depth / 1000.0);
#endif
						/* Stop for the minimum duration to switch gas unless we switch to o2 */
						if (!last_segment_min_switch && get_o2(dive->get_cylinder(current_cylinder)->gasmix) != 1000) {
							add_segment(ds, dive->depth_to_bar(depth),
								dive->get_cylinder(current_cylinder)->gasmix,
								prefs.min_switch_duration, po2, divemode, prefs.decosac, true);
							clock += prefs.min_switch_duration;
							last_segment_min_switch = true;
						}
					} else {
						/* The user has selected the option to switch gas only at required stops.
						 * Remember that we are waiting to switch gas
						 */
						pendinggaschange = true;
					}
				}
				gi--;
			}
			--stopidx;

			/* Save the current state and try to ascend to the next stopdepth */
			while (1) {
				/* Check if ascending to next stop is clear, go back and wait if we hit the ceiling on the way */
				if (trial_ascent(ds, 0, depth, stoplevels[stopidx], avg_depth, bottom_time,
						dive->get_cylinder(current_cylinder)->gasmix, po2, diveplan.surface_pressure.mbar / 1000.0, dive, divemode)) {
					decostoptable.push_back( decostop { depth.mm, 0 });
					break; /* We did not hit the ceiling */
				}

				/* Add a minute of deco time and then try again */
				if (!decodive) {
					decodive = true;
					first_stop_depth = depth;
				}
				if (!stopping) {
					/* The last segment was an ascend segment.
					 * Add a waypoint for start of this deco stop */
					if (is_final_plan)
						plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
					previous_point_time = clock;
					stopping = true;
				}

				/* Are we waiting to switch gas?
				 * Occurs when the user has selected the option to switch only at required stops
				 */
				if (pendinggaschange) {
					current_cylinder = gaschanges[gi + 1].gasidx;
					if (divemode == CCR)
						po2 = setpoint_change(dive, current_cylinder);
#if DEBUG_PLAN & 16
					gas = dive->get_cylinder(current_cylinder)->gasmix;
					printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi + 1].gasidx,
						(get_o2(gas) + 5) / 10, (get_he(gas) + 5) / 10, gaschanges[gi + 1].depth / 1000.0);
#endif
					/* Stop for the minimum duration to switch gas unless we switch to o2 */
					if (!last_segment_min_switch && get_o2(dive->get_cylinder(current_cylinder)->gasmix) != 1000) {
						add_segment(ds, dive->depth_to_bar(depth),
							dive->get_cylinder(current_cylinder)->gasmix,
							prefs.min_switch_duration, po2, divemode, prefs.decosac, true);
						clock += prefs.min_switch_duration;
					}
					pendinggaschange = false;
				}

				int new_clock = wait_until(ds, dive, clock, clock, laststoptime * 2 + 1, timestep, depth, stoplevels[stopidx], avg_depth,
					bottom_time, dive->get_cylinder(current_cylinder)->gasmix, po2, diveplan.surface_pressure.mbar / 1000.0, divemode);
				laststoptime = new_clock - clock;
				/* Finish infinite deco */
				if (laststoptime >= 48 * 3600 && depth.mm >= 6000 && !o2breaking) {
					error = PLAN_ERROR_TIMEOUT;

					break;
				}

				o2breaking = false;
				stop_cylinder = current_cylinder;
				if (prefs.doo2breaks && prefs.last_stop) {
					/* The backgas breaks option limits time on oxygen to 12 minutes, followed by 6 minutes on
					 * backgas.  This could be customized if there were demand.
					 */
					if (break_cylinder == -1) {
						if (best_first_ascend_cylinder != -1 && get_o2(dive->get_cylinder(best_first_ascend_cylinder)->gasmix) <= 320)
							break_cylinder = best_first_ascend_cylinder;
						else
							break_cylinder = 0;
					}
					if (get_o2(dive->get_cylinder(current_cylinder)->gasmix) == 1000) {
						if (laststoptime >= 12 * 60) {
							laststoptime = 12 * 60;
							new_clock = clock + laststoptime;
							o2breaking = true;
							o2break_next = true;
							breakfrom_cylinder = current_cylinder;
							if (is_final_plan)
								plan_add_segment(diveplan, laststoptime, depth, current_cylinder, po2, false, divemode);
							previous_point_time = clock + laststoptime;
							current_cylinder = break_cylinder;
						}
					} else if (o2break_next) {
						if (laststoptime >= 6 * 60) {
							laststoptime = 6 * 60;
							new_clock = clock + laststoptime;
							o2breaking  = true;
							o2break_next = false;
							if (is_final_plan)
								plan_add_segment(diveplan, laststoptime, depth, current_cylinder, po2, false, divemode);
							previous_point_time = clock + laststoptime;
							current_cylinder = breakfrom_cylinder;
						}
					}
				}
				add_segment(ds, dive->depth_to_bar(depth), dive->get_cylinder(stop_cylinder)->gasmix,
					    laststoptime, po2, divemode, prefs.decosac, true);
				last_segment_min_switch = false;
				decostoptable.push_back(decostop { depth.mm, laststoptime } );

				clock += laststoptime;
				if (!o2breaking)
					break;
			}
			if (stopping) {
				/* Next we will ascend again. Add a waypoint if we have spend deco time */
				if (is_final_plan)
					plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				stopping = false;
			}
		}
		/* When calculating deco_time, we should pretend the final ascent rate is always the same,
		 * otherwise odd things can happen, such as CVA causing the final ascent to start *later*
		 * if the ascent rate is slower, which is completely nonsensical.
		 * Assume final ascent takes 20s, which is the time taken to ascend at 9m/min from 3m */
		ds->deco_time = clock - bottom_time - (m_or_ft(3, 10).mm * ( prefs.last_stop ? 2 : 1)) / last_ascend_rate + 20;
	} while (!is_final_plan && error == PLAN_OK);

	plan_add_segment(diveplan, clock - previous_point_time, 0_m, current_cylinder, po2, false, divemode);
	if (decoMode(true) == VPMB) {
		diveplan.eff_gfhigh = lrint(100.0 * regressionb(ds));
		diveplan.eff_gflow = lrint(100.0 * (regressiona(ds) * first_stop_depth.mm + regressionb(ds)));
	}

	if (prefs.surface_segment != 0) {
		// Switch to an empty air cylinder for breathing air at the surface.
		// FIXME: This is incredibly silly and emulates the old code when
		// we had a fixed cylinder table: It uses an extra fake cylinder
		// past the regular cylinder table, which is not visible to the UI.
		// Fix this as soon as possible!
		current_cylinder = static_cast<int>(dive->cylinders.size());
		plan_add_segment(diveplan, prefs.surface_segment, 0_m, current_cylinder, 0, false, OC);
	}
	create_dive_from_plan(diveplan, dive, dc, is_planner);
	diveplan.add_plan_to_notes(*dive, show_disclaimer, error);
	fixup_dc_duration(*dc);
	return decostoptable;
}
