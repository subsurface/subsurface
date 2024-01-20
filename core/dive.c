// SPDX-License-Identifier: GPL-2.0
/* dive.c */
/* maintains the internal dive list structure */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "dive.h"
#include "gettext.h"
#include "subsurface-string.h"
#include "libdivecomputer.h"
#include "device.h"
#include "divelist.h"
#include "divelog.h"
#include "divesite.h"
#include "errorhelper.h"
#include "event.h"
#include "extradata.h"
#include "interpolate.h"
#include "qthelper.h"
#include "membuffer.h"
#include "picture.h"
#include "sample.h"
#include "tag.h"
#include "trip.h"
#include "structured_list.h"
#include "fulltext.h"

// For user visible text but still not translated
const char *divemode_text_ui[] = {
	QT_TRANSLATE_NOOP("gettextFromC", "Open circuit"),
	QT_TRANSLATE_NOOP("gettextFromC", "CCR"),
	QT_TRANSLATE_NOOP("gettextFromC", "pSCR"),
	QT_TRANSLATE_NOOP("gettextFromC", "Freedive")
};

// For writing/reading files.
const char *divemode_text[] = {"OC", "CCR", "PSCR", "Freedive"};

static double calculate_depth_to_mbarf(int depth, pressure_t surface_pressure, int salinity);

/*
 * The legacy format for sample pressures has a single pressure
 * for each sample that can have any sensor, plus a possible
 * "o2pressure" that is fixed to the Oxygen sensor for a CCR dive.
 *
 * For more complex pressure data, we have to use explicit
 * cylinder indices for each sample.
 *
 * This function returns a negative number for "no legacy mode",
 * or a non-negative number that indicates the o2 sensor index.
 */
int legacy_format_o2pressures(const struct dive *dive, const struct divecomputer *dc)
{
	int i, o2sensor;

	o2sensor = (dc->divemode == CCR) ? get_cylinder_idx_by_use(dive, OXYGEN) : -1;
	for (i = 0; i < dc->samples; i++) {
		const struct sample *s = dc->sample + i;
		int seen_pressure = 0, idx;

		for (idx = 0; idx < MAX_SENSORS; idx++) {
			int sensor = s->sensor[idx];
			pressure_t p = s->pressure[idx];

			if (!p.mbar)
				continue;
			if (sensor == o2sensor)
				continue;
			if (seen_pressure)
				return -1;
			seen_pressure = 1;
		}
	}

	/*
	 * Use legacy mode: if we have no O2 sensor we return a
	 * positive sensor index that is guaranmteed to not match
	 * any sensor (we encode it as 8 bits).
	 */
	return o2sensor < 0 ? 256 : o2sensor;
}

/* warning: does not test idx for validity */
struct event *create_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx)
{
	/* The gas switch event format is insane for historical reasons */
	struct gasmix mix = get_cylinder(dive, idx)->gasmix;
	int o2 = get_o2(mix);
	int he = get_he(mix);
	struct event *ev;
	int value;

	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	value = o2 + (he << 16);

	ev = create_event(seconds, he ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE, 0, value, "gaschange");
	ev->gas.index = idx;
	ev->gas.mix = mix;
	return ev;
}

void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx)
{
	/* sanity check so we don't crash */
	/* FIXME: The planner uses a dummy cylinder one past the official number of cylinders
	 * in the table to mark no-cylinder surface interavals. This is horrendous. Fix ASAP. */
	//if (idx < 0 || idx >= dive->cylinders.nr) {
	if (idx < 0 || idx >= dive->cylinders.nr + 1 || idx >= dive->cylinders.allocated) {
		report_error("Unknown cylinder index: %d", idx);
		return;
	}
	struct event *ev = create_gas_switch_event(dive, dc, seconds, idx);
	add_event_to_dc(dc, ev);
}

/* since the name is an array as part of the structure (how silly is that?) we
 * have to actually remove the existing event and replace it with a new one.
 * WARNING, WARNING... this may end up freeing event in case that event is indeed
 * WARNING, WARNING... part of this divecomputer on this dive! */
void update_event_name(struct dive *d, int dc_number, struct event *event, const char *name)
{
	if (!d || !event)
		return;
	struct divecomputer *dc = get_dive_dc(d, dc_number);
	if (!dc)
		return;
	struct event **removep = &dc->events;
	struct event *remove;
	while ((*removep)->next && !same_event(*removep, event))
		removep = &(*removep)->next;
	if (!same_event(*removep, event))
		return;
	remove = *removep;
	*removep = (*removep)->next;
	add_event(dc, event->time.seconds, event->type, event->flags, event->value, name);
	free(remove);
	invalidate_dive_cache(d);
}

struct gasmix get_gasmix_from_event(const struct dive *dive, const struct event *ev)
{
	if (ev && event_is_gaschange(ev)) {
		int index = ev->gas.index;
		// FIXME: The planner uses one past cylinder-count to signify "surface air". Remove in due course.
		if (index == dive->cylinders.nr)
			return gasmix_air;
		if (index >= 0 && index < dive->cylinders.nr)
			return get_cylinder(dive, index)->gasmix;
		return ev->gas.mix;
	}
	return gasmix_air;
}

// we need this to be uniq. oh, and it has no meaning whatsoever
// - that's why we have the silly initial number and increment by 3 :-)
int dive_getUniqID()
{
	static int maxId = 83529;
	maxId += 3;
	return maxId;
}

struct dive *alloc_dive(void)
{
	struct dive *dive;

	dive = malloc(sizeof(*dive));
	if (!dive)
		exit(1);
	memset(dive, 0, sizeof(*dive));
	dive->id = dive_getUniqID();
	return dive;
}

/* copy an element in a list of dive computer extra data */
static void copy_extra_data(struct extra_data *sed, struct extra_data *ded)
{
	ded->key = copy_string(sed->key);
	ded->value = copy_string(sed->value);
}

/* this is very different from the copy_divecomputer later in this file;
 * this function actually makes full copies of the content */
static void copy_dc(const struct divecomputer *sdc, struct divecomputer *ddc)
{
	*ddc = *sdc;
	ddc->model = copy_string(sdc->model);
	ddc->serial = copy_string(sdc->serial);
	ddc->fw_version = copy_string(sdc->fw_version);
	copy_samples(sdc, ddc);
	copy_events(sdc, ddc);
	STRUCTURED_LIST_COPY(struct extra_data, sdc->extra_data, ddc->extra_data, copy_extra_data);
}

static void dc_cylinder_renumber(struct dive *dive, struct divecomputer *dc, const int mapping[]);

/* copy dive computer list and renumber the cylinders
 * space for the first divecomputer is provided by the
 * caller, the remainder is allocated */
static void copy_dc_renumber(struct dive *d, const struct divecomputer *sdc, struct divecomputer *ddc, const int cylinders_map[])
{
	for (;;) {
		copy_dc(sdc, ddc);
		dc_cylinder_renumber(d, ddc, cylinders_map);
		if (!sdc->next)
			break;
		sdc = sdc->next;
		ddc->next = calloc(1, sizeof(struct divecomputer));
		ddc = ddc->next;
	}
	ddc->next = NULL;
}

static void free_dive_structures(struct dive *d)
{
	if (!d)
		return;
	fulltext_unregister(d);
	/* free the strings */
	free(d->buddy);
	free(d->diveguide);
	free(d->notes);
	free(d->suit);
	/* free tags, additional dive computers, and pictures */
	taglist_free(d->tag_list);
	free_dive_dcs(&d->dc);
	clear_cylinder_table(&d->cylinders);
	free(d->cylinders.cylinders);
	clear_weightsystem_table(&d->weightsystems);
	free(d->weightsystems.weightsystems);
	clear_picture_table(&d->pictures);
	free(d->pictures.pictures);
}

void free_dive(struct dive *d)
{
	free_dive_structures(d);
	free(d);
}

/* copy_dive makes duplicates of many components of a dive;
 * in order not to leak memory, we need to free those .
 * copy_dive doesn't play with the divetrip and forward/backward pointers
 * so we can ignore those */
void clear_dive(struct dive *d)
{
	if (!d)
		return;
	free_dive_structures(d);
	memset(d, 0, sizeof(struct dive));
}

/* make a true copy that is independent of the source dive;
 * all data structures are duplicated, so the copy can be modified without
 * any impact on the source */
static void copy_dive_nodc(const struct dive *s, struct dive *d)
{
	clear_dive(d);
	/* simply copy things over, but then make actual copies of the
	 * relevant components that are referenced through pointers,
	 * so all the strings and the structured lists */
	*d = *s;
	memset(&d->cylinders, 0, sizeof(d->cylinders));
	memset(&d->weightsystems, 0, sizeof(d->weightsystems));
	memset(&d->pictures, 0, sizeof(d->pictures));
	d->full_text = NULL;
	invalidate_dive_cache(d);
	d->buddy = copy_string(s->buddy);
	d->diveguide = copy_string(s->diveguide);
	d->notes = copy_string(s->notes);
	d->suit = copy_string(s->suit);
	copy_cylinders(&s->cylinders, &d->cylinders);
	copy_weights(&s->weightsystems, &d->weightsystems);
	copy_pictures(&s->pictures, &d->pictures);
	d->tag_list = taglist_copy(s->tag_list);
}

void copy_dive(const struct dive *s, struct dive *d)
{
	copy_dive_nodc(s, d);

	// Copy the first dc explicitly, then the list of subsequent dc's
	copy_dc(&s->dc, &d->dc);
	STRUCTURED_LIST_COPY(struct divecomputer, s->dc.next, d->dc.next, copy_dc);
}

static void copy_dive_onedc(const struct dive *s, const struct divecomputer *sdc, struct dive *d)
{
	copy_dive_nodc(s, d);
	copy_dc(sdc, &d->dc);
	d->dc.next = NULL;
}

/* make a clone of the source dive and clean out the source dive;
 * this allows us to create a dive on the stack and then
 * add it to the divelist. */
struct dive *move_dive(struct dive *s)
{
	struct dive *dive = alloc_dive();
	*dive = *s;			   // so all the pointers in dive point to the things s pointed to
	memset(s, 0, sizeof(struct dive)); // and now the pointers in s are gone
	return dive;
}

#define CONDITIONAL_COPY_STRING(_component) \
	if (what._component)                \
		d->_component = copy_string(s->_component)

// copy elements, depending on bits in what that are set
void selective_copy_dive(const struct dive *s, struct dive *d, struct dive_components what, bool clear)
{
	if (clear)
		clear_dive(d);
	CONDITIONAL_COPY_STRING(notes);
	CONDITIONAL_COPY_STRING(diveguide);
	CONDITIONAL_COPY_STRING(buddy);
	CONDITIONAL_COPY_STRING(suit);
	if (what.rating)
		d->rating = s->rating;
	if (what.visibility)
		d->visibility = s->visibility;
	if (what.divesite) {
		unregister_dive_from_dive_site(d);
		add_dive_to_dive_site(d, s->dive_site);
	}
	if (what.tags)
		d->tag_list = taglist_copy(s->tag_list);
	if (what.cylinders)
		copy_cylinder_types(s, d);
	if (what.weights)
		copy_weights(&s->weightsystems, &d->weightsystems);
	if (what.number)
		d->number = s->number;
	if (what.when)
		d->when = s->when;
}
#undef CONDITIONAL_COPY_STRING

/* copies all events from all dive computers before a given time
   this is used when editing a dive in the planner to preserve the events
   of the old dive */
void copy_events_until(const struct dive *sd, struct dive *dd, int time)
{
	if (!sd || !dd)
		return;

	const struct divecomputer *s = &sd->dc;
	struct divecomputer *d = &dd->dc;

	while (s && d) {
		const struct event *ev;
		ev = s->events;
		while (ev != NULL) {
			// Don't add events the planner knows about
			if (ev->time.seconds < time && !event_is_gaschange(ev) && !event_is_divemodechange(ev))
				add_event(d, ev->time.seconds, ev->type, ev->flags, ev->value, ev->name);
			ev = ev->next;
		}
		s = s->next;
		d = d->next;
	}
}

int nr_cylinders(const struct dive *dive)
{
	return dive->cylinders.nr;
}

int nr_weightsystems(const struct dive *dive)
{
	return dive->weightsystems.nr;
}

void copy_used_cylinders(const struct dive *s, struct dive *d, bool used_only)
{
	int i;
	if (!s || !d)
		return;

	clear_cylinder_table(&d->cylinders);
	for (i = 0; i < s->cylinders.nr; i++) {
		if (!used_only || is_cylinder_used(s, i) || get_cylinder(s, i)->cylinder_use == NOT_USED)
			add_cloned_cylinder(&d->cylinders, *get_cylinder(s, i));
	}
}

/*
 * So when we re-calculate maxdepth and meandepth, we will
 * not override the old numbers if they are close to the
 * new ones.
 *
 * Why? Because a dive computer may well actually track the
 * max. depth and mean depth at finer granularity than the
 * samples it stores. So it's possible that the max and mean
 * have been reported more correctly originally.
 *
 * Only if the values calculated from the samples are clearly
 * different do we override the normal depth values.
 *
 * This considers 1m to be "clearly different". That's
 * a totally random number.
 */
static void update_depth(depth_t *depth, int new)
{
	if (new) {
		int old = depth->mm;

		if (abs(old - new) > 1000)
			depth->mm = new;
	}
}

static void update_temperature(temperature_t *temperature, int new)
{
	if (new) {
		int old = temperature->mkelvin;

		if (abs(old - new) > 1000)
			temperature->mkelvin = new;
	}
}

/* Which cylinders had gas used? */
#define SOME_GAS 5000
static bool cylinder_used(const cylinder_t *cyl)
{
	int start_mbar, end_mbar;

	start_mbar = cyl->start.mbar ?: cyl->sample_start.mbar;
	end_mbar = cyl->end.mbar ?: cyl->sample_end.mbar;

	// More than 5 bar used? This matches statistics.c
	// heuristics
	return start_mbar > end_mbar + SOME_GAS;
}

