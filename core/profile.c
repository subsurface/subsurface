// SPDX-License-Identifier: GPL-2.0
/* profile.c */
/* creates all the necessary data for drawing the dive profile
 */
#include "ssrf.h"
#include "gettext.h"
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "dive.h"
#include "divelist.h"
#include "event.h"
#include "interpolate.h"
#include "sample.h"
#include "subsurface-string.h"

#include "profile.h"
#include "gaspressures.h"
#include "deco.h"
#include "libdivecomputer/parser.h"
#include "libdivecomputer/version.h"
#include "membuffer.h"
#include "qthelper.h"
#include "format.h"

//#define DEBUG_GAS 1

#define MAX_PROFILE_DECO 7200

extern int ascent_velocity(int depth, int avg_depth, int bottom_time);

#ifdef DEBUG_PI
/* debugging tool - not normally used */
static void dump_pi(struct plot_info *pi)
{
	int i;

	printf("pi:{nr:%d maxtime:%d meandepth:%d maxdepth:%d \n"
	       "    maxpressure:%d mintemp:%d maxtemp:%d\n",
	       pi->nr, pi->maxtime, pi->meandepth, pi->maxdepth,
	       pi->maxpressure, pi->mintemp, pi->maxtemp);
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = &pi->entry[i];
		printf("    entry[%d]:{cylinderindex:%d sec:%d pressure:{%d,%d}\n"
		       "                time:%d:%02d temperature:%d depth:%d stopdepth:%d stoptime:%d ndl:%d smoothed:%d po2:%lf phe:%lf pn2:%lf sum-pp %lf}\n",
		       i, entry->sensor[0], entry->sec,
		       entry->pressure[0], entry->pressure[1],
		       entry->sec / 60, entry->sec % 60,
		       entry->temperature, entry->depth, entry->stopdepth, entry->stoptime, entry->ndl, entry->smoothed,
		       entry->pressures.o2, entry->pressures.he, entry->pressures.n2,
		       entry->pressures.o2 + entry->pressures.he + entry->pressures.n2);
	}
	printf("   }\n");
}
#endif

#define ROUND_UP(x, y) ((((x) + (y) - 1) / (y)) * (y))
#define DIV_UP(x, y) (((x) + (y) - 1) / (y))

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 */
int get_maxtime(const struct plot_info *pi)
{
	int seconds = pi->maxtime;
	int min = prefs.zoomed_plot ? 30 : 30 * 60;
	return MAX(min, seconds);
}

/* get the maximum depth to which we want to plot */
int get_maxdepth(const struct plot_info *pi)
{
	/* 3m to spare */
	int mm = pi->maxdepth + 3000;
	return prefs.zoomed_plot ? mm : MAX(30000, mm);
}

/* UNUSED! */
static int get_local_sac(struct plot_info *pi, int idx1, int idx2, struct dive *dive) __attribute__((unused));

/* Get local sac-rate (in ml/min) between entry1 and entry2 */
static int get_local_sac(struct plot_info *pi, int idx1, int idx2, struct dive *dive)
{
	int index = 0;
	cylinder_t *cyl;
	struct plot_data *entry1 = pi->entry + idx1;
	struct plot_data *entry2 = pi->entry + idx2;
	int duration = entry2->sec - entry1->sec;
	int depth, airuse;
	pressure_t a, b;
	double atm;

	if (duration <= 0)
		return 0;
	a.mbar = get_plot_pressure(pi, idx1, 0);
	b.mbar = get_plot_pressure(pi, idx2, 0);
	if (!b.mbar || a.mbar <= b.mbar)
		return 0;

	/* Mean pressure in ATM */
	depth = (entry1->depth + entry2->depth) / 2;
	atm = depth_to_atm(depth, dive);

	cyl = get_cylinder(dive, index);

	airuse = gas_volume(cyl, a) - gas_volume(cyl, b);

	/* milliliters per minute */
	return lrint(airuse / atm * 60 / duration);
}

static velocity_t velocity(int speed)
{
	velocity_t v;

	if (speed < -304) /* ascent faster than -60ft/min */
		v = CRAZY;
	else if (speed < -152) /* above -30ft/min */
		v = FAST;
	else if (speed < -76) /* -15ft/min */
		v = MODERATE;
	else if (speed < -25) /* -5ft/min */
		v = SLOW;
	else if (speed < 25) /* very hard to find data, but it appears that the recommendations
				for descent are usually about 2x ascent rate; still, we want
				stable to mean stable */
		v = STABLE;
	else if (speed < 152) /* between 5 and 30ft/min is considered slow */
		v = SLOW;
	else if (speed < 304) /* up to 60ft/min is moderate */
		v = MODERATE;
	else if (speed < 507) /* up to 100ft/min is fast */
		v = FAST;
	else /* more than that is just crazy - you'll blow your ears out */
		v = CRAZY;

	return v;
}

static void analyze_plot_info(struct plot_info *pi)
{
	int i;
	int nr = pi->nr;

	/* Smoothing function: 5-point triangular smooth */
	for (i = 2; i < nr; i++) {
		struct plot_data *entry = pi->entry + i;
		int depth;

		if (i < nr - 2) {
			depth = entry[-2].depth + 2 * entry[-1].depth + 3 * entry[0].depth + 2 * entry[1].depth + entry[2].depth;
			entry->smoothed = (depth + 4) / 9;
		}
		/* vertical velocity in mm/sec */
		/* Linus wants to smooth this - let's at least look at the samples that aren't FAST or CRAZY */
		if (entry[0].sec - entry[-1].sec) {
			entry->speed = (entry[0].depth - entry[-1].depth) / (entry[0].sec - entry[-1].sec);
			entry->velocity = velocity(entry->speed);
			/* if our samples are short and we aren't too FAST*/
			if (entry[0].sec - entry[-1].sec < 15 && entry->velocity < FAST) {
				int past = -2;
				while (i + past > 0 && entry[0].sec - entry[past].sec < 15)
					past--;
				entry->velocity = velocity((entry[0].depth - entry[past].depth) /
							   (entry[0].sec - entry[past].sec));
			}
		} else {
			entry->velocity = STABLE;
			entry->speed = 0;
		}
	}
}

/*
 * If the event has an explicit cylinder index,
 * we return that. If it doesn't, we return the best
 * match based on the gasmix.
 *
 * Some dive computers give cylinder indices, some
 * give just the gas mix.
 */
int get_cylinder_index(const struct dive *dive, const struct event *ev)
{
	int best;
	struct gasmix mix;

	if (ev->gas.index >= 0)
		return ev->gas.index;

	/*
	 * This should no longer happen!
	 *
	 * We now match up gas change events with their cylinders at dive
	 * event fixup time.
	 */
	SSRF_INFO("Still looking up cylinder based on gas mix in get_cylinder_index()!\n");

	mix = get_gasmix_from_event(dive, ev);
	best = find_best_gasmix_match(mix, &dive->cylinders);
	return best < 0 ? 0 : best;
}

struct event *get_next_event_mutable(struct event *event, const char *name)
{
	if (!name || !*name)
		return NULL;
	while (event) {
		if (same_string(event->name, name))
			return event;
		event = event->next;
	}
	return event;
}

const struct event *get_next_event(const struct event *event, const char *name)
{
	return get_next_event_mutable((struct event *)event, name);
}

static int count_events(const struct divecomputer *dc)
{
	int result = 0;
	struct event *ev = dc->events;
	while (ev != NULL) {
		result++;
		ev = ev->next;
	}
	return result;
}

static int set_setpoint(struct plot_info *pi, int i, int setpoint, int end)
{
	while (i < pi->nr) {
		struct plot_data *entry = pi->entry + i;
		if (entry->sec > end)
			break;
		entry->o2pressure.mbar = setpoint;
		i++;
	}
	return i;
}

static void check_setpoint_events(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi)
{
	UNUSED(dive);
	int i = 0;
	pressure_t setpoint;
	setpoint.mbar = 0;
	const struct event *ev = get_next_event(dc->events, "SP change");

	if (!ev)
		return;

	do {
		i = set_setpoint(pi, i, setpoint.mbar, ev->time.seconds);
		setpoint.mbar = ev->value;
		ev = get_next_event(ev->next, "SP change");
	} while (ev);
	set_setpoint(pi, i, setpoint.mbar, INT_MAX);
}

