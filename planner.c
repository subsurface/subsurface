/* planner.c
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */
#include <libintl.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <ctype.h>
#include "dive.h"
#include "divelist.h"
#include "display-gtk.h"


int decostoplevels[] = { 0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000,
		     30000, 33000, 36000, 39000, 42000, 45000, 48000, 51000, 54000, 57000,
		     60000, 63000, 66000, 69000, 72000, 75000, 78000, 81000, 84000, 87000,
		     90000};

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

void get_gas_from_events(struct divecomputer *dc, int time, int *o2, int *he)
{
	struct event *event = dc->events;
	while (event && event->time.seconds <= time) {
		if (!strcmp(event->name, "gaschange")) {
			*o2 = 10 * event->value & 0xffff;
			*he = 10 * event->value >> 16;
		}
		event = event->next;
	}
}


static int get_gasidx(struct dive *dive, int o2, int he)
{
	int gasidx = -1;

	while (++gasidx < MAX_CYLINDERS)
		if (dive->cylinder[gasidx].gasmix.o2.permille == o2 &&
		    dive->cylinder[gasidx].gasmix.he.permille == he)
			return gasidx;
	return -1;
}

static void get_gas_string(int o2, int he, char *text, int len)
{
	if (he == 0 && (o2 == 0 || (o2 >= O2_IN_AIR - 1 && o2 <= O2_IN_AIR + 1)))
		snprintf(text, len, _("air"));
	else if (he == 0)
		snprintf(text, len, _("EAN%d"), (o2 + 5) / 10);
	else
		snprintf(text, len, "(%d/%d)",  (o2 + 5) / 10, (he + 5) / 10);
}

/* returns the tissue tolerance at the end of this (partial) dive */
double tissue_at_end(struct dive *dive, char **cached_datap)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i, j, t0, t1, gasidx, lastdepth;
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
	for (i = 0; i < dc->samples; i++, sample++) {
		int o2 = 0, he = 0;
		t1 = sample->time.seconds;
		get_gas_from_events(&dive->dc, t0, &o2, &he);
		if ((gasidx = get_gasidx(dive, o2, he)) == -1) {
			printf("can't find gas %d/%d\n", o2/10, he/10);
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
int time_at_last_depth(struct dive *dive, int next_stop, char **cached_data_p)
{
	int depth, gasidx, o2 = 0, he = 0;
	double surface_pressure, tissue_tolerance;
	int wait = 0;
	struct sample *sample;

	if (!dive)
		return 0;
	surface_pressure = dive->surface_pressure.mbar / 1000.0;
	tissue_tolerance = tissue_at_end(dive, cached_data_p);
	sample = &dive->dc.sample[dive->dc.samples - 1];
	depth = sample->depth.mm;
	get_gas_from_events(&dive->dc, sample->time.seconds, &o2, &he);
	gasidx = get_gasidx(dive, o2, he);
	while (deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1) > next_stop) {
		wait++;
		tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
					       &dive->cylinder[gasidx].gasmix, 1, sample->po2, dive);
	}
	return wait;
}

int add_gas(struct dive *dive, int o2, int he)
{
	int i;
	struct gasmix *mix;
	cylinder_t *cyl;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cyl = dive->cylinder + i;
		if (cylinder_nodata(cyl))
			break;
		mix = &cyl->gasmix;
		if (o2 == mix->o2.permille && he == mix->he.permille)
			return i;
	}
	if (i == MAX_CYLINDERS) {
		printf("too many cylinders\n");
		return -1;
	}
	mix = &cyl->gasmix;
	mix->o2.permille = o2;
	mix->he.permille = he;
	return i;
}

struct dive *create_dive_from_plan(struct diveplan *diveplan)
{
	struct dive *dive;
	struct divedatapoint *dp;
	struct divecomputer *dc;
	int oldo2 = O2_IN_AIR, oldhe = 0;
	int lasttime = 0;