/* Get list of used cylinders. Returns the number of used cylinders. */
static int get_cylinder_used(const struct dive *dive, bool used[])
{
	int i, num = 0;

	for (i = 0; i < dive->cylinders.nr; i++) {
		used[i] = cylinder_used(get_cylinder(dive, i));
		if (used[i])
			num++;
	}
	return num;
}

/* Are there any used cylinders which we do not know usage about? */
static bool has_unknown_used_cylinders(const struct dive *dive, const struct divecomputer *dc,
				       const bool used_cylinders[], int num)
{
	int idx;
	const struct event *ev;
	bool *used_and_unknown = malloc(dive->cylinders.nr * sizeof(bool));
	memcpy(used_and_unknown, used_cylinders, dive->cylinders.nr * sizeof(bool));

	/* We know about using the O2 cylinder in a CCR dive */
	if (dc->divemode == CCR) {
		int o2_cyl = get_cylinder_idx_by_use(dive, OXYGEN);
		if (o2_cyl >= 0 && used_and_unknown[o2_cyl]) {
			used_and_unknown[o2_cyl] = false;
			num--;
		}
	}

	/* We know about the explicit first cylinder (or first) */
	idx = explicit_first_cylinder(dive, dc);
	if (used_and_unknown[idx]) {
		used_and_unknown[idx] = false;
		num--;
	}

	/* And we have possible switches to other gases */
	ev = get_next_event(dc->events, "gaschange");
	while (ev && num > 0) {
		idx = get_cylinder_index(dive, ev);
		if (idx >= 0 && used_and_unknown[idx]) {
			used_and_unknown[idx] = false;
			num--;
		}
		ev = get_next_event(ev->next, "gaschange");
	}

	free(used_and_unknown);
	return num > 0;
}

void per_cylinder_mean_depth(const struct dive *dive, struct divecomputer *dc, int *mean, int *duration)
{
	int i;
	int *depthtime;
	uint32_t lasttime = 0;
	int lastdepth = 0;
	int idx = 0;
	bool *used_cylinders;
	int num_used_cylinders;

	if (dive->cylinders.nr <= 0)
		return;

	for (i = 0; i < dive->cylinders.nr; i++)
		mean[i] = duration[i] = 0;
	if (!dc)
		return;

	/*
	 * There is no point in doing per-cylinder information
	 * if we don't actually know about the usage of all the
	 * used cylinders.
	 */
	used_cylinders = malloc(dive->cylinders.nr * sizeof(bool));
	num_used_cylinders = get_cylinder_used(dive, used_cylinders);
	if (has_unknown_used_cylinders(dive, dc, used_cylinders, num_used_cylinders)) {
		/*
		 * If we had more than one used cylinder, but
		 * do not know usage of them, we simply cannot
		 * account mean depth to them.
		 */
		if (num_used_cylinders > 1) {
			free(used_cylinders);
			return;
		}

		/*
		 * For a single cylinder, use the overall mean
		 * and duration
		 */
		for (i = 0; i < dive->cylinders.nr; i++) {
			if (used_cylinders[i]) {
				mean[i] = dc->meandepth.mm;
				duration[i] = dc->duration.seconds;
			}
		}

		free(used_cylinders);
		return;
	}
	free(used_cylinders);
	if (!dc->samples)
		fake_dc(dc);
	const struct event *ev = get_next_event(dc->events, "gaschange");
	depthtime = malloc(dive->cylinders.nr * sizeof(*depthtime));
	memset(depthtime, 0, dive->cylinders.nr * sizeof(*depthtime));
	for (i = 0; i < dc->samples; i++) {
		struct sample *sample = dc->sample + i;
		uint32_t time = sample->time.seconds;
		int depth = sample->depth.mm;

		/* Make sure to move the event past 'lasttime' */
		while (ev && lasttime >= ev->time.seconds) {
			idx = get_cylinder_index(dive, ev);
			ev = get_next_event(ev->next, "gaschange");
		}

		/* Do we need to fake a midway sample at an event? */
		if (ev && time > ev->time.seconds) {
			int newtime = ev->time.seconds;
			int newdepth = interpolate(lastdepth, depth, newtime - lasttime, time - lasttime);

			time = newtime;
			depth = newdepth;
			i--;
		}
		/* We ignore segments at the surface */
		if (depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD) {
			duration[idx] += time - lasttime;
			depthtime[idx] += (time - lasttime) * (depth + lastdepth) / 2;
		}
		lastdepth = depth;
		lasttime = time;
	}
	for (i = 0; i < dive->cylinders.nr; i++) {
		if (duration[i])
			mean[i] = (depthtime[i] + duration[i] / 2) / duration[i];
	}
	free(depthtime);
}

static void update_min_max_temperatures(struct dive *dive, temperature_t temperature)
{
	if (temperature.mkelvin) {
		if (!dive->maxtemp.mkelvin || temperature.mkelvin > dive->maxtemp.mkelvin)
			dive->maxtemp = temperature;
		if (!dive->mintemp.mkelvin || temperature.mkelvin < dive->mintemp.mkelvin)
			dive->mintemp = temperature;
	}
}

/*
 * If the cylinder tank pressures are within half a bar
 * (about 8 PSI) of the sample pressures, we consider it
 * to be a rounding error, and throw them away as redundant.
 */
static int same_rounded_pressure(pressure_t a, pressure_t b)
{
	return abs(a.mbar - b.mbar) <= 500;
}

/* Some dive computers (Cobalt) don't start the dive with cylinder 0 but explicitly
 * tell us what the first gas is with a gas change event in the first sample.
 * Sneakily we'll use a return value of 0 (or FALSE) when there is no explicit
 * first cylinder - in which case cylinder 0 is indeed the first cylinder.
 * We likewise return 0 if the event concerns a cylinder that doesn't exist.
 * If the dive has no cylinders, -1 is returned. */
int explicit_first_cylinder(const struct dive *dive, const struct divecomputer *dc)
{
	int res = 0;
	if (!dive->cylinders.nr)
		return -1;
	if (dc) {
		const struct event *ev = get_next_event(dc->events, "gaschange");
		if (ev && ((dc->sample && ev->time.seconds == dc->sample[0].time.seconds) || ev->time.seconds <= 1))
			res = get_cylinder_index(dive, ev);
		else if (dc->divemode == CCR)
			res = MAX(get_cylinder_idx_by_use(dive, DILUENT), res);
	}
	return res < dive->cylinders.nr ? res : 0;
}

/* this gets called when the dive mode has changed (so OC vs. CC)
 * there are two places we might have setpoints... events or in the samples
 */
void update_setpoint_events(const struct dive *dive, struct divecomputer *dc)
{
	struct event *ev;
	int new_setpoint = 0;

	if (dc->divemode == CCR)
		new_setpoint = prefs.defaultsetpoint;

	if (dc->divemode == OC &&
	    (same_string(dc->model, "Shearwater Predator") ||
	     same_string(dc->model, "Shearwater Petrel") ||
	     same_string(dc->model, "Shearwater Nerd"))) {
		// make sure there's no setpoint in the samples
		// this is an irreversible change - so switching a dive to OC
		// by mistake when it's actually CCR is _bad_
		// So we make sure, this comes from a Predator or Petrel and we only remove
		// pO2 values we would have computed anyway.
		const struct event *ev = get_next_event(dc->events, "gaschange");
		struct gasmix gasmix = get_gasmix_from_event(dive, ev);
		const struct event *next = get_next_event(ev, "gaschange");

		for (int i = 0; i < dc->samples; i++) {
			struct gas_pressures pressures;
			if (next && dc->sample[i].time.seconds >= next->time.seconds) {
				ev = next;
				gasmix = get_gasmix_from_event(dive, ev);
				next = get_next_event(ev, "gaschange");
			}
			fill_pressures(&pressures, lrint(calculate_depth_to_mbarf(dc->sample[i].depth.mm, dc->surface_pressure, 0)), gasmix ,0, dc->divemode);
			if (abs(dc->sample[i].setpoint.mbar - (int)(1000 * pressures.o2)) <= 50)
				dc->sample[i].setpoint.mbar = 0;
		}
	}

	// an "SP change" event at t=0 is currently our marker for OC vs CCR
	// this will need to change to a saner setup, but for now we can just
	// check if such an event is there and adjust it, or add that event
	ev = get_next_event_mutable(dc->events, "SP change");
	if (ev && ev->time.seconds == 0) {
		ev->value = new_setpoint;
	} else {
		if (!add_event(dc, 0, SAMPLE_EVENT_PO2, 0, new_setpoint, "SP change"))
			fprintf(stderr, "Could not add setpoint change event\n");
	}
}

/*
 * See if the size/workingpressure looks like some standard cylinder
 * size, eg "AL80".
 *
 * NOTE! We don't take compressibility into account when naming
 * cylinders. That makes a certain amount of sense, since the
 * cylinder name is independent from the gasmix, and different
 * gasmixes have different compressibility.
 */
static void match_standard_cylinder(cylinder_type_t *type)
{
	double cuft, bar;
	int psi, len;
	const char *fmt;
	char buffer[40], *p;

	/* Do we already have a cylinder description? */
	if (type->description)
		return;

	bar = type->workingpressure.mbar / 1000.0;
	cuft = ml_to_cuft(type->size.mliter);
	cuft *= bar_to_atm(bar);
	psi = lrint(to_PSI(type->workingpressure));

	switch (psi) {
	case 2300 ... 2500: /* 2400 psi: LP tank */
		fmt = "LP%d";
		break;
	case 2600 ... 2700: /* 2640 psi: LP+10% */
		fmt = "LP%d";
		break;
	case 2900 ... 3100: /* 3000 psi: ALx tank */
		fmt = "AL%d";
		break;
	case 3400 ... 3500: /* 3442 psi: HP tank */
		fmt = "HP%d";
		break;
	case 3700 ... 3850: /* HP+10% */
		fmt = "HP%d+";
		break;
	default:
		return;
	}
	len = snprintf(buffer, sizeof(buffer), fmt, (int)lrint(cuft));
	p = malloc(len + 1);
	if (!p)
		return;
	memcpy(p, buffer, len + 1);
	type->description = p;
}

/*
 * There are two ways to give cylinder size information:
 *  - total amount of gas in cuft (depends on working pressure and physical size)
 *  - physical size
 *
 * where "physical size" is the one that actually matters and is sane.
 *
 * We internally use physical size only. But we save the workingpressure
 * so that we can do the conversion if required.
 */
static void sanitize_cylinder_type(cylinder_type_t *type)
{
	/* If we have no working pressure, it had *better* be just a physical size! */
	if (!type->workingpressure.mbar)
		return;

	/* No size either? Nothing to go on */
	if (!type->size.mliter)
		return;

	/* Ok, we have both size and pressure: try to match a description */
	match_standard_cylinder(type);
}

static void sanitize_cylinder_info(struct dive *dive)
{
	int i;

	for (i = 0; i < dive->cylinders.nr; i++) {
		sanitize_gasmix(&get_cylinder(dive, i)->gasmix);
		sanitize_cylinder_type(&get_cylinder(dive, i)->type);
	}
}

/* some events should never be thrown away */
static bool is_potentially_redundant(const struct event *event)
{
	if (!strcmp(event->name, "gaschange"))
		return false;
	if (!strcmp(event->name, "bookmark"))
		return false;
	if (!strcmp(event->name, "heading"))
		return false;
	return true;
}

/* match just by name - we compare the details in the code that uses this helper */
static struct event *find_previous_event(struct divecomputer *dc, struct event *event)
{
	struct event *ev = dc->events;
	struct event *previous = NULL;

	if (empty_string(event->name))
		return NULL;
	while (ev && ev != event) {
		if (same_string(ev->name, event->name))
			previous = ev;
		ev = ev->next;
	}
	return previous;
}

pressure_t calculate_surface_pressure(const struct dive *dive)
{
	const struct divecomputer *dc;
	pressure_t res;
	int sum = 0, nr = 0;

	for_each_relevant_dc(dive, dc) {
		if (dc->surface_pressure.mbar) {
			sum += dc->surface_pressure.mbar;
			nr++;
		}
	}
	res.mbar = nr ? (sum + nr / 2) / nr : 0;
	return res;
}

static void fixup_surface_pressure(struct dive *dive)
{
	dive->surface_pressure = calculate_surface_pressure(dive);
}

/* if the surface pressure in the dive data is redundant to the calculated
 * value (i.e., it was added by running fixup on the dive) return 0,
 * otherwise return the surface pressure given in the dive */
pressure_t un_fixup_surface_pressure(const struct dive *d)
{
	pressure_t res = d->surface_pressure;
	if (res.mbar && res.mbar == calculate_surface_pressure(d).mbar)
		res.mbar = 0;
	return res;
}

static void fixup_water_salinity(struct dive *dive)
{
	struct divecomputer *dc;
	int sum = 0, nr = 0;

	for_each_relevant_dc (dive, dc) {
		if (dc->salinity) {
			if (dc->salinity < 500)
				dc->salinity += FRESHWATER_SALINITY;
			sum += dc->salinity;
			nr++;
		}
	}
	if (nr)
		dive->salinity = (sum + nr / 2) / nr;
}

int get_dive_salinity(const struct dive *dive)
{
	return dive->user_salinity ? dive->user_salinity : dive->salinity;
}

static void fixup_meandepth(struct dive *dive)
{
	struct divecomputer *dc;
	int sum = 0, nr = 0;

	for_each_relevant_dc (dive, dc) {
		if (dc->meandepth.mm) {
			sum += dc->meandepth.mm;
			nr++;
		}
	}

	if (nr)
		dive->meandepth.mm = (sum + nr / 2) / nr;
}

