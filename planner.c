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

unsigned int decostoplevels[] = { 0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000,
				  30000, 33000, 36000, 39000, 42000, 45000, 48000, 51000, 54000, 57000,
				  60000, 63000, 66000, 69000, 72000, 75000, 78000, 81000, 84000, 87000,
				  90000, 100000, 110000, 120000, 130000, 140000, 150000, 160000, 170000,
				  180000, 190000, 200000, 220000, 240000, 260000, 280000, 300000,
				  320000, 340000, 360000, 380000
};
double plangflow, plangfhigh;
char *disclaimer;

#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan)
{
	struct divedatapoint *dp;
	struct tm tm;

	if (!diveplan) {
		printf ("Diveplan NULL\n");
		return;
	}
	utc_mkdate(diveplan->when, &tm);

	printf("\nDiveplan @ %04d-%02d-%02d %02d:%02d:%02d (surfpres %dmbar):\n",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
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
	if (last_stop_6m == TRUE)
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

/* simple helper function to compare two permille values with
 * (rounded) percent granularity */
static inline bool match_percent(int a, int b)
{
	return (a + 5) / 10 == (b + 5) / 10;
}

int get_gasidx(struct dive *dive, int o2, int he)
{
	int gasidx = -1;

	/* we treat air as 0/0 because it is special */
	if (is_air(o2, he))
		o2 = 0;
	while (++gasidx < MAX_CYLINDERS)
		if (match_percent(dive->cylinder[gasidx].gasmix.o2.permille, o2) &&
		    match_percent(dive->cylinder[gasidx].gasmix.he.permille, he))
			return gasidx;
	return -1;
}

void get_gas_string(int o2, int he, char *text, int len)
{
	if (is_air(o2, he))
		snprintf(text, len, "%s", translate("gettextFromC","air"));
	else if (he == 0)
		snprintf(text, len, translate("gettextFromC","EAN%d"), (o2 + 5) / 10);
	else
		snprintf(text, len, "(%d/%d)",  (o2 + 5) / 10, (he + 5) / 10);
}

/* returns the tissue tolerance at the end of this (partial) dive */
double tissue_at_end(struct dive *dive, char **cached_datap, const char **error_string_p)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i, j, t0, t1, gasidx, lastdepth;
	int o2, he;
	double tissue_tolerance;
	static char buf[200];

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
	o2 = dive->cylinder[0].gasmix.o2.permille;
	he = dive->cylinder[0].gasmix.he.permille;
	for (i = 0; i < dc->samples; i++, sample++) {
		t1 = sample->time.seconds;
		get_gas_from_events(&dive->dc, t0, &o2, &he);
		if ((gasidx = get_gasidx(dive, o2, he)) == -1) {
			snprintf(buf, sizeof(buf),translate("gettextFromC","Can't find gas %d/%d"), (o2 + 5) / 10, (he + 5) / 10);
			*error_string_p = buf;
			gasidx = 0;
		}
		if (i > 0)
			lastdepth = psample->depth.mm;
		for (j = t0; j < t1; j++) {
			int depth = interpolate(lastdepth, sample->depth.mm, j - t0, t1 - t0);
			tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
						       &dive->cylinder[gasidx].gasmix, 1, sample->po2, dive);
		}
		psample = sample;
		t0 = t1;
	}
	return tissue_tolerance;
}

/* how many seconds until we can ascend to the next stop? */
static int time_at_last_depth(struct dive *dive, int o2, int he, unsigned int next_stop, char **cached_data_p, const char **error_string_p)
{
	int depth, gasidx;
	double surface_pressure, tissue_tolerance;
	int wait = 0;
	struct sample *sample;

	if (!dive)
		return 0;
	surface_pressure = dive->dc.surface_pressure.mbar / 1000.0;
	tissue_tolerance = tissue_at_end(dive, cached_data_p, error_string_p);
	sample = &dive->dc.sample[dive->dc.samples - 1];
	depth = sample->depth.mm;
	gasidx = get_gasidx(dive, o2, he);
	while (deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1) > next_stop) {
		wait++;
		tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
					       &dive->cylinder[gasidx].gasmix, 1, sample->po2, dive);
	}
	return wait;
}