	if (!diveplan || !diveplan->dp)
		return NULL;
#if DEBUG_PLAN & 4
	printf("in create_dive_from_plan\n");
	dump_plan(diveplan);
#endif
	dive = alloc_dive();
	dive->when = diveplan->when;
	dive->surface_pressure.mbar = diveplan->surface_pressure;
	dc = &dive->dc;
	dc->model = strdup("Simulated Dive");
	dp = diveplan->dp;

	/* let's start with the gas given on the first segment */
	if (dp->o2 || dp->he) {
		oldo2 = dp->o2;
		oldhe = dp->he;
	}
	add_gas(dive, oldo2, oldhe);
	while (dp) {
		int o2 = dp->o2, he = dp->he;
		int time = dp->time;
		int depth = dp->depth;
		struct sample *sample;

		if (time == 0) {
			/* special entries that just inform the algorithm about
			 * additional gases that are available */
			add_gas(dive, o2, he);
			dp = dp->next;
			continue;
		}
		if (!o2 && !he) {
			o2 = oldo2;
			he = oldhe;
		}

		/* Create new gas, and gas change event if necessary */
		if (o2 != oldo2 || he != oldhe) {
			int value = (o2 / 10) | (he / 10 << 16);
			add_gas(dive, o2, he);
			add_event(dc, lasttime, 11, 0, value, "gaschange");
			oldo2 = o2; oldhe = he;
		}

		/* Create sample */
		sample = prepare_sample(dc);
		sample->time.seconds = time;
		sample->depth.mm = depth;
		finish_sample(dc);
		lasttime = time;
		dp = dp->next;
	}
	if (dc->samples == 0) {
		/* not enough there yet to create a dive - most likely the first time is missing */
		free(dive);
		dive = NULL;
	}
#if DEBUG_PLAN & 32
	if (dive)
		save_dive(stdout, dive);
#endif
	return dive;
}

void free_dps(struct divedatapoint *dp)
{
	while (dp) {
		struct divedatapoint *ndp = dp->next;
		free(dp);
		dp = ndp;
	}
}

struct divedatapoint *create_dp(int time_incr, int depth, int o2, int he)
{
	struct divedatapoint *dp;

	dp = malloc(sizeof(struct divedatapoint));
	dp->time = time_incr;
	dp->depth = depth;
	dp->o2 = o2;
	dp->he = he;
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
		*ldpp = dp = create_dp(0, 0, 0, 0);
		ldpp = &((*ldpp)->next);
	}
	return dp;
}

void add_duration_to_nth_dp(struct diveplan *diveplan, int idx, int duration, gboolean is_rel)
{
	struct divedatapoint *pdp, *dp = get_nth_dp(diveplan, idx);
	if (idx > 0) {
		pdp = get_nth_dp(diveplan, idx - 1);
		if (duration && (is_rel || duration <= pdp->time))
			duration += pdp->time;
	}
	dp->time = duration;
}

/* this function is ONLY called from the dialog callback - so it
 * marks this entry as 'entered'.
 * Do NOT call from other parts of the planning code without changing
 * that logic */
void add_depth_to_nth_dp(struct diveplan *diveplan, int idx, int depth)
{
	struct divedatapoint *dp = get_nth_dp(diveplan, idx);
	dp->depth = depth;
	dp->entered = TRUE;
}

void add_gas_to_nth_dp(struct diveplan *diveplan, int idx, int o2, int he)
{
	struct divedatapoint *dp = get_nth_dp(diveplan, idx);
	dp->o2 = o2;
	dp->he = he;
}
void add_to_end_of_diveplan(struct diveplan *diveplan, struct divedatapoint *dp)
{
	struct divedatapoint **lastdp = &diveplan->dp;
	struct divedatapoint *ldp = *lastdp;
	int lasttime = 0;
	while(*lastdp) {
		ldp = *lastdp;
		if (ldp->time > lasttime)
			lasttime = ldp->time;
		lastdp = &(*lastdp)->next;
	}
	*lastdp = dp;
	if (ldp)
		dp->time += lasttime;
}

void plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he)
{
	struct divedatapoint *dp = create_dp(duration, depth, o2, he);
	add_to_end_of_diveplan(diveplan, dp);
}

struct gaschanges {
	int depth;
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
			do {
				if (dive->cylinder[j].gasmix.o2.permille == dp->o2 &&
				    dive->cylinder[j].gasmix.he.permille == dp->he) {
					gaschanges[i].gasidx = j;
					break;
				}
				j++;
			} while (j < MAX_CYLINDERS);
		}
		dp = dp->next;
	}
	*gaschangenr = nr;
#if DEBUG_PLAN & 16
	for (nr = 0; nr < *gaschangenr; nr++)
		printf("gaschange nr %d: @ %5.2lfm gasidx %d (%d/%d)\n", nr, gaschanges[nr].depth / 1000.0,
			gaschanges[nr].gasidx, dive->cylinder[gaschanges[nr].gasidx].gasmix.o2.permille / 10,
			dive->cylinder[gaschanges[nr].gasidx].gasmix.he.permille / 10);
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
	for (k = gnr + dnr -1; k >= 0; k--) {
		printf("stoplevel[%d]: %5.2lfm\n", k, stoplevels[k]/1000.0);
		if (stoplevels[k] == 0)
			break;
	}
#endif
	return stoplevels;
}

static void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive)
{
	char buffer[2000];
	int consumption[MAX_CYLINDERS] = { 0, };
	int len, gasidx, lastdepth = 0, lasttime = 0;
	struct divedatapoint *dp = diveplan->dp;
	int o2, he;

	if (!dp)
		return;

	snprintf(buffer, sizeof(buffer), "Subsurface dive plan\nbased on GFlow = %.0f and GFhigh = %.0f\n\n",
						prefs.gflow * 100, prefs.gfhigh * 100);
	/* we start with gas 0, then check if that was changed */
	o2 = dive->cylinder[0].gasmix.o2.permille;
	he = dive->cylinder[0].gasmix.he.permille;
	do {
		const char *depth_unit;
		char gas[12];
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
		get_gas_string(o2, he, gas, 12);
		gasidx = get_gasidx(dive, o2, he);
		len = strlen(buffer);
		if (dp->depth != lastdepth) {
			used = diveplan->bottomsac / 1000.0 * depth_to_mbar((dp->depth + lastdepth) / 2, dive) *
						(dp->time - lasttime) / 60;
			snprintf(buffer + len, sizeof(buffer) - len, "Transition to %.*f %s in %d:%02d min - runtime %d:%02u on %s\n",
							decimals, depthvalue, depth_unit,
							FRACTION(dp->time - lasttime, 60),
							FRACTION(dp->time, 60),
							gas);
		} else {
			/* we use deco SAC rate during the calculated deco stops, bottom SAC rate everywhere else */
			int sac = dp->entered ? diveplan->bottomsac : diveplan->decosac;
			used = sac / 1000.0 * depth_to_mbar(dp->depth, dive) * (dp->time - lasttime) / 60;
			snprintf(buffer + len, sizeof(buffer) - len, "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s\n",
							decimals, depthvalue, depth_unit,
							FRACTION(dp->time - lasttime, 60),
							FRACTION(dp->time, 60),
							gas);
		}
		consumption[gasidx] += used;
		get_gas_string(newo2, newhe, gas, 12);
		if (o2 != newo2 || he != newhe) {
			len = strlen(buffer);
			snprintf(buffer + len, sizeof(buffer) - len, "Switch gas to %s\n", gas);
		}
		o2 = newo2;
		he = newhe;
		lasttime = dp->time;
		lastdepth = dp->depth;
	} while((dp = dp->next) != NULL);
	len = strlen(buffer);
	snprintf(buffer + len, sizeof(buffer) - len, "Gas consumption:\n");
	for (gasidx = 0; gasidx < MAX_CYLINDERS; gasidx++) {
		double volume;
		const char *unit;
		char gas[12];
		if (consumption[gasidx] == 0)
			continue;
		len = strlen(buffer);
		volume = get_volume_units(consumption[gasidx], NULL, &unit);
		get_gas_string(dive->cylinder[gasidx].gasmix.o2.permille,
				dive->cylinder[gasidx].gasmix.he.permille, gas, 12);
		snprintf(buffer + len, sizeof(buffer) - len, "%.0f%s of %s\n", volume, unit, gas);
	}
	dive->notes = strdup(buffer);
}

