// SPDX-License-Identifier: GPL-2.0
/* planner.c
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "ssrf.h"
#include "dive.h"
#include "divelist.h" // for init_decompression()
#include "sample.h"
#include "subsurface-string.h"
#include "deco.h"
#include "errorhelper.h"
#include "event.h"
#include "interpolate.h"
#include "planner.h"
#include "subsurface-time.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"
#include "qthelper.h"
#include "version.h"

#define TIMESTEP 2 /* second */

static int decostoplevels_metric[] = { 0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000,
					30000, 33000, 36000, 39000, 42000, 45000, 48000, 51000, 54000, 57000,
					60000, 63000, 66000, 69000, 72000, 75000, 78000, 81000, 84000, 87000,
					90000, 100000, 110000, 120000, 130000, 140000, 150000, 160000, 170000,
					180000, 190000, 200000, 220000, 240000, 260000, 280000, 300000,
					320000, 340000, 360000, 380000 };
static int decostoplevels_imperial[] = { 0, 3048, 6096, 9144, 12192, 15240, 18288, 21336, 24384, 27432,
					30480, 33528, 36576, 39624, 42672, 45720, 48768, 51816, 54864, 57912,
					60960, 64008, 67056, 70104, 73152, 76200, 79248, 82296, 85344, 88392,
					91440, 101600, 111760, 121920, 132080, 142240, 152400, 162560, 172720,
					182880, 193040, 203200, 223520, 243840, 264160, 284480, 304800,
					325120, 345440, 365760, 386080 };

#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan)
{
	struct divedatapoint *dp;
	struct tm tm;

	if (!diveplan) {
		printf("Diveplan NULL\n");
		return;
	}
	utc_mkdate(diveplan->when, &tm);

	printf("\nDiveplan @ %04d-%02d-%02d %02d:%02d:%02d (surfpres %dmbar):\n",
	       tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec,
	       diveplan->surface_pressure);
	dp = diveplan->dp;
	while (dp) {
		printf("\t%3u:%02u: %6dmm cylid: %2d setpoint: %d\n", FRACTION(dp->time, 60), dp->depth, dp->cylinderid, dp->setpoint);
		dp = dp->next;
	}
}
#endif

bool diveplan_empty(struct diveplan *diveplan)
{
	struct divedatapoint *dp;
	if (!diveplan || !diveplan->dp)
		return true;
	dp = diveplan->dp;
	while (dp) {
		if (dp->time)
			return false;
		dp = dp->next;
	}
	return true;
}

/* get the cylinder index at a certain time during the dive */
int get_cylinderid_at_time(struct dive *dive, struct divecomputer *dc, duration_t time)
{
	// we start with the first cylinder unless an event tells us otherwise
	int cylinder_idx = 0;
	struct event *event = dc->events;
	while (event && event->time.seconds <= time.seconds) {
		if (!strcmp(event->name, "gaschange"))
			cylinder_idx = get_cylinder_index(dive, event);
		event = event->next;
	}
	return cylinder_idx;
}

int get_gasidx(struct dive *dive, struct gasmix mix)
{
	return find_best_gasmix_match(mix, &dive->cylinders);
}

static void interpolate_transition(struct deco_state *ds, struct dive *dive, duration_t t0, duration_t t1, depth_t d0, depth_t d1, struct gasmix gasmix, o2pressure_t po2, enum divemode_t divemode)
{
	int32_t j;

	for (j = t0.seconds; j < t1.seconds; j++) {
		int depth = interpolate(d0.mm, d1.mm, j - t0.seconds, t1.seconds - t0.seconds);
		add_segment(ds, depth_to_bar(depth, dive), gasmix, 1, po2.mbar, divemode, prefs.bottomsac, true);
	}
	if (d1.mm > d0.mm)
		calc_crushing_pressure(ds, depth_to_bar(d1.mm, dive));
}

/* returns the tissue tolerance at the end of this (partial) dive */
static int tissue_at_end(struct deco_state *ds, struct dive *dive, struct deco_state **cached_datap)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i;
	depth_t lastdepth = {};
	duration_t t0 = {}, t1 = {};
	struct gasmix gas;
	int surface_interval = 0;

	if (!dive)
		return 0;
	if (*cached_datap) {
		restore_deco_state(*cached_datap, ds, true);
	} else {
		surface_interval = init_decompression(ds, dive, true);
		cache_deco_state(ds, cached_datap);
	}
	dc = &dive->dc;
	if (!dc->samples)
		return 0;
	psample = sample = dc->sample;

	const struct event *evdm = NULL;
	enum divemode_t divemode = UNDEF_COMP_TYPE;

	for (i = 0; i < dc->samples; i++, sample++) {
		o2pressure_t setpoint;

		if (i)
			setpoint = sample[-1].setpoint;
		else
			setpoint = sample[0].setpoint;

		t1 = sample->time;
		gas = get_gasmix_at_time(dive, dc, t0);
		if (i > 0)
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
		if ((decoMode(true) == VPMB) && (lastdepth.mm > sample->depth.mm)) {
			pressure_t ceiling_pressure;
			nuclear_regeneration(ds, t0.seconds);
			vpmb_start_gradient(ds);
			ceiling_pressure.mbar = depth_to_mbar(deco_allowed_depth(tissue_tolerance_calc(ds, dive,
													depth_to_bar(lastdepth.mm, dive), true),
										dive->surface_pressure.mbar / 1000.0,
										dive,
										1),
								dive);
			if (ceiling_pressure.mbar > ds->max_bottom_ceiling_pressure.mbar)
				ds->max_bottom_ceiling_pressure.mbar = ceiling_pressure.mbar;
		}

		divemode = get_current_divemode(&dive->dc, t0.seconds + 1, &evdm, &divemode);
		interpolate_transition(ds, dive, t0, t1, lastdepth, sample->depth, gas, setpoint, divemode);
		psample = sample;
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
	gas_used.mliter = lrint(depth_to_atm(mean_depth.mm, d) * sac / 60 * duration * factor / 1000);
	cyl->gas_used.mliter += gas_used.mliter;
	if (in_deco)
		cyl->deco_gas_used.mliter += gas_used.mliter;
	if (cyl->type.size.mliter) {
		delta_p.mbar = lrint(gas_used.mliter * 1000.0 / cyl->type.size.mliter * gas_compressibility_factor(cyl->gasmix, cyl->end.mbar / 1000.0));
		cyl->end.mbar -= delta_p.mbar;
	}
}