static void fixup_duration(struct dive *dive)
{
	struct divecomputer *dc;
	duration_t duration = { };

	for_each_relevant_dc (dive, dc) {
		duration.seconds = MAX(duration.seconds, dc->duration.seconds);
	}
	dive->duration.seconds = duration.seconds;
}

static void fixup_watertemp(struct dive *dive)
{
	if (!dive->watertemp.mkelvin)
		dive->watertemp.mkelvin = dc_watertemp(&dive->dc);
}

static void fixup_airtemp(struct dive *dive)
{
	if (!dive->airtemp.mkelvin)
		dive->airtemp.mkelvin = dc_airtemp(&dive->dc);
}

/* if the air temperature in the dive data is redundant to the one in its
 * first divecomputer (i.e., it was added by running fixup on the dive)
 * return 0, otherwise return the air temperature given in the dive */
static temperature_t un_fixup_airtemp(const struct dive *a)
{
	temperature_t res = a->airtemp;
	if (a->airtemp.mkelvin && a->airtemp.mkelvin == dc_airtemp(&a->dc))
		res.mkelvin = 0;
	return res;
}

/*
 * events are stored as a linked list, so the concept of
 * "consecutive, identical events" is somewhat hard to
 * implement correctly (especially given that on some dive
 * computers events are asynchronous, so they can come in
 * between what would be the non-constant sample rate).
 *
 * So what we do is that we throw away clearly redundant
 * events that are fewer than 61 seconds apart (assuming there
 * is no dive computer with a sample rate of more than 60
 * seconds... that would be pretty pointless to plot the
 * profile with)
 *
 * We first only mark the events for deletion so that we
 * still know when the previous event happened.
 */
static void fixup_dc_events(struct divecomputer *dc)
{
	struct event *event;

	event = dc->events;
	while (event) {
		struct event *prev;
		if (is_potentially_redundant(event)) {
			prev = find_previous_event(dc, event);
			if (prev && prev->value == event->value &&
			    prev->flags == event->flags &&
			    event->time.seconds - prev->time.seconds < 61)
				event->deleted = true;
		}
		event = event->next;
	}
	event = dc->events;
	while (event) {
		if (event->next && event->next->deleted) {
			struct event *nextnext = event->next->next;
			free(event->next);
			event->next = nextnext;
		} else {
			event = event->next;
		}
	}
}

static int interpolate_depth(struct divecomputer *dc, int idx, int lastdepth, int lasttime, int now)
{
	int i;
	int nextdepth = lastdepth;
	int nexttime = now;

	for (i = idx+1; i < dc->samples; i++) {
		struct sample *sample = dc->sample + i;
		if (sample->depth.mm < 0)
			continue;
		nextdepth = sample->depth.mm;
		nexttime = sample->time.seconds;
		break;
	}
	return interpolate(lastdepth, nextdepth, now-lasttime, nexttime-lasttime);
}

static void fixup_dc_depths(struct dive *dive, struct divecomputer *dc)
{
	int i;
	int maxdepth = dc->maxdepth.mm;
	int lasttime = 0, lastdepth = 0;

	for (i = 0; i < dc->samples; i++) {
		struct sample *sample = dc->sample + i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;

		if (depth < 0) {
			depth = interpolate_depth(dc, i, lastdepth, lasttime, time);
			sample->depth.mm = depth;
		}

		if (depth > SURFACE_THRESHOLD) {
			if (depth > maxdepth)
				maxdepth = depth;
		}

		lastdepth = depth;
		lasttime = time;
		if (sample->cns > dive->maxcns)
			dive->maxcns = sample->cns;
	}

	update_depth(&dc->maxdepth, maxdepth);
	if (!has_planned(dive, false) || !is_dc_planner(dc))
		if (maxdepth > dive->maxdepth.mm)
			dive->maxdepth.mm = maxdepth;
}

static void fixup_dc_ndl(struct divecomputer *dc)
{
	int i;

	for (i = 0; i < dc->samples; i++) {
		struct sample *sample = dc->sample + i;
		if (sample->ndl.seconds != 0)
			break;
		if (sample->ndl.seconds == 0)
			sample->ndl.seconds = -1;
	}
}

static void fixup_dc_temp(struct dive *dive, struct divecomputer *dc)
{
	int i;
	int mintemp = 0, lasttemp = 0;

	for (i = 0; i < dc->samples; i++) {
		struct sample *sample = dc->sample + i;
		int temp = sample->temperature.mkelvin;

		if (temp) {
			/*
			 * If we have consecutive identical
			 * temperature readings, throw away
			 * the redundant ones.
			 */
			if (lasttemp == temp)
				sample->temperature.mkelvin = 0;
			else
				lasttemp = temp;

			if (!mintemp || temp < mintemp)
				mintemp = temp;
		}

		update_min_max_temperatures(dive, sample->temperature);
	}
	update_temperature(&dc->watertemp, mintemp);
	update_min_max_temperatures(dive, dc->watertemp);
}

/* Remove redundant pressure information */
static void simplify_dc_pressures(struct divecomputer *dc)
{
	int i;
	int lastindex[2] = { -1, -1 };
	int lastpressure[2] = { 0 };

	for (i = 0; i < dc->samples; i++) {
		int j;
		struct sample *sample = dc->sample + i;

		for (j = 0; j < MAX_SENSORS; j++) {
			int pressure = sample->pressure[j].mbar;
			int index = sample->sensor[j];

			if (index == lastindex[j]) {
				/* Remove duplicate redundant pressure information */
				if (pressure == lastpressure[j])
					sample->pressure[j].mbar = 0;
			}
			lastindex[j] = index;
			lastpressure[j] = pressure;
		}
	}
}

/* Do we need a sensor -> cylinder mapping? */
static void fixup_start_pressure(struct dive *dive, int idx, pressure_t p)
{
	if (idx >= 0 && idx < dive->cylinders.nr) {
		cylinder_t *cyl = get_cylinder(dive, idx);
		if (p.mbar && !cyl->sample_start.mbar)
			cyl->sample_start = p;
	}
}

static void fixup_end_pressure(struct dive *dive, int idx, pressure_t p)
{
	if (idx >= 0 && idx < dive->cylinders.nr) {
		cylinder_t *cyl = get_cylinder(dive, idx);
		if (p.mbar && !cyl->sample_end.mbar)
			cyl->sample_end = p;
	}
}

/*
 * Check the cylinder pressure sample information and fill in the
 * overall cylinder pressures from those.
 *
 * We ignore surface samples for tank pressure information.
 *
 * At the beginning of the dive, let the cylinder cool down
 * if the diver starts off at the surface. And at the end
 * of the dive, there may be surface pressures where the
 * diver has already turned off the air supply (especially
 * for computers like the Uemis Zurich that end up saving
 * quite a bit of samples after the dive has ended).
 */
static void fixup_dive_pressures(struct dive *dive, struct divecomputer *dc)
{
	int i;

	/* Walk the samples from the beginning to find starting pressures.. */
	for (i = 0; i < dc->samples; i++) {
		int idx;
		struct sample *sample = dc->sample + i;

		if (sample->depth.mm < SURFACE_THRESHOLD)
			continue;

		for (idx = 0; idx < MAX_SENSORS; idx++)
			fixup_start_pressure(dive, sample->sensor[idx], sample->pressure[idx]);
	}

	/* ..and from the end for ending pressures */
	for (i = dc->samples; --i >= 0; ) {
		int idx;
		struct sample *sample = dc->sample + i;

		if (sample->depth.mm < SURFACE_THRESHOLD)
			continue;

		for (idx = 0; idx < MAX_SENSORS; idx++)
			fixup_end_pressure(dive, sample->sensor[idx], sample->pressure[idx]);
	}

	simplify_dc_pressures(dc);
}

/*
 * Match a gas change event against the cylinders we have
 */
static bool validate_gaschange(struct dive *dive, struct event *event)
{
	int index;
	int o2, he, value;

	/* We'll get rid of the per-event gasmix, but for now sanitize it */
	if (gasmix_is_air(event->gas.mix))
		event->gas.mix.o2.permille = 0;

	/* Do we already have a cylinder index for this gasmix? */
	if (event->gas.index >= 0)
		return true;

	index = find_best_gasmix_match(event->gas.mix, &dive->cylinders);
	if (index < 0 || index >= dive->cylinders.nr)
		return false;

	/* Fix up the event to have the right information */
	event->gas.index = index;
	event->gas.mix = get_cylinder(dive, index)->gasmix;

	/* Convert to odd libdivecomputer format */
	o2 = get_o2(event->gas.mix);
	he = get_he(event->gas.mix);

	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	value = o2 + (he << 16);

	event->value = value;
	if (he)
		event->type = SAMPLE_EVENT_GASCHANGE2;

	return true;
}

/* Clean up event, return true if event is ok, false if it should be dropped as bogus */
static bool validate_event(struct dive *dive, struct event *event)
{
	if (event_is_gaschange(event))
		return validate_gaschange(dive, event);
	return true;
}

static void fixup_dc_gasswitch(struct dive *dive, struct divecomputer *dc)
{
	struct event **evp, *event;

	evp = &dc->events;
	while ((event = *evp) != NULL) {
		if (validate_event(dive, event)) {
			evp = &event->next;
			continue;
		}

		/* Delete this event and try the next one */
		*evp = event->next;
	}
}

static void fixup_no_o2sensors(struct divecomputer *dc)
{
	// Its only relevant to look for sensor values on CCR and PSCR dives without any no_o2sensors recorded.
	if (dc->no_o2sensors != 0 || !(dc->divemode == CCR || dc->divemode == PSCR))
		return;

	for (int i = 0; i < dc->samples; i++) {
		int nsensor = 0, j;
		struct sample *s = dc->sample + i;

		// How many o2 sensors can we find in this sample?
		for (j = 0; j < MAX_O2_SENSORS; j++)
			if (s->o2sensor[j].mbar)
				nsensor++;

		// If we fond more than the previous found max, record it.
		if (nsensor > dc->no_o2sensors)
			dc->no_o2sensors = nsensor;

		// Already found the maximum posible amount.
		if (nsensor == MAX_O2_SENSORS)
			return;
	}
}

static void fixup_dc_sample_sensors(struct dive *dive, struct divecomputer *dc)
{
	unsigned long sensor_mask = 0;

	for (int i = 0; i < dc->samples; i++) {
		struct sample *s = dc->sample + i;
		for (int j = 0; j < MAX_SENSORS; j++) {
			int sensor = s->sensor[j];

			// No invalid sensor ID's, please
			if (sensor < 0 || sensor > MAX_SENSORS) {
				s->sensor[j] = NO_SENSOR;
				s->pressure[j].mbar = 0;
				continue;
			}

			// Don't bother tracking sensors with no data
			if (!s->pressure[j].mbar) {
				s->sensor[j] = NO_SENSOR;
				continue;
			}

			// Remember the set of sensors we had
			sensor_mask |= 1ul << sensor;
		}
	}

	// Ignore the sensors we have cylinders for
	sensor_mask >>= dive->cylinders.nr;

	// Do we need to add empty cylinders?
	while (sensor_mask) {
		add_empty_cylinder(&dive->cylinders);
		sensor_mask >>= 1;
	}
}

static void fixup_dive_dc(struct dive *dive, struct divecomputer *dc)
{
	/* Fixup duration and mean depth */
	fixup_dc_duration(dc);

	/* Fix up sample depth data */
	fixup_dc_depths(dive, dc);

	/* Fix up first sample ndl data */
	fixup_dc_ndl(dc);

	/* Fix up dive temperatures based on dive computer samples */
	fixup_dc_temp(dive, dc);

	/* Fix up gas switch events */
	fixup_dc_gasswitch(dive, dc);

	/* Fix up cylinder ids in pressure sensors */
	fixup_dc_sample_sensors(dive, dc);

	/* Fix up cylinder pressures based on DC info */
	fixup_dive_pressures(dive, dc);

	fixup_dc_events(dc);

	/* Fixup CCR / PSCR dives with o2sensor values, but without no_o2sensors */
	fixup_no_o2sensors(dc);

	/* If there are no samples, generate a fake profile based on depth and time */
	if (!dc->samples)
		fake_dc(dc);
}

struct dive *fixup_dive(struct dive *dive)
{
	int i;
	struct divecomputer *dc;

	sanitize_cylinder_info(dive);
	dive->maxcns = dive->cns;

	/*
	 * Use the dive's temperatures for minimum and maximum in case
	 * we do not have temperatures recorded by DC.
	 */

	update_min_max_temperatures(dive, dive->watertemp);

	for_each_dc (dive, dc)
		fixup_dive_dc(dive, dc);

	fixup_water_salinity(dive);
	if (!dive->surface_pressure.mbar)
		fixup_surface_pressure(dive);
	fixup_meandepth(dive);
	fixup_duration(dive);
	fixup_watertemp(dive);
	fixup_airtemp(dive);
	for (i = 0; i < dive->cylinders.nr; i++) {
		cylinder_t *cyl = get_cylinder(dive, i);
		add_cylinder_description(&cyl->type);
		if (same_rounded_pressure(cyl->sample_start, cyl->start))
			cyl->start.mbar = 0;
		if (same_rounded_pressure(cyl->sample_end, cyl->end))
			cyl->end.mbar = 0;
	}
	update_cylinder_related_info(dive);
	for (i = 0; i < dive->weightsystems.nr; i++) {
		weightsystem_t *ws = &dive->weightsystems.weightsystems[i];
		add_weightsystem_description(ws);
	}
	/* we should always have a uniq ID as that gets assigned during alloc_dive(),
	 * but we want to make sure... */
	if (!dive->id)
		dive->id = dive_getUniqID();