void fill_default_cylinder(cylinder_t *cyl)
{
	const char *cyl_name = prefs.default_cylinder != NULL ? prefs.default_cylinder : "AL80";
	struct tank_info_t *ti = tank_info;
	struct tank_info_t *al80 = NULL;

	while (ti->name != NULL) {
		if (strcmp(ti->name, cyl_name) == 0)
			break;
		if (strcmp(ti->name, "AL80") == 0)
			al80 = ti;
		ti++;
	}
	if (ti->name == NULL)
		ti = al80;
	cyl->type.description = strdup(ti->name);
	if (ti->ml) {
		cyl->type.size.mliter = ti->ml;
		cyl->type.workingpressure.mbar = ti->bar * 1000;
	} else {
		cyl->type.workingpressure.mbar = psi_to_mbar(ti->psi);
		if (ti->psi)
			cyl->type.size.mliter = cuft_to_l(ti->cuft) * 1000 / bar_to_atm(psi_to_bar(ti->psi));
	}
}

static int add_gas(struct dive *dive, int o2, int he)
{
	int i;
	struct gasmix *mix;
	cylinder_t *cyl;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cyl = dive->cylinder + i;
		mix = &cyl->gasmix;
		if (cylinder_nodata(cyl))
			break;
		if (match_percent(o2, mix->o2.permille) && match_percent(he, mix->he.permille))
			return i;
	}
	if (i == MAX_CYLINDERS) {
		return -1;
	}
	/* let's make it our default cylinder */
	fill_default_cylinder(cyl);
	mix->o2.permille = o2;
	mix->he.permille = he;
	return i;
}

struct dive *create_dive_from_plan(struct diveplan *diveplan, const char **error_string)
{
	struct dive *dive;
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	int oldo2 = O2_IN_AIR, oldhe = 0;
	int oldpo2 = 0;
	int lasttime = 0;

	*error_string = NULL;
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
				add_event(dc, lasttime, 20, 0, po2, "SP change"); // SAMPLE_EVENT_PO2
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
			oldo2 = o2; oldhe = he;
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
	*error_string = translate("gettextFromC","Too many gas mixes");
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
	dp->entered = FALSE;
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

struct divedatapoint * plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he, int po2)
{
	struct divedatapoint *dp = create_dp(duration, depth, o2, he, po2);
	add_to_end_of_diveplan(diveplan, dp);
	return(dp);
}

struct gaschanges {
	unsigned int depth;
	int gasidx;
};

static struct gaschanges *analyze_gaslist(struct diveplan *diveplan, struct dive *dive, int *gaschangenr)
{
	int nr = 0;
	struct gaschanges *gaschanges = NULL;
	struct divedatapoint *dp = diveplan->dp;

