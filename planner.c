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
#include "divelist.h"
#include "planner.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"

#define TIMESTEP 1 /* second */
#define DECOTIMESTEP 60 /* seconds. Unit of deco stop times */

int decostoplevels[] = { 0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000,
				  30000, 33000, 36000, 39000, 42000, 45000, 48000, 51000, 54000, 57000,
				  60000, 63000, 66000, 69000, 72000, 75000, 78000, 81000, 84000, 87000,
				  90000, 100000, 110000, 120000, 130000, 140000, 150000, 160000, 170000,
				  180000, 190000, 200000, 220000, 240000, 260000, 280000, 300000,
				  320000, 340000, 360000, 380000 };
double plangflow, plangfhigh;
char *disclaimer;

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
	       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec,
	       diveplan->surface_pressure);
	dp = diveplan->dp;
	while (dp) {
		printf("\t%3u:%02u: %dmm gas: %d o2 %d h2\n", FRACTION(dp->time, 60), dp->depth, dp->o2, dp->he);
		dp = dp->next;
	}
}
#endif

void set_last_stop(bool last_stop_6m)
{
	if (last_stop_6m == true)
		decostoplevels[1] = 6000;
	else
		decostoplevels[1] = 3000;
}

void get_gas_from_events(struct divecomputer *dc, int time, int *o2, int *he)
{
	// we don't modify the values passed in if nothing is found
	// so don't call with uninitialized o2/he !
	struct event *event = dc->events;
	while (event && event->time.seconds <= time) {
		if (!strcmp(event->name, "gaschange")) {
			*o2 = 10 * event->value & 0xffff;
			*he = 10 * event->value >> 16;
		}
		event = event->next;
	}
}

int get_gasidx(struct dive *dive, int o2, int he)
{
	int gasidx = -1;
	struct gasmix mix;

	mix.o2.permille = o2;
	mix.he.permille = he;

	/* we treat air as 0/0 because it is special */
	//if (is_air(o2, he))
	//	o2 = 0;
	while (++gasidx < MAX_CYLINDERS)
		if (gasmix_distance(&dive->cylinder[gasidx].gasmix, &mix) < 200)
			return gasidx;
	return -1;
}

void get_gas_string(int o2, int he, char *text, int len)
{
	if (is_air(o2, he))
		snprintf(text, len, "%s", translate("gettextFromC", "air"));
	else if (he == 0)
		snprintf(text, len, translate("gettextFromC", "EAN%d"), (o2 + 5) / 10);
	else
		snprintf(text, len, "(%d/%d)", (o2 + 5) / 10, (he + 5) / 10);
}

double interpolate_transition(struct dive *dive, int t0, int t1, int d0, int d1, const struct gasmix *gasmix, int ppo2)
{
	int j;
	double tissue_tolerance;

	for (j = t0; j < t1; j++) {
		int depth = interpolate(d0, d1, j - t0, t1 - t0);
		tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0, gasmix, 1, ppo2, dive);
	}
	return tissue_tolerance;
}

/* returns the tissue tolerance at the end of this (partial) dive */
double tissue_at_end(struct dive *dive, char **cached_datap)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i, t0, t1, gasidx, lastdepth;
	int o2, he;
	double tissue_tolerance;

	if (!dive)
		return 0.0;
	if (*cached_datap) {
		tissue_tolerance = restore_deco_state(*cached_datap);
	} else {
		tissue_tolerance = init_decompression(dive);
		cache_deco_state(tissue_tolerance, cached_datap);
	}
	dc = &dive->dc;
	if (!dc->samples)
		return tissue_tolerance;
	psample = sample = dc->sample;
	lastdepth = t0 = 0;
	/* we always start with gas 0 (unless an event tells us otherwise) */
	o2 = get_o2(&dive->cylinder[0].gasmix);
	he = get_he(&dive->cylinder[0].gasmix);
	for (i = 0; i < dc->samples; i++, sample++) {
		t1 = sample->time.seconds;
		get_gas_from_events(&dive->dc, t0, &o2, &he);
		if ((gasidx = get_gasidx(dive, o2, he)) == -1) {
			report_error(translate("gettextFromC", "Can't find gas %d/%d"), (o2 + 5) / 10, (he + 5) / 10);
			gasidx = 0;
		}
		if (i > 0)
			lastdepth = psample->depth.mm;
		tissue_tolerance = interpolate_transition(dive, t0, t1, lastdepth, sample->depth.mm, &dive->cylinder[gasidx].gasmix, sample->po2);
		psample = sample;
		t0 = t1;
	}
	return tissue_tolerance;
}