	return dive;
}

/* Don't pick a zero for MERGE_MIN() */
#define MERGE_MAX(res, a, b, n) res->n = MAX(a->n, b->n)
#define MERGE_MIN(res, a, b, n) res->n = (a->n) ? (b->n) ? MIN(a->n, b->n) : (a->n) : (b->n)
#define MERGE_TXT(res, a, b, n, sep) res->n = merge_text(a->n, b->n, sep)
#define MERGE_NONZERO(res, a, b, n) res->n = a->n ? a->n : b->n

/*
 * This is like add_sample(), but if the distance from the last sample
 * is excessive, we add two surface samples in between.
 *
 * This is so that if you merge two non-overlapping dives, we make sure
 * that the time in between the dives is at the surface, not some "last
 * sample that happened to be at a depth of 1.2m".
 */
static void merge_one_sample(const struct sample *sample, int time, struct divecomputer *dc)
{
	int last = dc->samples - 1;
	if (last >= 0) {
		struct sample *prev = dc->sample + last;
		int last_time = prev->time.seconds;
		int last_depth = prev->depth.mm;

		/*
		 * Only do surface events if the samples are more than
		 * a minute apart, and shallower than 5m
		 */
		if (time > last_time + 60 && last_depth < 5000) {
			struct sample surface = { 0 };

			/* Init a few values from prev sample to avoid useless info in XML */
			surface.bearing.degrees = prev->bearing.degrees;
			surface.ndl.seconds = prev->ndl.seconds;

			add_sample(&surface, last_time + 20, dc);
			add_sample(&surface, time - 20, dc);
		}
	}
	add_sample(sample, time, dc);
}

static void renumber_last_sample(struct divecomputer *dc, const int mapping[]);
static void sample_renumber(struct sample *s, int i, const int mapping[]);

/*
 * Merge samples. Dive 'a' is "offset" seconds before Dive 'b'
 */
static void merge_samples(struct divecomputer *res,
			  const struct divecomputer *a, const struct divecomputer *b,
			  const int *cylinders_map_a, const int *cylinders_map_b,
			  int offset)
{
	int asamples = a->samples;
	int bsamples = b->samples;
	struct sample *as = a->sample;
	struct sample *bs = b->sample;

	/*
	 * We want a positive sample offset, so that sample
	 * times are always positive. So if the samples for
	 * 'b' are before the samples for 'a' (so the offset
	 * is negative), we switch a and b around, and use
	 * the reverse offset.
	 */
	if (offset < 0) {
		const int *cylinders_map_tmp;
		offset = -offset;
		asamples = bsamples;
		bsamples = a->samples;
		as = bs;
		bs = a->sample;
		cylinders_map_tmp = cylinders_map_a;
		cylinders_map_a = cylinders_map_b;
		cylinders_map_b = cylinders_map_tmp;
	}

	for (;;) {
		int j;
		int at, bt;
		struct sample sample = { .bearing.degrees = -1, .ndl.seconds = -1 };

		if (!res)
			return;

		at = asamples ? as->time.seconds : -1;
		bt = bsamples ? bs->time.seconds + offset : -1;

		/* No samples? All done! */
		if (at < 0 && bt < 0)
			return;

		/* Only samples from a? */
		if (bt < 0) {
		add_sample_a:
			merge_one_sample(as, at, res);
			renumber_last_sample(res, cylinders_map_a);
			as++;
			asamples--;
			continue;
		}

		/* Only samples from b? */
		if (at < 0) {
		add_sample_b:
			merge_one_sample(bs, bt, res);
			renumber_last_sample(res, cylinders_map_b);
			bs++;
			bsamples--;
			continue;
		}

		if (at < bt)
			goto add_sample_a;
		if (at > bt)
			goto add_sample_b;

		/* same-time sample: add a merged sample. Take the non-zero ones */
		sample = *bs;
		sample_renumber(&sample, 0, cylinders_map_b);
		if (as->depth.mm)
			sample.depth = as->depth;
		if (as->temperature.mkelvin)
			sample.temperature = as->temperature;
		for (j = 0; j < MAX_SENSORS; ++j) {
			int sensor_id;

			sensor_id = cylinders_map_a[as->sensor[j]];
			if (sensor_id < 0)
				continue;

			if (as->pressure[j].mbar)
				sample.pressure[j] = as->pressure[j];
			if (as->sensor[j])
				sample.sensor[j] = sensor_id;
		}
		if (as->cns)
			sample.cns = as->cns;
		if (as->setpoint.mbar)
			sample.setpoint = as->setpoint;
		if (as->ndl.seconds)
			sample.ndl = as->ndl;
		if (as->stoptime.seconds)
			sample.stoptime = as->stoptime;
		if (as->stopdepth.mm)
			sample.stopdepth = as->stopdepth;
		if (as->in_deco)
			sample.in_deco = true;

		merge_one_sample(&sample, at, res);

		as++;
		bs++;
		asamples--;
		bsamples--;
	}
}

/*
 * Does the extradata key/value pair already exist in the
 * supplied dive computer data?
 *
 * This is not hugely efficient (with the whole "do this for
 * every value you merge" it's O(n**2)) but it's not like we
 * have very many extra_data entries per dive computer anyway.
 */
static bool extra_data_exists(const struct extra_data *ed, const struct divecomputer *dc)
{
	const struct extra_data *p;

	for (p = dc->extra_data; p; p = p->next) {
		if (strcmp(p->key, ed->key))
			continue;
		if (strcmp(p->value, ed->value))
			continue;
		return true;
	}
	return false;
}

/*
 * Merge extra_data.
 *
 * The extra data from 'a' has already been copied into 'res'. So
 * we really should just copy over the data from 'b' too.
 */
static void merge_extra_data(struct divecomputer *res,
			  const struct divecomputer *a, const struct divecomputer *b)
{
	struct extra_data **ed, *src;

	// Find the place to add things in the result
	ed = &res->extra_data;
	while (*ed)
		ed = &(*ed)->next;

	for (src = b->extra_data; src; src = src->next) {
		if (extra_data_exists(src, a))
			continue;
		*ed = malloc(sizeof(struct extra_data));
		if (!*ed)
			break;
		copy_extra_data(src, *ed);
		ed = &(*ed)->next;
	}

	// Terminate the result list
	*ed = NULL;
}

static char *merge_text(const char *a, const char *b, const char *sep)
{
	char *res;
	if (!a && !b)
		return NULL;
	if (!a || !*a)
		return copy_string(b);
	if (!b || !*b)
		return strdup(a);
	if (!strcmp(a, b))
		return copy_string(a);
	res = malloc(strlen(a) + strlen(b) + 32);
	if (!res)
		return (char *)a;
	sprintf(res, "%s%s%s", a, sep, b);
	return res;
}

#define SORT(a, b)  \
	if (a != b) \
		return a < b ? -1 : 1
#define SORT_FIELD(a, b, field) SORT(a->field, b->field)

static int sort_event(const struct event *a, const struct event *b, int time_a, int time_b)
{
	SORT(time_a, time_b);
	SORT_FIELD(a, b, type);
	SORT_FIELD(a, b, flags);
	SORT_FIELD(a, b, value);
	return strcmp(a->name, b->name);
}

static int same_gas(const struct event *a, const struct event *b)
{
	if (a->type == b->type && a->flags == b->flags && a->value == b->value && !strcmp(a->name, b->name) &&
			same_gasmix(a->gas.mix, b->gas.mix)) {
		return true;
	}
	return false;
}

static void event_renumber(struct event *ev, const int mapping[]);
static void add_initial_gaschange(struct dive *dive, struct divecomputer *dc, int offset, int idx);

static void merge_events(struct dive *d, struct divecomputer *res,
			 const struct divecomputer *src1, const struct divecomputer *src2,
			 const int *cylinders_map1, const int *cylinders_map2,
			 int offset)
{
	const struct event *a, *b;
	struct event **p = &res->events;
	const struct event *last_gas = NULL;

	/* Always use positive offsets */
	if (offset < 0) {
		const struct divecomputer *tmp;
		const int *cylinders_map_tmp;

		offset = -offset;
		tmp = src1;
		src1 = src2;
		src2 = tmp;

		cylinders_map_tmp = cylinders_map1;
		cylinders_map1 = cylinders_map2;
		cylinders_map2 = cylinders_map_tmp;
	}

	a = src1->events;
	b = src2->events;

	while (a || b) {
		int s;
		const struct event *pick;
		const int *cylinders_map;
		int event_offset;

		if (!b)
			goto pick_a;

		if (!a)
			goto pick_b;

		s = sort_event(a, b, a->time.seconds, b->time.seconds + offset);

		/* Identical events? Just skip one of them (we skip a) */
		if (!s) {
			a = a->next;
			continue;
		}

		/* Otherwise, pick the one that sorts first */
		if (s < 0) {
pick_a:
			pick = a;
			a = a->next;
			event_offset = 0;
			cylinders_map = cylinders_map1;
		} else {
pick_b:
			pick = b;
			b = b->next;
			event_offset = offset;
			cylinders_map = cylinders_map2;
		}

		/*
		 * If that's a gas-change that matches the previous
		 * gas change, we'll just skip it
		 */
		if (event_is_gaschange(pick)) {
			if (last_gas && same_gas(pick, last_gas))
				continue;
			last_gas = pick;
		}

		/* Add it to the target list */
		*p = clone_event(pick);
		(*p)->time.seconds += event_offset;
		event_renumber(*p, cylinders_map);
		p = &(*p)->next;
	}

	/* If the initial cylinder of a divecomputer was remapped, add a gas change event to that cylinder */
	if (cylinders_map1[0] > 0)
		add_initial_gaschange(d, res, 0, cylinders_map1[0]);
	if (cylinders_map2[0] > 0)
		add_initial_gaschange(d, res, offset, cylinders_map2[0]);
}

/* get_cylinder_idx_by_use(): Find the index of the first cylinder with a particular CCR use type.
 * The index returned corresponds to that of the first cylinder with a cylinder_use that
 * equals the appropriate enum value [oxygen, diluent, bailout] given by cylinder_use_type.
 * A negative number returned indicates that a match could not be found.
 * Call parameters: dive = the dive being processed
 *                  cylinder_use_type = an enum, one of {oxygen, diluent, bailout} */
extern int get_cylinder_idx_by_use(const struct dive *dive, enum cylinderuse cylinder_use_type)
{
	int cylinder_index;
	for (cylinder_index = 0; cylinder_index < dive->cylinders.nr; cylinder_index++) {
		if (get_cylinder(dive, cylinder_index)->cylinder_use == cylinder_use_type)
			return cylinder_index; // return the index of the cylinder with that cylinder use type
	}
	return -1; // negative number means cylinder_use_type not found in list of cylinders
}

/* Force an initial gaschange event to the (old) gas #0 */
static void add_initial_gaschange(struct dive *dive, struct divecomputer *dc, int offset, int idx)
{
	/* if there is a gaschange event up to 30 sec after the initial event,
	 * refrain from adding the initial event */
	const struct event *ev = dc->events;
	while(ev && (ev = get_next_event(ev, "gaschange")) != NULL) {
		if (ev->time.seconds > offset + 30)
			break;
		else if (ev->time.seconds > offset)
			return;
		ev = ev->next;
	}

	/* Old starting gas mix */
	add_gas_switch_event(dive, dc, offset, idx);
}

static void sample_renumber(struct sample *s, int i, const int mapping[])
{
	int j;

	for (j = 0; j < MAX_SENSORS; j++) {
		int sensor = -1;

		if (s->sensor[j] != NO_SENSOR)
			sensor = mapping[s->sensor[j]];
		if (sensor == -1) {
			// Remove sensor and gas pressure info
			if (i == 0) {
				s->sensor[j] = 0;
				s->pressure[j].mbar = 0;
			} else {
				s->sensor[j] = s[-1].sensor[j];
				s->pressure[j].mbar = s[-1].pressure[j].mbar;
			}
		} else {
			s->sensor[j] = sensor;
		}
	}
}

static void renumber_last_sample(struct divecomputer *dc, const int mapping[])
{
	int idx;

	if (dc->samples <= 0)
		return;
	idx = dc->samples - 1;
	sample_renumber(dc->sample + idx, idx, mapping);
}

static void event_renumber(struct event *ev, const int mapping[])
{
	if (!event_is_gaschange(ev))
		return;
	if (ev->gas.index < 0)
		return;
	ev->gas.index = mapping[ev->gas.index];
}

static void dc_cylinder_renumber(struct dive *dive, struct divecomputer *dc, const int mapping[])
{
	int i;
	struct event *ev;

	/* Remap or delete the sensor indices */
	for (i = 0; i < dc->samples; i++)
		sample_renumber(dc->sample + i, i, mapping);

	/* Remap the gas change indices */
	for (ev = dc->events; ev; ev = ev->next)
		event_renumber(ev, mapping);

	/* If the initial cylinder of a dive was remapped, add a gas change event to that cylinder */
	if (mapping[0] > 0)
		add_initial_gaschange(dive, dc, 0, mapping[0]);
}

/*
 * If the cylinder indices change (due to merging dives or deleting
 * cylinders in the middle), we need to change the indices in the
 * dive computer data for this dive.
 *
 * Also note that we assume that the initial cylinder is cylinder 0,
 * so if that got renamed, we need to create a fake gas change event
 */
void cylinder_renumber(struct dive *dive, int mapping[])
{
	struct divecomputer *dc;
	for_each_dc (dive, dc)
		dc_cylinder_renumber(dive, dc, mapping);
}

