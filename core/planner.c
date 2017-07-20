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
#include "dive.h"
#include "deco.h"
#include "divelist.h"
#include "planner.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"
#include "qthelperfromc.h"
#include "version.h"

#define TIMESTEP 2 /* second */
#define DECOTIMESTEP 60 /* seconds. Unit of deco stop times */

int decostoplevels_metric[] = { 0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000,
				  30000, 33000, 36000, 39000, 42000, 45000, 48000, 51000, 54000, 57000,
				  60000, 63000, 66000, 69000, 72000, 75000, 78000, 81000, 84000, 87000,
				  90000, 100000, 110000, 120000, 130000, 140000, 150000, 160000, 170000,
				  180000, 190000, 200000, 220000, 240000, 260000, 280000, 300000,
				  320000, 340000, 360000, 380000 };
int decostoplevels_imperial[] = { 0, 3048, 6096, 9144, 12192, 15240, 18288, 21336, 24384, 27432,
				30480, 33528, 36576, 39624, 42672, 45720, 48768, 51816, 54864, 57912,
				60960, 64008, 67056, 70104, 73152, 76200, 79248, 82296, 85344, 88392,
				91440, 101600, 111760, 121920, 132080, 142240, 152400, 162560, 172720,
				182880, 193040, 203200, 223520, 243840, 264160, 284480, 304800,
				325120, 345440, 365760, 386080 };

double plangflow, plangfhigh;

extern double regressiona();
extern double regressionb();
extern void reset_regression();

pressure_t first_ceiling_pressure, max_bottom_ceiling_pressure = {};

char *disclaimer;
int plot_depth = 0;
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
		printf("\t%3u:%02u: %dmm gas: %d o2 %d h2\n", FRACTION(dp->time, 60), dp->depth, get_o2(&dp->gasmix), get_he(&dp->gasmix));
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

/* get the gas at a certain time during the dive */
void get_gas_at_time(struct dive *dive, struct divecomputer *dc, duration_t time, struct gasmix *gas)
{
	// we always start with the first gas, so that's our gas
	// unless an event tells us otherwise
	struct event *event = dc->events;
	*gas = dive->cylinder[0].gasmix;
	while (event && event->time.seconds <= time.seconds) {
		if (!strcmp(event->name, "gaschange")) {
			int cylinder_idx = get_cylinder_index(dive, event);
			*gas = dive->cylinder[cylinder_idx].gasmix;
		}
		event = event->next;
	}
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

int get_gasidx(struct dive *dive, struct gasmix *mix)
{
	return find_best_gasmix_match(mix, dive->cylinder, 0);
}

void interpolate_transition(struct dive *dive, duration_t t0, duration_t t1, depth_t d0, depth_t d1, const struct gasmix *gasmix, o2pressure_t po2)
{
	uint32_t j;

	for (j = t0.seconds; j < t1.seconds; j++) {
		int depth = interpolate(d0.mm, d1.mm, j - t0.seconds, t1.seconds - t0.seconds);
		add_segment(depth_to_bar(depth, dive), gasmix, 1, po2.mbar, dive, prefs.bottomsac);
	}
	if (d1.mm > d0.mm)
		calc_crushing_pressure(depth_to_bar(d1.mm, &displayed_dive));
}

/* returns the tissue tolerance at the end of this (partial) dive */
unsigned int tissue_at_end(struct dive *dive, struct deco_state **cached_datap)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i;
	depth_t lastdepth = {};
	duration_t t0 = {}, t1 = {};
	struct gasmix gas;
	unsigned int surface_interval = 0;

	if (!dive)
		return 0;
	if (*cached_datap) {
		restore_deco_state(*cached_datap, true);
	} else {
		surface_interval = init_decompression(dive);
		cache_deco_state(cached_datap);
	}
	dc = &dive->dc;
	if (!dc->samples)
		return 0;
	psample = sample = dc->sample;

	for (i = 0; i < dc->samples; i++, sample++) {
		o2pressure_t setpoint;

		if (i)
			setpoint = sample[-1].setpoint;
		else
			setpoint = sample[0].setpoint;

		t1 = sample->time;
		get_gas_at_time(dive, dc, t0, &gas);
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
		if ((decoMode() == VPMB) && (lastdepth.mm > sample->depth.mm)) {
			pressure_t ceiling_pressure;
			nuclear_regeneration(t0.seconds);
			vpmb_start_gradient();
			ceiling_pressure.mbar = depth_to_mbar(deco_allowed_depth(tissue_tolerance_calc(dive,
													depth_to_bar(lastdepth.mm, dive)),
										dive->surface_pressure.mbar / 1000.0,
										dive,
										1),
								dive);
			if (ceiling_pressure.mbar > max_bottom_ceiling_pressure.mbar)
				max_bottom_ceiling_pressure.mbar = ceiling_pressure.mbar;
		}

		interpolate_transition(dive, t0, t1, lastdepth, sample->depth, &gas, setpoint);
		psample = sample;
		t0 = t1;
	}
	return surface_interval;
}