/* if a default cylinder is set, use that */
void fill_default_cylinder(cylinder_t *cyl)
{
	const char *cyl_name = prefs.default_cylinder;
	struct tank_info_t *ti = tank_info;

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
			cyl->type.size.mliter = cuft_to_l(ti->cuft) * 1000 / bar_to_atm(psi_to_bar(ti->psi));
	}
	cyl->depth.mm = 1600 * 1000 / O2_IN_AIR * 10 - 10000; // MOD of air
}

static int add_gas(struct dive *dive, int o2, int he)
{
	int i;
	struct gasmix *mix;
	struct gasmix mix_in;
	cylinder_t *cyl;

	mix_in.o2.permille = o2;
	mix_in.he.permille = he;
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cyl = dive->cylinder + i;
		mix = &cyl->gasmix;
		if (cylinder_nodata(cyl))
			break;
		if (gasmix_distance(mix, &mix_in) < 200)
			return i;
	}
	if (i == MAX_CYLINDERS) {
		return -1;
	}
	/* let's make it our default cylinder */
	fill_default_cylinder(cyl);
	mix->o2.permille = o2;
	mix->he.permille = he;
	sanitize_gasmix(mix);
	return i;
}

struct dive *create_dive_from_plan(struct diveplan *diveplan)
{
	struct dive *dive;
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	int oldo2 = O2_IN_AIR, oldhe = 0;
	int oldpo2 = 0;
	int lasttime = 0;

	if (!diveplan || !diveplan->dp)
		return NULL;
#if DEBUG_PLAN & 4
	printf("in create_dive_from_plan\n");
	dump_plan(diveplan);
#endif
	dive = alloc_dive();
	dive->when = diveplan->when;
	dive->dc.surface_pressure.mbar = diveplan->surface_pressure;
	dc = &dive->dc;
	dc->model = "planned dive"; /* do not translate here ! */
	dp = diveplan->dp;

	/* let's start with the gas given on the first segment */
	if (dp->o2 || dp->he) {
		oldo2 = dp->o2;
		oldhe = dp->he;
	}
	sample = prepare_sample(dc);
	sample->po2 = dp->po2;
	finish_sample(dc);
	if (add_gas(dive, oldo2, oldhe) < 0)
		goto gas_error_exit;
	while (dp) {
		int o2 = dp->o2, he = dp->he;
		int po2 = dp->po2;
		int time = dp->time;
		int depth = dp->depth;

		if (time == 0) {
			/* special entries that just inform the algorithm about
			 * additional gases that are available */
			if (add_gas(dive, o2, he) < 0)
				goto gas_error_exit;
			dp = dp->next;
			continue;
		}
		if (!o2 && !he) {
			o2 = oldo2;
			he = oldhe;
		}

		/* Check for SetPoint change */
		if (oldpo2 != po2) {
			if (lasttime)
				/* this is a bad idea - we should get a different SAMPLE_EVENT type
				 * reserved for this in libdivecomputer... overloading SMAPLE_EVENT_PO2
				 * with a different meaning will only cause confusion elsewhere in the code */
				add_event(dc, lasttime, SAMPLE_EVENT_PO2, 0, po2, "SP change");
			oldpo2 = po2;
		}

		/* Create new gas, and gas change event if necessary;
		 * Sadly, we inherited our gaschange event from libdivecomputer which only
		 * support percentage values, so round the entries */
		if (o2 != oldo2 || he != oldhe) {
			int plano2 = (o2 + 5) / 10 * 10;
			int planhe = (he + 5) / 10 * 10;
			int idx;
			if ((idx = add_gas(dive, plano2, planhe)) < 0)
				goto gas_error_exit;
			add_gas_switch_event(dive, dc, lasttime, idx);
			oldo2 = o2;
			oldhe = he;
		}
		/* Create sample */
		sample = prepare_sample(dc);
		/* set po2 at beginning of this segment */
		/* and keep it valid for last sample - where it likely doesn't matter */
		sample[-1].po2 = po2;
		sample->po2 = po2;
		sample->time.seconds = time;
		sample->depth.mm = depth;
		finish_sample(dc);
		lasttime = time;
		dp = dp->next;
	}
	if (dc->samples <= 1) {
		/* not enough there yet to create a dive - most likely the first time is missing */
		free(dive);
		dive = NULL;
	}
#if DEBUG_PLAN & 32
	if (dive)
		save_dive(stdout, dive);
#endif
	return dive;

gas_error_exit:
	free(dive);
	report_error(translate("gettextFromC", "Too many gas mixes"));
	return NULL;
}