/* overwrite the data in dive
 * return false if something goes wrong */
static void create_dive_from_plan(struct diveplan *diveplan, struct dive *dive, bool track_gas)
{
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	struct event *ev;
	cylinder_t *cyl;
	int oldpo2 = 0;
	int lasttime = 0, last_manual_point = 0;
	depth_t lastdepth = {.mm = 0};
	int lastcylid;
	enum divemode_t type = dive->dc.divemode;

	if (!diveplan || !diveplan->dp)
		return;
#if DEBUG_PLAN & 4
	printf("in create_dive_from_plan\n");
	dump_plan(diveplan);
#endif
	dive->salinity = diveplan->salinity;
	// reset the cylinders and clear out the samples and events of the
	// dive-to-be-planned so we can restart
	reset_cylinders(dive, track_gas);
	dc = &dive->dc;
	dc->when = dive->when = diveplan->when;
	dc->surface_pressure.mbar = diveplan->surface_pressure;
	dc->salinity = diveplan->salinity;
	free_samples(dc);
	while ((ev = dc->events)) {
		dc->events = dc->events->next;
		free(ev);
	}
	dp = diveplan->dp;
	/* Create first sample at time = 0, not based on dp because
	 * there is no real dp for time = 0, set first cylinder to 0
	 * O2 setpoint for this sample will be filled later from next dp */
	cyl = get_or_create_cylinder(dive, 0);
	sample = prepare_sample(dc);
	sample->sac.mliter = prefs.bottomsac;
	if (track_gas && cyl->type.workingpressure.mbar)
		sample->pressure[0].mbar = cyl->end.mbar;
	sample->manually_entered = true;
	finish_sample(dc);
	lastcylid = 0;
	while (dp) {
		int po2 = dp->setpoint;
		int time = dp->time;
		depth_t depth = dp->depth;

		if (time == 0) {
			/* special entries that just inform the algorithm about
			 * additional gases that are available */
			dp = dp->next;
			continue;
		}

		/* Check for SetPoint change */
		if (oldpo2 != po2) {
			/* this is a bad idea - we should get a different SAMPLE_EVENT type
			 * reserved for this in libdivecomputer... overloading SMAPLE_EVENT_PO2
			 * with a different meaning will only cause confusion elsewhere in the code */
			if (dp->divemode == type)
				add_event(dc, lasttime, SAMPLE_EVENT_PO2, 0, po2, QT_TRANSLATE_NOOP("gettextFromC", "SP change"));
			oldpo2 = po2;
		}

		/* Make sure we have the new gas, and create a gas change event */
		if (dp->cylinderid != lastcylid) {
			/* need to insert a first sample for the new gas */
			add_gas_switch_event(dive, dc, lasttime + 1, dp->cylinderid);
			cyl = get_cylinder(dive, dp->cylinderid); // FIXME: This actually may get one past the last cylinder for "surface air".
			if (!cyl) {
				report_error("Invalid cylinder in create_dive_from_plan(): %d", dp->cylinderid);
				continue;
			}
			sample = prepare_sample(dc);
			sample[-1].setpoint.mbar = po2;
			sample->time.seconds = lasttime + 1;
			sample->depth = lastdepth;
			sample->manually_entered = dp->entered;
			sample->sac.mliter = dp->entered ? prefs.bottomsac : prefs.decosac;
			finish_sample(dc);
			lastcylid = dp->cylinderid;
		}
		if (dp->divemode != type) {
			type = dp->divemode;
			add_event(dc, lasttime, SAMPLE_EVENT_BOOKMARK, 0, type, "modechange");
		}

		/* Create sample */
		sample = prepare_sample(dc);
		/* set po2 at beginning of this segment */
		/* and keep it valid for last sample - where it likely doesn't matter */
		sample[-1].setpoint.mbar = po2;
		sample->setpoint.mbar = po2;
		sample->time.seconds = lasttime = time;
		if (dp->entered) last_manual_point = dp->time;
		sample->depth = lastdepth = depth;
		sample->manually_entered = dp->entered;
		sample->sac.mliter = dp->entered ? prefs.bottomsac : prefs.decosac;
		if (track_gas && !sample[-1].setpoint.mbar) {    /* Don't track gas usage for CCR legs of dive */
			update_cylinder_pressure(dive, sample[-1].depth.mm, depth.mm, time - sample[-1].time.seconds,
					dp->entered ? diveplan->bottomsac : diveplan->decosac, cyl, !dp->entered, type);
			if (cyl->type.workingpressure.mbar)
				sample->pressure[0].mbar = cyl->end.mbar;
		}
		finish_sample(dc);
		dp = dp->next;
	}
	dive->dc.last_manual_time.seconds = last_manual_point;

#if DEBUG_PLAN & 32
	save_dive(stdout, dive);
#endif
	return;
}