/* if a default cylinder is set, use that */
void fill_default_cylinder(cylinder_t *cyl)
{
	const char *cyl_name = prefs.default_cylinder;
	struct tank_info_t *ti = tank_info;
	pressure_t pO2 = {.mbar = 1600};

	if (!cyl_name)
		return;
	while (ti->name != NULL) {
		if (strcmp(ti->name, cyl_name) == 0)
			break;
		ti++;
	}
	if (ti->name == NULL)
		/* didn't find it */
		return;
	cyl->type.description = strdup(ti->name);
	if (ti->ml) {
		cyl->type.size.mliter = ti->ml;
		cyl->type.workingpressure.mbar = ti->bar * 1000;
	} else {
		cyl->type.workingpressure.mbar = psi_to_mbar(ti->psi);
		if (ti->psi)
			cyl->type.size.mliter = lrint(cuft_to_l(ti->cuft) * 1000 / bar_to_atm(psi_to_bar(ti->psi)));
	}
	// MOD of air
	cyl->depth = gas_mod(&cyl->gasmix, pO2, &displayed_dive, 1);
}

/* calculate the new end pressure of the cylinder, based on its current end pressure and the
 * latest segment. */
static void update_cylinder_pressure(struct dive *d, int old_depth, int new_depth, int duration, int sac, cylinder_t *cyl, bool in_deco)
{
	volume_t gas_used;
	pressure_t delta_p;
	depth_t mean_depth;
	int factor = 1000;

	if (d->dc.divemode == PSCR)
		factor = prefs.pscr_ratio;

	if (!cyl)
		return;
	mean_depth.mm = (old_depth + new_depth) / 2;
	gas_used.mliter = lrint(depth_to_atm(mean_depth.mm, d) * sac / 60 * duration * factor / 1000);
	cyl->gas_used.mliter += gas_used.mliter;
	if (in_deco)
		cyl->deco_gas_used.mliter += gas_used.mliter;
	if (cyl->type.size.mliter) {
		delta_p.mbar = lrint(gas_used.mliter * 1000.0 / cyl->type.size.mliter * gas_compressibility_factor(&cyl->gasmix, cyl->end.mbar / 1000.0));
		cyl->end.mbar -= delta_p.mbar;
	}
}

/* simply overwrite the data in the displayed_dive
 * return false if something goes wrong */