int same_gasmix_cylinder(const cylinder_t *cyl, int cylid, const struct dive *dive, bool check_unused)
{
	struct gasmix mygas = cyl->gasmix;
	for (int i = 0; i < dive->cylinders.nr; i++) {
		if (i == cylid)
			continue;
		struct gasmix gas2 = get_cylinder(dive, i)->gasmix;
		if (gasmix_distance(mygas, gas2) == 0 && (is_cylinder_used(dive, i) || check_unused))
			return i;
	}
	return -1;
}

static int pdiff(pressure_t a, pressure_t b)
{
	return a.mbar && b.mbar && a.mbar != b.mbar;
}

static int different_manual_pressures(const cylinder_t *a, const cylinder_t *b)
{
	return pdiff(a->start, b->start) || pdiff(a->end, b->end);
}

/*
 * Can we find an exact match for a cylinder in another dive?
 * Take the "already matched" map into account, so that we
 * don't match multiple similar cylinders to one target.
 *
 * To match, the cylinders have to have the same gasmix and the
 * same cylinder use (ie OC/Diluent/Oxygen), and if pressures
 * have been added manually they need to match.
 */
static int match_cylinder(const cylinder_t *cyl, const struct dive *dive, const bool *try_match)
{
	int i;

	for (i = 0; i < dive->cylinders.nr; i++) {
		const cylinder_t *target;

		if (!try_match[i])
			continue;

		target = get_cylinder(dive, i);
		if (!same_gasmix(cyl->gasmix, target->gasmix))
			continue;
		if (cyl->cylinder_use != target->cylinder_use)
			continue;
		if (different_manual_pressures(cyl, target))
			continue;

		/* open question: Should we check sizes too? */
		return i;
	}
	return -1;
}

/*
 * Function used to merge manually set start or end pressures. This
 * is used to merge cylinders when merging dives. We store up to two
 * values for start _and_ end pressures: one derived from samples and
 * one entered manually, whereby the latter takes precedence. It may
 * happen that the user merges two dives where one has a manual,
 * the other only a sample-derived pressure. In such a case we want to
 * supplement the non-existing manual value by a sample derived one.
 * Otherwise, the merged dive would end up with incomplete pressure
 * information.
 * The last argument to the function specifies whether the larger
 * or smaller value of the two dives should be returned. Obviously,
 * for the starting pressure we want the larger and for the ending
 * pressure the smaller value.
 */
static pressure_t merge_pressures(pressure_t a, pressure_t sample_a, pressure_t b, pressure_t sample_b, bool take_min)
{
	if (!a.mbar && !b.mbar)
		return a;
	if (!a.mbar)
		a = sample_a;
	if (!b.mbar)
		b = sample_b;
	if (!a.mbar)
		a = b;
	if (!b.mbar)
		b = a;
	if (take_min)
		return a.mbar < b.mbar? a : b;
	return a.mbar > b.mbar? a : b;
}

/*
 * We matched things up so that they have the same gasmix and
 * use, but we might want to fill in any missing cylinder details
 * in 'a' if we had it from 'b'.
 */
static void merge_one_cylinder(cylinder_t *a, const cylinder_t *b)
{
	if (!a->type.size.mliter)
		a->type.size.mliter = b->type.size.mliter;
	if (!a->type.workingpressure.mbar)
		a->type.workingpressure.mbar = b->type.workingpressure.mbar;
	if (empty_string(a->type.description))
		a->type.description = copy_string(b->type.description);

	/* If either cylinder has manually entered pressures, try to merge them.
	 * Use pressures from divecomputer samples if only one cylinder has such a value.
	 * Yes, this is an actual use case we encountered.
	 * Note that we don't merge the sample-derived pressure values, as this is
	 * perfomed after merging in fixup_dive() */
	a->start = merge_pressures(a->start, a->sample_start, b->start, b->sample_start, false);
	a->end = merge_pressures(a->end, a->sample_end, b->end, b->sample_end, true);

	/* Really? */
	a->gas_used.mliter += b->gas_used.mliter;
	a->deco_gas_used.mliter += b->deco_gas_used.mliter;
	a->bestmix_o2 = a->bestmix_o2 && b->bestmix_o2;
	a->bestmix_he = a->bestmix_he && b->bestmix_he;
}

static bool cylinder_has_data(const cylinder_t *cyl)
{
	return !cyl->type.size.mliter &&
	       !cyl->type.workingpressure.mbar &&
	       !cyl->type.description &&
	       !cyl->gasmix.o2.permille &&
	       !cyl->gasmix.he.permille &&
	       !cyl->start.mbar &&
	       !cyl->end.mbar &&
	       !cyl->sample_start.mbar &&
	       !cyl->sample_end.mbar &&
	       !cyl->gas_used.mliter &&
	       !cyl->deco_gas_used.mliter;
}

static bool cylinder_in_use(const struct dive *dive, int idx)
{
	if (idx < 0 || idx >= dive->cylinders.nr)
		return false;

	/* This tests for gaschange events or pressure changes */
	if (is_cylinder_used(dive, idx) || prefs.include_unused_tanks)
		return true;

	/* This tests for typenames or gas contents */
	return cylinder_has_data(get_cylinder(dive, idx));
}

/*
 * Merging cylinder information is non-trivial, because the two dive computers
 * may have different ideas of what the different cylinder indexing is.
 *
 * Logic: take all the cylinder information from the preferred dive ('a'), and
 * then try to match each of the cylinders in the other dive by the gasmix that
 * is the best match and hasn't been used yet.
 *
 * For each dive, a cylinder-renumbering table is returned.
 */
static void merge_cylinders(struct dive *res, const struct dive *a, const struct dive *b,
			    int mapping_a[], int mapping_b[])
{
	int i;
	int max_cylinders = a->cylinders.nr + b->cylinders.nr;
	bool *used_in_a = malloc(max_cylinders * sizeof(bool));
	bool *used_in_b = malloc(max_cylinders * sizeof(bool));
	bool *try_to_match = malloc(max_cylinders * sizeof(bool));

	/* First, clear all cylinders in destination */
	clear_cylinder_table(&res->cylinders);

	/* Clear all cylinder mappings */
	for (i = 0; i < a->cylinders.nr; i++)
		mapping_a[i] = -1;
	for (i = 0; i < b->cylinders.nr; i++)
		mapping_b[i] = -1;

	/* Calculate usage map of cylinders, clear matching map */
	for (i = 0; i < max_cylinders; i++) {
		used_in_a[i] = cylinder_in_use(a, i);
		used_in_b[i] = cylinder_in_use(b, i);
		try_to_match[i] = false;
	}

	/*
	 * For each cylinder in 'a' that is used, copy it to 'res'.
	 * These are also potential matches for 'b' to use.
	 */
	for (i = 0; i < max_cylinders; i++) {
		int res_nr = res->cylinders.nr;
		if (!used_in_a[i])
			continue;
		mapping_a[i] = res_nr;
		try_to_match[res_nr] = true;
		add_cloned_cylinder(&res->cylinders, *get_cylinder(a, i));
	}

	/*
	 * For each cylinder in 'b' that is used, try to match it
	 * with an existing cylinder in 'res' from 'a'
	 */
	for (i = 0; i < b->cylinders.nr; i++) {
		int j;

		if (!used_in_b[i])
			continue;

		j = match_cylinder(get_cylinder(b, i), res, try_to_match);

		/* No match? Add it to the result */
		if (j < 0) {
			int res_nr = res->cylinders.nr;
			mapping_b[i] = res_nr;
			add_cloned_cylinder(&res->cylinders, *get_cylinder(b, i));
			continue;
		}

		/* Otherwise, merge the result to the one we found */
		mapping_b[i] = j;
		merge_one_cylinder(get_cylinder(res,j), get_cylinder(b, i));

		/* Don't match the same target more than once */
		try_to_match[j] = false;
	}

	free(used_in_a);
	free(used_in_b);
	free(try_to_match);
}

/* Check whether a weightsystem table contains a given weightsystem */
static bool has_weightsystem(const struct weightsystem_table *t, const weightsystem_t w)
{
	int i;
	for (i = 0; i < t->nr; i++) {
		if (same_weightsystem(w, t->weightsystems[i]))
			return true;
	}
	return false;
}

static void merge_equipment(struct dive *res, const struct dive *a, const struct dive *b)
{
	int i;
	for (i = 0; i < a->weightsystems.nr; i++) {
		if (!has_weightsystem(&res->weightsystems, a->weightsystems.weightsystems[i]))
			add_cloned_weightsystem(&res->weightsystems, a->weightsystems.weightsystems[i]);
	}
	for (i = 0; i < b->weightsystems.nr; i++) {
		if (!has_weightsystem(&res->weightsystems, b->weightsystems.weightsystems[i]))
			add_cloned_weightsystem(&res->weightsystems, b->weightsystems.weightsystems[i]);
	}
}

static void merge_temperatures(struct dive *res, const struct dive *a, const struct dive *b)
{
	temperature_t airtemp_a = un_fixup_airtemp(a);
	temperature_t airtemp_b = un_fixup_airtemp(b);
	res->airtemp = airtemp_a.mkelvin ? airtemp_a : airtemp_b;
	MERGE_NONZERO(res, a, b, watertemp.mkelvin);
}

/*
 * Pick a trip for a dive
 */
static struct dive_trip *get_preferred_trip(const struct dive *a, const struct dive *b)
{
	dive_trip_t *atrip, *btrip;

	/* If only one dive has a trip, choose that */
	atrip = a->divetrip;
	btrip = b->divetrip;
	if (!atrip)
		return btrip;
	if (!btrip)
		return atrip;

	/* Both dives have a trip - prefer the non-autogenerated one */
	if (atrip->autogen && !btrip->autogen)
		return btrip;
	if (!atrip->autogen && btrip->autogen)
		return atrip;

	/* Otherwise, look at the trip data and pick the "better" one */
	if (!atrip->location)
		return btrip;
	if (!btrip->location)
		return atrip;
	if (!atrip->notes)
		return btrip;
	if (!btrip->notes)
		return atrip;

	/*
	 * Ok, so both have location and notes.
	 * Pick the earlier one.
	 */
	if (a->when < b->when)
		return atrip;
	return btrip;
}

#if CURRENTLY_NOT_USED
/*
 * Sample 's' is between samples 'a' and 'b'. It is 'offset' seconds before 'b'.
 *
 * If 's' and 'a' are at the same time, offset is 0, and b is NULL.
 */
static int compare_sample(struct sample *s, struct sample *a, struct sample *b, int offset)
{
	unsigned int depth = a->depth.mm;
	int diff;

	if (offset) {
		unsigned int interval = b->time.seconds - a->time.seconds;
		unsigned int depth_a = a->depth.mm;
		unsigned int depth_b = b->depth.mm;

		if (offset > interval)
			return -1;

		/* pick the average depth, scaled by the offset from 'b' */
		depth = (depth_a * offset) + (depth_b * (interval - offset));
		depth /= interval;
	}
	diff = s->depth.mm - depth;
	if (diff < 0)
		diff = -diff;
	/* cut off at one meter difference */
	if (diff > 1000)
		diff = 1000;
	return diff * diff;
}

/*
 * Calculate a "difference" in samples between the two dives, given
 * the offset in seconds between them. Use this to find the best
 * match of samples between two different dive computers.
 */
static unsigned long sample_difference(struct divecomputer *a, struct divecomputer *b, int offset)
{
	int asamples = a->samples;
	int bsamples = b->samples;
	struct sample *as = a->sample;
	struct sample *bs = b->sample;
	unsigned long error = 0;
	int start = -1;

	if (!asamples || !bsamples)
		return 0;

	/*
	 * skip the first sample - this way we know can always look at
	 * as/bs[-1] to look at the samples around it in the loop.
	 */
	as++;
	bs++;
	asamples--;
	bsamples--;

	for (;;) {
		int at, bt, diff;


		/* If we run out of samples, punt */
		if (!asamples)
			return INT_MAX;
		if (!bsamples)
			return INT_MAX;

		at = as->time.seconds;
		bt = bs->time.seconds + offset;

		/* b hasn't started yet? Ignore it */
		if (bt < 0) {
			bs++;
			bsamples--;
			continue;
		}

		if (at < bt) {
			diff = compare_sample(as, bs - 1, bs, bt - at);
			as++;
			asamples--;
		} else if (at > bt) {
			diff = compare_sample(bs, as - 1, as, at - bt);
			bs++;
			bsamples--;
		} else {
			diff = compare_sample(as, bs, NULL, 0);
			as++;
			bs++;
			asamples--;
			bsamples--;
		}

		/* Invalid comparison point? */
		if (diff < 0)
			continue;

		if (start < 0)
			start = at;

		error += diff;

		if (at - start > 120)
			break;
	}
	return error;
}

/*
 * Dive 'a' is 'offset' seconds before dive 'b'
 *
 * This is *not* because the dive computers clocks aren't in sync,
 * it is because the dive computers may "start" the dive at different
 * points in the dive, so the sample at time X in dive 'a' is the
 * same as the sample at time X+offset in dive 'b'.
 *
 * For example, some dive computers take longer to "wake up" when
 * they sense that you are under water (ie Uemis Zurich if it was off
 * when the dive started). And other dive computers have different
 * depths that they activate at, etc etc.
 *
 * If we cannot find a shared offset, don't try to merge.
 */
static int find_sample_offset(struct divecomputer *a, struct divecomputer *b)
{
	int offset, best;
	unsigned long max;

	/* No samples? Merge at any time (0 offset) */
	if (!a->samples)
		return 0;
	if (!b->samples)
		return 0;

	/*
	 * Common special-case: merging a dive that came from
	 * the same dive computer, so the samples are identical.
	 * Check this first, without wasting time trying to find
	 * some minimal offset case.
	 */
	best = 0;
	max = sample_difference(a, b, 0);
	if (!max)
		return 0;

	/*
	 * Otherwise, look if we can find anything better within
	 * a thirty second window..
	 */
	for (offset = -30; offset <= 30; offset++) {
		unsigned long diff;

		diff = sample_difference(a, b, offset);
		if (diff > max)
			continue;
		best = offset;
		max = diff;
	}

	return best;
}
#endif