static void calculate_max_limits_new(const struct dive *dive, const struct divecomputer *given_dc, struct plot_info *pi, bool in_planner)
{
	const struct divecomputer *dc = &(dive->dc);
	bool seen = false;
	bool found_sample_beyond_last_event = false;
	int maxdepth = dive->maxdepth.mm;
	int maxtime = 0;
	int maxpressure = 0, minpressure = INT_MAX;
	int maxhr = 0, minhr = INT_MAX;
	int mintemp = dive->mintemp.mkelvin;
	int maxtemp = dive->maxtemp.mkelvin;
	int cyl;

	/* Get the per-cylinder maximum pressure if they are manual */
	for (cyl = 0; cyl < dive->cylinders.nr; cyl++) {
		int mbar_start = get_cylinder(dive, cyl)->start.mbar;
		int mbar_end = get_cylinder(dive, cyl)->end.mbar;
		if (mbar_start > maxpressure)
			maxpressure = mbar_start;
		if (mbar_end && mbar_end < minpressure)
			minpressure = mbar_end;
	}

	/* Then do all the samples from all the dive computers */
	do {
		if (dc == given_dc)
			seen = true;
		int i = dc->samples;
		int lastdepth = 0;
		struct sample *s = dc->sample;
		struct event *ev;

		/* Make sure we can fit all events */
		ev = dc->events;
		while (ev) {
			if (ev->time.seconds > maxtime)
				maxtime = ev->time.seconds;
			ev = ev->next;
		}

		while (--i >= 0) {
			int depth = s->depth.mm;
			int temperature = s->temperature.mkelvin;
			int heartbeat = s->heartbeat;

			for (int sensor = 0; sensor < MAX_SENSORS; ++sensor) {
				int pressure = s->pressure[sensor].mbar;
				if (pressure && pressure < minpressure)
					minpressure = pressure;
				if (pressure > maxpressure)
					maxpressure = pressure;
			}

			if (!mintemp && temperature < mintemp)
				mintemp = temperature;
			if (temperature > maxtemp)
				maxtemp = temperature;

			if (heartbeat > maxhr)
				maxhr = heartbeat;
			if (heartbeat && heartbeat < minhr)
				minhr = heartbeat;

			if (depth > maxdepth)
				maxdepth = s->depth.mm;
			/* Make sure that we get the first sample beyond the last event.
			 * If maxtime is somewhere in the middle of the last segment,
			 * populate_plot_entries() gets confused leading to display artifacts. */
			if ((depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD || in_planner || !found_sample_beyond_last_event) &&
			    s->time.seconds > maxtime) {
				found_sample_beyond_last_event = true;
				maxtime = s->time.seconds;
			}
			lastdepth = depth;
			s++;
		}

		dc = dc->next;
		if (dc == NULL && !seen) {
			dc = given_dc;
			seen = true;
		}
	} while (dc != NULL);

	if (minpressure > maxpressure)
		minpressure = 0;
	if (minhr > maxhr)
		minhr = maxhr;

	memset(pi, 0, sizeof(*pi));
	pi->maxdepth = maxdepth;
	pi->maxtime = maxtime;
	pi->maxpressure = maxpressure;
	pi->minpressure = minpressure;
	pi->minhr = minhr;
	pi->maxhr = maxhr;
	pi->mintemp = mintemp;
	pi->maxtemp = maxtemp;
}

/* copy the previous entry (we know this exists), update time and depth
 * and zero out the sensor pressure (since this is a synthetic entry)
 * increment the entry pointer and the count of synthetic entries. */
static void insert_entry(struct plot_info *pi, int idx, int time, int depth, int sac)
{
	struct plot_data *entry = pi->entry + idx;
	struct plot_data *prev = pi->entry + idx - 1;
	*entry = *prev;
	entry->sec = time;
	entry->depth = depth;
	entry->running_sum = prev->running_sum + (time - prev->sec) * (depth + prev->depth) / 2;
	entry->sac = sac;
	entry->ndl = -1;
	entry->bearing = -1;
}

void free_plot_info_data(struct plot_info *pi)
{
	free(pi->entry);
	free(pi->pressures);
	memset(pi, 0, sizeof(*pi));
}

static void populate_plot_entries(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi)
{
	UNUSED(dive);
	int idx, maxtime, nr, i;
	int lastdepth, lasttime, lasttemp = 0;
	struct plot_data *plot_data;
	struct event *ev = dc->events;
	maxtime = pi->maxtime;

	/*
	 * We want to have a plot_info event at least every 10s (so "maxtime/10+1"),
	 * but samples could be more dense than that (so add in dc->samples). We also
	 * need to have one for every event (so count events and add that) and
	 * additionally we want two surface events around the whole thing (thus the
	 * additional 4).  There is also one extra space for a final entry
	 * that has time > maxtime (because there can be surface samples
	 * past "maxtime" in the original sample data)
	 */
	nr = dc->samples + 6 + maxtime / 10 + count_events(dc);
	plot_data = calloc(nr, sizeof(struct plot_data));
	pi->entry = plot_data;
	pi->nr_cylinders = dive->cylinders.nr;
	pi->pressures = calloc(nr * (size_t)pi->nr_cylinders, sizeof(struct plot_pressure_data));
	if (!plot_data)
		return;
	pi->nr = nr;
	idx = 2; /* the two extra events at the start */

	lastdepth = 0;
	lasttime = 0;
	/* skip events at time = 0 */
	while (ev && ev->time.seconds == 0)
		ev = ev->next;
	for (i = 0; i < dc->samples; i++) {
		struct plot_data *entry = plot_data + idx;
		struct sample *sample = dc->sample + i;
		int time = sample->time.seconds;
		int offset, delta;
		int depth = sample->depth.mm;
		int sac = sample->sac.mliter;

		/* Add intermediate plot entries if required */
		delta = time - lasttime;
		if (delta <= 0) {
			time = lasttime;
			delta = 1; // avoid divide by 0
		}
		for (offset = 10; offset < delta; offset += 10) {
			if (lasttime + offset > maxtime)
				break;

			/* Add events if they are between plot entries */
			while (ev && (int)ev->time.seconds < lasttime + offset) {
				insert_entry(pi, idx, ev->time.seconds, interpolate(lastdepth, depth, ev->time.seconds - lasttime, delta), sac);
				entry++;
				idx++;
				ev = ev->next;
			}

			/* now insert the time interpolated entry */
			insert_entry(pi, idx, lasttime + offset, interpolate(lastdepth, depth, offset, delta), sac);
			entry++;
			idx++;

			/* skip events that happened at this time */
			while (ev && (int)ev->time.seconds == lasttime + offset)
				ev = ev->next;
		}

		/* Add events if they are between plot entries */
		while (ev && (int)ev->time.seconds < time) {
			insert_entry(pi, idx, ev->time.seconds, interpolate(lastdepth, depth, ev->time.seconds - lasttime, delta), sac);
			entry++;
			idx++;
			ev = ev->next;
		}

		entry->sec = time;
		entry->depth = depth;

		entry->running_sum = (entry - 1)->running_sum + (time - (entry - 1)->sec) * (depth + (entry - 1)->depth) / 2;
		entry->stopdepth = sample->stopdepth.mm;
		entry->stoptime = sample->stoptime.seconds;
		entry->ndl = sample->ndl.seconds;
		entry->tts = sample->tts.seconds;
		entry->in_deco = sample->in_deco;
		entry->cns = sample->cns;
		if (dc->divemode == CCR || (dc->divemode == PSCR && dc->no_o2sensors)) {
			entry->o2pressure.mbar = entry->o2setpoint.mbar = sample->setpoint.mbar;     // for rebreathers
			int i;
			for (i = 0; i < MAX_O2_SENSORS; i++)
				entry->o2sensor[i].mbar = sample->o2sensor[i].mbar;
		} else {
			entry->pressures.o2 = sample->setpoint.mbar / 1000.0;
		}
		if (sample->pressure[0].mbar && sample->sensor[0] != NO_SENSOR)
			set_plot_pressure_data(pi, idx, SENSOR_PR, sample->sensor[0], sample->pressure[0].mbar);
		if (sample->pressure[1].mbar && sample->sensor[1] != NO_SENSOR)
			set_plot_pressure_data(pi, idx, SENSOR_PR, sample->sensor[1], sample->pressure[1].mbar);
		if (sample->temperature.mkelvin)
			entry->temperature = lasttemp = sample->temperature.mkelvin;
		else
			entry->temperature = lasttemp;
		entry->heartbeat = sample->heartbeat;
		entry->bearing = sample->bearing.degrees;
		entry->sac = sample->sac.mliter;
		if (sample->rbt.seconds)
			entry->rbt = sample->rbt.seconds;
		/* skip events that happened at this time */
		while (ev && (int)ev->time.seconds == time)
			ev = ev->next;
		lasttime = time;
		lastdepth = depth;
		idx++;

		if (time > maxtime)
			break;
	}

	/* Add any remaining events */
	while (ev) {
		struct plot_data *entry = plot_data + idx;
		int time = ev->time.seconds;

		if (time > lasttime) {
			insert_entry(pi, idx, ev->time.seconds, 0, 0);
			lasttime = time;
			idx++;
			entry++;
		}
		ev = ev->next;
	}

	/* Add two final surface events */
	plot_data[idx++].sec = lasttime + 1;
	plot_data[idx++].sec = lasttime + 2;
	pi->nr = idx;
}