void plan(struct diveplan *diveplan, char **cached_datap, struct dive **divep)
{
	struct dive *dive;
	struct sample *sample;
	int wait_time, o2, he;
	int ceiling, depth, transitiontime;
	int stopidx, gi;
	double tissue_tolerance;
	struct gaschanges *gaschanges;
	int gaschangenr;
	int *stoplevels;

	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = SURFACE_PRESSURE;
	if (*divep)
		delete_single_dive(dive_table.nr - 1);
	*divep = dive = create_dive_from_plan(diveplan);
	if (!dive)
		return;
	record_dive(dive);

	sample = &dive->dc.sample[dive->dc.samples - 1];
	/* we start with gas 0, then check if that was changed */
	o2 = dive->cylinder[0].gasmix.o2.permille;
	he = dive->cylinder[0].gasmix.he.permille;
	get_gas_from_events(&dive->dc, sample->time.seconds, &o2, &he);
	depth = dive->dc.sample[dive->dc.samples - 1].depth.mm;
	tissue_tolerance = tissue_at_end(dive, cached_datap);
	ceiling = deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1);
#if DEBUG_PLAN & 4
	printf("gas %d/%d\n", o2, he);
	printf("depth %5.2lfm ceiling %5.2lfm\n", depth / 1000.0, ceiling / 1000.0);
#endif
	if (depth < ceiling) /* that's not good... */
		depth = ceiling;
	for (stopidx = 0; stopidx < sizeof(decostoplevels) / sizeof(int); stopidx++)
		if (decostoplevels[stopidx] >= depth)
			break;
	stopidx--;

	/* so now we now the first decostop level above us
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
		transitiontime = (depth - stoplevels[stopidx]) / 150;
#if DEBUG_PLAN & 2
		printf("transitiontime %d:%02d to depth %5.2lfm\n", FRACTION(transitiontime, 60), stoplevels[stopidx] / 1000.0);
#endif
		plan_add_segment(diveplan, transitiontime, stoplevels[stopidx], o2, he);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan);
		record_dive(dive);
	}
	while (stopidx > 0) { /* this indicates that we aren't surfacing directly */
		if (gi >= 0 && stoplevels[stopidx] == gaschanges[gi].depth) {
			o2 = dive->cylinder[gaschanges[gi].gasidx].gasmix.o2.permille;
			he = dive->cylinder[gaschanges[gi].gasidx].gasmix.he.permille;
#if DEBUG_PLAN & 16
			printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
				o2 / 10, he / 10, gaschanges[gi].depth / 1000.0);
#endif
			gi--;
		}
		wait_time = time_at_last_depth(dive, stoplevels[stopidx - 1], cached_datap);
		/* typically deco plans are done in one minute increments; we may want to
		 * make this configurable at some point */
		wait_time = ((wait_time + 59) / 60) * 60;
#if DEBUG_PLAN & 2
		printf("waittime %d:%02d at depth %5.2lfm\n", FRACTION(wait_time, 60), stoplevels[stopidx] / 1000.0);
#endif
		if (wait_time)
			plan_add_segment(diveplan, wait_time, stoplevels[stopidx], o2, he);
		transitiontime = (stoplevels[stopidx] - stoplevels[stopidx - 1]) / 150;
