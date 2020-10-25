// SPDX-License-Identifier: GPL-2.0

#include "divecomputer.h"
#include "event.h"
#include "extradata.h"
#include "pref.h"
#include "sample.h"
#include "structured_list.h"
#include "subsurface-string.h"

#include <string.h>
#include <stdlib.h>

/*
 * Good fake dive profiles are hard.
 *
 * "depthtime" is the integral of the dive depth over
 * time ("area" of the dive profile). We want that
 * area to match the average depth (avg_d*max_t).
 *
 * To do that, we generate a 6-point profile:
 *
 *  (0, 0)
 *  (t1, max_d)
 *  (t2, max_d)
 *  (t3, d)
 *  (t4, d)
 *  (max_t, 0)
 *
 * with the same ascent/descent rates between the
 * different depths.
 *
 * NOTE: avg_d, max_d and max_t are given constants.
 * The rest we can/should play around with to get a
 * good-looking profile.
 *
 * That six-point profile gives a total area of:
 *
 *   (max_d*max_t) - (max_d*t1) - (max_d-d)*(t4-t3)
 *
 * And the "same ascent/descent rates" requirement
 * gives us (time per depth must be same):
 *
 *   t1 / max_d = (t3-t2) / (max_d-d)
 *   t1 / max_d = (max_t-t4) / d
 *
 * We also obviously require:
 *
 *   0 <= t1 <= t2 <= t3 <= t4 <= max_t
 *
 * Let us call 'd_frac = d / max_d', and we get:
 *
 * Total area must match average depth-time:
 *
 *   (max_d*max_t) - (max_d*t1) - (max_d-d)*(t4-t3) = avg_d*max_t
 *      max_d*(max_t-t1-(1-d_frac)*(t4-t3)) = avg_d*max_t
 *             max_t-t1-(1-d_frac)*(t4-t3) = avg_d*max_t/max_d
 *                   t1+(1-d_frac)*(t4-t3) = max_t*(1-avg_d/max_d)
 *
 * and descent slope must match ascent slopes:
 *
 *   t1 / max_d = (t3-t2) / (max_d*(1-d_frac))
 *           t1 = (t3-t2)/(1-d_frac)
 *
 * and
 *
 *   t1 / max_d = (max_t-t4) / (max_d*d_frac)
 *           t1 = (max_t-t4)/d_frac
 *
 * In general, we have more free variables than we have constraints,
 * but we can aim for certain basics, like a good ascent slope.
 */
static int fill_samples(struct sample *s, int max_d, int avg_d, int max_t, double slope, double d_frac)
{
	double t_frac = max_t * (1 - avg_d / (double)max_d);
	int t1 = lrint(max_d / slope);
	int t4 = lrint(max_t - t1 * d_frac);
	int t3 = lrint(t4 - (t_frac - t1) / (1 - d_frac));
	int t2 = lrint(t3 - t1 * (1 - d_frac));

	if (t1 < 0 || t1 > t2 || t2 > t3 || t3 > t4 || t4 > max_t)
		return 0;

	s[1].time.seconds = t1;
	s[1].depth.mm = max_d;
	s[2].time.seconds = t2;
	s[2].depth.mm = max_d;
	s[3].time.seconds = t3;
	s[3].depth.mm = lrint(max_d * d_frac);
	s[4].time.seconds = t4;
	s[4].depth.mm = lrint(max_d * d_frac);

	return 1;
}

/* we have no average depth; instead of making up a random average depth
 * we should assume either a PADI rectangular profile (for short and/or
 * shallow dives) or more reasonably a six point profile with a 3 minute
 * safety stop at 5m */
static void fill_samples_no_avg(struct sample *s, int max_d, int max_t, double slope)
{
	// shallow or short dives are just trapecoids based on the given slope
	if (max_d < 10000 || max_t < 600) {
		s[1].time.seconds = lrint(max_d / slope);
		s[1].depth.mm = max_d;
		s[2].time.seconds = max_t - lrint(max_d / slope);
		s[2].depth.mm = max_d;
	} else {
		s[1].time.seconds = lrint(max_d / slope);
		s[1].depth.mm = max_d;
		s[2].time.seconds = max_t - lrint(max_d / slope) - 180;
		s[2].depth.mm = max_d;
		s[3].time.seconds = max_t - lrint(5000 / slope) - 180;
		s[3].depth.mm = 5000;
		s[4].time.seconds = max_t - lrint(5000 / slope);
		s[4].depth.mm = 5000;
	}
}