/*
 * Calculate the sac rate between the two plot entries 'first' and 'last'.
 *
 * Everything in between has a cylinder pressure for at least some of the cylinders.
 */
static int sac_between(const struct dive *dive, struct plot_info *pi, int first, int last, const bool gases[])
{
	int i, airuse;
	double pressuretime;

	if (first == last)
		return 0;

	/* Get airuse for the set of cylinders over the range */
	airuse = 0;
	for (i = 0; i < pi->nr_cylinders; i++) {
		pressure_t a, b;
		cylinder_t *cyl;
		int cyluse;

		if (!gases[i])
			continue;

		a.mbar = get_plot_pressure(pi, first, i);
		b.mbar = get_plot_pressure(pi, last, i);
		cyl = get_cylinder(dive, i);
		cyluse = gas_volume(cyl, a) - gas_volume(cyl, b);
		if (cyluse > 0)
			airuse += cyluse;
	}
	if (!airuse)
		return 0;

	/* Calculate depthpressure integrated over time */
	pressuretime = 0.0;
	do {
		struct plot_data *entry = pi->entry + first;
		struct plot_data *next = entry + 1;
		int depth = (entry->depth + next->depth) / 2;
		int time = next->sec - entry->sec;
		double atm = depth_to_atm(depth, dive);

		pressuretime += atm * time;
	} while (++first < last);

	/* Turn "atmseconds" into "atmminutes" */
	pressuretime /= 60;

	/* SAC = mliter per minute */
	return lrint(airuse / pressuretime);
}

/* Is there pressure data for all gases? */
static bool all_pressures(struct plot_info *pi, int idx, const bool gases[])
{
	int i;

	for (i = 0; i < pi->nr_cylinders; i++) {
		if (gases[i] && !get_plot_pressure(pi, idx, i))
			return false;
	}

	return true;
}

/* Which of the set of gases have pressure data? Returns false if none of them. */
static bool filter_pressures(struct plot_info *pi, int idx, const bool gases_in[], bool gases_out[])
{
	int i;
	bool has_pressure = false;

	for (i = 0; i < pi->nr_cylinders; i++) {
		gases_out[i] = gases_in[i] && get_plot_pressure(pi, idx, i);
		has_pressure |= gases_out[i];
	}

	return has_pressure;
}

/*
 * Try to do the momentary sac rate for this entry, averaging over one
 * minute. This is premature optimization, but instead of allocating
 * an array of gases, the caller passes in scratch memory in the last
 * argument.
 */
static void fill_sac(const struct dive *dive, struct plot_info *pi, int idx, const bool gases_in[], bool gases[])
{
	struct plot_data *entry = pi->entry + idx;
	int first, last;
	int time;

	if (entry->sac)
		return;

	/*
	 * We may not have pressure data for all the cylinders,
	 * but we'll calculate the SAC for the ones we do have.
	 */
	if (!filter_pressures(pi, idx, gases_in, gases))
		return;

	/*
	 * Try to go back 30 seconds to get 'first'.
	 * Stop if the cylinder pressure data set changes.
	 */
	first = idx;
	time = entry->sec - 30;
	while (idx > 0) {
		struct plot_data *entry = pi->entry + idx;
		struct plot_data *prev = pi->entry + idx - 1;

		if (prev->depth < SURFACE_THRESHOLD && entry->depth < SURFACE_THRESHOLD)
			break;
		if (prev->sec < time)
			break;
		if (!all_pressures(pi, idx - 1, gases))
			break;
		idx--;
		first = idx;
	}

	/* Now find an entry a minute after the first one */
	last = first;
	time = pi->entry[first].sec + 60;
	while (++idx < pi->nr) {
		struct plot_data *entry = pi->entry + last;
		struct plot_data *next = pi->entry + last + 1;
		if (next->depth < SURFACE_THRESHOLD && entry->depth < SURFACE_THRESHOLD)
			break;
		if (next->sec > time)
			break;
		if (!all_pressures(pi, idx + 1, gases))
			break;
		last = idx;
	}

	/* Ok, now calculate the SAC between 'first' and 'last' */
	entry->sac = sac_between(dive, pi, first, last, gases);
}

/*
 * Create a bitmap of cylinders that match our current gasmix
 */
static void matching_gases(const struct dive *dive, struct gasmix gasmix, bool gases[])
{
	int i;

	for (i = 0; i < dive->cylinders.nr; i++)
		gases[i] = same_gasmix(gasmix, get_cylinder(dive, i)->gasmix);
}

static void calculate_sac(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi)
{
	struct gasmix gasmix = gasmix_invalid;
	const struct event *ev = NULL;
	bool *gases, *gases_scratch;

	gases = calloc(pi->nr_cylinders, sizeof(*gases));

	/* This might be premature optimization, but let's allocate the gas array for
	 * the fill_sac function only once an not once per sample */
	gases_scratch = malloc(pi->nr_cylinders * sizeof(*gases));

	for (int i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		struct gasmix newmix = get_gasmix(dive, dc, entry->sec, &ev, gasmix);
		if (!same_gasmix(newmix, gasmix)) {
			gasmix = newmix;
			matching_gases(dive, newmix, gases);
		}

		fill_sac(dive, pi, i, gases, gases_scratch);
	}

	free(gases);
	free(gases_scratch);
}

static void populate_secondary_sensor_data(const struct divecomputer *dc, struct plot_info *pi)
{
	int *seen = calloc(pi->nr_cylinders, sizeof(*seen));
	for (int idx = 0; idx < pi->nr; ++idx)
		for (int c = 0; c < pi->nr_cylinders; ++c)
			if (get_plot_pressure_data(pi, idx, SENSOR_PR, c))
				++seen[c]; // Count instances so we can differentiate a real sensor from just start and end pressure
	int idx = 0;
	/* We should try to see if it has interesting pressure data here */
	for (int i = 0; i < dc->samples && idx < pi->nr; i++) {
		struct sample *sample = dc->sample + i;
		for (; idx < pi->nr; ++idx) {
			if (idx == pi->nr - 1 || pi->entry[idx].sec >= sample->time.seconds)
				// We've either found the entry at or just after the sample's time,
				// or this is the last entry so use for the last sensor readings if there are any.
				break;
		}
		for (int s = 0; s < MAX_SENSORS; ++s)
			// Copy sensor data if available, but don't add if this dc already has sensor data
			if (sample->sensor[s] != NO_SENSOR && seen[sample->sensor[s]] < 3 && sample->pressure[s].mbar)
				set_plot_pressure_data(pi, idx, SENSOR_PR, sample->sensor[s], sample->pressure[s].mbar);
	}
	free(seen);
}

/*
 * This adds a pressure entry to the plot_info based on the gas change
 * information and the manually filled in pressures.
 */
static void add_plot_pressure(struct plot_info *pi, int time, int cyl, pressure_t p)
{
	for (int i = 0; i < pi->nr; i++) {
		if (i == pi->nr - 1 || pi->entry[i].sec >= time) {
			set_plot_pressure_data(pi, i, SENSOR_PR, cyl, p.mbar);
			return;
		}
	}
}