void free_dps(struct diveplan *diveplan)
{
	if (!diveplan)
		return;
	struct divedatapoint *dp = diveplan->dp;
	while (dp) {
		struct divedatapoint *ndp = dp->next;
		free(dp);
		dp = ndp;
	}
	diveplan->dp = NULL;
}

struct divedatapoint *create_dp(int time_incr, int depth, int cylinderid, int po2)
{
	struct divedatapoint *dp;

	dp = malloc(sizeof(struct divedatapoint));
	dp->time = time_incr;
	dp->depth.mm = depth;
	dp->cylinderid = cylinderid;
	dp->minimum_gas.mbar = 0;
	dp->setpoint = po2;
	dp->entered = false;
	dp->next = NULL;
	return dp;
}

static void add_to_end_of_diveplan(struct diveplan *diveplan, struct divedatapoint *dp)
{
	struct divedatapoint **lastdp = &diveplan->dp;
	struct divedatapoint *ldp = *lastdp;
	int lasttime = 0;
	while (*lastdp) {
		ldp = *lastdp;
		if (ldp->time > lasttime)
			lasttime = ldp->time;
		lastdp = &(*lastdp)->next;
	}
	*lastdp = dp;
	if (ldp)
		dp->time += lasttime;
}

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int cylinderid, int po2, bool entered, enum divemode_t divemode)
{
	struct divedatapoint *dp = create_dp(duration, depth, cylinderid, divemode == CCR ? po2 : 0);
	dp->entered = entered;
	dp->divemode = divemode;
	add_to_end_of_diveplan(diveplan, dp);
	return dp;
}

struct gaschanges {
	int depth;
	int gasidx;
};

// Return new setpoint if cylinderi is a setpoint change an 0 if not

static int setpoint_change(struct dive *dive, int cylinderid)
{
	cylinder_t *cylinder = get_cylinder(dive, cylinderid);
	if (!cylinder->type.description)
		return 0;
	if (!strncmp(cylinder->type.description, "SP ", 3)) {
		float sp;
		sscanf(cylinder->type.description + 3, "%f", &sp);
		return (int) (sp * 1000);
	} else {
		return 0;
	}
}

static struct gaschanges *analyze_gaslist(struct diveplan *diveplan, struct dive *dive, int *gaschangenr, int depth, int *asc_cylinder, bool ccr)
{
	int nr = 0;
	struct gaschanges *gaschanges = NULL;
	struct divedatapoint *dp = diveplan->dp;
	int best_depth = get_cylinder(dive, *asc_cylinder)->depth.mm;
	bool total_time_zero = true;
	while (dp) {
		if (dp->time == 0 && total_time_zero && (ccr == (bool) setpoint_change(dive, dp->cylinderid))) {
			if (dp->depth.mm <= depth) {
				int i = 0;
				nr++;
				gaschanges = realloc(gaschanges, nr * sizeof(struct gaschanges));
				while (i < nr - 1) {
					if (dp->depth.mm < gaschanges[i].depth) {
						memmove(gaschanges + i + 1, gaschanges + i, (nr - i - 1) * sizeof(struct gaschanges));
						break;
					}
					i++;
				}
				gaschanges[i].depth = dp->depth.mm;
				gaschanges[i].gasidx = dp->cylinderid;
				assert(gaschanges[i].gasidx != -1);
			} else {
				/* is there a better mix to start deco? */
				if (dp->depth.mm < best_depth) {
					best_depth = dp->depth.mm;
					*asc_cylinder = dp->cylinderid;
				}
			}
		} else {
			total_time_zero = false;
		}
		dp = dp->next;
	}
	*gaschangenr = nr;
#if DEBUG_PLAN & 16
	for (nr = 0; nr < *gaschangenr; nr++) {
		int idx = gaschanges[nr].gasidx;
		printf("gaschange nr %d: @ %5.2lfm gasidx %d (%s)\n", nr, gaschanges[nr].depth / 1000.0,
		       idx, gasname(&get_cylinder(&dive, idx)->gasmix));
	}
#endif
	return gaschanges;
}