static void create_dive_from_plan(struct diveplan *diveplan, bool track_gas)
{
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	struct event *ev;
	cylinder_t *cyl;
	int oldpo2 = 0;
	int lasttime = 0;
	depth_t lastdepth = {.mm = 0};
	int lastcylid = 0;
	enum dive_comp_type type = displayed_dive.dc.divemode;

	if (!diveplan || !diveplan->dp)
		return;
#if DEBUG_PLAN & 4
	printf("in create_dive_from_plan\n");
	dump_plan(diveplan);
#endif
	displayed_dive.salinity = diveplan->salinity;
	// reset the cylinders and clear out the samples and events of the
	// displayed dive so we can restart
	reset_cylinders(&displayed_dive, track_gas);
	dc = &displayed_dive.dc;
	dc->when = displayed_dive.when = diveplan->when;
	dc->surface_pressure.mbar = diveplan->surface_pressure;
	dc->salinity = diveplan->salinity;
	free(dc->sample);
	dc->sample = NULL;
	dc->samples = 0;
	dc->alloc_samples = 0;
	while ((ev = dc->events)) {
		dc->events = dc->events->next;
		free(ev);
	}
	dp = diveplan->dp;
	cyl = &displayed_dive.cylinder[lastcylid];
	sample = prepare_sample(dc);
	sample->setpoint.mbar = dp->setpoint;
	sample->sac.mliter = prefs.bottomsac;
	oldpo2 = dp->setpoint;
	if (track_gas && cyl->type.workingpressure.mbar)
		sample->pressure[0].mbar = cyl->end.mbar;
	sample->manually_entered = true;
	finish_sample(dc);
	while (dp) {
		int po2 = dp->setpoint;
		if (dp->setpoint)
			type = CCR;
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
			add_event(dc, lasttime, SAMPLE_EVENT_PO2, 0, po2, QT_TRANSLATE_NOOP("gettextFromC", "SP change"));
			oldpo2 = po2;
		}

		/* Make sure we have the new gas, and create a gas change event */
		if (dp->cylinderid != lastcylid) {
			/* need to insert a first sample for the new gas */
			add_gas_switch_event(&displayed_dive, dc, lasttime + 1, dp->cylinderid);
			cyl = &displayed_dive.cylinder[dp->cylinderid];
			sample = prepare_sample(dc);
			sample[-1].setpoint.mbar = po2;
			sample->time.seconds = lasttime + 1;
			sample->depth = lastdepth;
			sample->manually_entered = dp->entered;
			sample->sac.mliter = dp->entered ? prefs.bottomsac : prefs.decosac;
			if (track_gas && cyl->type.workingpressure.mbar)
				sample->pressure[0].mbar = cyl->sample_end.mbar;
			finish_sample(dc);
			lastcylid = dp->cylinderid;
		}
		/* Create sample */
		sample = prepare_sample(dc);
		/* set po2 at beginning of this segment */
		/* and keep it valid for last sample - where it likely doesn't matter */
		sample[-1].setpoint.mbar = po2;
		sample->setpoint.mbar = po2;
		sample->time.seconds = lasttime = time;
		sample->depth = lastdepth = depth;
		sample->manually_entered = dp->entered;
		sample->sac.mliter = dp->entered ? prefs.bottomsac : prefs.decosac;
		if (track_gas && !sample[-1].setpoint.mbar) {    /* Don't track gas usage for CCR legs of dive */
			update_cylinder_pressure(&displayed_dive, sample[-1].depth.mm, depth.mm, time - sample[-1].time.seconds,
					dp->entered ? diveplan->bottomsac : diveplan->decosac, cyl, !dp->entered);
			if (cyl->type.workingpressure.mbar)
				sample->pressure[0].mbar = cyl->end.mbar;
		}
		finish_sample(dc);
		dp = dp->next;
	}
	dc->divemode = type;
#if DEBUG_PLAN & 32
	save_dive(stdout, &displayed_dive);
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

void add_to_end_of_diveplan(struct diveplan *diveplan, struct divedatapoint *dp)
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
	if (ldp && dp->time != 0)
		dp->time += lasttime;
}

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int cylinderid, int po2, bool entered)
{
	struct divedatapoint *dp = create_dp(duration, depth, cylinderid, po2);
	dp->entered = entered;
	add_to_end_of_diveplan(diveplan, dp);
	return dp;
}

struct gaschanges {
	int depth;
	int gasidx;
};