static void setup_gas_sensor_pressure(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi)
{
	int prev, i;
	const struct event *ev;

	if (pi->nr_cylinders == 0)
		return;

	int *seen = malloc(pi->nr_cylinders * sizeof(*seen));
	int *first = malloc(pi->nr_cylinders * sizeof(*first));
	int *last = malloc(pi->nr_cylinders * sizeof(*last));
	const struct divecomputer *secondary;

	for (i = 0; i < pi->nr_cylinders; i++) {
		seen[i] = 0;
		first[i] = 0;
		last[i] = INT_MAX;
	}
	prev = explicit_first_cylinder(dive, dc);
	seen[prev] = 1;

	for (ev = get_next_event(dc->events, "gaschange"); ev != NULL; ev = get_next_event(ev->next, "gaschange")) {
		int cyl = ev->gas.index;
		int sec = ev->time.seconds;

		if (cyl < 0)
			continue;

		last[prev] = sec;
		prev = cyl;

		last[cyl] = sec;
		if (!seen[cyl]) {
			// The end time may be updated by a subsequent cylinder change
			first[cyl] = sec;
			seen[cyl] = 1;
		}
	}
	last[prev] = INT_MAX;

	// Fill in "seen[]" array - mark cylinders we're not interested
	// in as negative.
	for (i = 0; i < pi->nr_cylinders; i++) {
		const cylinder_t *cyl = get_cylinder(dive, i);
		int start = cyl->start.mbar;
		int end = cyl->end.mbar;

		/*
		 * Fundamentally uninteresting?
		 *
		 * A dive computer with no pressure data isn't interesting
		 * to plot pressures for even if we've seen it..
		 */
		if (!start || !end || start == end) {
			seen[i] = -1;
			continue;
		}

		/* If we've seen it, we're definitely interested */
		if (seen[i])
			continue;

		/* If it's only mentioned by other dc's, ignore it */
		for_each_dc(dive, secondary) {
			if (has_gaschange_event(dive, secondary, i)) {
				seen[i] = -1;
				break;
			}
		}
	}

	for (i = 0; i < pi->nr_cylinders; i++) {
		if (seen[i] >= 0) {
			const cylinder_t *cyl = get_cylinder(dive, i);

			add_plot_pressure(pi, first[i], i, cyl->start);
			add_plot_pressure(pi, last[i], i, cyl->end);
		}
	}

	/*
	 * Here, we should try to walk through all the dive computers,
	 * and try to see if they have sensor data different from the
	 * current dive computer (dc).
	 */
	secondary = &dive->dc;
	do {
		if (secondary == dc)
			continue;
		populate_secondary_sensor_data(secondary, pi);
	} while ((secondary = secondary->next) != NULL);

	free(seen);
	free(first);
	free(last);
}

/* calculate DECO STOP / TTS / NDL */
static void calculate_ndl_tts(struct deco_state *ds, const struct dive *dive, struct plot_data *entry, struct gasmix gasmix,
			      double surface_pressure, enum divemode_t divemode, bool in_planner)
{
	/* should this be configurable? */
	/* ascent speed up to first deco stop */
	const int ascent_s_per_step = 1;
	const int ascent_s_per_deco_step = 1;
	/* how long time steps in deco calculations? */
	const int time_stepsize = 60;
	const int deco_stepsize = M_OR_FT(3, 10);
	/* at what depth is the current deco-step? */
	int next_stop = ROUND_UP(deco_allowed_depth(
					 tissue_tolerance_calc(ds, dive, depth_to_bar(entry->depth, dive), in_planner),
					 surface_pressure, dive, 1), deco_stepsize);
	int ascent_depth = entry->depth;
	/* at what time should we give up and say that we got enuff NDL? */
	/* If iterating through a dive, entry->tts_calc needs to be reset */
	entry->tts_calc = 0;

	/* If we don't have a ceiling yet, calculate ndl. Don't try to calculate
	 * a ndl for lower values than 3m it would take forever */
	if (next_stop == 0) {
		if (entry->depth < 3000) {
			entry->ndl = MAX_PROFILE_DECO;
			return;
		}
		/* stop if the ndl is above max_ndl seconds, and call it plenty of time */
		while (entry->ndl_calc < MAX_PROFILE_DECO &&
		       deco_allowed_depth(tissue_tolerance_calc(ds, dive, depth_to_bar(entry->depth, dive), in_planner),
					  surface_pressure, dive, 1) <= 0
		       ) {
			entry->ndl_calc += time_stepsize;
			add_segment(ds, depth_to_bar(entry->depth, dive),
				    gasmix, time_stepsize, entry->o2pressure.mbar, divemode, prefs.bottomsac, in_planner);
		}
		/* we don't need to calculate anything else */
		return;
	}

	/* We are in deco */
	entry->in_deco_calc = true;

	/* Add segments for movement to stopdepth */
	for (; ascent_depth > next_stop; ascent_depth -= ascent_s_per_step * ascent_velocity(ascent_depth, entry->running_sum / entry->sec, 0), entry->tts_calc += ascent_s_per_step) {
		add_segment(ds, depth_to_bar(ascent_depth, dive),
			    gasmix, ascent_s_per_step, entry->o2pressure.mbar, divemode, prefs.decosac, in_planner);
		next_stop = ROUND_UP(deco_allowed_depth(tissue_tolerance_calc(ds, dive, depth_to_bar(ascent_depth, dive), in_planner),
							surface_pressure, dive, 1), deco_stepsize);
	}
	ascent_depth = next_stop;

	/* And how long is the current deco-step? */
	entry->stoptime_calc = 0;
	entry->stopdepth_calc = next_stop;
	next_stop -= deco_stepsize;

	/* And how long is the total TTS */
	while (next_stop >= 0) {
		/* save the time for the first stop to show in the graph */
		if (ascent_depth == entry->stopdepth_calc)
			entry->stoptime_calc += time_stepsize;

		entry->tts_calc += time_stepsize;
		if (entry->tts_calc > MAX_PROFILE_DECO)
			break;
		add_segment(ds, depth_to_bar(ascent_depth, dive),
			    gasmix, time_stepsize, entry->o2pressure.mbar, divemode, prefs.decosac, in_planner);

		if (deco_allowed_depth(tissue_tolerance_calc(ds, dive, depth_to_bar(ascent_depth,dive), in_planner), surface_pressure, dive, 1) <= next_stop) {
			/* move to the next stop and add the travel between stops */
			for (; ascent_depth > next_stop; ascent_depth -= ascent_s_per_deco_step * ascent_velocity(ascent_depth, entry->running_sum / entry->sec, 0), entry->tts_calc += ascent_s_per_deco_step)
				add_segment(ds, depth_to_bar(ascent_depth, dive),
					    gasmix, ascent_s_per_deco_step, entry->o2pressure.mbar, divemode, prefs.decosac, in_planner);
			ascent_depth = next_stop;
			next_stop -= deco_stepsize;
		}
	}
}

/* Let's try to do some deco calculations.
 */