/* sort all the stops into one ordered list */
static int *sort_stops(int *dstops, int dnr, struct gaschanges *gstops, int gnr)
{
	int i, gi, di;
	int total = dnr + gnr;
	int *stoplevels = malloc(total * sizeof(int));

	/* no gaschanges */
	if (gnr == 0) {
		memcpy(stoplevels, dstops, dnr * sizeof(int));
		return stoplevels;
	}
	i = total - 1;
	gi = gnr - 1;
	di = dnr - 1;
	while (i >= 0) {
		if (dstops[di] > gstops[gi].depth) {
			stoplevels[i] = dstops[di];
			di--;
		} else if (dstops[di] == gstops[gi].depth) {
			stoplevels[i] = dstops[di];
			di--;
			gi--;
		} else {
			stoplevels[i] = gstops[gi].depth;
			gi--;
		}
		i--;
		if (di < 0) {
			while (gi >= 0)
				stoplevels[i--] = gstops[gi--].depth;
			break;
		}
		if (gi < 0) {
			while (di >= 0)
				stoplevels[i--] = dstops[di--];
			break;
		}
	}
	while (i >= 0)
		stoplevels[i--] = 0;

#if DEBUG_PLAN & 16
	int k;
	for (k = gnr + dnr - 1; k >= 0; k--) {
		printf("stoplevel[%d]: %5.2lfm\n", k, stoplevels[k] / 1000.0);
		if (stoplevels[k] == 0)
			break;
	}
#endif
	return stoplevels;
}

int ascent_velocity(int depth, int avg_depth, int bottom_time)
{
	UNUSED(bottom_time);
	/* We need to make this configurable */

	/* As an example (and possibly reasonable default) this is the Tech 1 provedure according
	 * to http://www.globalunderwaterexplorers.org/files/Standards_and_Procedures/SOP_Manual_Ver2.0.2.pdf */

	if (depth * 4 > avg_depth * 3) {
		return prefs.ascrate75;
	} else {
		if (depth * 2 > avg_depth) {
			return prefs.ascrate50;
		} else {
			if (depth > 6000)
				return prefs.ascratestops;
			else
				return prefs.ascratelast6m;
		}
	}
}

static void track_ascent_gas(int depth, struct dive *dive, int cylinder_id, int avg_depth, int bottom_time, bool safety_stop, enum divemode_t divemode)
{
	cylinder_t *cylinder = get_cylinder(dive, cylinder_id);
	while (depth > 0) {
		int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
		if (deltad > depth)
			deltad = depth;
		update_cylinder_pressure(dive, depth, depth - deltad, TIMESTEP, prefs.decosac, cylinder, true, divemode);
		if (depth <= 5000 && depth >= (5000 - deltad) && safety_stop) {
			update_cylinder_pressure(dive, 5000, 5000, 180, prefs.decosac, cylinder, true, divemode);
			safety_stop = false;
		}
		depth -= deltad;
	}
}

// Determine whether ascending to the next stop will break the ceiling.  Return true if the ascent is ok, false if it isn't.
static bool trial_ascent(struct deco_state *ds, int wait_time, int trial_depth, int stoplevel, int avg_depth, int bottom_time, struct gasmix gasmix, int po2, double surface_pressure, struct dive *dive, enum divemode_t divemode)
{

	bool clear_to_ascend = true;
	struct deco_state *trial_cache = NULL;

	// For consistency with other VPM-B implementations, we should not start the ascent while the ceiling is
	// deeper than the next stop (thus the offgasing during the ascent is ignored).
	// However, we still need to make sure we don't break the ceiling due to on-gassing during ascent.
	cache_deco_state(ds, &trial_cache);
	if (wait_time)
		add_segment(ds, depth_to_bar(trial_depth, dive),
			    gasmix,
			    wait_time, po2, divemode, prefs.decosac, true);
	if (decoMode(true) == VPMB) {
		double tolerance_limit = tissue_tolerance_calc(ds, dive, depth_to_bar(stoplevel, dive), true);
		update_regression(ds, dive);
		if (deco_allowed_depth(tolerance_limit, surface_pressure, dive, 1) > stoplevel) {
			restore_deco_state(trial_cache, ds, false);
			free(trial_cache);
			return false;
		}
	}

	while (trial_depth > stoplevel) {
		double tolerance_limit;
		int deltad = ascent_velocity(trial_depth, avg_depth, bottom_time) * TIMESTEP;
		if (deltad > trial_depth) /* don't test against depth above surface */
			deltad = trial_depth;
		add_segment(ds, depth_to_bar(trial_depth, dive),
			    gasmix,
			    TIMESTEP, po2, divemode, prefs.decosac, true);
		tolerance_limit = tissue_tolerance_calc(ds, dive, depth_to_bar(trial_depth, dive), true);
		if (decoMode(true) == VPMB)
			update_regression(ds, dive);
		if (deco_allowed_depth(tolerance_limit, surface_pressure, dive, 1) > trial_depth - deltad) {
			/* We should have stopped */
			clear_to_ascend = false;
			break;
		}
		trial_depth -= deltad;
	}
	restore_deco_state(trial_cache, ds, false);
	free(trial_cache);
	return clear_to_ascend;
}

/* Determine if there is enough gas for the dive.  Return true if there is enough.
 * Also return true if this cannot be calculated because the cylinder doesn't have
 * size or a starting pressure.
 */