void fake_dc(struct divecomputer *dc)
{
	alloc_samples(dc, 6);
	struct sample *fake = dc->sample;
	int i;

	dc->samples = 6;

	/* The dive has no samples, so create a few fake ones */
	int max_t = dc->duration.seconds;
	int max_d = dc->maxdepth.mm;
	int avg_d = dc->meandepth.mm;

	memset(fake, 0, 6 * sizeof(struct sample));
	fake[5].time.seconds = max_t;
	for (i = 0; i < 6; i++) {
		fake[i].bearing.degrees = -1;
		fake[i].ndl.seconds = -1;
	}
	if (!max_t || !max_d) {
		dc->samples = 0;
		return;
	}

	/* Set last manually entered time to the total dive length */
	dc->last_manual_time = dc->duration;

	/*
	 * We want to fake the profile so that the average
	 * depth ends up correct. However, in the absence of
	 * a reasonable average, let's just make something
	 * up. Note that 'avg_d == max_d' is _not_ a reasonable
	 * average.
	 * We explicitly treat avg_d == 0 differently */
	if (avg_d == 0) {
		/* we try for a sane slope, but bow to the insanity of
		 * the user supplied data */
		fill_samples_no_avg(fake, max_d, max_t, MAX(2.0 * max_d / max_t, (double)prefs.ascratelast6m));
		if (fake[3].time.seconds == 0) { // just a 4 point profile
			dc->samples = 4;
			fake[3].time.seconds = max_t;
		}
		return;
	}
	if (avg_d < max_d / 10 || avg_d >= max_d) {
		avg_d = (max_d + 10000) / 3;
		if (avg_d > max_d)
			avg_d = max_d * 2 / 3;
	}
	if (!avg_d)
		avg_d = 1;

	/*
	 * Ok, first we try a basic profile with a specific ascent
	 * rate (5 meters per minute) and d_frac (1/3).
	 */
	if (fill_samples(fake, max_d, avg_d, max_t, (double)prefs.ascratelast6m, 0.33))
		return;

	/*
	 * Ok, assume that didn't work because we cannot make the
	 * average come out right because it was a quick deep dive
	 * followed by a much shallower region
	 */
	if (fill_samples(fake, max_d, avg_d, max_t, 10000.0 / 60, 0.10))
		return;

	/*
	 * Uhhuh. That didn't work. We'd need to find a good combination that
	 * satisfies our constraints. Currently, we don't, we just give insane
	 * slopes.
	 */
	if (fill_samples(fake, max_d, avg_d, max_t, 10000.0, 0.01))
		return;

	/* Even that didn't work? Give up, there's something wrong */
}

/* Find the divemode at time 'time' (in seconds) into the dive. Sequentially step through the divemode-change events,
 * saving the dive mode for each event. When the events occur AFTER 'time' seconds, the last stored divemode
 * is returned. This function is self-tracking, relying on setting the event pointer 'evp' so that, in each iteration
 * that calls this function, the search does not have to begin at the first event of the dive */
enum divemode_t get_current_divemode(const struct divecomputer *dc, int time, const struct event **evp, enum divemode_t *divemode)
{
	const struct event *ev = *evp;
	if (dc) {
		if (*divemode == UNDEF_COMP_TYPE) {
			*divemode = dc->divemode;
			ev = get_next_event(dc->events, "modechange");
		}
	} else {
		ev = NULL;
	}
	while (ev && ev->time.seconds < time) {
		*divemode = (enum divemode_t) ev->value;
		ev = get_next_event(ev->next, "modechange");
	}
	*evp = ev;
	return *divemode;
}


/* helper function to make it easier to work with our structures
 * we don't interpolate here, just use the value from the last sample up to that time */