static void calculate_deco_information(struct deco_state *ds, const struct deco_state *planner_ds, const struct dive *dive,
				       const struct divecomputer *dc, struct plot_info *pi)
{
	int i, count_iteration = 0;
	double surface_pressure = (dc->surface_pressure.mbar ? dc->surface_pressure.mbar : get_surface_pressure_in_mbar(dive, true)) / 1000.0;
	bool first_iteration = true;
	int prev_deco_time = 10000000, time_deep_ceiling = 0;
	bool in_planner = planner_ds != NULL;

	if (!in_planner) {
		ds->deco_time = 0;
		ds->first_ceiling_pressure.mbar = 0;
	} else {
		ds->deco_time = planner_ds->deco_time;
		ds->first_ceiling_pressure = planner_ds->first_ceiling_pressure;
	}
	struct deco_state *cache_data_initial = NULL;
	lock_planner();
	/* For VPM-B outside the planner, cache the initial deco state for CVA iterations */
	if (decoMode(in_planner) == VPMB) {
		cache_deco_state(ds, &cache_data_initial);
	}
	/* For VPM-B outside the planner, iterate until deco time converges (usually one or two iterations after the initial)
	 * Set maximum number of iterations to 10 just in case */

	while ((abs(prev_deco_time - ds->deco_time) >= 30) && (count_iteration < 10)) {
		int last_ndl_tts_calc_time = 0, first_ceiling = 0, current_ceiling, last_ceiling = 0, final_tts = 0 , time_clear_ceiling = 0;
		if (decoMode(in_planner) == VPMB)
			ds->first_ceiling_pressure.mbar = depth_to_mbar(first_ceiling, dive);
		struct gasmix gasmix = gasmix_invalid;
		const struct event *ev = NULL, *evd = NULL;
		enum divemode_t current_divemode = UNDEF_COMP_TYPE;

		for (i = 1; i < pi->nr; i++) {
			struct plot_data *entry = pi->entry + i;
			int j, t0 = (entry - 1)->sec, t1 = entry->sec;
			int time_stepsize = 20, max_ceiling = -1;

			current_divemode = get_current_divemode(dc, entry->sec, &evd, &current_divemode);
			gasmix = get_gasmix(dive, dc, t1, &ev, gasmix);
			entry->ambpressure = depth_to_bar(entry->depth, dive);
			entry->gfline = get_gf(ds, entry->ambpressure, dive) * (100.0 - AMB_PERCENTAGE) + AMB_PERCENTAGE;
			if (t0 > t1) {
				SSRF_INFO("non-monotonous dive stamps %d %d\n", t0, t1);
				int xchg = t1;
				t1 = t0;
				t0 = xchg;
			}
			if (t0 != t1 && t1 - t0 < time_stepsize)
				time_stepsize = t1 - t0;
			for (j = t0 + time_stepsize; j <= t1; j += time_stepsize) {
				int depth = interpolate(entry[-1].depth, entry[0].depth, j - t0, t1 - t0);
				add_segment(ds, depth_to_bar(depth, dive),
					    gasmix, time_stepsize, entry->o2pressure.mbar, current_divemode, entry->sac, in_planner);
				entry->icd_warning = ds->icd_warning;
				if ((t1 - j < time_stepsize) && (j < t1))
					time_stepsize = t1 - j;
			}
			if (t0 == t1) {
				entry->ceiling = (entry - 1)->ceiling;
			} else {
				/* Keep updating the VPM-B gradients until the start of the ascent phase of the dive. */
				if (decoMode(in_planner) == VPMB && last_ceiling >= first_ceiling && first_iteration == true) {
					nuclear_regeneration(ds, t1);
					vpmb_start_gradient(ds);
					/* For CVA iterations, calculate next gradient */
					if (!first_iteration || in_planner)
						vpmb_next_gradient(ds, ds->deco_time, surface_pressure / 1000.0, in_planner);
				}
				entry->ceiling = deco_allowed_depth(tissue_tolerance_calc(ds, dive, depth_to_bar(entry->depth, dive), in_planner), surface_pressure, dive, !prefs.calcceiling3m);
				if (prefs.calcceiling3m)
					current_ceiling = deco_allowed_depth(tissue_tolerance_calc(ds, dive, depth_to_bar(entry->depth, dive), in_planner), surface_pressure, dive, true);
				else
					current_ceiling = entry->ceiling;
				last_ceiling = current_ceiling;
				/* If using VPM-B, take first_ceiling_pressure as the deepest ceiling */
				if (decoMode(in_planner) == VPMB) {
					if  (current_ceiling >= first_ceiling ||
					     (time_deep_ceiling == t0 && entry->depth == (entry - 1)->depth)) {
						time_deep_ceiling = t1;
						first_ceiling = current_ceiling;
						ds->first_ceiling_pressure.mbar = depth_to_mbar(first_ceiling, dive);
						if (first_iteration) {
							nuclear_regeneration(ds, t1);
							vpmb_start_gradient(ds);
							/* For CVA calculations, deco time = dive time remaining is a good guess,
							   but we want to over-estimate deco_time for the first iteration so it
							   converges correctly, so add 30min*/
							if (!in_planner)
								ds->deco_time = pi->maxtime - t1 + 1800;
							vpmb_next_gradient(ds, ds->deco_time, surface_pressure / 1000.0, in_planner);
						}
					}
					// Use the point where the ceiling clears as the end of deco phase for CVA calculations
					if (current_ceiling > 0)
						time_clear_ceiling = 0;
					else if (time_clear_ceiling == 0 && t1 > time_deep_ceiling)
						time_clear_ceiling = t1;
				}
			}
			entry->surface_gf = 0.0;
			entry->current_gf = 0.0;
			for (j = 0; j < 16; j++) {
				double m_value = ds->buehlmann_inertgas_a[j] + entry->ambpressure / ds->buehlmann_inertgas_b[j];
				double surface_m_value = ds->buehlmann_inertgas_a[j] + surface_pressure / ds->buehlmann_inertgas_b[j];
				entry->ceilings[j] = deco_allowed_depth(ds->tolerated_by_tissue[j], surface_pressure, dive, 1);
				if (entry->ceilings[j] > max_ceiling)
					max_ceiling = entry->ceilings[j];
				double current_gf = (ds->tissue_inertgas_saturation[j] - entry->ambpressure) / (m_value - entry->ambpressure);
				entry->percentages[j] = ds->tissue_inertgas_saturation[j] < entry->ambpressure ?
					lrint(ds->tissue_inertgas_saturation[j] / entry->ambpressure * AMB_PERCENTAGE) :
					lrint(AMB_PERCENTAGE + current_gf * (100.0 - AMB_PERCENTAGE));
				if (current_gf > entry->current_gf)
					entry->current_gf = current_gf;
				double surface_gf = 100.0 * (ds->tissue_inertgas_saturation[j] - surface_pressure) / (surface_m_value - surface_pressure);
				if (surface_gf > entry->surface_gf)
					entry->surface_gf = surface_gf;
			}

			// In the planner, if the ceiling is violated, add an event.
			// TODO: This *really* shouldn't be done here. This is a contract
			// between the planner and the profile that the planner uses a dive
			// that can be trampled upon. But ultimately, the ceiling-violation
			// marker should be handled differently!
			// Don't scream if we violate the ceiling by a few cm.
			if (in_planner && !pi->waypoint_above_ceiling &&
			    entry->depth < max_ceiling - 100 && entry->sec > 0) {
				struct dive *non_const_dive = (struct dive *)dive; // cast away const!
				add_event(&non_const_dive->dc, entry->sec, SAMPLE_EVENT_CEILING, -1, max_ceiling / 1000,
					  translate("gettextFromC", "planned waypoint above ceiling"));
				pi->waypoint_above_ceiling = true;
			}

			/* should we do more calculations?
			* We don't for print-mode because this info doesn't show up there
			* If the ceiling hasn't cleared by the last data point, we need tts for VPM-B CVA calculation
			* It is not necessary to do these calculation on the first VPMB iteration, except for the last data point */
			if ((prefs.calcndltts && (decoMode(in_planner) != VPMB || in_planner || !first_iteration)) ||
			    (decoMode(in_planner) == VPMB && !in_planner && i == pi->nr - 1)) {
				/* only calculate ndl/tts on every 30 seconds */
				if ((entry->sec - last_ndl_tts_calc_time) < 30 && i != pi->nr - 1) {
					struct plot_data *prev_entry = (entry - 1);
					entry->stoptime_calc = prev_entry->stoptime_calc;
					entry->stopdepth_calc = prev_entry->stopdepth_calc;
					entry->tts_calc = prev_entry->tts_calc;
					entry->ndl_calc = prev_entry->ndl_calc;
					continue;
				}
				last_ndl_tts_calc_time = entry->sec;

				/* We are going to mess up deco state, so store it for later restore */
				struct deco_state *cache_data = NULL;
				cache_deco_state(ds, &cache_data);
				calculate_ndl_tts(ds, dive, entry, gasmix, surface_pressure, current_divemode, in_planner);
				if (decoMode(in_planner) == VPMB && !in_planner && i == pi->nr - 1)
					final_tts = entry->tts_calc;
				/* Restore "real" deco state for next real time step */
				restore_deco_state(cache_data, ds, decoMode(in_planner) == VPMB);
				free(cache_data);
			}
		}
		if (decoMode(in_planner) == VPMB && !in_planner) {
			int this_deco_time;
			prev_deco_time = ds->deco_time;
			// Do we need to update deco_time?
			if (final_tts > 0)
				ds->deco_time = last_ndl_tts_calc_time + final_tts - time_deep_ceiling;
			else if (time_clear_ceiling > 0)
				/* Consistent with planner, deco_time ends after ascending (20s @9m/min from 3m)
				 * at end of whole minute after clearing ceiling. The deepest ceiling when planning a dive
				 * comes typically 10-60s after the end of the bottom time, so add 20s to the calculated
				 * deco time. */
					ds->deco_time = ROUND_UP(time_clear_ceiling - time_deep_ceiling + 20, 60) + 20;
			vpmb_next_gradient(ds, ds->deco_time, surface_pressure / 1000.0, in_planner);
			final_tts = 0;
			last_ndl_tts_calc_time = 0;
			first_ceiling = 0;
			first_iteration = false;
			count_iteration ++;
			this_deco_time = ds->deco_time;
			restore_deco_state(cache_data_initial, ds, true);
			ds->deco_time = this_deco_time;
		} else {
			// With Buhlmann iterating isn't needed.  This makes the while condition false.
			prev_deco_time = ds->deco_time = 0;
		}
	}

	free(cache_data_initial);
#if DECO_CALC_DEBUG & 1
	dump_tissues(ds);
#endif
	unlock_planner();
}

/* Sort the o2 pressure values. There are so few that a simple bubble sort
 * will do */

void sort_o2_pressures(int *sensorn, int np, struct plot_data *entry)
{
	int smallest, position, old;

	for (int i = 0; i < np - 1; i++) {
		position = i;
		smallest = entry->o2sensor[sensorn[i]].mbar;
		for (int j = i+1; j < np; j++)
			if (entry->o2sensor[sensorn[j]].mbar < smallest) {
				position = j;
				smallest = entry->o2sensor[sensorn[j]].mbar;
			}
		old = sensorn[i];
		sensorn[i] = position;
		sensorn[position] = old;
	}
}

/* Function calculate_ccr_po2: This function takes information from one plot_data structure (i.e. one point on
 * the dive profile), containing the oxygen sensor values of a CCR system and, for that plot_data structure,
 * calculates the po2 value from the sensor data. If there are at least 3 sensors, sensors are voted out until
 * their span is within diff_limit.
 */