void free_dps(struct divedatapoint *dp)
{
	while (dp) {
		struct divedatapoint *ndp = dp->next;
		free(dp);
		dp = ndp;
	}
}

struct divedatapoint *create_dp(int time_incr, int depth, int o2, int he, int po2)
{
	struct divedatapoint *dp;

	dp = malloc(sizeof(struct divedatapoint));
	dp->time = time_incr;
	dp->depth = depth;
	dp->o2 = o2;
	dp->he = he;
	dp->po2 = po2;
	dp->entered = false;
	dp->next = NULL;
	return dp;
}

struct divedatapoint *get_nth_dp(struct diveplan *diveplan, int idx)
{
	struct divedatapoint **ldpp, *dp = diveplan->dp;
	int i = 0;
	ldpp = &diveplan->dp;

	while (dp && i++ < idx) {
		ldpp = &dp->next;
		dp = dp->next;
	}
	while (i++ <= idx) {
		*ldpp = dp = create_dp(0, 0, 0, 0, 0);
		ldpp = &((*ldpp)->next);
	}
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

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he, int po2, bool entered)
{
	struct divedatapoint *dp = create_dp(duration, depth, o2, he, po2);
	dp->entered = entered;
	add_to_end_of_diveplan(diveplan, dp);
	return (dp);
}

struct gaschanges {
	int depth;
	int gasidx;
};

static struct gaschanges *analyze_gaslist(struct diveplan *diveplan, struct dive *dive, int *gaschangenr, int depth)
{
	int nr = 0;
	struct gaschanges *gaschanges = NULL;
	struct divedatapoint *dp = diveplan->dp;
	struct gasmix mix;

	while (dp) {
		if (dp->time == 0 && dp->depth <= depth) {
			int i = 0, j = 0;
			nr++;
			gaschanges = realloc(gaschanges, nr * sizeof(struct gaschanges));
			while (i < nr - 1) {
				if (dp->depth < gaschanges[i].depth) {
					memmove(gaschanges + i + 1, gaschanges + i, (nr - i - 1) * sizeof(struct gaschanges));
					break;
				}
				i++;
			}
			gaschanges[i].depth = dp->depth;
			gaschanges[i].gasidx = -1;
			mix.o2.permille = dp->o2;
			mix.he.permille = dp->he;
			do {
				if (!gasmix_distance(&dive->cylinder[j].gasmix, &mix)) {
					gaschanges[i].gasidx = j;
					break;
				}
				j++;
			} while (j < MAX_CYLINDERS);
			assert(gaschanges[i].gasidx != -1);
		}
		dp = dp->next;
	}
	*gaschangenr = nr;
#if DEBUG_PLAN & 16
	for (nr = 0; nr < *gaschangenr; nr++)
		printf("gaschange nr %d: @ %5.2lfm gasidx %d (%d/%d)\n", nr, gaschanges[nr].depth / 1000.0,
		       gaschanges[nr].gasidx, (get_o2(&dive->cylinder[gaschanges[nr].gasidx].gasmix) + 5) / 10,
		       (get_he(&dive->cylinder[gaschanges[nr].gasidx].gasmix) + 5) / 10);
#endif
	return gaschanges;
}