/*
 * Are a and b "similar" values, when given a reasonable lower end expected
 * difference?
 *
 * So for example, we'd expect different dive computers to give different
 * max. depth readings. You might have them on different arms, and they
 * have different pressure sensors and possibly different ideas about
 * water salinity etc.
 *
 * So have an expected minimum difference, but also allow a larger relative
 * error value.
 */
static int similar(unsigned long a, unsigned long b, unsigned long expected)
{
	if (!a && !b)
		return 1;

	if (a && b) {
		unsigned long min, max, diff;

		min = a;
		max = b;
		if (a > b) {
			min = b;
			max = a;
		}
		diff = max - min;

		/* Smaller than expected difference? */
		if (diff < expected)
			return 1;
		/* Error less than 10% or the maximum */
		if (diff * 10 < max)
			return 1;
	}
	return 0;
}

/*
 * Match every dive computer against each other to see if
 * we have a matching dive.
 *
 * Return values:
 *  -1 for "is definitely *NOT* the same dive"
 *   0 for "don't know"
 *   1 for "is definitely the same dive"
 */
static int match_dc_dive(const struct divecomputer *a, const struct divecomputer *b)
{
	do {
		const struct divecomputer *tmp = b;
		do {
			int match = match_one_dc(a, tmp);
			if (match)
				return match;
			tmp = tmp->next;
		} while (tmp);
		a = a->next;
	} while (a);
	return 0;
}

/*
 * Do we want to automatically try to merge two dives that
 * look like they are the same dive?
 *
 * This happens quite commonly because you download a dive
 * that you already had, or perhaps because you maintained
 * multiple dive logs and want to load them all together
 * (possibly one of them was imported from another dive log
 * application entirely).
 *
 * NOTE! We mainly look at the dive time, but it can differ
 * between two dives due to a few issues:
 *
 *  - rounding the dive date to the nearest minute in other dive
 *    applications
 *
 *  - dive computers with "relative datestamps" (ie the dive
 *    computer doesn't actually record an absolute date at all,
 *    but instead at download-time synchronizes its internal
 *    time with real-time on the downloading computer)
 *
 *  - using multiple dive computers with different real time on
 *    the same dive
 *
 * We do not merge dives that look radically different, and if
 * the dates are *too* far off the user will have to join two
 * dives together manually. But this tries to handle the sane
 * cases.
 */
static int likely_same_dive(const struct dive *a, const struct dive *b)
{
	int match, fuzz = 20 * 60;

	/* don't merge manually added dives with anything */
	if (is_manually_added_dc(&a->dc) ||
	    is_manually_added_dc(&b->dc))
		return 0;

	/*
	 * Do some basic sanity testing of the values we
	 * have filled in during 'fixup_dive()'
	 */
	if (!similar(a->maxdepth.mm, b->maxdepth.mm, 1000) ||
	    (a->meandepth.mm && b->meandepth.mm && !similar(a->meandepth.mm, b->meandepth.mm, 1000)) ||
	    !a->duration.seconds || !b->duration.seconds ||
	    !similar(a->duration.seconds, b->duration.seconds, 5 * 60))
		return 0;

	/* See if we can get an exact match on the dive computer */
	match = match_dc_dive(&a->dc, &b->dc);
	if (match)
		return match > 0;

	/*
	 * Allow a time difference due to dive computer time
	 * setting etc. Check if they overlap.
	 */
	fuzz = MAX(a->duration.seconds, b->duration.seconds) / 2;
	if (fuzz < 60)
		fuzz = 60;

	return (a->when <= b->when + fuzz) && (a->when >= b->when - fuzz);
}

/*
 * This could do a lot more merging. Right now it really only
 * merges almost exact duplicates - something that happens easily
 * with overlapping dive downloads.
 *
 * If new dives are merged into the dive table, dive a is supposed to
 * be the old dive and dive b is supposed to be the newly imported
 * dive. If the flag "prefer_downloaded" is set, data of the latter
 * will take priority over the former.
 *
 * Attn: The dive_site parameter of the dive will be set, but the caller
 * still has to register the dive in the dive site!
 */
struct dive *try_to_merge(struct dive *a, struct dive *b, bool prefer_downloaded)
{
	struct dive *res;
	struct dive_site *site;

	if (!likely_same_dive(a, b))
		return NULL;

	res = merge_dives(a, b, 0, prefer_downloaded, NULL, &site);
	res->dive_site = site; /* Caller has to call add_dive_to_dive_site()! */
	return res;
}

static int same_sample(struct sample *a, struct sample *b)
{
	if (a->time.seconds != b->time.seconds)
		return 0;
	if (a->depth.mm != b->depth.mm)
		return 0;
	if (a->temperature.mkelvin != b->temperature.mkelvin)
		return 0;
	if (a->pressure[0].mbar != b->pressure[0].mbar)
		return 0;
	return a->sensor[0] == b->sensor[0];
}

static int same_dc(struct divecomputer *a, struct divecomputer *b)
{
	int i;
	const struct event *eva, *evb;

	i = match_one_dc(a, b);
	if (i)
		return i > 0;

	if (a->when && b->when && a->when != b->when)
		return 0;
	if (a->samples != b->samples)
		return 0;
	for (i = 0; i < a->samples; i++)
		if (!same_sample(a->sample + i, b->sample + i))
			return 0;
	eva = a->events;
	evb = b->events;
	while (eva && evb) {
		if (!same_event(eva, evb))
			return 0;
		eva = eva->next;
		evb = evb->next;
	}
	return eva == evb;
}

static int might_be_same_device(const struct divecomputer *a, const struct divecomputer *b)
{
	/* No dive computer model? That matches anything */
	if (!a->model || !b->model)
		return 1;

	/* Otherwise at least the model names have to match */
	if (strcasecmp(a->model, b->model))
		return 0;

	/* No device ID? Match */
	if (!a->deviceid || !b->deviceid)
		return 1;

	return a->deviceid == b->deviceid;
}

static void remove_redundant_dc(struct divecomputer *dc, int prefer_downloaded)
{
	do {
		struct divecomputer **p = &dc->next;

		/* Check this dc against all the following ones.. */
		while (*p) {
			struct divecomputer *check = *p;
			if (same_dc(dc, check) || (prefer_downloaded && might_be_same_device(dc, check))) {
				*p = check->next;
				check->next = NULL;
				free_dc(check);
				continue;
			}
			p = &check->next;
		}

		/* .. and then continue down the chain, but we */
		prefer_downloaded = 0;
		dc = dc->next;
	} while (dc);
}

static const struct divecomputer *find_matching_computer(const struct divecomputer *match, const struct divecomputer *list)
{
	const struct divecomputer *p;

	while ((p = list) != NULL) {
		list = list->next;

		if (might_be_same_device(match, p))
			break;
	}
	return p;
}

static void copy_dive_computer(struct divecomputer *res, const struct divecomputer *a)
{
	*res = *a;
	res->model = copy_string(a->model);
	res->serial = copy_string(a->serial);
	res->fw_version = copy_string(a->fw_version);
	STRUCTURED_LIST_COPY(struct extra_data, a->extra_data, res->extra_data, copy_extra_data);
	res->samples = res->alloc_samples = 0;
	res->sample = NULL;
	res->events = NULL;
	res->next = NULL;
}

/*
 * Join dive computers with a specific time offset between
 * them.
 *
 * Use the dive computer ID's (or names, if ID's are missing)
 * to match them up. If we find a matching dive computer, we
 * merge them. If not, we just take the data from 'a'.
 */
static void interleave_dive_computers(struct dive *d, struct divecomputer *res,
				      const struct divecomputer *a, const struct divecomputer *b,
				      const int cylinders_map_a[], const int cylinders_map_b[],
				      int offset)
{
	do {
		const struct divecomputer *match;

		copy_dive_computer(res, a);

		match = find_matching_computer(a, b);
		if (match) {
			merge_events(d, res, a, match, cylinders_map_a, cylinders_map_b, offset);
			merge_samples(res, a, match, cylinders_map_a, cylinders_map_b, offset);
			merge_extra_data(res, a, match);
			/* Use the diveid of the later dive! */
			if (offset > 0)
				res->diveid = match->diveid;
		} else {
			copy_dc_renumber(d, a, res, cylinders_map_a);
		}
		a = a->next;
		if (!a)
			break;
		res->next = calloc(1, sizeof(struct divecomputer));
		res = res->next;
	} while (res);
}


/*
 * Join dive computer information.
 *
 * If we have old-style dive computer information (no model
 * name etc), we will prefer a new-style one and just throw
 * away the old. We're assuming it's a re-download.
 *
 * Otherwise, we'll just try to keep all the information,
 * unless the user has specified that they prefer the
 * downloaded computer, in which case we'll aggressively
 * try to throw out old information that *might* be from
 * that one.
 */
static void join_dive_computers(struct dive *d, struct divecomputer *res,
				const struct divecomputer *a, const struct divecomputer *b,
				const int cylinders_map_a[], const int cylinders_map_b[],
				int prefer_downloaded)
{
	struct divecomputer *tmp;

	if (a->model && !b->model) {
		copy_dc_renumber(d, a, res, cylinders_map_a);
		return;
	}
	if (b->model && !a->model) {
		copy_dc_renumber(d, b, res, cylinders_map_b);
		return;
	}

	copy_dc_renumber(d, a, res, cylinders_map_a);
	tmp = res;
	while (tmp->next)
		tmp = tmp->next;

	tmp->next = calloc(1, sizeof(*tmp));
	copy_dc_renumber(d, b, tmp->next, cylinders_map_b);

	remove_redundant_dc(res, prefer_downloaded);
}

// Does this dive have a dive computer for which is_dc_planner has value planned
bool has_planned(const struct dive *dive, bool planned)
{
	const struct divecomputer *dc = &dive->dc;

	while (dc) {
		if (is_dc_planner(&dive->dc) == planned)
			return true;
		dc = dc->next;
	}
	return false;
}

/*
 * Merging two dives can be subtle, because there's two different ways
 * of merging:
 *
 * (a) two distinctly _different_ dives that have the same dive computer
 *     are merged into one longer dive, because the user asked for it
 *     in the divelist.
 *
 *     Because this case is with the same dive computer, we *know* the
 *     two must have a different start time, and "offset" is the relative
 *     time difference between the two.
 *
 * (b) two different dive computers that we might want to merge into
 *     one single dive with multiple dive computers.
 *
 *     This is the "try_to_merge()" case, which will have offset == 0,
 *     even if the dive times might be different.
 *
 * If new dives are merged into the dive table, dive a is supposed to
 * be the old dive and dive b is supposed to be the newly imported
 * dive. If the flag "prefer_downloaded" is set, data of the latter
 * will take priority over the former.
 *
 * The trip the new dive should be associated with (if any) is returned
 * in the "trip" output parameter.
 *
 * The dive site the new dive should be added to (if any) is returned
 * in the "dive_site" output parameter.
 */
struct dive *merge_dives(const struct dive *a, const struct dive *b, int offset, bool prefer_downloaded, struct dive_trip **trip, struct dive_site **site)
{
	struct dive *res = alloc_dive();
	int *cylinders_map_a, *cylinders_map_b;

	if (offset) {
		/*
		 * If "likely_same_dive()" returns true, that means that
		 * it is *not* the same dive computer, and we do not want
		 * to try to turn it into a single longer dive. So we'd
		 * join them as two separate dive computers at zero offset.
		 */
		if (likely_same_dive(a, b))
			offset = 0;
	}

	if (is_dc_planner(&a->dc)) {
		const struct dive *tmp = a;
		a = b;
		b = tmp;
	}
	res->when = prefer_downloaded ? b->when : a->when;
	res->selected = a->selected || b->selected;
	if (trip)
		*trip = get_preferred_trip(a, b);
	MERGE_TXT(res, a, b, notes, "\n--\n");
	MERGE_TXT(res, a, b, buddy, ", ");
	MERGE_TXT(res, a, b, diveguide, ", ");
	MERGE_MAX(res, a, b, rating);
	MERGE_TXT(res, a, b, suit, ", ");
	MERGE_MAX(res, a, b, number);
	MERGE_NONZERO(res, a, b, visibility);
	MERGE_NONZERO(res, a, b, wavesize);
	MERGE_NONZERO(res, a, b, current);
	MERGE_NONZERO(res, a, b, surge);
	MERGE_NONZERO(res, a, b, chill);
	copy_pictures(a->pictures.nr ? &a->pictures : &b->pictures, &res->pictures);
	taglist_merge(&res->tag_list, a->tag_list, b->tag_list);
	/* if we get dives without any gas / cylinder information in an import, make sure
	 * that there is at leatst one entry in the cylinder map for that dive */
	cylinders_map_a = malloc(MAX(1, a->cylinders.nr) * sizeof(*cylinders_map_a));
	cylinders_map_b = malloc(MAX(1, b->cylinders.nr) * sizeof(*cylinders_map_b));
	merge_cylinders(res, a, b, cylinders_map_a, cylinders_map_b);
	merge_equipment(res, a, b);
	merge_temperatures(res, a, b);
	if (prefer_downloaded) {
		/* If we prefer downloaded, do those first, and get rid of "might be same" computers */
		join_dive_computers(res, &res->dc, &b->dc, &a->dc, cylinders_map_b, cylinders_map_a, 1);
	} else if (offset && might_be_same_device(&a->dc, &b->dc))
		interleave_dive_computers(res, &res->dc, &a->dc, &b->dc, cylinders_map_a, cylinders_map_b, offset);
	else
		join_dive_computers(res, &res->dc, &a->dc, &b->dc, cylinders_map_a, cylinders_map_b, 0);