static int calculate_ccr_po2(struct plot_data *entry, const struct divecomputer *dc)
{
	int sump = 0, minp = 0, maxp = 0;
	int sensorn[MAX_O2_SENSORS];
	int i, np = 0;

	for (i = 0; i < dc->no_o2sensors && i < MAX_O2_SENSORS; i++)
		if (entry->o2sensor[i].mbar) { // Valid reading
			sensorn[np++] = i;
			sump += entry->o2sensor[i].mbar;
		}
	if (np == 0)
		return entry->o2pressure.mbar;
	else if (np == 1)
		return entry->o2sensor[sensorn[0]].mbar;

	maxp = np - 1;
	sort_o2_pressures(sensorn, np, entry);

	// This is the Shearwater voting logic: If there are still at least three sensors and one
	// differs by more than 20% from the closest it is voted out.
	while (maxp - minp > 1) {
		if (entry->o2sensor[sensorn[minp + 1]].mbar - entry->o2sensor[sensorn[minp]].mbar >
		    sump / (maxp - minp + 1) / 5) {
			sump -= entry->o2sensor[sensorn[minp]].mbar;
			++minp;
			continue;
		}
		if (entry->o2sensor[sensorn[maxp]].mbar - entry->o2sensor[sensorn[maxp - 1]].mbar >
		    sump / (maxp - minp +1) / 5) {
			sump -= entry->o2sensor[sensorn[maxp]].mbar;
			--maxp;
			continue;
		}
		break;
	}

	return sump / (maxp - minp + 1);

}

static double gas_density(const struct gas_pressures *pressures)
{
	return (pressures->o2 * O2_DENSITY + pressures->he * HE_DENSITY + pressures->n2 * N2_DENSITY) / 1000.0;
}

static void calculate_gas_information_new(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi)
{
	int i;
	double amb_pressure;
	struct gasmix gasmix = gasmix_invalid;
	const struct event *evg = NULL, *evd = NULL;
	enum divemode_t current_divemode = UNDEF_COMP_TYPE;

	for (i = 1; i < pi->nr; i++) {
		double fn2, fhe;
		struct plot_data *entry = pi->entry + i;

		gasmix = get_gasmix(dive, dc, entry->sec, &evg, gasmix);
		amb_pressure = depth_to_bar(entry->depth, dive);
		current_divemode = get_current_divemode(dc, entry->sec, &evd, &current_divemode);
		fill_pressures(&entry->pressures, amb_pressure, gasmix, (current_divemode == OC) ? 0.0 : entry->o2pressure.mbar / 1000.0, current_divemode);
		fn2 = 1000.0 * entry->pressures.n2 / amb_pressure;
		fhe = 1000.0 * entry->pressures.he / amb_pressure;
		if (dc->divemode == PSCR) { // OC pO2 is calulated for PSCR with or without external PO2 monitoring.
			struct gasmix gasmix2 = get_gasmix(dive, dc, entry->sec, &evg, gasmix);
			entry->scr_OC_pO2.mbar = (int) depth_to_mbar(entry->depth, dive) * get_o2(gasmix2) / 1000;
		}

		/* Calculate MOD, EAD, END and EADD based on partial pressures calculated before
		 * so there is no difference in calculating between OC and CC
		 * END takes O₂ + N₂ (air) into account ("Narcotic" for trimix dives)
		 * EAD just uses N₂ ("Air" for nitrox dives) */
		pressure_t modpO2 = { .mbar = (int)(prefs.modpO2 * 1000) };
		entry->mod = gas_mod(gasmix, modpO2, dive, 1).mm;
		entry->end = mbar_to_depth(lrint(depth_to_mbarf(entry->depth, dive) * (1000 - fhe) / 1000.0), dive);
		entry->ead = mbar_to_depth(lrint(depth_to_mbarf(entry->depth, dive) * fn2 / (double)N2_IN_AIR), dive);
		entry->eadd = mbar_to_depth(lrint(depth_to_mbarf(entry->depth, dive) *
				      (entry->pressures.o2 / amb_pressure * O2_DENSITY +
				       entry->pressures.n2 / amb_pressure * N2_DENSITY +
				       entry->pressures.he / amb_pressure * HE_DENSITY) /
				      (O2_IN_AIR * O2_DENSITY + N2_IN_AIR * N2_DENSITY) * 1000), dive);
		entry->density = gas_density(&entry->pressures);
		if (entry->mod < 0)
			entry->mod = 0;
		if (entry->ead < 0)
			entry->ead = 0;
		if (entry->end < 0)
			entry->end = 0;
		if (entry->eadd < 0)
			entry->eadd = 0;
	}
}

static void fill_o2_values(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi)
/* In the samples from each dive computer, there may be uninitialised oxygen
 * sensor or setpoint values, e.g. when events were inserted into the dive log
 * or if the dive computer does not report o2 values with every sample. But
 * for drawing the profile a complete series of valid o2 pressure values is
 * required. This function takes the oxygen sensor data and setpoint values
 * from the structures of plotinfo and replaces the zero values with their
 * last known values so that the oxygen sensor data are complete and ready
 * for plotting. This function called by: create_plot_info_new() */
{
	int i, j;
	pressure_t last_sensor[3], o2pressure;
	pressure_t amb_pressure;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;

		if (dc->divemode == CCR || (dc->divemode == PSCR && dc->no_o2sensors)) {
			if (i == 0) { // For 1st iteration, initialise the last_sensor values
				for (j = 0; j < dc->no_o2sensors; j++)
					last_sensor[j].mbar = pi->entry->o2sensor[j].mbar;
			} else { // Now re-insert the missing oxygen pressure values
				for (j = 0; j < dc->no_o2sensors; j++)
					if (entry->o2sensor[j].mbar)
						last_sensor[j].mbar = entry->o2sensor[j].mbar;
					else
						entry->o2sensor[j].mbar = last_sensor[j].mbar;
			} // having initialised the empty o2 sensor values for this point on the profile,
			amb_pressure.mbar = depth_to_mbar(entry->depth, dive);
			o2pressure.mbar = calculate_ccr_po2(entry, dc); // ...calculate the po2 based on the sensor data
			entry->o2pressure.mbar = MIN(o2pressure.mbar, amb_pressure.mbar);
		} else {
			entry->o2pressure.mbar = 0; // initialise po2 to zero for dctype = OC
		}
	}
}

#ifdef DEBUG_GAS
/* A CCR debug function that writes the cylinder pressure and the oxygen values to the file debug_print_profiledata.dat:
 * Called in create_plot_info_new()
 */
static void debug_print_profiledata(struct plot_info *pi)
{
	FILE *f1;
	struct plot_data *entry;
	int i;
	if (!(f1 = fopen("debug_print_profiledata.dat", "w"))) {
		printf("File open error for: debug_print_profiledata.dat\n");
	} else {
		fprintf(f1, "id t1 gas gasint t2 t3 dil dilint t4 t5 setpoint sensor1 sensor2 sensor3 t6 po2 fo2\n");
		for (i = 0; i < pi->nr; i++) {
			entry = pi->entry + i;
			fprintf(f1, "%d gas=%8d %8d ; dil=%8d %8d ; o2_sp= %d %d %d %d PO2= %f\n", i, get_plot_sensor_pressure(pi, i),
				get_plot_interpolated_pressure(pi, i), O2CYLINDER_PRESSURE(entry), INTERPOLATED_O2CYLINDER_PRESSURE(entry),
				entry->o2pressure.mbar, entry->o2sensor[0].mbar, entry->o2sensor[1].mbar, entry->o2sensor[2].mbar, entry->pressures.o2);
		}
		fclose(f1);
	}
}
#endif

/*
 * Initialize a plot_info structure to all-zeroes
 */
void init_plot_info(struct plot_info *pi)
{
	memset(pi, 0, sizeof(*pi));
}

/*
 * Create a plot-info with smoothing and ranged min/max
 *
 * This also makes sure that we have extra empty events on both
 * sides, so that you can do end-points without having to worry
 * about it.
 *
 * The old data will be freed. Before the first call, the plot
 * info must be initialized with init_plot_info().
 */