/* sort all the stops into one ordered list */
static unsigned int *sort_stops(int *dstops, int dnr, struct gaschanges *gstops, int gnr)
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

static void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive)
{
	char buffer[20000];
	int consumption[MAX_CYLINDERS] = { 0, };
	int len, gasidx, lastdepth = 0, lasttime = 0;
	struct divedatapoint *dp = diveplan->dp;
	int o2, he;

	if (!dp)
		return;

	snprintf(buffer, sizeof(buffer), translate("gettextFromC", "%s\nSubsurface dive plan\nbased on GFlow = %.0f and GFhigh = %.0f\n\n"),
		 disclaimer, diveplan->gflow * 100, diveplan->gfhigh * 100);
	/* we start with gas 0, then check if that was changed */
	o2 = get_o2(&dive->cylinder[0].gasmix);
	he = get_he(&dive->cylinder[0].gasmix);
	do {
		const char *depth_unit;
		char gas[64];
		double depthvalue;
		int decimals;
		double used;
		int newo2 = o2, newhe = he;
		struct divedatapoint *nextdp;

		if (dp->time == 0)
			continue;
		depthvalue = get_depth_units(dp->depth, &decimals, &depth_unit);
		/* do we change gas after this segment? We need to look at the gas
		 * for the next segment (that isn't just a record of available gas !!)
		 * to find out */
		nextdp = dp->next;
		while (nextdp && nextdp->time == 0)
			nextdp = nextdp->next;
		if (nextdp) {
			newo2 = nextdp->o2;
			newhe = nextdp->he;
			if (newhe == 0 && newo2 == 0) {
				/* same as last segment */
				newo2 = o2;
				newhe = he;
			}
		}
		/* do we want to skip this leg as it is devoid of anything useful? */
		if (!dp->entered && o2 == newo2 && he == newhe && nextdp && dp->depth != lastdepth && nextdp->depth != dp->depth)
			continue;
		get_gas_string(o2, he, gas, sizeof(gas));
		gasidx = get_gasidx(dive, o2, he);
		len = strlen(buffer);
		if (dp->depth != lastdepth) {
			used = diveplan->bottomsac / 1000.0 * depth_to_mbar((dp->depth + lastdepth) / 2, dive) *
			       (dp->time - lasttime) / 60;
			snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC", "Transition to %.*f %s in %d:%02d min - runtime %d:%02u on %s\n"),
				 decimals, depthvalue, depth_unit,
				 FRACTION(dp->time - lasttime, 60),
				 FRACTION(dp->time, 60),
				 gas);
		} else {
			/* we use deco SAC rate during the calculated deco stops, bottom SAC rate everywhere else */
			int sac = dp->entered ? diveplan->bottomsac : diveplan->decosac;
			used = sac / 1000.0 * depth_to_mbar(dp->depth, dive) * (dp->time - lasttime) / 60;
			snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s\n"),
				 decimals, depthvalue, depth_unit,
				 FRACTION(dp->time - lasttime, 60),
				 FRACTION(dp->time, 60),
				 gas);
		}
		if (gasidx != -1)
			consumption[gasidx] += used;
		get_gas_string(newo2, newhe, gas, sizeof(gas));
		if (o2 != newo2 || he != newhe) {
			len = strlen(buffer);
			snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC", "Switch gas to %s\n"), gas);
		}
		o2 = newo2;
		he = newhe;
		lasttime = dp->time;
		lastdepth = dp->depth;
	} while ((dp = dp->next) != NULL);
	len = strlen(buffer);
	snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC", "Gas consumption:\n"));
	for (gasidx = 0; gasidx < MAX_CYLINDERS; gasidx++) {
		double volume;
		const char *unit;
		char gas[64];
		if (dive->cylinder[gasidx].type.size.mliter)
			dive->cylinder[gasidx].end.mbar = dive->cylinder[gasidx].start.mbar - consumption[gasidx] / dive->cylinder[gasidx].type.size.mliter / 1000;
		if (consumption[gasidx] == 0)
			continue;
		len = strlen(buffer);
		volume = get_volume_units(consumption[gasidx], NULL, &unit);
		get_gas_string(get_o2(&dive->cylinder[gasidx].gasmix),
			       get_he(&dive->cylinder[gasidx].gasmix), gas, sizeof(gas));
		snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC", "%.0f%s of %s\n"), volume, unit, gas);
	}
	dive->notes = strdup(buffer);
}