	while (dp) {
		if (dp->time == 0) {
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
			do {
				if (dive->cylinder[j].gasmix.o2.permille == dp->o2 &&
				    dive->cylinder[j].gasmix.he.permille == dp->he) {
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
			gaschanges[nr].gasidx, (dive->cylinder[gaschanges[nr].gasidx].gasmix.o2.permille + 5) / 10,
			(dive->cylinder[gaschanges[nr].gasidx].gasmix.he.permille + 5) / 10);
#endif
	return gaschanges;
}

/* sort all the stops into one ordered list */
static unsigned int *sort_stops(unsigned int *dstops, int dnr, struct gaschanges *gstops, int gnr)
{
	int i, gi, di;
	int total = dnr + gnr;
	unsigned int *stoplevels = malloc(total * sizeof(unsigned int));

	/* no gaschanges */
	if (gnr == 0) {
		memcpy(stoplevels, dstops, dnr * sizeof(unsigned int));
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
	for (k = gnr + dnr -1; k >= 0; k--) {
		printf("stoplevel[%d]: %5.2lfm\n", k, stoplevels[k]/1000.0);
		if (stoplevels[k] == 0)
			break;
	}
#endif
	return stoplevels;
}

#if DO_WE_WANT_THIS_IN_QT
static void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive)
{
	char buffer[20000];
	int consumption[MAX_CYLINDERS] = { 0, };
	int len, gasidx, lastdepth = 0, lasttime = 0;
	struct divedatapoint *dp = diveplan->dp;
	int o2, he;

	if (!dp)
		return;

	snprintf(buffer, sizeof(buffer), translate("gettextFromC","%s\nSubsurface dive plan\nbased on GFlow = %.0f and GFhigh = %.0f\n\n"),
					disclaimer, plangflow * 100, plangfhigh * 100);
	/* we start with gas 0, then check if that was changed */
	o2 = dive->cylinder[0].gasmix.o2.permille;
	he = dive->cylinder[0].gasmix.he.permille;
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
			snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC","Transition to %.*f %s in %d:%02d min - runtime %d:%02u on %s\n"),
							decimals, depthvalue, depth_unit,
							FRACTION(dp->time - lasttime, 60),
							FRACTION(dp->time, 60),
							gas);
		} else {
			/* we use deco SAC rate during the calculated deco stops, bottom SAC rate everywhere else */
			int sac = dp->entered ? diveplan->bottomsac : diveplan->decosac;
			used = sac / 1000.0 * depth_to_mbar(dp->depth, dive) * (dp->time - lasttime) / 60;
			snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC","Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s\n"),
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
			snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC","Switch gas to %s\n"), gas);
		}
		o2 = newo2;
		he = newhe;
		lasttime = dp->time;
		lastdepth = dp->depth;
	} while ((dp = dp->next) != NULL);
	len = strlen(buffer);
	snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC","Gas consumption:\n"));
	for (gasidx = 0; gasidx < MAX_CYLINDERS; gasidx++) {
		double volume;
		const char *unit;
		char gas[64];
		if (consumption[gasidx] == 0)
			continue;
		len = strlen(buffer);
		volume = get_volume_units(consumption[gasidx], NULL, &unit);
		get_gas_string(dive->cylinder[gasidx].gasmix.o2.permille,
				dive->cylinder[gasidx].gasmix.he.permille, gas, sizeof(gas));
		snprintf(buffer + len, sizeof(buffer) - len, translate("gettextFromC","%.0f%s of %s\n"), volume, unit, gas);
	}
	dive->notes = strdup(buffer);
}
#endif

void plan(struct diveplan *diveplan, char **cached_datap, struct dive **divep, bool add_deco, const char **error_string_p)
{
	struct dive *dive;
	struct sample *sample;
	int wait_time, o2, he, po2;
	int transitiontime, gi;
	unsigned int stopidx, depth, ceiling;
	double tissue_tolerance;
	struct gaschanges *gaschanges = NULL;
	int gaschangenr;
	unsigned int *stoplevels = NULL;

	set_gf(diveplan->gflow, diveplan->gfhigh, default_prefs.gf_low_at_maxdepth);
	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = SURFACE_PRESSURE;
	if (*divep)
		delete_single_dive(dive_table.nr - 1);
	*divep = dive = create_dive_from_plan(diveplan, error_string_p);
	if (!dive)
		return;
	record_dive(dive);

	sample = &dive->dc.sample[dive->dc.samples - 1];
	/* we start with gas 0, then check if that was changed */
	o2 = dive->cylinder[0].gasmix.o2.permille;
	he = dive->cylinder[0].gasmix.he.permille;
	get_gas_from_events(&dive->dc, sample->time.seconds, &o2, &he);
	po2 = dive->dc.sample[dive->dc.samples - 1].po2;
	depth = dive->dc.sample[dive->dc.samples - 1].depth.mm;

	/* if all we wanted was the dive just get us back to the surface */
	if (!add_deco) {
		transitiontime = depth / 75; /* this still needs to be made configurable */
		plan_add_segment(diveplan, transitiontime, 0, o2, he, po2);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan, error_string_p);
		if (dive)
			record_dive(dive);
		return;
	}

	tissue_tolerance = tissue_at_end(dive, cached_datap, error_string_p);
	ceiling = deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1);
#if DEBUG_PLAN & 4
	printf("gas %d/%d\n", o2, he);
	printf("depth %5.2lfm ceiling %5.2lfm\n", depth / 1000.0, ceiling / 1000.0);