static struct gaschanges *analyze_gaslist(struct diveplan *diveplan, int *gaschangenr, int depth, int *asc_cylinder)
{
	int nr = 0;
	struct gaschanges *gaschanges = NULL;
	struct divedatapoint *dp = diveplan->dp;
	int best_depth = displayed_dive.cylinder[*asc_cylinder].depth.mm;
	bool total_time_zero = true;
	while (dp) {
		if (dp->time == 0 && total_time_zero) {
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
		       idx, gasname(&displayed_dive.cylinder[idx].gasmix));
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
	(void) bottom_time;
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

void track_ascent_gas(int depth, cylinder_t *cylinder, int avg_depth, int bottom_time, bool safety_stop)
{
	while (depth > 0) {
		int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
		if (deltad > depth)
			deltad = depth;
		update_cylinder_pressure(&displayed_dive, depth, depth - deltad, TIMESTEP, prefs.decosac, cylinder, true);
		if (depth <= 5000 && depth >= (5000 - deltad) && safety_stop) {
			update_cylinder_pressure(&displayed_dive, 5000, 5000, 180, prefs.decosac, cylinder, true);
			safety_stop = false;
		}
		depth -= deltad;
	}
}

// Determine whether ascending to the next stop will break the ceiling.  Return true if the ascent is ok, false if it isn't.
bool trial_ascent(int trial_depth, int stoplevel, int avg_depth, int bottom_time, struct gasmix *gasmix, int po2, double surface_pressure)
{

	bool clear_to_ascend = true;
	struct deco_state *trial_cache = NULL;

	// For consistency with other VPM-B implementations, we should not start the ascent while the ceiling is
	// deeper than the next stop (thus the offgasing during the ascent is ignored).
	// However, we still need to make sure we don't break the ceiling due to on-gassing during ascent.
	cache_deco_state(&trial_cache);
	if (decoMode() == VPMB && (deco_allowed_depth(tissue_tolerance_calc(&displayed_dive,
										 depth_to_bar(stoplevel, &displayed_dive)),
							   surface_pressure, &displayed_dive, 1) > stoplevel)) {
		restore_deco_state(trial_cache, false);
		free(trial_cache);
		return false;
	}

	while (trial_depth > stoplevel) {
		int deltad = ascent_velocity(trial_depth, avg_depth, bottom_time) * TIMESTEP;
		if (deltad > trial_depth) /* don't test against depth above surface */
			deltad = trial_depth;
		add_segment(depth_to_bar(trial_depth, &displayed_dive),
			    gasmix,
			    TIMESTEP, po2, &displayed_dive, prefs.decosac);
		if (deco_allowed_depth(tissue_tolerance_calc(&displayed_dive, depth_to_bar(trial_depth, &displayed_dive)),
				       surface_pressure, &displayed_dive, 1) > trial_depth - deltad) {
			/* We should have stopped */
			clear_to_ascend = false;
			break;
		}
		trial_depth -= deltad;
	}
	restore_deco_state(trial_cache, false);
	free(trial_cache);
	return clear_to_ascend;
}

/* Determine if there is enough gas for the dive.  Return true if there is enough.
 * Also return true if this cannot be calculated because the cylinder doesn't have
 * size or a starting pressure.
 */
bool enough_gas(int current_cylinder)
{
	cylinder_t *cyl;
	cyl = &displayed_dive.cylinder[current_cylinder];

	if (!cyl->start.mbar)
		return true;
	if (cyl->type.size.mliter)
		return (float)(cyl->end.mbar - prefs.reserve_gas) * cyl->type.size.mliter / 1000.0 > (float) cyl->deco_gas_used.mliter;
	else
		return true;
}

// Work out the stops. Return value is if there were any mandatory stops.

bool plan(struct diveplan *diveplan, struct deco_state **cached_datap, bool is_planner, bool show_disclaimer)
{
	int bottom_depth;
	int bottom_gi;
	int bottom_stopidx;
	bool is_final_plan = true;
	int deco_time;
	int previous_deco_time;
	struct deco_state *bottom_cache = NULL;
	struct sample *sample;
	int po2;
	int transitiontime, gi;
	int current_cylinder;
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
	int avg_depth, max_depth, bottom_time = 0;
	int last_ascend_rate;
	int best_first_ascend_cylinder;
	struct gasmix gas, bottom_gas;
	int o2time = 0;
	int breaktime = -1;
	int breakcylinder = 0;
	int error = 0;
	bool decodive = false;
	int first_stop_depth = 0;

	set_gf(diveplan->gflow, diveplan->gfhigh, prefs.gf_low_at_maxdepth);
	set_vpmb_conservatism(diveplan->vpmb_conservatism);
	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = SURFACE_PRESSURE;
	displayed_dive.surface_pressure.mbar = diveplan->surface_pressure;
	clear_deco(displayed_dive.surface_pressure.mbar / 1000.0);
	max_bottom_ceiling_pressure.mbar = first_ceiling_pressure.mbar = 0;
	create_dive_from_plan(diveplan, is_planner);

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
	sample = &displayed_dive.dc.sample[displayed_dive.dc.samples - 1];

	current_cylinder = get_cylinderid_at_time(&displayed_dive, &displayed_dive.dc, sample->time);
	gas = displayed_dive.cylinder[current_cylinder].gasmix;

	po2 = sample->setpoint.mbar;
	depth = displayed_dive.dc.sample[displayed_dive.dc.samples - 1].depth.mm;
	average_max_depth(diveplan, &avg_depth, &max_depth);
	last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);

	/* if all we wanted was the dive just get us back to the surface */
	if (!is_planner) {
		transitiontime = depth / 75; /* this still needs to be made configurable */
		plan_add_segment(diveplan, transitiontime, 0, current_cylinder, po2, false);
		create_dive_from_plan(diveplan, is_planner);
		return false;
	}

#if DEBUG_PLAN & 4
	printf("gas %s\n", gasname(&gas));
	printf("depth %5.2lfm \n", depth / 1000.0);
	printf("current_cylinder %i\n", current_cylinder);
#endif

	best_first_ascend_cylinder = current_cylinder;
	/* Find the gases available for deco */

	if (po2) {	// Don't change gas in CCR mode
		gaschanges = NULL;
		gaschangenr = 0;
	} else {
		gaschanges = analyze_gaslist(diveplan, &gaschangenr, depth, &best_first_ascend_cylinder);
	}
	/* Find the first potential decostopdepth above current depth */
	for (stopidx = 0; stopidx < decostoplevelcount; stopidx++)
		if (*(decostoplevels + stopidx) >= depth)
			break;
	if (stopidx > 0)
		stopidx--;
	/* Stoplevels are either depths of gas changes or potential deco stop depths. */
	stoplevels = sort_stops(decostoplevels, stopidx + 1, gaschanges, gaschangenr);
	stopidx += gaschangenr;

	/* Keep time during the ascend */
	bottom_time = clock = previous_point_time = displayed_dive.dc.sample[displayed_dive.dc.samples - 1].time.seconds;
	gi = gaschangenr - 1;

	/* Set tissue tolerance and initial vpmb gradient at start of ascent phase */
	diveplan->surface_interval = tissue_at_end(&displayed_dive, cached_datap);
	nuclear_regeneration(clock);
	vpmb_start_gradient();

	if (decoMode() == RECREATIONAL) {
		bool safety_stop = prefs.safetystop && max_depth >= 10000;
		track_ascent_gas(depth, &displayed_dive.cylinder[current_cylinder], avg_depth, bottom_time, safety_stop);
		// How long can we stay at the current depth and still directly ascent to the surface?
		do {
			add_segment(depth_to_bar(depth, &displayed_dive),
				    &displayed_dive.cylinder[current_cylinder].gasmix,
				    DECOTIMESTEP, po2, &displayed_dive, prefs.bottomsac);
			update_cylinder_pressure(&displayed_dive, depth, depth, DECOTIMESTEP, prefs.bottomsac, &displayed_dive.cylinder[current_cylinder], false);
			clock += DECOTIMESTEP;
		} while (trial_ascent(depth, 0, avg_depth, bottom_time, &displayed_dive.cylinder[current_cylinder].gasmix,
				      po2, diveplan->surface_pressure / 1000.0) &&
			 enough_gas(current_cylinder));

		// We did stay one DECOTIMESTEP too many.
		// In the best of all worlds, we would roll back also the last add_segment in terms of caching deco state, but
		// let's ignore that since for the eventual ascent in recreational mode, nobody looks at the ceiling anymore,
		// so we don't really have to compute the deco state.
		update_cylinder_pressure(&displayed_dive, depth, depth, -DECOTIMESTEP, prefs.bottomsac, &displayed_dive.cylinder[current_cylinder], false);
		clock -= DECOTIMESTEP;
		plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, true);
		previous_point_time = clock;
		do {
			/* Ascend to surface */
			int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
			if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
				plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
				previous_point_time = clock;
				last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
			}
			if (depth - deltad < 0)
				deltad = depth;

			clock += TIMESTEP;
			depth -= deltad;
			if (depth <= 5000 && depth >= (5000 - deltad) && safety_stop) {
				plan_add_segment(diveplan, clock - previous_point_time, 5000, current_cylinder, po2, false);
				previous_point_time = clock;
				clock += 180;
				plan_add_segment(diveplan, clock - previous_point_time, 5000, current_cylinder, po2, false);
				previous_point_time = clock;
				safety_stop = false;
			}
		} while (depth > 0);
		plan_add_segment(diveplan, clock - previous_point_time, 0, current_cylinder, po2, false);
		create_dive_from_plan(diveplan, is_planner);
		add_plan_to_notes(diveplan, &displayed_dive, show_disclaimer, error);
		fixup_dc_duration(&displayed_dive.dc);