	/* The CNS values will be recalculated from the sample in fixup_dive() */
	res->cns = res->maxcns = 0;

	/* we take the first dive site, unless it's empty */
	*site = a->dive_site && !dive_site_is_empty(a->dive_site) ? a->dive_site : b->dive_site;
	if (!dive_site_has_gps_location(*site) && dive_site_has_gps_location(b->dive_site)) {
		/* we picked the first dive site and that didn't have GPS data, but the new dive has
		 * GPS data (that could be a download from a GPS enabled dive computer).
		 * Keep the dive site, but add the GPS data */
		(*site)->location = b->dive_site->location;
	}
	fixup_dive(res);
	free(cylinders_map_a);
	free(cylinders_map_b);
	return res;
}

// copy_dive(), but retaining the new ID for the copied dive
static struct dive *create_new_copy(const struct dive *from)
{
	struct dive *to = alloc_dive();
	int id;

	// alloc_dive() gave us a new ID, we just need to
	// make sure it's not overwritten.
	id = to->id;
	copy_dive(from, to);
	to->id = id;
	return to;
}

struct start_end_pressure {
	pressure_t start;
	pressure_t end;
};

static void force_fixup_dive(struct dive *d)
{
	struct divecomputer *dc = &d->dc;
	int old_temp = dc->watertemp.mkelvin;
	int old_mintemp = d->mintemp.mkelvin;
	int old_maxtemp = d->maxtemp.mkelvin;
	duration_t old_duration = d->duration;
	struct start_end_pressure *old_pressures = malloc(d->cylinders.nr * sizeof(*old_pressures));

	d->maxdepth.mm = 0;
	dc->maxdepth.mm = 0;
	d->watertemp.mkelvin = 0;
	dc->watertemp.mkelvin = 0;
	d->duration.seconds = 0;
	d->maxtemp.mkelvin = 0;
	d->mintemp.mkelvin = 0;
	for (int i = 0; i < d->cylinders.nr; i++) {
		cylinder_t *cyl = get_cylinder(d, i);
		old_pressures[i].start = cyl->start;
		old_pressures[i].end = cyl->end;
		cyl->start.mbar = 0;
		cyl->end.mbar = 0;
	}

	fixup_dive(d);

	if (!d->watertemp.mkelvin)
		d->watertemp.mkelvin = old_temp;

	if (!dc->watertemp.mkelvin)
		dc->watertemp.mkelvin = old_temp;

	if (!d->maxtemp.mkelvin)
		d->maxtemp.mkelvin = old_maxtemp;

	if (!d->mintemp.mkelvin)
		d->mintemp.mkelvin = old_mintemp;

	if (!d->duration.seconds)
		d->duration = old_duration;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (!get_cylinder(d, i)->start.mbar)
			get_cylinder(d, i)->start = old_pressures[i].start;
		if (!get_cylinder(d, i)->end.mbar)
			get_cylinder(d, i)->end = old_pressures[i].end;
	}
	free(old_pressures);
}

/*
 * Split a dive that has a surface interval from samples 'a' to 'b'
 * into two dives, but don't add them to the log yet.
 * Returns the nr of the old dive or <0 on failure.
 * Moreover, on failure both output dives are set to NULL.
 * On success, the newly allocated dives are returned in out1 and out2.
 */
static int split_dive_at(const struct dive *dive, int a, int b, struct dive **out1, struct dive **out2)
{
	int i, nr;
	uint32_t t;
	struct dive *d1, *d2;
	struct divecomputer *dc1, *dc2;
	struct event *event, **evp;

	/* if we can't find the dive in the dive list, don't bother */
	if ((nr = get_divenr(dive)) < 0)
		return -1;

	/* Splitting should leave at least 3 samples per dive */
	if (a < 3 || b > dive->dc.samples - 4)
		return -1;

	/* We're not trying to be efficient here.. */
	d1 = create_new_copy(dive);
	d2 = create_new_copy(dive);
	d1->divetrip = d2->divetrip = 0;

	/* now unselect the first first segment so we don't keep all
	 * dives selected by mistake. But do keep the second one selected
	 * so the algorithm keeps splitting the dive further */
	d1->selected = false;

	dc1 = &d1->dc;
	dc2 = &d2->dc;
	/*
	 * Cut off the samples of d1 at the beginning
	 * of the interval.
	 */
	dc1->samples = a;

	/* And get rid of the 'b' first samples of d2 */
	dc2->samples -= b;
	memmove(dc2->sample, dc2->sample+b, dc2->samples * sizeof(struct sample));

	/* Now the secondary dive computers */
	t = dc2->sample[0].time.seconds;
	while ((dc1 = dc1->next))	{
		i = 0;
		while (dc1->samples < i && dc1->sample[i].time.seconds <= t)
			++i;
		dc1->samples = i;
	}
	while ((dc2 = dc2->next)) {
		i = 0;
		while (dc2->samples < i && dc2->sample[i].time.seconds < t)
			++i;
		dc2->samples -= i;
		memmove(dc2->sample, dc2->sample + i, dc2->samples * sizeof(struct sample));
	}
	dc1 = &d1->dc;
	dc2 = &d2->dc;
	/*
	 * This is where we cut off events from d1,
	 * and shift everything in d2
	 */
	d2->when += t;
	while (dc1 && dc2) {
		dc2->when += t;
		for (i = 0; i < dc2->samples; i++)
			dc2->sample[i].time.seconds -= t;

		/* Remove the events past 't' from d1 */
		evp = &dc1->events;
		while ((event = *evp) != NULL && event->time.seconds < t)
			evp = &event->next;
		*evp = NULL;
		while (event) {
			struct event *next = event->next;
			free(event);
			event = next;
		}

		/* Remove the events before 't' from d2, and shift the rest */
		evp = &dc2->events;
		while ((event = *evp) != NULL) {
			if (event->time.seconds < t) {
				*evp = event->next;
				free(event);
			} else {
				event->time.seconds -= t;
			}
		}
		dc1 = dc1->next;
		dc2 = dc2->next;
	}

	force_fixup_dive(d1);
	force_fixup_dive(d2);

	/*
	 * Was the dive numbered? If it was the last dive, then we'll
	 * increment the dive number for the tail part that we split off.
	 * Otherwise the tail is unnumbered.
	 */
	if (d2->number) {
		if (divelog.dives->nr == nr + 1)
			d2->number++;
		else
			d2->number = 0;
	}

	*out1 = d1;
	*out2 = d2;
	return nr;
}

/* in freedive mode we split for as little as 10 seconds on the surface,
 * otherwise we use a minute */
static bool should_split(const struct divecomputer *dc, int t1, int t2)
{
	int threshold = dc->divemode == FREEDIVE ? 10 : 60;

	return t2 - t1 >= threshold;
}

/*
 * Try to split a dive into multiple dives at a surface interval point.
 *
 * NOTE! We will split when there is at least one surface event that has
 * non-surface events on both sides.
 *
 * The surface interval points are determined using the first dive computer.
 *
 * In other words, this is a (simplified) reversal of the dive merging.
 */
int split_dive(const struct dive *dive, struct dive **new1, struct dive **new2)
{
	int i;
	int at_surface, surface_start;
	const struct divecomputer *dc;

	*new1 = *new2 = NULL;
	if (!dive)
		return -1;

	dc = &dive->dc;
	surface_start = 0;
	at_surface = 1;
	for (i = 1; i < dc->samples; i++) {
		struct sample *sample = dc->sample+i;
		int surface_sample = sample->depth.mm < SURFACE_THRESHOLD;

		/*
		 * We care about the transition from and to depth 0,
		 * not about the depth staying similar.
		 */
		if (at_surface == surface_sample)
			continue;
		at_surface = surface_sample;

		// Did it become surface after having been non-surface? We found the start
		if (at_surface) {
			surface_start = i;
			continue;
		}

		// Going down again? We want at least a minute from
		// the surface start.
		if (!surface_start)
			continue;
		if (!should_split(dc, dc->sample[surface_start].time.seconds, sample[-1].time.seconds))
			continue;

		return split_dive_at(dive, surface_start, i-1, new1, new2);
	}
	return -1;
}

int split_dive_at_time(const struct dive *dive, duration_t time, struct dive **new1, struct dive **new2)
{
	int i = 0;

	if (!dive)
		return -1;

	struct sample *sample = dive->dc.sample;
	*new1 = *new2 = NULL;
	while(sample->time.seconds < time.seconds) {
		++sample;
		++i;
		if (dive->dc.samples == i)
			return -1;
	}
	return split_dive_at(dive, i, i - 1, new1, new2);
}

/*
 * "dc_maxtime()" is how much total time this dive computer
 * has for this dive. Note that it can differ from "duration"
 * if there are surface events in the middle.
 *
 * Still, we do ignore all but the last surface samples from the
 * end, because some divecomputers just generate lots of them.
 */
static inline int dc_totaltime(const struct divecomputer *dc)
{
	int time = dc->duration.seconds;
	int nr = dc->samples;

	while (nr--) {
		struct sample *s = dc->sample + nr;
		time = s->time.seconds;
		if (s->depth.mm >= SURFACE_THRESHOLD)
			break;
	}
	return time;
}

/*
 * The end of a dive is actually not trivial, because "duration"
 * is not the duration until the end, but the time we spend under
 * water, which can be very different if there are surface events
 * during the dive.
 *
 * So walk the dive computers, looking for the longest actual
 * time in the samples (and just default to the dive duration if
 * there are no samples).
 */
static inline int dive_totaltime(const struct dive *dive)
{
	int time =  dive->duration.seconds;
	const struct divecomputer *dc;

	for_each_relevant_dc(dive, dc) {
		int dc_time = dc_totaltime(dc);
		if (dc_time > time)
			time = dc_time;
	}
	return time;
}

timestamp_t dive_endtime(const struct dive *dive)
{
	return dive->when + dive_totaltime(dive);
}

bool time_during_dive_with_offset(const struct dive *dive, timestamp_t when, timestamp_t offset)
{
	timestamp_t start = dive->when;
	timestamp_t end = dive_endtime(dive);
	return start - offset <= when && when <= end + offset;
}

/* this sets a usually unused copy of the preferences with the units
 * that were active the last time the dive list was saved to git storage
 * (this isn't used in XML files); storing the unit preferences in the
 * data file is usually pointless (that's a setting of the software,
 * not a property of the data), but it's a great hint of what the user
 * might expect to see when creating a backend service that visualizes
 * the dive list without Subsurface running - so this is basically a
 * functionality for the core library that Subsurface itself doesn't
 * use but that another consumer of the library (like an HTML exporter)
 * will need */
void set_informational_units(const char *units)
{
	if (strstr(units, "METRIC")) {
		git_prefs.unit_system = METRIC;
	} else if (strstr(units, "IMPERIAL")) {
		git_prefs.unit_system = IMPERIAL;
	} else if (strstr(units, "PERSONALIZE")) {
		git_prefs.unit_system = PERSONALIZE;
		if (strstr(units, "METERS"))
			git_prefs.units.length = METERS;
		if (strstr(units, "FEET"))
			git_prefs.units.length = FEET;
		if (strstr(units, "LITER"))
			git_prefs.units.volume = LITER;
		if (strstr(units, "CUFT"))
			git_prefs.units.volume = CUFT;
		if (strstr(units, "BAR"))
			git_prefs.units.pressure = BAR;
		if (strstr(units, "PSI"))
			git_prefs.units.pressure = PSI;
		if (strstr(units, "CELSIUS"))
			git_prefs.units.temperature = CELSIUS;
		if (strstr(units, "FAHRENHEIT"))
			git_prefs.units.temperature = FAHRENHEIT;
		if (strstr(units, "KG"))
			git_prefs.units.weight = KG;
		if (strstr(units, "LBS"))
			git_prefs.units.weight = LBS;
		if (strstr(units, "SECONDS"))
			git_prefs.units.vertical_speed_time = SECONDS;
		if (strstr(units, "MINUTES"))
			git_prefs.units.vertical_speed_time = MINUTES;
	}

}

void set_git_prefs(const char *prefs)
{
	if (strstr(prefs, "TANKBAR"))
		git_prefs.tankbar = 1;
	if (strstr(prefs, "DCCEILING"))
		git_prefs.dcceiling = 1;
	if (strstr(prefs, "SHOW_SETPOINT"))
		git_prefs.show_ccr_setpoint = 1;
	if (strstr(prefs, "SHOW_SENSORS"))
		git_prefs.show_ccr_sensors = 1;
	if (strstr(prefs, "PO2_GRAPH"))
		git_prefs.pp_graphs.po2 = 1;
}

/* clones a dive and moves given dive computer to front */
struct dive *make_first_dc(const struct dive *d, int dc_number)
{
	struct dive *res;
	struct divecomputer *dc, *newdc, *old_dc;

	/* copy the dive */
	res = alloc_dive();
	copy_dive(d, res);

	/* make a new unique id, since we still can't handle two equal ids */
	res->id = dive_getUniqID();

	if (dc_number == 0)
		return res;

	dc = &res->dc;
	newdc = malloc(sizeof(*newdc));
	old_dc = get_dive_dc(res, dc_number);

	/* skip the current DC in the linked list */
	for (dc = &res->dc; dc && dc->next != old_dc; dc = dc->next)
		;
	if (!dc) {
		free(newdc);
		fprintf(stderr, "data inconsistent: can't find the current DC");
		return res;
	}
	dc->next = old_dc->next;
	*newdc = res->dc;
	res->dc = *old_dc;
	res->dc.next = newdc;
	free(old_dc);

	return res;
}