static bool enough_gas(const struct dive *dive, int current_cylinder)
{
	cylinder_t *cyl;
	if (current_cylinder < 0 || current_cylinder >= dive->cylinders.nr)
		return false;
	cyl = get_cylinder(dive, current_cylinder);

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
static int wait_until(struct deco_state *ds, struct dive *dive, int clock, int min, int leap, int stepsize, int depth, int target_depth, int avg_depth, int bottom_time, struct gasmix gasmix, int po2, double surface_pressure, enum divemode_t divemode)
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

static void average_max_depth(struct diveplan *dive, int *avg_depth, int *max_depth)
{
	int integral = 0;
	int last_time = 0;
	int last_depth = 0;
	struct divedatapoint *dp = dive->dp;

	*max_depth = 0;

	while (dp) {
		if (dp->time) {
			/* Ignore gas indication samples */
			integral += (dp->depth.mm + last_depth) * (dp->time - last_time) / 2;
			last_time = dp->time;
			last_depth = dp->depth.mm;
			if (dp->depth.mm > *max_depth)
				*max_depth = dp->depth.mm;
		}
		dp = dp->next;
	}
	if (last_time)
		*avg_depth = integral / last_time;
	else
		*avg_depth = *max_depth = 0;
}

bool plan(struct deco_state *ds, struct diveplan *diveplan, struct dive *dive, int timestep, struct decostop *decostoptable, struct deco_state **cached_datap, bool is_planner, bool show_disclaimer)
{

	int bottom_depth;
	int bottom_gi;
	int bottom_stopidx;
	bool is_final_plan = true;
	int bottom_time;
	int previous_deco_time;
	struct deco_state *bottom_cache = NULL;
	struct sample *sample;
	int po2;
	int transitiontime, gi;
	int current_cylinder, stop_cylinder;
	int stopidx;
	int depth;
	struct gaschanges *gaschanges = NULL;
	int gaschangenr;
	int *decostoplevels;
	int decostoplevelcount;
	int *stoplevels = NULL;
	bool stopping = false;
	bool pendinggaschange = false;
	int clock, previous_point_time;
	int avg_depth, max_depth;
	int last_ascend_rate;
	int best_first_ascend_cylinder;
	struct gasmix gas, bottom_gas;
	bool o2break_next = false;
	int break_cylinder = -1, breakfrom_cylinder = 0;
	bool last_segment_min_switch = false;
	int error = 0;
	bool decodive = false;
	int first_stop_depth = 0;
	int laststoptime = timestep;
	bool o2breaking = false;
	int decostopcounter = 0;
	enum divemode_t divemode = dive->dc.divemode;

	set_gf(diveplan->gflow, diveplan->gfhigh);
	set_vpmb_conservatism(diveplan->vpmb_conservatism);
	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = SURFACE_PRESSURE;
	dive->surface_pressure.mbar = diveplan->surface_pressure;
	clear_deco(ds, dive->surface_pressure.mbar / 1000.0, true);
	ds->max_bottom_ceiling_pressure.mbar = ds->first_ceiling_pressure.mbar = 0;
	create_dive_from_plan(diveplan, dive, is_planner);

	// Do we want deco stop array in metres or feet?
	if (prefs.units.length == METERS ) {
		decostoplevels = decostoplevels_metric;
		decostoplevelcount = sizeof(decostoplevels_metric) / sizeof(int);
	} else {
		decostoplevels = decostoplevels_imperial;
		decostoplevelcount = sizeof(decostoplevels_imperial) / sizeof(int);
	}

	/* If the user has selected last stop to be at 6m/20', we need to get rid of the 3m/10' stop.
	 * Otherwise reinstate the last stop 3m/10' stop.
	 */
	if (prefs.last_stop)
		*(decostoplevels + 1) = 0;
	else
		*(decostoplevels + 1) = M_OR_FT(3,10);

	/* Let's start at the last 'sample', i.e. the last manually entered waypoint. */
	sample = &dive->dc.sample[dive->dc.samples - 1];

	/* Keep time during the ascend */
	bottom_time = clock = previous_point_time = dive->dc.sample[dive->dc.samples - 1].time.seconds;

	current_cylinder = get_cylinderid_at_time(dive, &dive->dc, sample->time);
	// Find the divemode at the end of the dive
	const struct event *ev = NULL;
	divemode = UNDEF_COMP_TYPE;
	divemode = get_current_divemode(&dive->dc, bottom_time, &ev, &divemode);
	gas = get_cylinder(dive, current_cylinder)->gasmix;

	po2 = sample->setpoint.mbar;
	depth = dive->dc.sample[dive->dc.samples - 1].depth.mm;
	average_max_depth(diveplan, &avg_depth, &max_depth);
	last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);

	/* if all we wanted was the dive just get us back to the surface */
	if (!is_planner) {
		/* Attn: for manually entered dives, we depend on the last segment having the
		 * same ascent rate as in fake_dc(). If you change it here, also change it there.
		 */
		transitiontime = lrint(depth / (double)prefs.ascratelast6m);
		plan_add_segment(diveplan, transitiontime, 0, current_cylinder, po2, false, divemode);
		create_dive_from_plan(diveplan, dive, is_planner);
		return false;
	}

#if DEBUG_PLAN & 4
	printf("gas %s\n", gasname(&gas));
	printf("depth %5.2lfm \n", depth / 1000.0);
	printf("current_cylinder %i\n", current_cylinder);
#endif

	best_first_ascend_cylinder = current_cylinder;
	/* Find the gases available for deco */

	gaschanges = analyze_gaslist(diveplan, dive, &gaschangenr, depth, &best_first_ascend_cylinder, divemode == CCR && !prefs.dobailout);

	/* Find the first potential decostopdepth above current depth */
	for (stopidx = 0; stopidx < decostoplevelcount; stopidx++)
		if (*(decostoplevels + stopidx) >= depth)
			break;
	if (stopidx > 0)
		stopidx--;
	/* Stoplevels are either depths of gas changes or potential deco stop depths. */
	stoplevels = sort_stops(decostoplevels, stopidx + 1, gaschanges, gaschangenr);
	stopidx += gaschangenr;

	gi = gaschangenr - 1;

	/* Set tissue tolerance and initial vpmb gradient at start of ascent phase */
	diveplan->surface_interval = tissue_at_end(ds, dive, cached_datap);
	nuclear_regeneration(ds, clock);
	vpmb_start_gradient(ds);
	if (decoMode(true) == RECREATIONAL) {
		bool safety_stop = prefs.safetystop && max_depth >= 10000;
		track_ascent_gas(depth, dive, current_cylinder, avg_depth, bottom_time, safety_stop, divemode);
		// How long can we stay at the current depth and still directly ascent to the surface?
		do {
			add_segment(ds, depth_to_bar(depth, dive),
				    get_cylinder(dive, current_cylinder)->gasmix,
				    timestep, po2, divemode, prefs.bottomsac, true);
			update_cylinder_pressure(dive, depth, depth, timestep, prefs.bottomsac, get_cylinder(dive, current_cylinder), false, divemode);
			clock += timestep;
		} while (trial_ascent(ds, 0, depth, 0, avg_depth, bottom_time, get_cylinder(dive, current_cylinder)->gasmix,
				      po2, diveplan->surface_pressure / 1000.0, dive, divemode) &&
			 enough_gas(dive, current_cylinder) && clock < 6 * 3600);

		// We did stay one DECOTIMESTEP too many.
		// In the best of all worlds, we would roll back also the last add_segment in terms of caching deco state, but
		// let's ignore that since for the eventual ascent in recreational mode, nobody looks at the ceiling anymore,
		// so we don't really have to compute the deco state.
		update_cylinder_pressure(dive, depth, depth, -timestep, prefs.bottomsac, get_cylinder(dive, current_cylinder), false, divemode);
		clock -= timestep;
		plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, true, divemode);
		previous_point_time = clock;
		do {
			/* Ascend to surface */
			int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
			if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
				plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
			}
			if (depth - deltad < 0)
				deltad = depth;

			clock += TIMESTEP;
			depth -= deltad;
			if (depth <= 5000 && depth >= (5000 - deltad) && safety_stop) {
				plan_add_segment(diveplan, clock - previous_point_time, 5000, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				clock += 180;
				plan_add_segment(diveplan, clock - previous_point_time, 5000, current_cylinder, po2, false, divemode);
				previous_point_time = clock;
				safety_stop = false;
			}
		} while (depth > 0);
		plan_add_segment(diveplan, clock - previous_point_time, 0, current_cylinder, po2, false, divemode);
		create_dive_from_plan(diveplan, dive, is_planner);
		add_plan_to_notes(diveplan, dive, show_disclaimer, error);
		fixup_dc_duration(&dive->dc);

		free(stoplevels);
		free(gaschanges);
		return false;
	}

	if (best_first_ascend_cylinder != current_cylinder) {
		current_cylinder = best_first_ascend_cylinder;
		gas = get_cylinder(dive, current_cylinder)->gasmix;

#if DEBUG_PLAN & 16
		printf("switch to gas %d (%d/%d) @ %5.2lfm\n", best_first_ascend_cylinder,
		       (get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[best_first_ascend_cylinder].depth / 1000.0);
#endif
	}

	// VPM-B or Buehlmann Deco
	tissue_at_end(ds, dive, cached_datap);
	if ((divemode == CCR || divemode == PSCR) && prefs.dobailout) {
		divemode = OC;
		po2 = 0;
		int bailoutsegment = MAX(prefs.min_switch_duration, 60 * prefs.problemsolvingtime);
		add_segment(ds, depth_to_bar(depth, dive),
			get_cylinder(dive, current_cylinder)->gasmix,
			bailoutsegment, po2, divemode, prefs.bottomsac, true);
		plan_add_segment(diveplan, bailoutsegment, depth, current_cylinder, po2, false, divemode);
		bottom_time += bailoutsegment;
		last_segment_min_switch = true;
	}
	previous_deco_time = 100000000;
	ds->deco_time = 10000000;
	cache_deco_state(ds, &bottom_cache);  // Lets us make several iterations
	bottom_depth = depth;
	bottom_gi = gi;
	bottom_gas = gas;
	bottom_stopidx = stopidx;

	//CVA
	do {
		decostopcounter = 0;
		is_final_plan = (decoMode(true) == BUEHLMANN) || (previous_deco_time - ds->deco_time < 10);  // CVA time converges
		if (ds->deco_time != 10000000)
			vpmb_next_gradient(ds, ds->deco_time, diveplan->surface_pressure / 1000.0, true);

		previous_deco_time = ds->deco_time;
		restore_deco_state(bottom_cache, ds, true);

		depth = bottom_depth;
		gi = bottom_gi;
		clock = previous_point_time = bottom_time;
		gas = bottom_gas;
		stopping = false;
		decodive = false;
		first_stop_depth = 0;
		stopidx = bottom_stopidx;
		ds->first_ceiling_pressure.mbar = depth_to_mbar(
					deco_allowed_depth(tissue_tolerance_calc(ds, dive, depth_to_bar(depth, dive), true),
							   diveplan->surface_pressure / 1000.0, dive, 1),
					dive);
		if (ds->max_bottom_ceiling_pressure.mbar > ds->first_ceiling_pressure.mbar)
			ds->first_ceiling_pressure.mbar = ds->max_bottom_ceiling_pressure.mbar;

		last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
		/* Always prefer the best_first_ascend_cylinder if it has the right gasmix.
		 * Otherwise take first cylinder from list with rightgasmix  */
		if (same_gasmix(gas, get_cylinder(dive, best_first_ascend_cylinder)->gasmix))
			current_cylinder = best_first_ascend_cylinder;
		else
			current_cylinder = get_gasidx(dive, gas);
		if (current_cylinder == -1) {
			report_error(translate("gettextFromC", "Can't find gas %s"), gasname(gas));
			current_cylinder = 0;
		}
		reset_regression(ds);
		while (1) {
			/* We will break out when we hit the surface */
			do {
				/* Ascend to next stop depth */
				int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
				if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
					if (is_final_plan)
						plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
					previous_point_time = clock;
					stopping = false;
					last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
				}
				if (depth - deltad < stoplevels[stopidx])
					deltad = depth - stoplevels[stopidx];

				add_segment(ds, depth_to_bar(depth, dive),
								get_cylinder(dive, current_cylinder)->gasmix,
								TIMESTEP, po2, divemode, prefs.decosac, true);
				last_segment_min_switch = false;
				clock += TIMESTEP;
				depth -= deltad;
				/* Print VPM-Gradient as gradient factor, this has to be done from within deco.c */
				if (decodive)
					ds->plot_depth = depth;
			} while (depth > 0 && depth > stoplevels[stopidx]);

			if (depth <= 0)
				break; /* We are at the surface */

			if (gi >= 0 && stoplevels[stopidx] <= gaschanges[gi].depth) {
				/* We have reached a gas change.
				 * Record this in the dive plan */

				/* Check we need to change cylinder.
				 * We might not if the cylinder was chosen by the user
				 * or user has selected only to switch only at required stops.
				 * If current gas is hypoxic, we want to switch asap */

				if (current_cylinder != gaschanges[gi].gasidx) {
					if (!prefs.switch_at_req_stop ||
							!trial_ascent(ds, 0, depth, stoplevels[stopidx - 1], avg_depth, bottom_time,
							get_cylinder(dive, current_cylinder)->gasmix, po2, diveplan->surface_pressure / 1000.0, dive, divemode) || get_o2(get_cylinder(dive, current_cylinder)->gasmix) < 160) {
						if (is_final_plan)
							plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false, divemode);
						stopping = true;
						previous_point_time = clock;
						current_cylinder = gaschanges[gi].gasidx;
						gas = get_cylinder(dive, current_cylinder)->gasmix;
						if (divemode == CCR)
							po2 = setpoint_change(dive, current_cylinder);
#if DEBUG_PLAN & 16
						printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
							(get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[gi].depth / 1000.0);
#endif
						/* Stop for the minimum duration to switch gas unless we switch to o2 */
						if (!last_segment_min_switch && get_o2(get_cylinder(dive, current_cylinder)->gasmix) != 1000) {
							add_segment(ds, depth_to_bar(depth, dive),
								get_cylinder(dive, current_cylinder)->gasmix,
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
						get_cylinder(dive, current_cylinder)->gasmix, po2, diveplan->surface_pressure / 1000.0, dive, divemode)) {
					decostoptable[decostopcounter].depth = depth;
					decostoptable[decostopcounter].time = 0;
					decostopcounter++;
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
					gas = get_cylinder(dive, current_cylinder)->gasmix;
					if (divemode == CCR)
						po2 = setpoint_change(dive, current_cylinder);
#if DEBUG_PLAN & 16
					printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi + 1].gasidx,
						(get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[gi + 1].depth / 1000.0);
#endif
					/* Stop for the minimum duration to switch gas unless we switch to o2 */
					if (!last_segment_min_switch && get_o2(get_cylinder(dive, current_cylinder)->gasmix) != 1000) {
						add_segment(ds, depth_to_bar(depth, dive),
							get_cylinder(dive, current_cylinder)->gasmix,
							prefs.min_switch_duration, po2, divemode, prefs.decosac, true);
						clock += prefs.min_switch_duration;
						last_segment_min_switch = true;
					}
					pendinggaschange = false;
				}

				int new_clock = wait_until(ds, dive, clock, clock, laststoptime * 2 + 1, timestep, depth, stoplevels[stopidx], avg_depth,
					bottom_time, get_cylinder(dive, current_cylinder)->gasmix, po2, diveplan->surface_pressure / 1000.0, divemode);
				laststoptime = new_clock - clock;
				/* Finish infinite deco */
				if (laststoptime >= 48 * 3600 && depth >= 6000) {
					error = LONGDECO;
					break;
				}

				o2breaking = false;
				stop_cylinder = current_cylinder;
				if (prefs.doo2breaks && prefs.last_stop) {
					/* The backgas breaks option limits time on oxygen to 12 minutes, followed by 6 minutes on
					 * backgas.  This could be customized if there were demand.
					 */
					if (break_cylinder == -1) {
						if (get_o2(get_cylinder(dive, best_first_ascend_cylinder)->gasmix) <= 320)
							break_cylinder = best_first_ascend_cylinder;
						else
							break_cylinder = 0;
					}
					if (get_o2(get_cylinder(dive, current_cylinder)->gasmix) == 1000) {
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
							gas = get_cylinder(dive, current_cylinder)->gasmix;
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
							gas = get_cylinder(dive, current_cylinder)->gasmix;
						}
					}
				}
				add_segment(ds, depth_to_bar(depth, dive), get_cylinder(dive, stop_cylinder)->gasmix,
					    laststoptime, po2, divemode, prefs.decosac, true);
				last_segment_min_switch = false;
				decostoptable[decostopcounter].depth = depth;
				decostoptable[decostopcounter].time = laststoptime;
				++decostopcounter;

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
		ds->deco_time = clock - bottom_time - stoplevels[stopidx + 1] / last_ascend_rate + 20;
	} while (!is_final_plan);
	decostoptable[decostopcounter].depth = 0;

	plan_add_segment(diveplan, clock - previous_point_time, 0, current_cylinder, po2, false, divemode);
	if (decoMode(true) == VPMB) {
		diveplan->eff_gfhigh = lrint(100.0 * regressionb(ds));
		diveplan->eff_gflow = lrint(100.0 * (regressiona(ds) * first_stop_depth + regressionb(ds)));
	}

	if (prefs.surface_segment != 0) {
		// Switch to an empty air cylinder for breathing air at the surface.
		// FIXME: This is incredibly silly and emulates the old code when
		// we had a fixed cylinder table: It uses an extra fake cylinder
		// past the regular cylinder table, which is not visible to the UI.
		// Fix this as soon as possible!
		current_cylinder = dive->cylinders.nr;
		plan_add_segment(diveplan, prefs.surface_segment, 0, current_cylinder, 0, false, OC);
	}
	create_dive_from_plan(diveplan, dive, is_planner);
	add_plan_to_notes(diveplan, dive, show_disclaimer, error);
	fixup_dc_duration(&dive->dc);

	free(stoplevels);
	free(gaschanges);
	free(bottom_cache);
	return decodive;
}