		free(stoplevels);
		free(gaschanges);
		return false;
	}

	if (best_first_ascend_cylinder != current_cylinder) {
		current_cylinder = best_first_ascend_cylinder;
		gas = displayed_dive.cylinder[current_cylinder].gasmix;

#if DEBUG_PLAN & 16
		printf("switch to gas %d (%d/%d) @ %5.2lfm\n", best_first_ascend_cylinder,
		       (get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[best_first_ascend_cylinder].depth / 1000.0);
#endif
	}

	// VPM-B or Buehlmann Deco
	tissue_at_end(&displayed_dive, cached_datap);
	previous_deco_time = 100000000;
	deco_time = 10000000;
	cache_deco_state(&bottom_cache);  // Lets us make several iterations
	bottom_depth = depth;
	bottom_gi = gi;
	bottom_gas = gas;
	bottom_stopidx = stopidx;

	//CVA
	do {
		is_final_plan = (decoMode() == BUEHLMANN) || (previous_deco_time - deco_time < 10);  // CVA time converges
		if (deco_time != 10000000)
			vpmb_next_gradient(deco_time, diveplan->surface_pressure / 1000.0);

		previous_deco_time = deco_time;
		restore_deco_state(bottom_cache, true);

		depth = bottom_depth;
		gi = bottom_gi;
		clock = previous_point_time = bottom_time;
		gas = bottom_gas;
		stopping = false;
		decodive = false;
		first_stop_depth = 0;
		stopidx = bottom_stopidx;
		breaktime = -1;
		breakcylinder = 0;
		o2time = 0;
		first_ceiling_pressure.mbar = depth_to_mbar(deco_allowed_depth(tissue_tolerance_calc(&displayed_dive,
												     depth_to_bar(depth, &displayed_dive)),
									    diveplan->surface_pressure / 1000.0,
									    &displayed_dive,
									    1),
							 &displayed_dive);
		if (max_bottom_ceiling_pressure.mbar > first_ceiling_pressure.mbar)
			first_ceiling_pressure.mbar = max_bottom_ceiling_pressure.mbar;

		last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
		if ((current_cylinder = get_gasidx(&displayed_dive, &gas)) == -1) {
			report_error(translate("gettextFromC", "Can't find gas %s"), gasname(&gas));
			current_cylinder = 0;
		}
		reset_regression();
		while (1) {
			/* We will break out when we hit the surface */
			do {
				/* Ascend to next stop depth */
				int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
				if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
					if (is_final_plan)
						plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
					previous_point_time = clock;
					stopping = false;
					last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
				}
				if (depth - deltad < stoplevels[stopidx])
					deltad = depth - stoplevels[stopidx];

				add_segment(depth_to_bar(depth, &displayed_dive),
								&displayed_dive.cylinder[current_cylinder].gasmix,
								TIMESTEP, po2, &displayed_dive, prefs.decosac);
				clock += TIMESTEP;
				depth -= deltad;
				/* Print VPM-Gradient as gradient factor, this has to be done from within deco.c */
				if (decodive)
					plot_depth = depth;
			} while (depth > 0 && depth > stoplevels[stopidx]);

			if (depth <= 0)
				break; /* We are at the surface */

			if (gi >= 0 && stoplevels[stopidx] <= gaschanges[gi].depth) {
				/* We have reached a gas change.
				 * Record this in the dive plan */
				if (is_final_plan)
					plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
				previous_point_time = clock;
				stopping = true;

				/* Check we need to change cylinder.
				 * We might not if the cylinder was chosen by the user
				 * or user has selected only to switch only at required stops.
				 * If current gas is hypoxic, we want to switch asap */

				if (current_cylinder != gaschanges[gi].gasidx) {
					if (!prefs.switch_at_req_stop ||
							!trial_ascent(depth, stoplevels[stopidx - 1], avg_depth, bottom_time,
							&displayed_dive.cylinder[current_cylinder].gasmix, po2, diveplan->surface_pressure / 1000.0) || get_o2(&displayed_dive.cylinder[current_cylinder].gasmix) < 160) {
						current_cylinder = gaschanges[gi].gasidx;
						gas = displayed_dive.cylinder[current_cylinder].gasmix;
#if DEBUG_PLAN & 16
						printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
							(get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[gi].depth / 1000.0);
#endif
						/* Stop for the minimum duration to switch gas */
						add_segment(depth_to_bar(depth, &displayed_dive),
							&displayed_dive.cylinder[current_cylinder].gasmix,
							prefs.min_switch_duration, po2, &displayed_dive, prefs.decosac);
						clock += prefs.min_switch_duration;
						if (prefs.doo2breaks && get_o2(&displayed_dive.cylinder[current_cylinder].gasmix) == 1000)
							o2time += prefs.min_switch_duration;
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
				if (trial_ascent(depth, stoplevels[stopidx], avg_depth, bottom_time,
						&displayed_dive.cylinder[current_cylinder].gasmix, po2, diveplan->surface_pressure / 1000.0))
					break; /* We did not hit the ceiling */

				/* Add a minute of deco time and then try again */
				if (!decodive) {
					decodive = true;
					first_stop_depth = depth;
				}
				if (!stopping) {
					/* The last segment was an ascend segment.
					 * Add a waypoint for start of this deco stop */
					if (is_final_plan)
						plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
					previous_point_time = clock;
					stopping = true;
				}

				/* Are we waiting to switch gas?
				 * Occurs when the user has selected the option to switch only at required stops
				 */
				if (pendinggaschange) {
					current_cylinder = gaschanges[gi + 1].gasidx;
					gas = displayed_dive.cylinder[current_cylinder].gasmix;
#if DEBUG_PLAN & 16
					printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi + 1].gasidx,
						(get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[gi + 1].depth / 1000.0);
#endif
					/* Stop for the minimum duration to switch gas */
					add_segment(depth_to_bar(depth, &displayed_dive),
						&displayed_dive.cylinder[current_cylinder].gasmix,
						prefs.min_switch_duration, po2, &displayed_dive, prefs.decosac);
					clock += prefs.min_switch_duration;
					if (prefs.doo2breaks && get_o2(&displayed_dive.cylinder[current_cylinder].gasmix) == 1000)
						o2time += prefs.min_switch_duration;
					pendinggaschange = false;
				}

				/* Deco stop should end when runtime is at a whole minute */
				int this_decotimestep;
				this_decotimestep = DECOTIMESTEP - clock % DECOTIMESTEP;

				add_segment(depth_to_bar(depth, &displayed_dive),
								&displayed_dive.cylinder[current_cylinder].gasmix,
								this_decotimestep, po2, &displayed_dive, prefs.decosac);
				clock += this_decotimestep;
				/* Finish infinite deco */
				if (clock >= 48 * 3600 && depth >= 6000) {
					error = LONGDECO;
					break;
				}
				if (prefs.doo2breaks) {
					/* The backgas breaks option limits time on oxygen to 12 minutes, followed by 6 minutes on
					 * backgas (first defined gas).  This could be customized if there were demand.
					 */
					if (get_o2(&displayed_dive.cylinder[current_cylinder].gasmix) == 1000) {
						o2time += DECOTIMESTEP;
						if (o2time >= 12 * 60) {
							breaktime = 0;
							breakcylinder = current_cylinder;
							if (is_final_plan)
								plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
							previous_point_time = clock;
							current_cylinder = 0;
							gas = displayed_dive.cylinder[current_cylinder].gasmix;
						}
					} else {
						if (breaktime >= 0) {
							breaktime += DECOTIMESTEP;
							if (breaktime >= 6 * 60) {
								o2time = 0;
								if (is_final_plan)
									plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
								previous_point_time = clock;
								current_cylinder = breakcylinder;
								gas = displayed_dive.cylinder[current_cylinder].gasmix;
								breaktime = -1;
							}
						}
					}
				}
			}
			if (stopping) {
				/* Next we will ascend again. Add a waypoint if we have spend deco time */
				if (is_final_plan)
					plan_add_segment(diveplan, clock - previous_point_time, depth, current_cylinder, po2, false);
				previous_point_time = clock;
				stopping = false;
			}
		}

		deco_time = clock - bottom_time;
	} while (!is_final_plan);

	plan_add_segment(diveplan, clock - previous_point_time, 0, current_cylinder, po2, false);
	if (decoMode() == VPMB) {
		diveplan->eff_gfhigh = lrint(100.0 * regressionb());
		diveplan->eff_gflow = lrint(100.0 * (regressiona() * first_stop_depth + regressionb()));
	}

	create_dive_from_plan(diveplan, is_planner);
	add_plan_to_notes(diveplan, &displayed_dive, show_disclaimer, error);
	fixup_dc_duration(&displayed_dive.dc);

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