int ascend_velocity(int depth, int avg_depth, int bottom_time)
{
	/* We need to make this configurable */

	/* As an example (and possibly reasonable default) this is the Tech 1 provedure according
	 * to http://www.globalunderwaterexplorers.org/files/Standards_and_Procedures/SOP_Manual_Ver2.0.2.pdf */

	if (depth <= 6000)
		return 1000 / 60;

	if (depth * 4 > avg_depth *3)
		return 9000 / 60;
	else
		return 6000 / 60;
}

void plan(struct diveplan *diveplan, char **cached_datap, struct dive **divep, bool add_deco)
{
	struct dive *dive;
	struct sample *sample;
	int o2, he, po2;
	int transitiontime, gi;
	int current_cylinder;
	unsigned int stopidx;
	int depth;
	double tissue_tolerance;
	struct gaschanges *gaschanges = NULL;
	int gaschangenr;
	int *stoplevels = NULL;
	char *trial_cache = NULL;
	bool stopping = false;
	bool clear_to_ascend;
	int clock, previous_point_time;
	int avg_depth, bottom_time;
	int last_ascend_rate;

	set_gf(diveplan->gflow, diveplan->gfhigh, default_prefs.gf_low_at_maxdepth);
	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = SURFACE_PRESSURE;
	if (*divep)
		delete_single_dive(dive_table.nr - 1);
	*divep = dive = create_dive_from_plan(diveplan);
	if (!dive)
		return;
	record_dive(dive);

	/* Let's start at the last 'sample', i.e. the last manually entered waypoint. */
	sample = &dive->dc.sample[dive->dc.samples - 1];
	/* we start with gas 0, then check if that was changed */
	o2 = get_o2(&dive->cylinder[0].gasmix);
	he = get_he(&dive->cylinder[0].gasmix);
	get_gas_from_events(&dive->dc, sample->time.seconds, &o2, &he);
	po2 = dive->dc.sample[dive->dc.samples - 1].po2;
	if ((current_cylinder = get_gasidx(dive, o2, he)) == -1) {
		report_error(translate("gettextFromC", "Can't find gas %d/%d"), (o2 + 5) / 10, (he + 5) / 10);
		current_cylinder = 0;
	}
	depth = dive->dc.sample[dive->dc.samples - 1].depth.mm;
	avg_depth = average_depth(diveplan);
	last_ascend_rate = ascend_velocity(depth, avg_depth, bottom_time);

	/* if all we wanted was the dive just get us back to the surface */
	if (!add_deco) {
		transitiontime = depth / 75; /* this still needs to be made configurable */
		plan_add_segment(diveplan, transitiontime, 0, o2, he, po2, false);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan);
		if (dive)
			record_dive(dive);
		return;
	}

	tissue_tolerance = tissue_at_end(dive, cached_datap);

#if DEBUG_PLAN & 4
	printf("gas %d/%d\n", o2, he);
	printf("depth %5.2lfm ceiling %5.2lfm\n", depth / 1000.0, ceiling / 1000.0);