/*
 * Get a value in tenths (so "10.2" == 102, "9" = 90)
 *
 * Return negative for errors.
 */
static int get_tenths(const char *begin, const char **endp)
{
	char *end;
	int value = strtol(begin, &end, 10);

	if (begin == end)
		return -1;
	value *= 10;

	/* Fraction? We only look at the first digit */
	if (*end == '.') {
		end++;
		if (!isdigit(*end))
			return -1;
		value += *end - '0';
		do {
			end++;
		} while (isdigit(*end));
	}
	*endp = end;
	return value;
}

static int get_permille(const char *begin, const char **end)
{
	int value = get_tenths(begin, end);
	if (value >= 0) {
		/* Allow a percentage sign */
		if (**end == '%')
			++*end;
	}
	return value;
}

int validate_gas(const char *text, struct gasmix *gas)
{
	int o2, he;

	if (!text)
		return 0;

	while (isspace(*text))
		text++;

	if (!*text)
		return 0;

	if (!strcasecmp(text, translate("gettextFromC", "air"))) {
		o2 = O2_IN_AIR;
		he = 0;
		text += strlen(translate("gettextFromC", "air"));
	} else if (!strcasecmp(text, translate("gettextFromC", "oxygen"))) {
		o2 = 1000;
		he = 0;
		text += strlen(translate("gettextFromC", "oxygen"));
	} else if (!strncasecmp(text, translate("gettextFromC", "ean"), 3)) {
		o2 = get_permille(text + 3, &text);
		he = 0;
	} else {
		o2 = get_permille(text, &text);
		he = 0;
		if (*text == '/')
			he = get_permille(text + 1, &text);
	}

	/* We don't want any extra crud */
	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	/* Validate the gas mix */
	if (*text || o2 < 1 || o2 > 1000 || he < 0 || o2 + he > 1000)
		return 0;

	/* Let it rip */
	gas->o2.permille = o2;
	gas->he.permille = he;
	return 1;
}

int validate_po2(const char *text, int *mbar_po2)
{
	int po2;

	if (!text)
		return 0;

	po2 = get_tenths(text, &text);
	if (po2 < 0)
		return 0;

	while (isspace(*text))
		text++;

	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	*mbar_po2 = po2 * 100;
	return 1;
}