int get_depth_at_time(const struct divecomputer *dc, unsigned int time)
{
	int depth = 0;
	if (dc && dc->sample)
		for (int i = 0; i < dc->samples; i++) {
			if (dc->sample[i].time.seconds > time)
				break;
			depth = dc->sample[i].depth.mm;
		}
	return depth;
}


/* The first divecomputer is embedded in the dive structure. Free its data but not
 * the structure itself. For all remainding dcs in the list, free data *and* structures. */
void free_dive_dcs(struct divecomputer *dc)
{
	free_dc_contents(dc);
	STRUCTURED_LIST_FREE(struct divecomputer, dc->next, free_dc);
}

/* make room for num samples; if not enough space is available, the sample
 * array is reallocated and the existing samples are copied. */
void alloc_samples(struct divecomputer *dc, int num)
{
	if (num > dc->alloc_samples) {
		dc->alloc_samples = (num * 3) / 2 + 10;
		dc->sample = realloc(dc->sample, dc->alloc_samples * sizeof(struct sample));
		if (!dc->sample)
			dc->samples = dc->alloc_samples = 0;
	}
}

void free_samples(struct divecomputer *dc)
{
	if (dc) {
		free(dc->sample);
		dc->sample = 0;
		dc->samples = 0;
		dc->alloc_samples = 0;
	}
}

struct sample *prepare_sample(struct divecomputer *dc)
{
	if (dc) {
		int nr = dc->samples;
		struct sample *sample;
		alloc_samples(dc, nr + 1);
		if (!dc->sample)
			return NULL;
		sample = dc->sample + nr;
		memset(sample, 0, sizeof(*sample));

		// Copy the sensor numbers - but not the pressure values
		// from the previous sample if any.
		if (nr) {
			for (int idx = 0; idx < MAX_SENSORS; idx++)
				sample->sensor[idx] = sample[-1].sensor[idx];
		}
		// Init some values with -1
		sample->bearing.degrees = -1;
		sample->ndl.seconds = -1;

		return sample;
	}
	return NULL;
}


void finish_sample(struct divecomputer *dc)
{
	dc->samples++;
}

struct sample *add_sample(const struct sample *sample, int time, struct divecomputer *dc)
{
	struct sample *p = prepare_sample(dc);

	if (p) {
		*p = *sample;
		p->time.seconds = time;
		finish_sample(dc);
	}
	return p;
}

/*
 * Calculate how long we were actually under water, and the average
 * depth while under water.
 *
 * This ignores any surface time in the middle of the dive.
 */
void fixup_dc_duration(struct divecomputer *dc)
{
	int duration, i;
	int lasttime, lastdepth, depthtime;

	duration = 0;
	lasttime = 0;
	lastdepth = 0;
	depthtime = 0;
	for (i = 0; i < dc->samples; i++) {
		struct sample *sample = dc->sample + i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;

		/* We ignore segments at the surface */
		if (depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD) {
			duration += time - lasttime;
			depthtime += (time - lasttime) * (depth + lastdepth) / 2;
		}
		lastdepth = depth;
		lasttime = time;
	}
	if (duration) {
		dc->duration.seconds = duration;
		dc->meandepth.mm = (depthtime + duration / 2) / duration;
	}
}


/*
 * What do the dive computers say the water temperature is?
 * (not in the samples, but as dc property for dcs that support that)
 */
unsigned int dc_watertemp(const struct divecomputer *dc)
{
	int sum = 0, nr = 0;

	do {
		if (dc->watertemp.mkelvin) {
			sum += dc->watertemp.mkelvin;
			nr++;
		}
	} while ((dc = dc->next) != NULL);
	if (!nr)
		return 0;
	return (sum + nr / 2) / nr;
}

/*
 * What do the dive computers say the air temperature is?
 */
unsigned int dc_airtemp(const struct divecomputer *dc)
{
	int sum = 0, nr = 0;

	do {
		if (dc->airtemp.mkelvin) {
			sum += dc->airtemp.mkelvin;
			nr++;
		}
	} while ((dc = dc->next) != NULL);
	if (!nr)
		return 0;
	return (sum + nr / 2) / nr;
}