void create_plot_info_new(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi, const struct deco_state *planner_ds)
{
	int o2, he, o2max;
	struct deco_state plot_deco_state;
	bool in_planner = planner_ds != NULL;
	init_decompression(&plot_deco_state, dive, in_planner);
	free_plot_info_data(pi);
	calculate_max_limits_new(dive, dc, pi, in_planner);
	get_dive_gas(dive, &o2, &he, &o2max);
	if (dc->divemode == FREEDIVE) {
		pi->dive_type = FREEDIVING;
	} else if (he > 0) {
		pi->dive_type = TRIMIX;
	} else {
		if (o2)
			pi->dive_type = NITROX;
		else
			pi->dive_type = AIR;
	}

	populate_plot_entries(dive, dc, pi);

	check_setpoint_events(dive, dc, pi);     /* Populate setpoints */
	setup_gas_sensor_pressure(dive, dc, pi); /* Try to populate our gas pressure knowledge */
	for (int cyl = 0; cyl < pi->nr_cylinders; cyl++)
		populate_pressure_information(dive, dc, pi, cyl);
	fill_o2_values(dive, dc, pi);			 /* .. and insert the O2 sensor data having 0 values. */
	calculate_sac(dive, dc, pi);			 /* Calculate sac */

	calculate_deco_information(&plot_deco_state, planner_ds, dive, dc, pi); /* and ceiling information, using gradient factor values in Preferences) */

	calculate_gas_information_new(dive, dc, pi);	 /* Calculate gas partial pressures */

#ifdef DEBUG_GAS
	debug_print_profiledata(pi);
#endif

	pi->meandepth = dive->dc.meandepth.mm;
	analyze_plot_info(pi);
}

static void plot_string(const struct dive *d, const struct plot_info *pi, int idx, struct membuffer *b)
{
	int pressurevalue, mod, ead, end, eadd;
	const char *depth_unit, *pressure_unit, *temp_unit, *vertical_speed_unit;
	double depthvalue, tempvalue, speedvalue, sacvalue;
	int decimals, cyl;
	const char *unit;
	const struct plot_data *entry = pi->entry + idx;

	depthvalue = get_depth_units(entry->depth, NULL, &depth_unit);
	put_format_loc(b, translate("gettextFromC", "@: %d:%02d\nD: %.1f%s\n"), FRACTION(entry->sec, 60), depthvalue, depth_unit);
	for (cyl = 0; cyl < pi->nr_cylinders; cyl++) {
		int mbar = get_plot_pressure(pi, idx, cyl);
		if (!mbar)
			continue;
		struct gasmix mix = get_cylinder(d, cyl)->gasmix;
		pressurevalue = get_pressure_units(mbar, &pressure_unit);
		put_format_loc(b, translate("gettextFromC", "P: %d%s (%s)\n"), pressurevalue, pressure_unit, gasname(mix));
	}
	if (entry->temperature) {
		tempvalue = get_temp_units(entry->temperature, &temp_unit);
		put_format_loc(b, translate("gettextFromC", "T: %.1f%s\n"), tempvalue, temp_unit);
	}
	speedvalue = get_vertical_speed_units(abs(entry->speed), NULL, &vertical_speed_unit);
	/* Ascending speeds are positive, descending are negative */
	if (entry->speed > 0)
		speedvalue *= -1;
	put_format_loc(b, translate("gettextFromC", "V: %.1f%s\n"), speedvalue, vertical_speed_unit);
	sacvalue = get_volume_units(entry->sac, &decimals, &unit);
	if (entry->sac && prefs.show_sac)
		put_format_loc(b, translate("gettextFromC", "SAC: %.*f%s/min\n"), decimals, sacvalue, unit);
	if (entry->cns)
		put_format_loc(b, translate("gettextFromC", "CNS: %u%%\n"), entry->cns);
	if (prefs.pp_graphs.po2 && entry->pressures.o2 > 0) {
		put_format_loc(b, translate("gettextFromC", "pO₂: %.2fbar\n"), entry->pressures.o2);
		if (entry->scr_OC_pO2.mbar)
			put_format_loc(b, translate("gettextFromC", "SCR ΔpO₂: %.2fbar\n"), entry->scr_OC_pO2.mbar/1000.0 - entry->pressures.o2);
	}
	if (prefs.pp_graphs.pn2 && entry->pressures.n2 > 0)
		put_format_loc(b, translate("gettextFromC", "pN₂: %.2fbar\n"), entry->pressures.n2);
	if (prefs.pp_graphs.phe && entry->pressures.he > 0)
		put_format_loc(b, translate("gettextFromC", "pHe: %.2fbar\n"), entry->pressures.he);
	if (prefs.mod && entry->mod > 0) {
		mod = lrint(get_depth_units(entry->mod, NULL, &depth_unit));
		put_format_loc(b, translate("gettextFromC", "MOD: %d%s\n"), mod, depth_unit);
	}
	eadd = lrint(get_depth_units(entry->eadd, NULL, &depth_unit));

	if (prefs.ead) {
		switch (pi->dive_type) {
		case NITROX:
			if (entry->ead > 0) {
				ead = lrint(get_depth_units(entry->ead, NULL, &depth_unit));
				put_format_loc(b, translate("gettextFromC", "EAD: %d%s\nEADD: %d%s / %.1fg/ℓ\n"), ead, depth_unit, eadd, depth_unit, entry->density);
				break;
			}
		case TRIMIX:
			if (entry->end > 0) {
				end = lrint(get_depth_units(entry->end, NULL, &depth_unit));
				put_format_loc(b, translate("gettextFromC", "END: %d%s\nEADD: %d%s / %.1fg/ℓ\n"), end, depth_unit, eadd, depth_unit, entry->density);
				break;
			}
		case AIR:
			if (entry->density > 0) {
				put_format_loc(b, translate("gettextFromC", "Density: %.1fg/ℓ\n"), entry->density);
			}
		case FREEDIVING:
			/* nothing */
			break;
		}
	}
	if (entry->stopdepth) {
		depthvalue = get_depth_units(entry->stopdepth, NULL, &depth_unit);
		if (entry->ndl > 0) {
			/* this is a safety stop as we still have ndl */
			if (entry->stoptime)
				put_format_loc(b, translate("gettextFromC", "Safety stop: %umin @ %.0f%s\n"), DIV_UP(entry->stoptime, 60),
					       depthvalue, depth_unit);
			else
				put_format_loc(b, translate("gettextFromC", "Safety stop: unknown time @ %.0f%s\n"),
					       depthvalue, depth_unit);
		} else {
			/* actual deco stop */
			if (entry->stoptime)
				put_format_loc(b, translate("gettextFromC", "Deco: %umin @ %.0f%s\n"), DIV_UP(entry->stoptime, 60),
					       depthvalue, depth_unit);
			else
				put_format_loc(b, translate("gettextFromC", "Deco: unknown time @ %.0f%s\n"),
					       depthvalue, depth_unit);
		}
	} else if (entry->in_deco) {
		put_string(b, translate("gettextFromC", "In deco\n"));
	} else if (entry->ndl >= 0) {
		put_format_loc(b, translate("gettextFromC", "NDL: %umin\n"), DIV_UP(entry->ndl, 60));
	}
	if (entry->tts)
		put_format_loc(b, translate("gettextFromC", "TTS: %umin\n"), DIV_UP(entry->tts, 60));
	if (entry->stopdepth_calc && entry->stoptime_calc) {
		depthvalue = get_depth_units(entry->stopdepth_calc, NULL, &depth_unit);
		put_format_loc(b, translate("gettextFromC", "Deco: %umin @ %.0f%s (calc)\n"), DIV_UP(entry->stoptime_calc, 60),
				  depthvalue, depth_unit);
	} else if (entry->in_deco_calc) {
		/* This means that we have no NDL left,
		 * and we have no deco stop,
		 * so if we just accend to the surface slowly
		 * (ascent_mm_per_step / ascent_s_per_step)
		 * everything will be ok. */
		put_string(b, translate("gettextFromC", "In deco (calc)\n"));
	} else if (prefs.calcndltts && entry->ndl_calc != 0) {
		if(entry->ndl_calc < MAX_PROFILE_DECO)
			put_format_loc(b, translate("gettextFromC", "NDL: %umin (calc)\n"), DIV_UP(entry->ndl_calc, 60));
		else
			put_string(b, translate("gettextFromC", "NDL: >2h (calc)\n"));
	}
	if (entry->tts_calc) {
		if (entry->tts_calc < MAX_PROFILE_DECO)
			put_format_loc(b, translate("gettextFromC", "TTS: %umin (calc)\n"), DIV_UP(entry->tts_calc, 60));
		else
			put_string(b, translate("gettextFromC", "TTS: >2h (calc)\n"));
	}
	if (entry->rbt)
		put_format_loc(b, translate("gettextFromC", "RBT: %umin\n"), DIV_UP(entry->rbt, 60));
	if (prefs.decoinfo) {
		if (entry->current_gf > 0.0)
			put_format(b, translate("gettextFromC", "GF %d%%\n"), (int)(100.0 * entry->current_gf));
		if (entry->surface_gf > 0.0)
			put_format(b, translate("gettextFromC", "Surface GF %.0f%%\n"), entry->surface_gf);
		if (entry->ceiling) {
			depthvalue = get_depth_units(entry->ceiling, NULL, &depth_unit);
			put_format_loc(b, translate("gettextFromC", "Calculated ceiling %.1f%s\n"), depthvalue, depth_unit);
			if (prefs.calcalltissues) {
				int k;
				for (k = 0; k < 16; k++) {
					if (entry->ceilings[k]) {
						depthvalue = get_depth_units(entry->ceilings[k], NULL, &depth_unit);
						put_format_loc(b, translate("gettextFromC", "Tissue %.0fmin: %.1f%s\n"), buehlmann_N2_t_halflife[k], depthvalue, depth_unit);
					}
				}
			}
		}
	}
	if (entry->icd_warning)
		put_format(b, "%s", translate("gettextFromC", "ICD in leading tissue\n"));
	if (entry->heartbeat && prefs.hrgraph)
		put_format_loc(b, translate("gettextFromC", "heart rate: %d\n"), entry->heartbeat);
	if (entry->bearing >= 0)
		put_format_loc(b, translate("gettextFromC", "bearing: %d\n"), entry->bearing);
	if (entry->running_sum) {
		depthvalue = get_depth_units(entry->running_sum / entry->sec, NULL, &depth_unit);
		put_format_loc(b, translate("gettextFromC", "mean depth to here %.1f%s\n"), depthvalue, depth_unit);
	}

	strip_mb(b);
}