#if DEBUG_PLAN & 2
		printf("transitiontime %d:%02d to depth %5.2lfm\n", FRACTION(transitiontime, 60), stoplevels[stopidx - 1] / 1000.0);
#endif
		plan_add_segment(diveplan, transitiontime, stoplevels[stopidx - 1], o2, he);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan);
		record_dive(dive);
		stopidx--;
	}
	add_plan_to_notes(diveplan, dive);
	/* now make the dive visible in the dive list */
	report_dives(FALSE, FALSE);
	mark_divelist_changed(TRUE);
	show_and_select_dive(dive);
	free(stoplevels);
	free(gaschanges);
}


/* and now the UI for all this */
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

static int get_thousandths(const char *begin, const char **endp)
{
	char *end;
	int value = strtol(begin, &end, 10);

	if (begin == end)
		return -1;
	value *= 1000;

	/* now deal with up to three more digits after decimal point */
	if (*end == '.') {
		int factor = 100;
		do {
			++end;
			if (!isdigit(*end))
				break;
			value += (*end - '0') * factor;
			factor /= 10;
		} while (factor);
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

static int validate_gas(const char *text, int *o2_p, int *he_p)
{
	int o2, he;

	if (!text)
		return 0;

	while (isspace(*text))
		text++;

	if (!*text)
		return 0;

	if (!strcasecmp(text, "air")) {
		o2 = O2_IN_AIR; he = 0; text += 3;
	} else if (!strncasecmp(text, "ean", 3)) {
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

static int validate_time(const char *text, int *sec_p, int *rel_p)
{
	int min, sec, rel;
	char *end;

	if (!text)
		return 0;

	while (isspace(*text))
		text++;

	rel = 0;
	if (*text == '+') {
		rel = 1;
		text++;
		while (isspace(*text))
			text++;
	}

	min = strtol(text, &end, 10);
	if (text == end)
		return 0;

	if (min < 0 || min > 1000)
		return 0;

	/* Ok, minutes look ok */
	text = end;
	sec = 0;
	if (*text == ':') {
		text++;
		sec = strtol(text, &end, 10);
		if (end != text+2)
			return 0;
		if (sec < 0)
			return 0;
		text = end;
		if (*text == ':') {
			if (sec >= 60)
				return 0;
			min = min*60 + sec;
			text++;
			sec = strtol(text, &end, 10);
			if (end != text+2)
				return 0;
			if (sec < 0)
				return 0;
			text = end;
		}
	}

	/* Maybe we should accept 'min' at the end? */
	if (isspace(*text))
		text++;
	if (*text)
		return 0;

	*sec_p = min*60 + sec;
	*rel_p = rel;
	return 1;
}

static int validate_depth(const char *text, int *mm_p)
{
	int depth, imperial;

	if (!text)
		return 0;

	depth = get_tenths(text, &text);
	if (depth < 0)
		return 0;

	while (isspace(*text))
		text++;

	imperial = get_units()->length == FEET;
	if (*text == 'm') {
		imperial = 0;
		text++;
	} else if (!strcasecmp(text, "ft")) {
		imperial = 1;
		text += 2;
	}
	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	if (imperial) {
		depth = feet_to_mm(depth / 10.0);
	} else {
		depth *= 100;
	}
	*mm_p = depth;
	return 1;
}

static int validate_volume(const char *text, int *sac)
{
	int volume, imperial;

	if (!text)
		return 0;

	volume = get_thousandths(text, &text);
	if (volume < 0)
		return 0;

	while (isspace(*text))
		text++;

	imperial = get_units()->volume == CUFT;
	if (*text == 'l') {
		imperial = 0;
		text++;
	} else if (!strncasecmp(text, "cuft", 4)) {
		imperial = 1;
		text += 4;
	}
	while (isspace(*text) || *text == '/')
		text++;
	if (!strncasecmp(text, "min", 3))
		text += 3;
	while (isspace(*text))
		text++;
	if (*text)
		return 0;
	if (imperial)
		volume = cuft_to_l(volume) + 0.5;	/* correct for mcuft -> ml */
	*sac = volume;
	return 1;
}

static GtkWidget *add_entry_to_box(GtkWidget *box, const char *label)
{
	GtkWidget *entry, *frame;

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 16);
	if (label) {
		frame = gtk_frame_new(label);
		gtk_container_add(GTK_CONTAINER(frame), entry);
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 2);
	}
	return entry;
}

#define MAX_WAYPOINTS 8
GtkWidget *entry_depth[MAX_WAYPOINTS], *entry_duration[MAX_WAYPOINTS], *entry_gas[MAX_WAYPOINTS];
int nr_waypoints = 0;
static GtkListStore *gas_model = NULL;
struct diveplan diveplan = {};
char *cache_data = NULL;
struct dive *planned_dive = NULL;

/* make a copy of the diveplan so far and display the corresponding dive */
void show_planned_dive(void)
{
	struct diveplan tempplan;
	struct divedatapoint *dp, **dpp;

	memcpy(&tempplan, &diveplan, sizeof(struct diveplan));
	dpp = &tempplan.dp;
	dp = diveplan.dp;
	while(dp && *dpp) {
		*dpp = malloc(sizeof(struct divedatapoint));
		memcpy(*dpp, dp, sizeof(struct divedatapoint));
		dp = dp->next;
		dpp = &(*dpp)->next;
	}
#if DEBUG_PLAN & 1
	printf("in show_planned_dive:\n");
	dump_plan(&tempplan);
#endif
	plan(&tempplan, &cache_data, &planned_dive);
	free_dps(tempplan.dp);
}

static gboolean gas_focus_out_cb(GtkWidget *entry, GdkEvent *event, gpointer data)
{
	const char *gastext;
	int o2, he;
	int idx = data - NULL;

	gastext = gtk_entry_get_text(GTK_ENTRY(entry));
	o2 = he = 0;
	if (validate_gas(gastext, &o2, &he))
		add_string_list_entry(gastext, gas_model);
	add_gas_to_nth_dp(&diveplan, idx, o2, he);
	show_planned_dive();
	return FALSE;
}

static void gas_changed_cb(GtkWidget *combo, gpointer data)
{
	const char *gastext;
	int o2, he;
	int idx = data - NULL;

	gastext = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
	/* stupidly this gets called for two reasons:
	 * a) any keystroke into the entry field
	 * b) mouse selection of a dropdown
	 * we only care about b) (a) is handled much better with the focus-out event)
	 * so let's check that the text returned is actually in our model before going on
	 */
	if (match_list(gas_model, gastext) != MATCH_EXACT)
		return;
	o2 = he = 0;
	if (!validate_gas(gastext, &o2, &he)) {
		/* this should never happen as only validated texts should be
		 * in the dropdown */
		printf("invalid gas for row %d\n",idx);
	}
	add_gas_to_nth_dp(&diveplan, idx, o2, he);
	show_planned_dive();
}

static gboolean depth_focus_out_cb(GtkWidget *entry, GdkEvent *event, gpointer data)
{
	const char *depthtext;
	int depth;
	int idx = data - NULL;

	depthtext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_depth(depthtext, &depth)) {
		add_depth_to_nth_dp(&diveplan, idx, depth);
		show_planned_dive();
	} else {
		/* we need to instead change the color of the input field or something */
		printf("invalid depth for row %d\n", idx);
	}
	return FALSE;
}

static gboolean duration_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *durationtext;
	int duration, is_rel;
	int idx = data - NULL;

	durationtext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_time(durationtext, &duration, &is_rel))
		add_duration_to_nth_dp(&diveplan, idx, duration, is_rel);
	show_planned_dive();
	return FALSE;
}