/* copies all events in this dive computer */
void copy_events(const struct divecomputer *s, struct divecomputer *d)
{
	const struct event *ev;
	struct event **pev;
	if (!s || !d)
		return;
	ev = s->events;
	pev = &d->events;
	while (ev != NULL) {
		struct event *new_ev = clone_event(ev);
		*pev = new_ev;
		pev = &new_ev->next;
		ev = ev->next;
	}
	*pev = NULL;
}

void copy_samples(const struct divecomputer *s, struct divecomputer *d)
{
	/* instead of carefully copying them one by one and calling add_sample
	 * over and over again, let's just copy the whole blob */
	if (!s || !d)
		return;
	int nr = s->samples;
	d->samples = nr;
	d->alloc_samples = nr;
	// We expect to be able to read the memory in the other end of the pointer
	// if its a valid pointer, so don't expect malloc() to return NULL for
	// zero-sized malloc, do it ourselves.
	d->sample = NULL;

	if(!nr)
		return;

	d->sample = malloc(nr * sizeof(struct sample));
	if (d->sample)
		memcpy(d->sample, s->sample, nr * sizeof(struct sample));
}

void add_event_to_dc(struct divecomputer *dc, struct event *ev)
{
	struct event **p;

	p = &dc->events;

	/* insert in the sorted list of events */
	while (*p && (*p)->time.seconds <= ev->time.seconds)
		p = &(*p)->next;
	ev->next = *p;
	*p = ev;
}

struct event *add_event(struct divecomputer *dc, unsigned int time, int type, int flags, int value, const char *name)
{
	struct event *ev = create_event(time, type, flags, value, name);

	if (!ev)
		return NULL;

	add_event_to_dc(dc, ev);

	remember_event(name);
	return ev;
}

/* Substitutes an event in a divecomputer for another. No reordering is performed! */
void swap_event(struct divecomputer *dc, struct event *from, struct event *to)
{
	for (struct event **ep = &dc->events; *ep; ep = &(*ep)->next) {
		if (*ep == from) {
			to->next = from->next;
			*ep = to;
			from->next = NULL; // For good measure.
			break;
		}
	}
}

/* Remove given event from dive computer. Does *not* free the event. */
void remove_event_from_dc(struct divecomputer *dc, struct event *event)
{
	for (struct event **ep = &dc->events; *ep; ep = &(*ep)->next) {
		if (*ep == event) {
			*ep = event->next;
			event->next = NULL; // For good measure.
			break;
		}
	}
}

void add_extra_data(struct divecomputer *dc, const char *key, const char *value)
{
	struct extra_data **ed = &dc->extra_data;

	while (*ed)
		ed = &(*ed)->next;
	*ed = malloc(sizeof(struct extra_data));
	if (*ed) {
		(*ed)->key = strdup(key);
		(*ed)->value = strdup(value);
		(*ed)->next = NULL;
	}
}

bool is_dc_planner(const struct divecomputer *dc)
{
	return same_string(dc->model, "planned dive");
}

/*
 * Match two dive computer entries against each other, and
 * tell if it's the same dive. Return 0 if "don't know",
 * positive for "same dive" and negative for "definitely
 * not the same dive"
 */
int match_one_dc(const struct divecomputer *a, const struct divecomputer *b)
{
	/* Not same model? Don't know if matching.. */
	if (!a->model || !b->model)
		return 0;
	if (strcasecmp(a->model, b->model))
		return 0;

	/* Different device ID's? Don't know */
	if (a->deviceid != b->deviceid)
		return 0;

	/* Do we have dive IDs? */
	if (!a->diveid || !b->diveid)
		return 0;

	/*
	 * If they have different dive ID's on the same
	 * dive computer, that's a definite "same or not"
	 */
	return a->diveid == b->diveid && a->when == b->when ? 1 : -1;
}

static void free_extra_data(struct extra_data *ed)
{
	free((void *)ed->key);
	free((void *)ed->value);
}

void free_dc_contents(struct divecomputer *dc)
{
	free(dc->sample);
	free((void *)dc->model);
	free((void *)dc->serial);
	free((void *)dc->fw_version);
	free_events(dc->events);
	STRUCTURED_LIST_FREE(struct extra_data, dc->extra_data, free_extra_data);
}

void free_dc(struct divecomputer *dc)
{
	free_dc_contents(dc);
	free(dc);
}