static void delete_divecomputer(struct dive *d, int num)
{
	int i;

	/* Refuse to delete the last dive computer */
	if (!d->dc.next)
		return;

	if (num == 0) {
		/* remove the first one, so copy the second one in place of the first and free the second one
		 * be careful about freeing the no longer needed structures - since we copy things around we can't use free_dc()*/
		struct divecomputer *fdc = d->dc.next;
		free_dc_contents(&d->dc);
		memcpy(&d->dc, fdc, sizeof(struct divecomputer));
		free(fdc);
	} else {
		struct divecomputer *pdc = &d->dc;
		for (i = 0; i < num - 1 && pdc; i++)
			pdc = pdc->next;
		if (pdc && pdc->next) {
			struct divecomputer *dc = pdc->next;
			pdc->next = dc->next;
			free_dc(dc);
		}
	}
}

/* Clone a dive and delete goven dive computer */
struct dive *clone_delete_divecomputer(const struct dive *d, int dc_number)
{
	struct dive *res;

	/* copy the dive */
	res = alloc_dive();
	copy_dive(d, res);

	/* make a new unique id, since we still can't handle two equal ids */
	res->id = dive_getUniqID();

	delete_divecomputer(res, dc_number);
	force_fixup_dive(res);

	return res;
}

/*
 * This splits the dive src by dive computer. The first output dive has all
 * dive computers except num, the second only dive computer num.
 * The dives will not be associated with a trip.
 * On error, both output parameters are set to NULL.
 */
void split_divecomputer(const struct dive *src, int num, struct dive **out1, struct dive **out2)
{
	const struct divecomputer *srcdc = get_dive_dc_const(src, num);

	if (src && srcdc) {
		// Copy the dive, but only using the selected dive computer
		*out2 = alloc_dive();
		copy_dive_onedc(src, srcdc, *out2);

		// This will also make fixup_dive() to allocate a new dive id...
		(*out2)->id = 0;
		fixup_dive(*out2);

		// Copy the dive with all dive computers
		*out1 = create_new_copy(src);

		// .. and then delete the split-out dive computer
		delete_divecomputer(*out1, num);

		(*out1)->divetrip = (*out2)->divetrip = NULL;
	} else {
		*out1 = *out2 = NULL;
	}
}

//Calculate O2 in best mix
fraction_t best_o2(depth_t depth, const struct dive *dive, bool in_planner)
{
	fraction_t fo2;
	int po2 = in_planner ? prefs.bottompo2 : (int)(prefs.modpO2 * 1000.0);

	fo2.permille = (po2 * 100 / depth_to_mbar(depth.mm, dive)) * 10;	//use integer arithmetic to round down to nearest percent
	// Don't permit >100% O2
	if (fo2.permille > 1000)
		fo2.permille = 1000;
	return fo2;
}

//Calculate He in best mix. O2 is considered narcopic
fraction_t best_he(depth_t depth, const struct dive *dive, bool o2narcotic, fraction_t fo2)
{
	fraction_t fhe;
	int pnarcotic, ambient;
	pnarcotic = depth_to_mbar(prefs.bestmixend.mm, dive);
	ambient = depth_to_mbar(depth.mm, dive);
	if (o2narcotic) {
		fhe.permille = (100 - 100 * pnarcotic / ambient) * 10;	//use integer arithmetic to round up to nearest percent
	} else {
		fhe.permille = 1000 - fo2.permille - N2_IN_AIR * pnarcotic / ambient;
	}
	if (fhe.permille < 0)
		fhe.permille = 0;
	return fhe;
}

void invalidate_dive_cache(struct dive *dive)
{
	memset(dive->git_id, 0, 20);
}

bool dive_cache_is_valid(const struct dive *dive)
{
	static const unsigned char null_id[20] = { 0, };
	return !!memcmp(dive->git_id, null_id, 20);
}

int get_surface_pressure_in_mbar(const struct dive *dive, bool non_null)
{
	int mbar = dive->surface_pressure.mbar;
	if (!mbar && non_null)
		mbar = SURFACE_PRESSURE;
	return mbar;
}

/* This returns the conversion factor that you need to multiply
 * a (relative) depth in mm to obtain a (relative) pressure in mbar.
 * As everywhere in Subsurface, the expected unit of a salinity is
 * g/10l such that sea water has a salinity of 10300
 */
static double salinity_to_specific_weight(int salinity)
{
	return salinity * 0.981 / 100000.0;
}

/* Pa = N/m^2 - so we determine the weight (in N) of the mass of 10m
 * of water (and use standard salt water at 1.03kg per liter if we don't know salinity)
 * and add that to the surface pressure (or to 1013 if that's unknown) */
static double calculate_depth_to_mbarf(int depth, pressure_t surface_pressure, int salinity)
{
	double specific_weight;
	int mbar = surface_pressure.mbar;

	if (!mbar)
		mbar = SURFACE_PRESSURE;
	if (!salinity)
		salinity = SEAWATER_SALINITY;
	if (salinity < 500)
		salinity += FRESHWATER_SALINITY;
	specific_weight = salinity_to_specific_weight(salinity);
	return mbar + depth * specific_weight;
}

int depth_to_mbar(int depth, const struct dive *dive)
{
	return lrint(depth_to_mbarf(depth, dive));
}

double depth_to_mbarf(int depth, const struct dive *dive)
{
	// To downloaded and planned dives, use DC's values
	int salinity = dive->dc.salinity;
	pressure_t surface_pressure = dive->dc.surface_pressure;

	if (is_manually_added_dc(&dive->dc)) { // To manual dives, salinity and pressure in another place...
		surface_pressure = dive->surface_pressure;
		salinity = dive->user_salinity;
	}
	return calculate_depth_to_mbarf(depth, surface_pressure, salinity);
}

double depth_to_bar(int depth, const struct dive *dive)
{
	return depth_to_mbar(depth, dive) / 1000.0;
}

double depth_to_atm(int depth, const struct dive *dive)
{
	return mbar_to_atm(depth_to_mbar(depth, dive));
}

/* for the inverse calculation we use just the relative pressure
 * (that's the one that some dive computers like the Uemis Zurich
 * provide - for the other models that do this libdivecomputer has to
 * take care of this, but the Uemis we support natively */
int rel_mbar_to_depth(int mbar, const struct dive *dive)
{
	// To downloaded and planned dives, use DC's salinity. Manual dives, use user's salinity
	int salinity = is_manually_added_dc(&dive->dc) ? dive->user_salinity : dive->dc.salinity;
	if (!salinity)
		salinity = SEAWATER_SALINITY;

	/* whole mbar gives us cm precision */
	double specific_weight = salinity_to_specific_weight(salinity);
	return (int)lrint(mbar / specific_weight);
}

int mbar_to_depth(int mbar, const struct dive *dive)
{
	// To downloaded and planned dives, use DC's pressure. Manual dives, use user's pressure
	pressure_t surface_pressure = is_manually_added_dc(&dive->dc)
		? dive->surface_pressure
		: dive->dc.surface_pressure;

	if (!surface_pressure.mbar)
		surface_pressure.mbar = SURFACE_PRESSURE;
		
	return rel_mbar_to_depth(mbar - surface_pressure.mbar, dive);
}

/* MOD rounded to multiples of roundto mm */
depth_t gas_mod(struct gasmix mix, pressure_t po2_limit, const struct dive *dive, int roundto)
{
	depth_t rounded_depth;

	double depth = (double) mbar_to_depth(po2_limit.mbar * 1000 / get_o2(mix), dive);
	rounded_depth.mm = (int)lrint(depth / roundto) * roundto;
	return rounded_depth;
}

/* Maximum narcotic depth rounded to multiples of roundto mm */
depth_t gas_mnd(struct gasmix mix, depth_t end, const struct dive *dive, int roundto)
{
	depth_t rounded_depth;
	pressure_t ppo2n2;
	ppo2n2.mbar = depth_to_mbar(end.mm, dive);

	int maxambient = prefs.o2narcotic ?
					(int)lrint(ppo2n2.mbar / (1 - get_he(mix) / 1000.0))
			      :
					get_n2(mix) > 0 ?
						(int)lrint(ppo2n2.mbar * N2_IN_AIR / get_n2(mix))
					:
						// Actually: Infinity
						1000000;
	rounded_depth.mm = (int)lrint(((double)mbar_to_depth(maxambient, dive)) / roundto) * roundto;
	return rounded_depth;
}

struct dive *get_dive(int nr)
{
	if (nr >= divelog.dives->nr || nr < 0)
		return NULL;
	return divelog.dives->dives[nr];
}

struct dive_site *get_dive_site_for_dive(const struct dive *dive)
{
	return dive->dive_site;
}

const char *get_dive_country(const struct dive *dive)
{
	struct dive_site *ds = dive->dive_site;
	return ds ? taxonomy_get_country(&ds->taxonomy) : NULL;
}

const char *get_dive_location(const struct dive *dive)
{
	const struct dive_site *ds = dive->dive_site;
	if (ds && ds->name)
		return ds->name;
	return NULL;
}

unsigned int number_of_computers(const struct dive *dive)
{
	unsigned int total_number = 0;
	const struct divecomputer *dc = &dive->dc;

	if (!dive)
		return 1;

	do {
		total_number++;
		dc = dc->next;
	} while (dc);
	return total_number;
}

struct divecomputer *get_dive_dc(struct dive *dive, int nr)
{
	struct divecomputer *dc;
	if (!dive)
		return NULL;
	dc = &dive->dc;

	while (nr-- > 0) {
		dc = dc->next;
		if (!dc)
			return &dive->dc;
	}
	return dc;
}

const struct divecomputer *get_dive_dc_const(const struct dive *dive, int nr)
{
	return get_dive_dc((struct dive *)dive, nr);
}

struct dive *get_dive_by_uniq_id(int id)
{
	int i;
	struct dive *dive = NULL;

	for_each_dive (i, dive) {
		if (dive->id == id)
			break;
	}
#ifdef DEBUG
	if (dive == NULL) {
		fprintf(stderr, "Invalid id %x passed to get_dive_by_diveid, try to fix the code\n", id);
		exit(1);
	}
#endif
	return dive;
}

int get_idx_by_uniq_id(int id)
{
	int i;
	struct dive *dive = NULL;

	for_each_dive (i, dive) {
		if (dive->id == id)
			break;
	}
#ifdef DEBUG
	if (dive == NULL) {
		fprintf(stderr, "Invalid id %x passed to get_dive_by_diveid, try to fix the code\n", id);
		exit(1);
	}
#endif
	return i;
}

bool dive_site_has_gps_location(const struct dive_site *ds)
{
	return ds && has_location(&ds->location);
}

int dive_has_gps_location(const struct dive *dive)
{
	if (!dive)
		return false;
	return dive_site_has_gps_location(dive->dive_site);
}

/* Extract GPS location of a dive computer stored in the GPS1
 * or GPS2 extra data fields */
static location_t dc_get_gps_location(const struct divecomputer *dc)
{
	location_t res = { };

	for (struct extra_data *data = dc->extra_data; data; data = data->next) {
		if (!strcmp(data->key, "GPS1")) {
			parse_location(data->value, &res);
			/* If we found a valid GPS1 field exit early since
			 * it has priority over GPS2 */
			if (has_location(&res))
				break;
		} else if (!strcmp(data->key, "GPS2")) {
			/* For GPS2 fields continue searching, as we might
			 * still find a GPS1 field */
			parse_location(data->value, &res);
		}
	}
	return res;
}

/* Get GPS location for a dive. Highest priority is given to the GPS1
 * extra data written by libdivecomputer, as this comes from a real GPS
 * device. If that doesn't exits, use the currently set dive site.
 * This function is potentially slow, therefore only call sparingly
 * and remember the result.
 */
location_t dive_get_gps_location(const struct dive *d)
{
	location_t res = { };

	for (const struct divecomputer *dc = &d->dc; dc; dc = dc->next) {
		res = dc_get_gps_location(dc);
		if (has_location(&res))
			return res;
	}

	/* No libdivecomputer generated GPS data found.
	 * Let's use the location of the current dive site.
	 */
	if (d->dive_site)
		res = d->dive_site->location;

	return res;
}

/* When evaluated at the time of a gasswitch, this returns the new gas */
struct gasmix get_gasmix(const struct dive *dive, const struct divecomputer *dc, int time, const struct event **evp, struct gasmix gasmix)
{
	const struct event *ev = *evp;
	struct gasmix res;

	/* if there is no cylinder, return air */
	if (dive->cylinders.nr <= 0)
		return gasmix_air;

	if (!ev) {
		/* on first invocation, get initial gas mix and first event (if any) */
		int cyl = explicit_first_cylinder(dive, dc);
		res = get_cylinder(dive, cyl)->gasmix;
		ev = dc ? get_next_event(dc->events, "gaschange") : NULL;
	} else {
		res = gasmix;
	}

	while (ev && ev->time.seconds <= time) {
		res = get_gasmix_from_event(dive, ev);
		ev = get_next_event(ev->next, "gaschange");
	}
	*evp = ev;
	return res;
}

/* get the gas at a certain time during the dive */
/* If there is a gasswitch at that time, it returns the new gasmix */
struct gasmix get_gasmix_at_time(const struct dive *d, const struct divecomputer *dc, duration_t time)
{
	const struct event *ev = NULL;
	struct gasmix gasmix = gasmix_air;
	return get_gasmix(d, dc, time.seconds, &ev, gasmix);
}

/* Does that cylinder have any pressure readings? */
extern bool cylinder_with_sensor_sample(const struct dive *dive, int cylinder_id)
{
	for (const struct divecomputer *dc = &dive->dc; dc; dc = dc->next) {
		for (int i = 0; i < dc->samples; ++i) {
			struct sample *sample = dc->sample + i;
			for (int j = 0; j < MAX_SENSORS; ++j) {
				if (!sample->pressure[j].mbar)
					continue;
				if (sample->sensor[j] == cylinder_id)
					return true;
			}
		}
	}
	return false;
}