/* Subsurface follows the lead of most divecomputers to use times
 * without timezone - so all times are implicitly assumed to be
 * local time of the dive location; so in order to give the current
 * time in that way we actually need to add the timezone offset */
static timestamp_t current_time_notz(void)
{
	timestamp_t now = time(NULL);
	int offset = g_time_zone_get_offset(g_time_zone_new_local(), 1);
	return now + offset;
}

static gboolean starttime_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *starttimetext;
	int starttime, is_rel;

	starttimetext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_time(starttimetext, &starttime, &is_rel)) {
		/* we alway make this relative for now */
		diveplan.when = current_time_notz() + starttime;
		show_planned_dive();
	} else {
		/* we need to instead change the color of the input field or something */
		printf("invalid starttime\n");
	}
	return FALSE;
}

static gboolean surfpres_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *surfprestext;

	surfprestext = gtk_entry_get_text(GTK_ENTRY(entry));
	diveplan.surface_pressure = atoi(surfprestext);
	show_planned_dive();
	return FALSE;
}

static gboolean sac_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *sactext;

	sactext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_volume(sactext, data))
		show_planned_dive();
	return FALSE;
}

static GtkWidget *add_gas_combobox_to_box(GtkWidget *box, const char *label, int idx)
{
	GtkWidget *frame, *combo;
	GtkEntryCompletion *completion;
	GtkEntry *entry;

	if (!gas_model) {
		gas_model = gtk_list_store_new(1, G_TYPE_STRING);
		add_string_list_entry("AIR", gas_model);
		add_string_list_entry("EAN32", gas_model);
		add_string_list_entry("EAN36 @ 1.6", gas_model);
	}
	combo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(gas_model), 0);
	gtk_widget_add_events(combo, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(combo)), "focus-out-event", G_CALLBACK(gas_focus_out_cb), NULL + idx);
	g_signal_connect(combo, "changed", G_CALLBACK(gas_changed_cb), NULL + idx);
	if (label) {
		frame = gtk_frame_new(label);
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frame), combo);
	} else {
		gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 2);
	}
	entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(gas_model));
	gtk_entry_completion_set_inline_completion(completion, TRUE);
	gtk_entry_completion_set_inline_selection(completion, TRUE);
	gtk_entry_completion_set_popup_single_match(completion, FALSE);
	gtk_entry_set_completion(entry, completion);
	g_object_unref(completion);

	return combo;
}