int get_plot_details_new(const struct dive *d, const struct plot_info *pi, int time, struct membuffer *mb)
{
	int i;

	/* The two first and the two last plot entries do not have useful data */
	if (pi->nr <= 4)
		return 0;
	for (i = 2; i < pi->nr - 3; i++) {
		if (pi->entry[i].sec >= time)
			break;
	}
	plot_string(d, pi, i, mb);
	return i;
}

/* Compare two plot_data entries and writes the results into a string */
void compare_samples(const struct dive *d, const struct plot_info *pi, int idx1, int idx2, char *buf, int bufsize, bool sum)
{
	struct plot_data *start, *stop, *data;
	const char *depth_unit, *pressure_unit, *vertical_speed_unit;
	char *buf2 = malloc(bufsize);
	int avg_speed, max_asc_speed, max_desc_speed;
	int delta_depth, avg_depth, max_depth, min_depth;
	int pressurevalue;
	int last_sec, delta_time;

	double depthvalue, speedvalue;

	if (bufsize > 0)
		buf[0] = '\0';
	if (idx1 < 0 || idx2 < 0) {
		free(buf2);
		return;
	}

	if (pi->entry[idx1].sec > pi->entry[idx2].sec) {
		int tmp = idx2;
		idx2 = idx1;
		idx1 = tmp;
	} else if (pi->entry[idx1].sec == pi->entry[idx2].sec) {
		free(buf2);
		return;
	}
	start = pi->entry + idx1;
	stop = pi->entry + idx2;

	avg_speed = 0;
	max_asc_speed = 0;
	max_desc_speed = 0;

	delta_depth = abs(start->depth - stop->depth);
	delta_time = abs(start->sec - stop->sec);
	avg_depth = 0;
	max_depth = 0;
	min_depth = INT_MAX;

	last_sec = start->sec;

	volume_t cylinder_volume = { .mliter = 0, };
	int *start_pressures = calloc((size_t)pi->nr_cylinders, sizeof(int));
	int *last_pressures = calloc((size_t)pi->nr_cylinders, sizeof(int));
	int *bar_used = calloc((size_t)pi->nr_cylinders, sizeof(int));
	int *volumes_used = calloc((size_t)pi->nr_cylinders, sizeof(int));
	bool *cylinder_is_used = calloc((size_t)pi->nr_cylinders, sizeof(bool));

	data = start;
	for (int i = idx1; i < idx2; ++i) {
		data = pi->entry + i;
		if (sum)
			avg_speed += abs(data->speed) * (data->sec - last_sec);
		else
			avg_speed += data->speed * (data->sec - last_sec);
		avg_depth += data->depth * (data->sec - last_sec);

		if (data->speed > max_desc_speed)
			max_desc_speed = data->speed;
		if (data->speed < max_asc_speed)
			max_asc_speed = data->speed;

		if (data->depth < min_depth)
			min_depth = data->depth;
		if (data->depth > max_depth)
			max_depth = data->depth;

		for (unsigned cylinder_index = 0; cylinder_index < pi->nr_cylinders; cylinder_index++) {
			int next_pressure = get_plot_pressure(pi, i, cylinder_index);
			if (next_pressure && !start_pressures[cylinder_index])
				start_pressures[cylinder_index] = next_pressure;

			if (start_pressures[cylinder_index]) {
				if (last_pressures[cylinder_index]) {
					bar_used[cylinder_index] += last_pressures[cylinder_index] - next_pressure;

					cylinder_t *cyl = get_cylinder(d, cylinder_index);

					volumes_used[cylinder_index] += gas_volume(cyl, (pressure_t){ last_pressures[cylinder_index] }) - gas_volume(cyl, (pressure_t){ next_pressure });
				}

				// check if the gas in this cylinder is being used
				if (next_pressure < start_pressures[cylinder_index] - 1000 && !cylinder_is_used[cylinder_index]) {
					cylinder_is_used[cylinder_index] = true;
				}
			}

			last_pressures[cylinder_index] = next_pressure;
		}

		last_sec = data->sec;
	}

	free(start_pressures);
	free(last_pressures);

	avg_depth /= stop->sec - start->sec;
	avg_speed /= stop->sec - start->sec;

	snprintf_loc(buf, bufsize, translate("gettextFromC", "ΔT:%d:%02dmin"), delta_time / 60, delta_time % 60);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(delta_depth, NULL, &depth_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s ΔD:%.1f%s"), buf2, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(min_depth, NULL, &depth_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s ↓D:%.1f%s"), buf2, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(max_depth, NULL, &depth_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s ↑D:%.1f%s"), buf2, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	depthvalue = get_depth_units(avg_depth, NULL, &depth_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s øD:%.1f%s\n"), buf2, depthvalue, depth_unit);
	memcpy(buf2, buf, bufsize);

	speedvalue = get_vertical_speed_units(abs(max_desc_speed), NULL, &vertical_speed_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s ↓V:%.2f%s"), buf2, speedvalue, vertical_speed_unit);
	memcpy(buf2, buf, bufsize);

	speedvalue = get_vertical_speed_units(abs(max_asc_speed), NULL, &vertical_speed_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s ↑V:%.2f%s"), buf2, speedvalue, vertical_speed_unit);
	memcpy(buf2, buf, bufsize);

	speedvalue = get_vertical_speed_units(abs(avg_speed), NULL, &vertical_speed_unit);
	snprintf_loc(buf, bufsize, translate("gettextFromC", "%s øV:%.2f%s"), buf2, speedvalue, vertical_speed_unit);

	int total_bar_used = 0;
	int total_volume_used = 0;
	bool cylindersizes_are_identical = true;
	bool sac_is_determinable = true;
	for (unsigned cylinder_index = 0; cylinder_index < pi->nr_cylinders; cylinder_index++)
		if (cylinder_is_used[cylinder_index]) {
			total_bar_used += bar_used[cylinder_index];
			total_volume_used += volumes_used[cylinder_index];

			cylinder_t *cyl = get_cylinder(d, cylinder_index);
			if (cyl->type.size.mliter) {
				if (cylinder_volume.mliter && cylinder_volume.mliter != cyl->type.size.mliter) {
					cylindersizes_are_identical = false;
				} else {
					cylinder_volume.mliter = cyl->type.size.mliter;
				}
			} else {
				sac_is_determinable = false;
			}
		}
	free(bar_used);
	free(volumes_used);
	free(cylinder_is_used);

	// No point printing 'bar used' if we know it's meaningless because cylinders of different size were used
	if (cylindersizes_are_identical && total_bar_used) {
			pressurevalue = get_pressure_units(total_bar_used, &pressure_unit);
			memcpy(buf2, buf, bufsize);
			snprintf_loc(buf, bufsize, translate("gettextFromC", "%s ΔP:%d%s"), buf2, pressurevalue, pressure_unit);
	}

	// We can't calculate the SAC if the volume for some of the cylinders used is unknown
	if (sac_is_determinable && total_volume_used) {
		double volume_value;
		int volume_precision;
		const char *volume_unit;

		/* Mean pressure in ATM */
		double atm = depth_to_atm(avg_depth, d);

		/* milliliters per minute */
		int sac = lrint(total_volume_used / atm * 60 / delta_time);
		memcpy(buf2, buf, bufsize);
		volume_value = get_volume_units(sac, &volume_precision, &volume_unit);
		snprintf_loc(buf, bufsize, translate("gettextFromC", "%s SAC:%.*f%s/min"), buf2, volume_precision, volume_value, volume_unit);
	}

	free(buf2);
}