#endif
	if (depth < ceiling) /* that's not good... */
		depth = ceiling;
	if (depth == 0 && ceiling == 0) /* we are done here */
		goto done;
	for (stopidx = 0; stopidx < sizeof(decostoplevels) / sizeof(int); stopidx++)
		if (decostoplevels[stopidx] >= ceiling)
			break;
	if (stopidx > 0)
		stopidx--;

	/* so now we know the first decostop level above us
	 * NOTE, this could be the surface or a long list of potential stops
	 * we do NOT start only at the ceiling, as the ceiling may come down
	 * further during the ascent.
	 * Next we need to figure out if there are better gases available
	 * and at which depths we are supposed to switch to them */
	gaschanges = analyze_gaslist(diveplan, dive, &gaschangenr);
	stoplevels = sort_stops(decostoplevels, stopidx + 1, gaschanges, gaschangenr);

	gi = gaschangenr - 1;
	stopidx += gaschangenr;
	if (depth > stoplevels[stopidx]) {
		/* right now all the transitions are at 30ft/min - this needs to be configurable */
		transitiontime = (depth - stoplevels[stopidx]) / 150;
#if DEBUG_PLAN & 2
		printf("transitiontime %d:%02d to depth %5.2lfm\n", FRACTION(transitiontime, 60), stoplevels[stopidx] / 1000.0);
#endif
		plan_add_segment(diveplan, transitiontime, stoplevels[stopidx], o2, he, po2);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan, error_string_p);
		if (!dive)
			goto error_exit;
		record_dive(dive);
	}
	while (stopidx > 0) { /* this indicates that we aren't surfacing directly */
		/* if we are in a double-step, eg, when 3m/10ft stop is disabled,
		 * just skip the first stop at that depth */
		if (stoplevels[stopidx] == stoplevels[stopidx - 1]) {
			stopidx--;
			continue;
		}
		if (gi >= 0 && stoplevels[stopidx] == gaschanges[gi].depth) {
			o2 = dive->cylinder[gaschanges[gi].gasidx].gasmix.o2.permille;
			he = dive->cylinder[gaschanges[gi].gasidx].gasmix.he.permille;
#if DEBUG_PLAN & 16
			printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
				(o2 + 5) / 10, (he + 5) / 10, gaschanges[gi].depth / 1000.0);
#endif
			gi--;
		}
		wait_time = time_at_last_depth(dive, o2, he, stoplevels[stopidx - 1], cached_datap, error_string_p);
		/* typically deco plans are done in one minute increments; we may want to
		 * make this configurable at some point */
		wait_time = ((wait_time + 59) / 60) * 60;
#if DEBUG_PLAN & 2
		tissue_tolerance = tissue_at_end(dive, cached_datap, error_string_p);
		ceiling = deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1);
		printf("waittime %d:%02d at depth %5.2lfm; ceiling %5.2lfm\n", FRACTION(wait_time, 60),
								stoplevels[stopidx] / 1000.0, ceiling / 1000.0);
#endif
		if (wait_time)
			plan_add_segment(diveplan, wait_time, stoplevels[stopidx], o2, he, po2);
		/* right now all the transitions are at 30ft/min - this needs to be configurable */
		transitiontime = (stoplevels[stopidx] - stoplevels[stopidx - 1]) / 150;
#if DEBUG_PLAN & 2
		printf("transitiontime %d:%02d to depth %5.2lfm\n", FRACTION(transitiontime, 60), stoplevels[stopidx - 1] / 1000.0);
#endif
		plan_add_segment(diveplan, transitiontime, stoplevels[stopidx - 1], o2, he, po2);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan, error_string_p);
		if (!dive)
			goto error_exit;
		record_dive(dive);
		stopidx--;
	}

done:

#if DO_WE_WANT_THIS_IN_QT
	add_plan_to_notes(diveplan, dive);
#endif

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

	if (!strcasecmp(text, translate("gettextFromC","air"))) {
		o2 = O2_IN_AIR; he = 0; text += strlen(translate("gettextFromC","air"));
	} else if (!strncasecmp(text, translate("gettextFromC","ean"), 3)) {
		o2 = get_permille(text+3, &text); he = 0;
	} else {
		o2 = get_permille(text, &text); he = 0;
		if (*text == '/')
			he = get_permille(text+1, &text);
	}

	/* We don't want any extra crud */
	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	/* Validate the gas mix */
	if (*text || o2 < 1 || o2 > 1000 || he < 0 || o2+he > 1000)
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