static void add_waypoint_widgets(GtkWidget *box, int idx)
{
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
	if (idx == 0) {
		entry_depth[idx] = add_entry_to_box(hbox, _("Ending Depth"));
		entry_duration[idx] = add_entry_to_box(hbox, _("Segment Time"));
		entry_gas[idx] = add_gas_combobox_to_box(hbox, _("Gas Used"), idx);
	} else {
		entry_depth[idx] = add_entry_to_box(hbox, NULL);
		entry_duration[idx] = add_entry_to_box(hbox, NULL);
		entry_gas[idx] = add_gas_combobox_to_box(hbox, NULL, idx);
	}
	gtk_widget_add_events(entry_depth[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_depth[idx], "focus-out-event", G_CALLBACK(depth_focus_out_cb), NULL + idx);
	gtk_widget_add_events(entry_duration[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_duration[idx], "focus-out-event", G_CALLBACK(duration_focus_out_cb), NULL + idx);
}

static void add_waypoint_cb(GtkButton *button, gpointer _data)
{
	GtkWidget *vbox = _data;
	if (nr_waypoints < MAX_WAYPOINTS) {
		GtkWidget *ovbox, *dialog;
		add_waypoint_widgets(vbox, nr_waypoints);
		nr_waypoints++;
		ovbox = gtk_widget_get_parent(GTK_WIDGET(button));
		dialog = gtk_widget_get_parent(ovbox);
		gtk_widget_show_all(dialog);
	} else {
		// some error
	}
}

static void add_entry_with_callback(GtkWidget *box, int length, char *label, char *initialtext,
				gboolean (*callback)(GtkWidget *, GdkEvent *, gpointer ), gpointer data)
{
	GtkWidget *entry = add_entry_to_box(box, label);
	gtk_entry_set_max_length(GTK_ENTRY(entry), length);
	gtk_entry_set_text(GTK_ENTRY(entry), initialtext);
	gtk_widget_add_events(entry, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry, "focus-out-event", G_CALLBACK(callback), data);
}

/* set up the dialog where the user can input their dive plan */
void input_plan()
{
	GtkWidget *planner, *content, *vbox, *hbox, *outervbox, *add_row, *label;
	char *bottom_sac, *deco_sac;

	if (diveplan.dp)
		free_dps(diveplan.dp);
	memset(&diveplan, 0, sizeof(diveplan));
	planned_dive = NULL;
	planner = gtk_dialog_new_with_buttons(_("Dive Plan - THIS IS JUST A SIMULATION; DO NOT USE FOR DIVING"),
					GTK_WINDOW(main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);
	content = gtk_dialog_get_content_area (GTK_DIALOG (planner));
	outervbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add (GTK_CONTAINER (content), outervbox);
	label = gtk_label_new(_("<small>Add segments below.\nEach line describes part of the planned dive.\n"
						"An entry with depth, time and gas describes a segment that ends "
						"at the given depth, takes the given time (if relative, e.g. '+3:30') "
						"or ends at the given time (if absolute), and uses the given gas.\n"
						"An empty gas means 'use previous gas' (or AIR if no gas was specified).\n"
						"An entry that has a depth and a gas given but no time is special; it "
						"informs the planner that the gas specified is available for the ascent "
						"once the depth given has been reached.</small>"));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(outervbox), label, TRUE, TRUE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(outervbox), vbox, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	add_entry_with_callback(hbox, 12, _("Dive starts when?"), "+60:00", starttime_focus_out_cb, NULL);
	add_entry_with_callback(hbox, 12, _("Surface Pressure (mbar)"), SURFACE_PRESSURE_STRING, surfpres_focus_out_cb, NULL);
	if (get_units()->volume == CUFT) {
		bottom_sac = "0.7 cuft/min";
		deco_sac = "0.6 cuft/min";
		diveplan.bottomsac = 1000 * cuft_to_l(0.7);
		diveplan.decosac = 1000 * cuft_to_l(0.6);
	} else {
		bottom_sac = "20 l/min";
		deco_sac = "17 l/min";
		diveplan.bottomsac = 20000;
		diveplan.decosac = 17000;
	}
	add_entry_with_callback(hbox, 12, _("SAC during dive"), bottom_sac, sac_focus_out_cb, &diveplan.bottomsac);
	add_entry_with_callback(hbox, 12, _("SAC during decostop"), deco_sac, sac_focus_out_cb, &diveplan.decosac);

	diveplan.when = current_time_notz() + 3600;
	diveplan.surface_pressure = SURFACE_PRESSURE;
	nr_waypoints = 4;
	add_waypoint_widgets(vbox, 0);
	add_waypoint_widgets(vbox, 1);
	add_waypoint_widgets(vbox, 2);
	add_waypoint_widgets(vbox, 3);
	add_row = gtk_button_new_with_label(_("Add waypoint"));
	g_signal_connect(G_OBJECT(add_row), "clicked", G_CALLBACK(add_waypoint_cb), vbox);
	gtk_box_pack_start(GTK_BOX(outervbox), add_row, FALSE, FALSE, 0);
	gtk_widget_show_all(planner);
	if (gtk_dialog_run(GTK_DIALOG(planner)) == GTK_RESPONSE_ACCEPT) {
		plan(&diveplan, &cache_data, &planned_dive);
	} else {
		if (planned_dive) {
			/* we have added a dive during the dynamic construction
			 * in the dialog; get rid of it */
			delete_single_dive(dive_table.nr - 1);
			report_dives(FALSE, FALSE);
			planned_dive = NULL;
		}
	}
	gtk_widget_destroy(planner);
}