#endif

	/* Find the gases available for deco */
	gaschanges = analyze_gaslist(diveplan, dive, &gaschangenr, depth);
	/* Find the first potential decostopdepth above current depth */
	for (stopidx = 0; stopidx < sizeof(decostoplevels) / sizeof(int); stopidx++)
		if (decostoplevels[stopidx] >= depth)
			break;
	if (stopidx > 0)
		stopidx--;
	/* Stoplevels are either depths of gas changes or potential deco stop depths. */
	stoplevels = sort_stops(decostoplevels, stopidx + 1, gaschanges, gaschangenr);
	stopidx += gaschangenr;

	/* Keep time during the ascend */
	bottom_time = clock = previous_point_time = dive->dc.sample[dive->dc.samples - 1].time.seconds;
	gi = gaschangenr - 1;

	while (1) {
		/* We will break out when we hit the surface */
		do {
			/* Ascend to next stop depth */
			assert(deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1) < depth);
			int deltad = ascend_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
			if (ascend_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
				plan_add_segment(diveplan, clock - previous_point_time, depth, o2, he, po2, false);
				previous_point_time = clock;
				stopping = false;
				last_ascend_rate = ascend_velocity(depth, avg_depth, bottom_time);
			}
			if (depth - deltad < stoplevels[stopidx])
				deltad = depth - stoplevels[stopidx];

			tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0, &dive->cylinder[current_cylinder].gasmix, TIMESTEP, po2, dive);
			clock += TIMESTEP;
			depth -= deltad;
		} while (depth > stoplevels[stopidx]);

		if (depth <= 0)
			break;	/* We are at the surface */


		if (gi >= 0 && stoplevels[stopidx] == gaschanges[gi].depth) {
			/* We have reached a gas change.
			 * Record this in the dive plan */
			plan_add_segment(diveplan, clock - previous_point_time, depth, o2, he, po2, false);
			previous_point_time = clock;
			stopping = true;

			current_cylinder = gaschanges[gi].gasidx;
			o2 = get_o2(&dive->cylinder[current_cylinder].gasmix);
			he = get_he(&dive->cylinder[current_cylinder].gasmix);
#if DEBUG_PLAN & 16
			printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
				       (o2 + 5) / 10, (he + 5) / 10, gaschanges[gi].depth / 1000.0);
#endif
			gi--;
		}

		--stopidx;

		/* Save the current state and try to ascend to the next stopdepth */
		int trial_depth = depth;
		cache_deco_state(tissue_tolerance, &trial_cache);
		while(1) {
			/* Check if ascending to next stop is clear, go back and wait if we hit the ceiling on the way */
			clear_to_ascend = true;
			while (trial_depth > stoplevels[stopidx]) {
				int deltad = ascend_velocity(trial_depth, avg_depth, bottom_time) * TIMESTEP;
				tissue_tolerance = add_segment(depth_to_mbar(trial_depth, dive) / 1000.0, &dive->cylinder[current_cylinder].gasmix, TIMESTEP, po2, dive);
				if (deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1) > trial_depth - deltad){
					/* We should have stopped */
					clear_to_ascend = false;
					break;
				}
				trial_depth -= deltad;
			}
			restore_deco_state(trial_cache);

			if(clear_to_ascend)
				break;	/* We did not hit the ceiling */

			/* Add a minute of deco time and then try again */
			if (!stopping) {
				/* The last segment was an ascend segment.
				 * Add a waypoint for start of this deco stop */
				plan_add_segment(diveplan, clock - previous_point_time, depth, o2, he, po2, false);
				previous_point_time = clock;
				stopping = true;
			}
			tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0, &dive->cylinder[current_cylinder].gasmix, DECOTIMESTEP, po2, dive);
			cache_deco_state(tissue_tolerance, &trial_cache);
			clock += DECOTIMESTEP;
			trial_depth = depth;
		}
		if (stopping) {
			/* Next we will ascend again. Add a waypoint if we have spend deco time */
			plan_add_segment(diveplan, clock - previous_point_time, depth, o2, he, po2, false);
			previous_point_time = clock;
			stopping = false;
		}

	}

	/* We made it to the surface */
	plan_add_segment(diveplan, clock - previous_point_time, 0, o2, he, po2, false);
	delete_single_dive(dive_table.nr - 1);
	*divep = dive = create_dive_from_plan(diveplan);
	if (!dive)
		goto error_exit;
	record_dive(dive);

	add_plan_to_notes(diveplan, dive);

error_exit:
	free(stoplevels);
	free(gaschanges);
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

int validate_gas(const char *text, int *o2_p, int *he_p)
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
	*o2_p = o2;
	*he_p = he;
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
