// SPDX-License-Identifier: GPL-2.0
/* dive.cpp */
/* maintains the internal dive list structure */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <memory>
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
#include "format.h"
#include "interpolate.h"
#include "qthelper.h"
#include "membuffer.h"
#include "picture.h"
#include "range.h"
#include "sample.h"
#include "tag.h"
#include "trip.h"
#include "fulltext.h"

#include <time.h>

// For user visible text but still not translated
const char *divemode_text_ui[] = {
	QT_TRANSLATE_NOOP("gettextFromC", "Open circuit"),
	QT_TRANSLATE_NOOP("gettextFromC", "CCR"),
	QT_TRANSLATE_NOOP("gettextFromC", "pSCR"),
	QT_TRANSLATE_NOOP("gettextFromC", "Freedive")
};

// For writing/reading files.
const char *divemode_text[] = {"OC", "CCR", "PSCR", "Freedive"};

// Even for dives without divecomputer, we allocate a divecomputer structure.
// It's the "manually added" divecomputer.
dive::dive() : dcs(1)
{
	id = dive_getUniqID();
}

dive::dive(const dive &) = default;
dive::dive(dive &&) = default;
dive &dive::operator=(const dive &) = default;
dive::~dive() = default;

// create a dive an hour from now with a default depth (15m/45ft) and duration (40 minutes)
// as a starting point for the user to edit
std::unique_ptr<dive> dive::default_dive()
{
	auto d = std::make_unique<dive>();
	d->when = time(nullptr) + gettimezoneoffset() + 3600;
	d->dcs[0].duration.seconds = 40 * 60;
	d->dcs[0].maxdepth.mm = M_OR_FT(15, 45);
	d->dcs[0].meandepth.mm = M_OR_FT(13, 39); // this creates a resonable looking safety stop
	make_manually_added_dive_dc(&d->dcs[0]);
	fake_dc(&d->dcs[0]);
	add_default_cylinder(d.get());
	fixup_dive(d.get());
	return d;
}

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
	int o2sensor;

	o2sensor = (dc->divemode == CCR) ? get_cylinder_idx_by_use(dive, OXYGEN) : -1;
	for (const auto &s: dc->samples) {
		int seen_pressure = 0, idx;

		for (idx = 0; idx < MAX_SENSORS; idx++) {
			int sensor = s.sensor[idx];
			pressure_t p = s.pressure[idx];

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
struct event create_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx)
{
	/* The gas switch event format is insane for historical reasons */
	struct gasmix mix = get_cylinder(dive, idx)->gasmix;
	int o2 = get_o2(mix);
	int he = get_he(mix);

	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	int value = o2 + (he << 16);

	struct event ev(seconds, he ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE, 0, value, "gaschange");
	ev.gas.index = idx;
	ev.gas.mix = mix;
	return ev;
}

void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int seconds, int idx)
{
	/* sanity check so we don't crash */
	/* FIXME: The planner uses a dummy cylinder one past the official number of cylinders
	 * in the table to mark no-cylinder surface interavals. This is horrendous. Fix ASAP. */
	//if (idx < 0 || idx >= dive->cylinders.size()) {
	if (idx < 0 || static_cast<size_t>(idx) >= dive->cylinders.size() + 1) {
		report_error("Unknown cylinder index: %d", idx);
		return;
	}
	struct event ev = create_gas_switch_event(dive, dc, seconds, idx);
	add_event_to_dc(dc, std::move(ev));
}

struct gasmix get_gasmix_from_event(const struct dive *dive, const struct event &ev)
{
	if (ev.is_gaschange()) {
		int index = ev.gas.index;
		// FIXME: The planner uses one past cylinder-count to signify "surface air". Remove in due course.
		if (index >= 0 && static_cast<size_t>(index) < dive->cylinders.size() + 1)
			return get_cylinder(dive, index)->gasmix;
		return ev.gas.mix;
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

static void dc_cylinder_renumber(struct dive &dive, struct divecomputer &dc, const int mapping[]);

/* copy dive computer list and renumber the cylinders */
static void copy_dc_renumber(struct dive &d, const struct dive &s, const int cylinders_map[])
{
	for (const divecomputer &dc: s.dcs) {
		d.dcs.push_back(dc);
		dc_cylinder_renumber(d, d.dcs.back(), cylinders_map);
	}
}

/* copy_dive makes duplicates of many components of a dive;
 * in order not to leak memory, we need to free those.
 * copy_dive doesn't play with the divetrip and forward/backward pointers
 * so we can ignore those */
void clear_dive(struct dive *d)
{
	if (!d)
		return;
	fulltext_unregister(d);
	*d = dive();
}

/* make a true copy that is independent of the source dive;
 * all data structures are duplicated, so the copy can be modified without
 * any impact on the source */
void copy_dive(const struct dive *s, struct dive *d)
{
	/* simply copy things over, but then clear fulltext cache and dive cache. */
	*d = *s;
	invalidate_dive_cache(d);
}

#define CONDITIONAL_COPY_STRING(_component) \
	if (what._component)                \
		d->_component = s->_component

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
		s->dive_site->add_dive(d);
	}
	if (what.tags)
		d->tags = s->tags;
	if (what.cylinders)
		copy_cylinder_types(s, d);
	if (what.weights)
		d->weightsystems = s->weightsystems;
	if (what.number)
		d->number = s->number;
	if (what.when)
		d->when = s->when;
}
#undef CONDITIONAL_COPY_STRING

/* copies all events from the given dive computer before a given time
   this is used when editing a dive in the planner to preserve the events
   of the old dive */
void copy_events_until(const struct dive *sd, struct dive *dd, int dcNr, int time)
{
	if (!sd || !dd)
		return;

	const struct divecomputer *s = &sd->dcs[0];
	struct divecomputer *d = get_dive_dc(dd, dcNr);

	if (!s || !d)
		return;

	for (const auto &ev: s->events) {
		// Don't add events the planner knows about
		if (ev.time.seconds < time && !ev.is_gaschange() && !ev.is_divemodechange())
			add_event(d, ev.time.seconds, ev.type, ev.flags, ev.value, ev.name);
	}
}

void copy_used_cylinders(const struct dive *s, struct dive *d, bool used_only)
{
	if (!s || !d)
		return;

	d->cylinders.clear();
	for (auto [i, cyl]: enumerated_range(s->cylinders)) {
		if (!used_only || is_cylinder_used(s, i) || get_cylinder(s, i)->cylinder_use == NOT_USED)
			d->cylinders.push_back(cyl);
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
static void update_depth(depth_t *depth, int new_depth)
{
	if (new_depth) {
		int old = depth->mm;

		if (abs(old - new_depth) > 1000)
			depth->mm = new_depth;
	}
}

static void update_temperature(temperature_t *temperature, int new_temp)
{
	if (new_temp) {
		int old = temperature->mkelvin;

		if (abs(old - new_temp) > 1000)
			temperature->mkelvin = new_temp;
	}
}

/* Which cylinders had gas used? */
#define SOME_GAS 5000
static bool cylinder_used(const cylinder_t &cyl)
{
	int start_mbar, end_mbar;

	start_mbar = cyl.start.mbar ?: cyl.sample_start.mbar;
	end_mbar = cyl.end.mbar ?: cyl.sample_end.mbar;

	// More than 5 bar used? This matches statistics.cpp
	// heuristics
	return start_mbar > end_mbar + SOME_GAS;
}

/* Get list of used cylinders. Returns the number of used cylinders. */
static int get_cylinder_used(const struct dive *dive, bool used[])
{
	int num = 0;

	for (auto [i, cyl]: enumerated_range(dive->cylinders)) {
		used[i] = cylinder_used(cyl);
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
	auto used_and_unknown = std::make_unique<bool[]>(dive->cylinders.size());
	std::copy(used_cylinders, used_cylinders + dive->cylinders.size(), used_and_unknown.get());

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
	if (idx >= 0 && used_and_unknown[idx]) {
		used_and_unknown[idx] = false;
		num--;
	}

	/* And we have possible switches to other gases */
	event_loop loop("gaschange");
	const struct event *ev;
	while ((ev = loop.next(*dc)) != nullptr && num > 0) {
		idx = get_cylinder_index(dive, *ev);
		if (idx >= 0 && used_and_unknown[idx]) {
			used_and_unknown[idx] = false;
			num--;
		}
	}

	return num > 0;
}

void per_cylinder_mean_depth(const struct dive *dive, struct divecomputer *dc, int *mean, int *duration)
{
	int32_t lasttime = 0;
	int lastdepth = 0;
	int idx = 0;
	int num_used_cylinders;

	if (dive->cylinders.empty())
		return;

	for (size_t i = 0; i < dive->cylinders.size(); i++)
		mean[i] = duration[i] = 0;
	if (!dc)
		return;

	/*
	 * There is no point in doing per-cylinder information
	 * if we don't actually know about the usage of all the
	 * used cylinders.
	 */
	auto used_cylinders = std::make_unique<bool[]>(dive->cylinders.size());
	num_used_cylinders = get_cylinder_used(dive, used_cylinders.get());
	if (has_unknown_used_cylinders(dive, dc, used_cylinders.get(), num_used_cylinders)) {
		/*
		 * If we had more than one used cylinder, but
		 * do not know usage of them, we simply cannot
		 * account mean depth to them.
		 */
		if (num_used_cylinders > 1)
			return;

		/*
		 * For a single cylinder, use the overall mean
		 * and duration
		 */
		for (size_t i = 0; i < dive->cylinders.size(); i++) {
			if (used_cylinders[i]) {
				mean[i] = dc->meandepth.mm;
				duration[i] = dc->duration.seconds;
			}
		}

		return;
	}
	if (dc->samples.empty())
		fake_dc(dc);
	event_loop loop("gaschange");
	const struct event *ev = loop.next(*dc);
	std::vector<int> depthtime(dive->cylinders.size(), 0);
	for (auto it = dc->samples.begin(); it != dc->samples.end(); ++it) {
		int32_t time = it->time.seconds;
		int depth = it->depth.mm;

		/* Make sure to move the event past 'lasttime' */
		while (ev && lasttime >= ev->time.seconds) {
			idx = get_cylinder_index(dive, *ev);
			ev = loop.next(*dc);
		}

		/* Do we need to fake a midway sample at an event? */
		if (ev && it != dc->samples.begin() && time > ev->time.seconds) {
			int newtime = ev->time.seconds;
			int newdepth = interpolate(lastdepth, depth, newtime - lasttime, time - lasttime);

			time = newtime;
			depth = newdepth;
			--it;
		}
		/* We ignore segments at the surface */
		if (depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD) {
			duration[idx] += time - lasttime;
			depthtime[idx] += (time - lasttime) * (depth + lastdepth) / 2;
		}
		lastdepth = depth;
		lasttime = time;
	}
	for (size_t i = 0; i < dive->cylinders.size(); i++) {
		if (duration[i])
			mean[i] = (depthtime[i] + duration[i] / 2) / duration[i];
	}
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
	if (dive->cylinders.empty())
		return -1;
	if (dc) {
		const struct event *ev = get_first_event(*dc, "gaschange");
		if (ev && ((!dc->samples.empty() && ev->time.seconds == dc->samples[0].time.seconds) || ev->time.seconds <= 1))
			res = get_cylinder_index(dive, *ev);
		else if (dc->divemode == CCR)
			res = std::max(get_cylinder_idx_by_use(dive, DILUENT), res);
	}
	return static_cast<size_t>(res) < dive->cylinders.size() ? res : 0;
}

static double calculate_depth_to_mbarf(int depth, pressure_t surface_pressure, int salinity);

/* this gets called when the dive mode has changed (so OC vs. CC)
 * there are two places we might have setpoints... events or in the samples
 */
void update_setpoint_events(const struct dive *dive, struct divecomputer *dc)
{
	int new_setpoint = 0;

	if (dc->divemode == CCR)
		new_setpoint = prefs.defaultsetpoint;

	if (dc->divemode == OC &&
	    (dc->model == "Shearwater Predator" ||
	     dc->model == "Shearwater Petrel" ||
	     dc->model == "Shearwater Nerd")) {
		// make sure there's no setpoint in the samples
		// this is an irreversible change - so switching a dive to OC
		// by mistake when it's actually CCR is _bad_
		// So we make sure, this comes from a Predator or Petrel and we only remove
		// pO2 values we would have computed anyway.
		event_loop loop("gaschange");
		const struct event *ev = loop.next(*dc);
		struct gasmix gasmix = get_gasmix_from_event(dive, *ev);
		const struct event *next = loop.next(*dc);

		for (auto &sample: dc->samples) {
			if (next && sample.time.seconds >= next->time.seconds) {
				ev = next;
				gasmix = get_gasmix_from_event(dive, *ev);
				next = loop.next(*dc);
			}
			gas_pressures pressures = fill_pressures(lrint(calculate_depth_to_mbarf(sample.depth.mm, dc->surface_pressure, 0)), gasmix ,0, dc->divemode);
			if (abs(sample.setpoint.mbar - (int)(1000 * pressures.o2)) <= 50)
				sample.setpoint.mbar = 0;
		}
	}

	// an "SP change" event at t=0 is currently our marker for OC vs CCR
	// this will need to change to a saner setup, but for now we can just
	// check if such an event is there and adjust it, or add that event
	struct event *ev = get_first_event(*dc, "SP change");
	if (ev && ev->time.seconds == 0) {
		ev->value = new_setpoint;
	} else {
		if (!add_event(dc, 0, SAMPLE_EVENT_PO2, 0, new_setpoint, "SP change"))
			report_info("Could not add setpoint change event");
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
static void match_standard_cylinder(cylinder_type_t &type)
{
	/* Do we already have a cylinder description? */
	if (!type.description.empty())
		return;

	double bar = type.workingpressure.mbar / 1000.0;
	double cuft = ml_to_cuft(type.size.mliter);
	cuft *= bar_to_atm(bar);
	int psi = lrint(to_PSI(type.workingpressure));

	const char *fmt;
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
	type.description = format_string_std(fmt, (int)lrint(cuft));
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
static void sanitize_cylinder_type(cylinder_type_t &type)
{
	/* If we have no working pressure, it had *better* be just a physical size! */
	if (!type.workingpressure.mbar)
		return;

	/* No size either? Nothing to go on */
	if (!type.size.mliter)
		return;

	/* Ok, we have both size and pressure: try to match a description */
	match_standard_cylinder(type);
}

static void sanitize_cylinder_info(struct dive *dive)
{
	for (auto &cyl :dive->cylinders) {
		sanitize_gasmix(cyl.gasmix);
		sanitize_cylinder_type(cyl.type);
	}
}

/* some events should never be thrown away */
static bool is_potentially_redundant(const struct event &event)
{
	if (event.name == "gaschange")
		return false;
	if (event.name == "bookmark")
		return false;
	if (event.name == "heading")
		return false;
	return true;
}

pressure_t calculate_surface_pressure(const struct dive *dive)
{
	pressure_t res;
	int sum = 0, nr = 0;

	bool logged = dive->is_logged();
	for (auto &dc: dive->dcs) {
		if ((logged || !is_dc_planner(&dc)) && dc.surface_pressure.mbar) {
			sum += dc.surface_pressure.mbar;
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
	int sum = 0, nr = 0;

	bool logged = dive->is_logged();
	for (auto &dc: dive->dcs) {
		if ((logged || !is_dc_planner(&dc)) && dc.salinity) {
			if (dc.salinity < 500)
				dc.salinity += FRESHWATER_SALINITY;
			sum += dc.salinity;
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
	int sum = 0, nr = 0;

	bool logged = dive->is_logged();
	for (auto &dc: dive->dcs) {
		if ((logged || !is_dc_planner(&dc)) && dc.meandepth.mm) {
			sum += dc.meandepth.mm;
			nr++;
		}
	}

	if (nr)
		dive->meandepth.mm = (sum + nr / 2) / nr;
}

static void fixup_duration(struct dive *dive)
{
	duration_t duration = { };

	bool logged = dive->is_logged();
	for (auto &dc: dive->dcs) {
		if (logged || !is_dc_planner(&dc))
			duration.seconds = std::max(duration.seconds, dc.duration.seconds);
	}
	dive->duration.seconds = duration.seconds;
}

static void fixup_watertemp(struct dive *dive)
{
	if (!dive->watertemp.mkelvin)
		dive->watertemp = dive->dc_watertemp();
}

static void fixup_airtemp(struct dive *dive)
{
	if (!dive->airtemp.mkelvin)
		dive->airtemp = dive->dc_airtemp();
}

/* if the air temperature in the dive data is redundant to the one in its
 * first divecomputer (i.e., it was added by running fixup on the dive)
 * return 0, otherwise return the air temperature given in the dive */
static temperature_t un_fixup_airtemp(const struct dive &a)
{
	return a.airtemp.mkelvin == a.dc_airtemp().mkelvin ?
		temperature_t() : a.airtemp;
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
 */
static void fixup_dc_events(struct divecomputer &dc)
{
	std::vector<int> to_delete;

	for (auto [idx, event]: enumerated_range(dc.events)) {
		if (!is_potentially_redundant(event))
			continue;
		for (int idx2 = idx - 1; idx2 > 0; --idx2) {
			const auto &prev = dc.events[idx2];
			if (prev.name == event.name && prev.flags == event.flags &&
			    event.time.seconds - prev.time.seconds < 61)
				to_delete.push_back(idx);
		}
	}
	// Delete from back to not invalidate indexes
	for (auto it = to_delete.rbegin(); it != to_delete.rend(); ++it)
		dc.events.erase(dc.events.begin() + *it);
}

static int interpolate_depth(struct divecomputer &dc, int idx, int lastdepth, int lasttime, int now)
{
	int nextdepth = lastdepth;
	int nexttime = now;

	for (auto it = dc.samples.begin() + idx; it != dc.samples.end(); ++it) {
		if (it->depth.mm < 0)
			continue;
		nextdepth = it->depth.mm;
		nexttime = it->time.seconds;
		break;
	}
	return interpolate(lastdepth, nextdepth, now-lasttime, nexttime-lasttime);
}

static void fixup_dc_depths(struct dive *dive, struct divecomputer &dc)
{
	int maxdepth = dc.maxdepth.mm;
	int lasttime = 0, lastdepth = 0;

	for (const auto [idx, sample]: enumerated_range(dc.samples)) {
		int time = sample.time.seconds;
		int depth = sample.depth.mm;

		if (depth < 0 && idx + 2 < static_cast<int>(dc.samples.size())) {
			depth = interpolate_depth(dc, idx, lastdepth, lasttime, time);
			sample.depth.mm = depth;
		}

		if (depth > SURFACE_THRESHOLD) {
			if (depth > maxdepth)
				maxdepth = depth;
		}

		lastdepth = depth;
		lasttime = time;
		if (sample.cns > dive->maxcns)
			dive->maxcns = sample.cns;
	}

	update_depth(&dc.maxdepth, maxdepth);
	if (!dive->is_logged() || !is_dc_planner(&dc))
		if (maxdepth > dive->maxdepth.mm)
			dive->maxdepth.mm = maxdepth;
}

static void fixup_dc_ndl(struct divecomputer &dc)
{
	for (auto &sample: dc.samples) {
		if (sample.ndl.seconds != 0)
			break;
		if (sample.ndl.seconds == 0)
			sample.ndl.seconds = -1;
	}
}

static void fixup_dc_temp(struct dive *dive, struct divecomputer &dc)
{
	int mintemp = 0, lasttemp = 0;

	for (auto &sample: dc.samples) {
		int temp = sample.temperature.mkelvin;

		if (temp) {
			/*
			 * If we have consecutive identical
			 * temperature readings, throw away
			 * the redundant ones.
			 */
			if (lasttemp == temp)
				sample.temperature.mkelvin = 0;
			else
				lasttemp = temp;

			if (!mintemp || temp < mintemp)
				mintemp = temp;
		}

		update_min_max_temperatures(dive, sample.temperature);
	}
	update_temperature(&dc.watertemp, mintemp);
	update_min_max_temperatures(dive, dc.watertemp);
}

/* Remove redundant pressure information */
static void simplify_dc_pressures(struct divecomputer &dc)
{
	int lastindex[2] = { -1, -1 };
	int lastpressure[2] = { 0 };

	for (auto &sample: dc.samples) {
		int j;

		for (j = 0; j < MAX_SENSORS; j++) {
			int pressure = sample.pressure[j].mbar;
			int index = sample.sensor[j];

			if (index == lastindex[j]) {
				/* Remove duplicate redundant pressure information */
				if (pressure == lastpressure[j])
					sample.pressure[j].mbar = 0;
			}
			lastindex[j] = index;
			lastpressure[j] = pressure;
		}
	}
}

/* Do we need a sensor -> cylinder mapping? */
static void fixup_start_pressure(struct dive *dive, int idx, pressure_t p)
{
	if (idx >= 0 && static_cast<size_t>(idx) < dive->cylinders.size()) {
		cylinder_t &cyl = dive->cylinders[idx];
		if (p.mbar && !cyl.sample_start.mbar)
			cyl.sample_start = p;
	}
}

static void fixup_end_pressure(struct dive *dive, int idx, pressure_t p)
{
	if (idx >= 0 && static_cast<size_t>(idx) < dive->cylinders.size()) {
		cylinder_t &cyl = dive->cylinders[idx];
		if (p.mbar && !cyl.sample_end.mbar)
			cyl.sample_end = p;
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
static void fixup_dive_pressures(struct dive *dive, struct divecomputer &dc)
{
	/* Walk the samples from the beginning to find starting pressures.. */
	for (auto &sample: dc.samples) {
		if (sample.depth.mm < SURFACE_THRESHOLD)
			continue;

		for (int idx = 0; idx < MAX_SENSORS; idx++)
			fixup_start_pressure(dive, sample.sensor[idx], sample.pressure[idx]);
	}

	/* ..and from the end for ending pressures */
	for (auto it = dc.samples.rbegin(); it != dc.samples.rend(); ++it) {
		if (it->depth.mm < SURFACE_THRESHOLD)
			continue;

		for (int idx = 0; idx < MAX_SENSORS; idx++)
			fixup_end_pressure(dive, it->sensor[idx], it->pressure[idx]);
	}

	simplify_dc_pressures(dc);
}

/*
 * Match a gas change event against the cylinders we have
 */
static bool validate_gaschange(struct dive *dive, struct event &event)
{
	int index;
	int o2, he, value;

	/* We'll get rid of the per-event gasmix, but for now sanitize it */
	if (gasmix_is_air(event.gas.mix))
		event.gas.mix.o2.permille = 0;

	/* Do we already have a cylinder index for this gasmix? */
	if (event.gas.index >= 0)
		return true;

	index = find_best_gasmix_match(event.gas.mix, dive->cylinders);
	if (index < 0 || static_cast<size_t>(index) >= dive->cylinders.size())
		return false;

	/* Fix up the event to have the right information */
	event.gas.index = index;
	event.gas.mix = dive->cylinders[index].gasmix;

	/* Convert to odd libdivecomputer format */
	o2 = get_o2(event.gas.mix);
	he = get_he(event.gas.mix);

	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	value = o2 + (he << 16);

	event.value = value;
	if (he)
		event.type = SAMPLE_EVENT_GASCHANGE2;

	return true;
}

/* Clean up event, return true if event is ok, false if it should be dropped as bogus */
static bool validate_event(struct dive *dive, struct event &event)
{
	if (event.is_gaschange())
		return validate_gaschange(dive, event);
	return true;
}

static void fixup_dc_gasswitch(struct dive *dive, struct divecomputer &dc)
{
	// erase-remove idiom
	auto &events = dc.events;
	events.erase(std::remove_if(events.begin(), events.end(),
				    [dive](auto &ev) { return !validate_event(dive, ev); }),
		     events.end());
}

static void fixup_no_o2sensors(struct divecomputer &dc)
{
	// Its only relevant to look for sensor values on CCR and PSCR dives without any no_o2sensors recorded.
	if (dc.no_o2sensors != 0 || !(dc.divemode == CCR || dc.divemode == PSCR))
		return;

	for (const auto &sample: dc.samples) {
		int nsensor = 0;

		// How many o2 sensors can we find in this sample?
		for (int j = 0; j < MAX_O2_SENSORS; j++)
			if (sample.o2sensor[j].mbar)
				nsensor++;

		// If we fond more than the previous found max, record it.
		if (nsensor > dc.no_o2sensors)
			dc.no_o2sensors = nsensor;

		// Already found the maximum posible amount.
		if (nsensor == MAX_O2_SENSORS)
			return;
	}
}

static void fixup_dc_sample_sensors(struct dive *dive, struct divecomputer &dc)
{
	unsigned long sensor_mask = 0;

	for (auto &sample: dc.samples) {
		for (int j = 0; j < MAX_SENSORS; j++) {
			int sensor = sample.sensor[j];

			// No invalid sensor ID's, please
			if (sensor < 0 || sensor > MAX_SENSORS) {
				sample.sensor[j] = NO_SENSOR;
				sample.pressure[j].mbar = 0;
				continue;
			}

			// Don't bother tracking sensors with no data
			if (!sample.pressure[j].mbar) {
				sample.sensor[j] = NO_SENSOR;
				continue;
			}

			// Remember the set of sensors we had
			sensor_mask |= 1ul << sensor;
		}
	}

	// Ignore the sensors we have cylinders for
	sensor_mask >>= dive->cylinders.size();

	// Do we need to add empty cylinders?
	while (sensor_mask) {
		add_empty_cylinder(&dive->cylinders);
		sensor_mask >>= 1;
	}
}

static void fixup_dive_dc(struct dive *dive, struct divecomputer &dc)
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
	if (dc.samples.empty())
		fake_dc(&dc);
}

struct dive *fixup_dive(struct dive *dive)
{
	sanitize_cylinder_info(dive);
	dive->maxcns = dive->cns;

	/*
	 * Use the dive's temperatures for minimum and maximum in case
	 * we do not have temperatures recorded by DC.
	 */

	update_min_max_temperatures(dive, dive->watertemp);

	for (auto &dc: dive->dcs)
		fixup_dive_dc(dive, dc);

	fixup_water_salinity(dive);
	if (!dive->surface_pressure.mbar)
		fixup_surface_pressure(dive);
	fixup_meandepth(dive);
	fixup_duration(dive);
	fixup_watertemp(dive);
	fixup_airtemp(dive);
	for (auto &cyl: dive->cylinders) {
		add_cylinder_description(cyl.type);
		if (same_rounded_pressure(cyl.sample_start, cyl.start))
			cyl.start.mbar = 0;
		if (same_rounded_pressure(cyl.sample_end, cyl.end))
			cyl.end.mbar = 0;
	}
	divelog.dives.update_cylinder_related_info(dive);
	for (auto &ws: dive->weightsystems)
		add_weightsystem_description(ws);
	/* we should always have a uniq ID as that gets assigned during dive creation,
	 * but we want to make sure... */
	if (!dive->id)
		dive->id = dive_getUniqID();

	return dive;
}

/* Don't pick a zero for MERGE_MIN() */
#define MERGE_MAX(res, a, b, n) res->n = std::max(a->n, b->n)
#define MERGE_MIN(res, a, b, n) res->n = (a->n) ? (b->n) ? std::min(a->n, b->n) : (a->n) : (b->n)
#define MERGE_TXT(res, a, b, n, sep) res->n = merge_text(a->n, b->n, sep)
#define MERGE_NONZERO(res, a, b, n) (res)->n = (a)->n ? (a)->n : (b)->n

/*
 * This is like append_sample(), but if the distance from the last sample
 * is excessive, we add two surface samples in between.
 *
 * This is so that if you merge two non-overlapping dives, we make sure
 * that the time in between the dives is at the surface, not some "last
 * sample that happened to be at a depth of 1.2m".
 */
static void merge_one_sample(const struct sample &sample, struct divecomputer &dc)
{
	if (!dc.samples.empty()) {
		const struct sample &prev = dc.samples.back();
		int last_time = prev.time.seconds;
		int last_depth = prev.depth.mm;

		/*
		 * Only do surface events if the samples are more than
		 * a minute apart, and shallower than 5m
		 */
		if (sample.time.seconds > last_time + 60 && last_depth < 5000) {
			struct sample surface;

			/* Init a few values from prev sample to avoid useless info in XML */
			surface.bearing.degrees = prev.bearing.degrees;
			surface.ndl.seconds = prev.ndl.seconds;
			surface.time.seconds = last_time + 20;

			append_sample(surface, &dc);

			surface.time.seconds = sample.time.seconds - 20;
			append_sample(surface, &dc);
		}
	}
	append_sample(sample, &dc);
}

static void renumber_last_sample(struct divecomputer &dc, const int mapping[]);
static void sample_renumber(struct sample &s, const struct sample *next, const int mapping[]);

/*
 * Merge samples. Dive 'a' is "offset" seconds before Dive 'b'
 */
static void merge_samples(struct divecomputer &res,
			  const struct divecomputer &a, const struct divecomputer &b,
			  const int *cylinders_map_a, const int *cylinders_map_b,
			  int offset)
{
	auto as = a.samples.begin();
	auto bs = b.samples.begin();
	auto a_end = a.samples.end();
	auto b_end = b.samples.end();

	/*
	 * We want a positive sample offset, so that sample
	 * times are always positive. So if the samples for
	 * 'b' are before the samples for 'a' (so the offset
	 * is negative), we switch a and b around, and use
	 * the reverse offset.
	 */
	if (offset < 0) {
		offset = -offset;
		std::swap(as, bs);
		std::swap(a_end, b_end);
		std::swap(cylinders_map_a, cylinders_map_b);
	}

	for (;;) {
		int at = as != a_end ? as->time.seconds : -1;
		int bt = bs != b_end ? bs->time.seconds + offset : -1;

		/* No samples? All done! */
		if (at < 0 && bt < 0)
			return;

		/* Only samples from a? */
		if (bt < 0) {
		add_sample_a:
			merge_one_sample(*as, res);
			renumber_last_sample(res, cylinders_map_a);
			as++;
			continue;
		}

		/* Only samples from b? */
		if (at < 0) {
		add_sample_b:
			merge_one_sample(*bs, res);
			renumber_last_sample(res, cylinders_map_b);
			bs++;
			continue;
		}

		if (at < bt)
			goto add_sample_a;
		if (at > bt)
			goto add_sample_b;

		/* same-time sample: add a merged sample. Take the non-zero ones */
		struct sample sample = *bs;
		sample_renumber(sample, nullptr, cylinders_map_b);
		if (as->depth.mm)
			sample.depth = as->depth;
		if (as->temperature.mkelvin)
			sample.temperature = as->temperature;
		for (int j = 0; j < MAX_SENSORS; ++j) {
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

		merge_one_sample(sample, res);

		as++;
		bs++;
	}
}

static bool operator==(const struct extra_data &e1, const struct extra_data &e2)
{
	return std::tie(e1.key, e1.value) == std::tie(e2.key, e2.value);
}

/*
 * Merge extra_data.
 *
 * The extra data from 'a' has already been copied into 'res'. So
 * we really should just copy over the data from 'b' too.
 *
 * This is not hugely efficient (with the whole "check this for
 * every value you merge" it's O(n**2)) but it's not like we
 * have very many extra_data entries per dive computer anyway.
 */
static void merge_extra_data(struct divecomputer &res,
			  const struct divecomputer &a, const struct divecomputer &b)
{
	for (auto &ed: b.extra_data) {
		if (range_contains(a.extra_data, ed))
			continue;

		res.extra_data.push_back(ed);
	}
}

static std::string merge_text(const std::string &a, const std::string &b, const char *sep)
{
	if (a.empty())
		return b;
	if (b.empty())
		return a;
	if (a == b)
		return a;
	return a + sep + b;
}

#define SORT(a, b)  \
	if (a != b) \
		return a < b ? -1 : 1
#define SORT_FIELD(a, b, field) SORT(a.field, b.field)

static int sort_event(const struct event &a, const struct event &b, int time_a, int time_b)
{
	SORT(time_a, time_b);
	SORT_FIELD(a, b, type);
	SORT_FIELD(a, b, flags);
	SORT_FIELD(a, b, value);
	return a.name.compare(b.name);
}

static int same_gas(const struct event *a, const struct event *b)
{
	if (a->type == b->type && a->flags == b->flags && a->value == b->value && a->name == b->name &&
			same_gasmix(a->gas.mix, b->gas.mix)) {
		return true;
	}
	return false;
}

static void event_renumber(struct event &ev, const int mapping[]);
static void add_initial_gaschange(struct dive &dive, struct divecomputer &dc, int offset, int idx);

static void merge_events(struct dive &d, struct divecomputer &res,
			 const struct divecomputer &src1_in, const struct divecomputer &src2_in,
			 const int *cylinders_map1, const int *cylinders_map2,
			 int offset)
{
	const struct event *last_gas = NULL;

	/* Always use positive offsets */
	auto src1 = &src1_in;
	auto src2 = &src2_in;
	if (offset < 0) {
		offset = -offset;
		std::swap(src1, src2);
		std::swap(cylinders_map1, cylinders_map2); // The pointers, not the contents are swapped.
	}

	auto a = src1->events.begin();
	auto b = src2->events.begin();

	while (a != src1->events.end() || b != src2->events.end()) {
		int s = 0;
		const struct event *pick;
		const int *cylinders_map;
		int event_offset;

		if (b == src2->events.end())
			goto pick_a;

		if (a == src1->events.end())
			goto pick_b;

		s = sort_event(*a, *b, a->time.seconds, b->time.seconds + offset);

		/* Identical events? Just skip one of them (we skip a) */
		if (!s) {
			++a;
			continue;
		}

		/* Otherwise, pick the one that sorts first */
		if (s < 0) {
pick_a:
			pick = &*a;
			++a;
			event_offset = 0;
			cylinders_map = cylinders_map1;
		} else {
pick_b:
			pick = &*b;
			++b;
			event_offset = offset;
			cylinders_map = cylinders_map2;
		}

		/*
		 * If that's a gas-change that matches the previous
		 * gas change, we'll just skip it
		 */
		if (pick->is_gaschange()) {
			if (last_gas && same_gas(pick, last_gas))
				continue;
			last_gas = pick;
		}

		/* Add it to the target list */
		res.events.push_back(*pick);
		res.events.back().time.seconds += event_offset;
		event_renumber(res.events.back(), cylinders_map);
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
int get_cylinder_idx_by_use(const struct dive *dive, enum cylinderuse cylinder_use_type)
{
	auto it = std::find_if(dive->cylinders.begin(), dive->cylinders.end(), [cylinder_use_type]
			       (auto &cyl) { return cyl.cylinder_use == cylinder_use_type; });
	return it != dive->cylinders.end() ? it - dive->cylinders.begin() : -1;
}

/* Force an initial gaschange event to the (old) gas #0 */
static void add_initial_gaschange(struct dive &dive, struct divecomputer &dc, int offset, int idx)
{
	/* if there is a gaschange event up to 30 sec after the initial event,
	 * refrain from adding the initial event */
	event_loop loop("gaschange");
	while(auto ev = loop.next(dc)) {
		if (ev->time.seconds > offset + 30)
			break;
		else if (ev->time.seconds > offset)
			return;
	}

	/* Old starting gas mix */
	add_gas_switch_event(&dive, &dc, offset, idx);
}

static void sample_renumber(struct sample &s, const struct sample *prev, const int mapping[])
{
	for (int j = 0; j < MAX_SENSORS; j++) {
		int sensor = -1;

		if (s.sensor[j] != NO_SENSOR)
			sensor = mapping[s.sensor[j]];
		if (sensor == -1) {
			// Remove sensor and gas pressure info
			if (!prev) {
				s.sensor[j] = 0;
				s.pressure[j].mbar = 0;
			} else {
				s.sensor[j] = prev->sensor[j];
				s.pressure[j].mbar = prev->pressure[j].mbar;
			}
		} else {
			s.sensor[j] = sensor;
		}
	}
}

static void renumber_last_sample(struct divecomputer &dc, const int mapping[])
{
	if (dc.samples.empty())
		return;
	sample *prev = dc.samples.size() > 1 ? &dc.samples[dc.samples.size() - 2] : nullptr;
	sample_renumber(dc.samples.back(), prev, mapping);
}

static void event_renumber(struct event &ev, const int mapping[])
{
	if (!ev.is_gaschange())
		return;
	if (ev.gas.index < 0)
		return;
	ev.gas.index = mapping[ev.gas.index];
}

static void dc_cylinder_renumber(struct dive &dive, struct divecomputer &dc, const int mapping[])
{
	/* Remap or delete the sensor indices */
	for (auto [i, sample]: enumerated_range(dc.samples))
		sample_renumber(sample, i > 0 ? &dc.samples[i-1] : nullptr, mapping);

	/* Remap the gas change indices */
	for (auto &ev: dc.events)
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
void cylinder_renumber(struct dive &dive, int mapping[])
{
	for (auto &dc: dive.dcs)
		dc_cylinder_renumber(dive, dc, mapping);
}

int same_gasmix_cylinder(const cylinder_t &cyl, int cylid, const struct dive *dive, bool check_unused)
{
	struct gasmix mygas = cyl.gasmix;
	for (auto [i, cyl]: enumerated_range(dive->cylinders)) {
		if (i == cylid)
			continue;
		struct gasmix gas2 = cyl.gasmix;
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
static int match_cylinder(const cylinder_t *cyl, const struct dive &dive, const bool try_match[])
{
	for (auto [i, target]: enumerated_range(dive.cylinders)) {
		if (!try_match[i])
			continue;

		if (!same_gasmix(cyl->gasmix, target.gasmix))
			continue;
		if (cyl->cylinder_use != target.cylinder_use)
			continue;
		if (different_manual_pressures(cyl, &target))
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
	if (a->type.description.empty())
		a->type.description = b->type.description;

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

static bool cylinder_has_data(const cylinder_t &cyl)
{
	return !cyl.type.size.mliter &&
	       !cyl.type.workingpressure.mbar &&
	       cyl.type.description.empty() &&
	       !cyl.gasmix.o2.permille &&
	       !cyl.gasmix.he.permille &&
	       !cyl.start.mbar &&
	       !cyl.end.mbar &&
	       !cyl.sample_start.mbar &&
	       !cyl.sample_end.mbar &&
	       !cyl.gas_used.mliter &&
	       !cyl.deco_gas_used.mliter;
}

static bool cylinder_in_use(const struct dive *dive, int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= dive->cylinders.size())
		return false;

	/* This tests for gaschange events or pressure changes */
	if (is_cylinder_used(dive, idx) || prefs.include_unused_tanks)
		return true;

	/* This tests for typenames or gas contents */
	return cylinder_has_data(dive->cylinders[idx]);
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
static void merge_cylinders(struct dive &res, const struct dive &a, const struct dive &b,
			    int mapping_a[], int mapping_b[])
{
	size_t max_cylinders = a.cylinders.size() + b.cylinders.size();
	auto used_in_a = std::make_unique<bool[]>(max_cylinders);
	auto used_in_b = std::make_unique<bool[]>(max_cylinders);
	auto try_to_match = std::make_unique<bool[]>(max_cylinders);
	std::fill(try_to_match.get(), try_to_match.get() + max_cylinders, false);

	/* First, clear all cylinders in destination */
	res.cylinders.clear();

	/* Clear all cylinder mappings */
	std::fill(mapping_a, mapping_a + a.cylinders.size(), -1);
	std::fill(mapping_b, mapping_b + b.cylinders.size(), -1);

	/* Calculate usage map of cylinders, clear matching map */
	for (size_t i = 0; i < max_cylinders; i++) {
		used_in_a[i] = cylinder_in_use(&a, i);
		used_in_b[i] = cylinder_in_use(&b, i);
	}

	/*
	 * For each cylinder in 'a' that is used, copy it to 'res'.
	 * These are also potential matches for 'b' to use.
	 */
	for (size_t i = 0; i < max_cylinders; i++) {
		size_t res_nr = res.cylinders.size();
		if (!used_in_a[i])
			continue;
		mapping_a[i] = static_cast<int>(res_nr);
		try_to_match[res_nr] = true;
		res.cylinders.push_back(a.cylinders[i]);
	}

	/*
	 * For each cylinder in 'b' that is used, try to match it
	 * with an existing cylinder in 'res' from 'a'
	 */
	for (size_t i = 0; i < b.cylinders.size(); i++) {
		int j;

		if (!used_in_b[i])
			continue;

		j = match_cylinder(get_cylinder(&b, i), res, try_to_match.get());

		/* No match? Add it to the result */
		if (j < 0) {
			size_t res_nr = res.cylinders.size();
			mapping_b[i] = static_cast<int>(res_nr);
			res.cylinders.push_back(b.cylinders[i]);
			continue;
		}

		/* Otherwise, merge the result to the one we found */
		mapping_b[i] = j;
		merge_one_cylinder(get_cylinder(&res, j), get_cylinder(&b, i));

		/* Don't match the same target more than once */
		try_to_match[j] = false;
	}
}

/* Check whether a weightsystem table contains a given weightsystem */
static bool has_weightsystem(const weightsystem_table &t, const weightsystem_t &w)
{
	return any_of(t.begin(), t.end(), [&w] (auto &w2) { return same_weightsystem(w, w2); });
}

static void merge_equipment(struct dive &res, const struct dive &a, const struct dive &b)
{
	for (auto &ws: a.weightsystems) {
		if (!has_weightsystem(res.weightsystems, ws))
			res.weightsystems.push_back(ws);
	}
	for (auto &ws: b.weightsystems) {
		if (!has_weightsystem(res.weightsystems, ws))
			res.weightsystems.push_back(ws);
	}
}

static void merge_temperatures(struct dive &res, const struct dive &a, const struct dive &b)
{
	temperature_t airtemp_a = un_fixup_airtemp(a);
	temperature_t airtemp_b = un_fixup_airtemp(b);
	res.airtemp = airtemp_a.mkelvin ? airtemp_a : airtemp_b;
	MERGE_NONZERO(&res, &a, &b, watertemp.mkelvin);
}

/*
 * Pick a trip for a dive
 */
static struct dive_trip *get_preferred_trip(const struct dive *a, const struct dive *b)
{
	dive_trip *atrip, *btrip;

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
	if (atrip->location.empty())
		return btrip;
	if (btrip->location.empty())
		return atrip;
	if (atrip->notes.empty())
		return btrip;
	if (btrip->notes.empty())
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
 * If 's' and 'a' are at the same time, offset is 0.
 */
static int compare_sample(const struct sample &s, const struct sample &a, const struct sample &b, int offset)
{
	unsigned int depth = a.depth.mm;
	int diff;

	if (offset) {
		unsigned int interval = b.time.seconds - a.time.seconds;
		unsigned int depth_a = a.depth.mm;
		unsigned int depth_b = b.depth.mm;

		if (offset > interval)
			return -1;

		/* pick the average depth, scaled by the offset from 'b' */
		depth = (depth_a * offset) + (depth_b * (interval - offset));
		depth /= interval;
	}
	diff = s.depth.mm - depth;
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
	if (a->samples.empty() || b->samples.empty())
		return;

	unsigned long error = 0;
	int start = -1;

	if (!asamples || !bsamples)
		return 0;

	/*
	 * skip the first sample - this way we know can always look at
	 * as/bs[-1] to look at the samples around it in the loop.
	 */
	auto as = a->samples.begin() + 1;
	auto bs = a->samples.begin() + 1;

	for (;;) {
		/* If we run out of samples, punt */
		if (as == a->samples.end())
			return INT_MAX;
		if (bs == b->samples.end())
			return INT_MAX;

		int at = as->time.seconds;
		int bt = bs->time.seconds + offset;

		/* b hasn't started yet? Ignore it */
		if (bt < 0) {
			++bs;
			continue;
		}

		int diff;
		if (at < bt) {
			diff = compare_sample(*as, *std::prev(bs), *bs, bt - at);
			++as;
		} else if (at > bt) {
			diff = compare_sample(*bs, *std::prev(as), *as, at - bt);
			++bs;
		} else {
			diff = compare_sample(*as, *bs, *bs, 0);
			++as;
			++bs;
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
	/* No samples? Merge at any time (0 offset) */
	if (a->samples.empty())
		return 0;
	if (b->samples.empty())
		return 0;

	/*
	 * Common special-case: merging a dive that came from
	 * the same dive computer, so the samples are identical.
	 * Check this first, without wasting time trying to find
	 * some minimal offset case.
	 */
	int best = 0;
	unsigned long max = sample_difference(a, b, 0);
	if (!max)
		return 0;

	/*
	 * Otherwise, look if we can find anything better within
	 * a thirty second window..
	 */
	for (int offset = -30; offset <= 30; offset++) {
		unsigned long diff;

		int diff = sample_difference(a, b, offset);
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
static int match_dc_dive(const struct dive &a, const struct dive &b)
{
	for (auto &dc1: a.dcs) {
		for (auto &dc2: b.dcs) {
			int match = match_one_dc(dc1, dc2);
			if (match)
				return match;
		}
	}
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
static bool likely_same_dive(const struct dive &a, const struct dive &b)
{
	/* don't merge manually added dives with anything */
	if (is_dc_manually_added_dive(&a.dcs[0]) ||
	    is_dc_manually_added_dive(&b.dcs[0]))
		return 0;

	/*
	 * Do some basic sanity testing of the values we
	 * have filled in during 'fixup_dive()'
	 */
	if (!similar(a.maxdepth.mm, b.maxdepth.mm, 1000) ||
	    (a.meandepth.mm && b.meandepth.mm && !similar(a.meandepth.mm, b.meandepth.mm, 1000)) ||
	    !a.duration.seconds || !b.duration.seconds ||
	    !similar(a.duration.seconds, b.duration.seconds, 5 * 60))
		return 0;

	/* See if we can get an exact match on the dive computer */
	if (match_dc_dive(a, b))
		return true;

	/*
	 * Allow a time difference due to dive computer time
	 * setting etc. Check if they overlap.
	 */
	int fuzz = std::max(a.duration.seconds, b.duration.seconds) / 2;
	fuzz = std::max(fuzz, 60);

	return (a.when <= b.when + fuzz) && (a.when >= b.when - fuzz);
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
struct std::unique_ptr<dive> try_to_merge(const struct dive &a, const struct dive &b, bool prefer_downloaded)
{
	if (!likely_same_dive(a, b))
		return {};

	auto [res, trip, site] = merge_dives(a, b, 0, prefer_downloaded);
	res->dive_site = site; /* Caller has to call site->add_dive()! */
	return std::move(res);
}

static bool operator==(const sample &a, const sample &b)
{
	if (a.time.seconds != b.time.seconds)
		return false;
	if (a.depth.mm != b.depth.mm)
		return false;
	if (a.temperature.mkelvin != b.temperature.mkelvin)
		return false;
	if (a.pressure[0].mbar != b.pressure[0].mbar)
		return false;
	return a.sensor[0] == b.sensor[0];
}

static int same_dc(const struct divecomputer &a, const struct divecomputer &b)
{
	int i;
	i = match_one_dc(a, b);
	if (i)
		return i > 0;

	if (a.when && b.when && a.when != b.when)
		return 0;
	if (a.samples != b.samples)
		return 0;
	return a.events == b.events;
}

static int might_be_same_device(const struct divecomputer &a, const struct divecomputer &b)
{
	/* No dive computer model? That matches anything */
	if (a.model.empty() || b.model.empty())
		return 1;

	/* Otherwise at least the model names have to match */
	if (strcasecmp(a.model.c_str(), b.model.c_str()))
		return 0;

	/* No device ID? Match */
	if (!a.deviceid || !b.deviceid)
		return 1;

	return a.deviceid == b.deviceid;
}

static void remove_redundant_dc(struct dive &d, bool prefer_downloaded)
{
	// Note: since the vector doesn't grow and we only erase
	// elements after the iterator, this is fine.
	for (auto it = d.dcs.begin(); it != d.dcs.end(); ++it) {
		// Remove all following DCs that compare as equal.
		// Use the (infamous) erase-remove idiom.
		auto it2 = std::remove_if(std::next(it), d.dcs.end(),
			    [d, prefer_downloaded, &it] (const divecomputer &dc) {
				return same_dc(*it, dc) ||
				       (prefer_downloaded && might_be_same_device(*it, dc));
			    });
		d.dcs.erase(it2, d.dcs.end());

		prefer_downloaded = false;
	}
}

static const struct divecomputer *find_matching_computer(const struct divecomputer &match, const struct dive &d)
{
	for (const auto &dc: d.dcs) {
		if (might_be_same_device(match, dc))
			return &dc;
	}
	return nullptr;
}

static void copy_dive_computer(struct divecomputer &res, const struct divecomputer &a)
{
	res = a;
	res.samples.clear();
	res.events.clear();
}

/*
 * Join dive computers with a specific time offset between
 * them.
 *
 * Use the dive computer ID's (or names, if ID's are missing)
 * to match them up. If we find a matching dive computer, we
 * merge them. If not, we just take the data from 'a'.
 */
static void interleave_dive_computers(struct dive &res,
				      const struct dive &a, const struct dive &b,
				      const int cylinders_map_a[], const int cylinders_map_b[],
				      int offset)
{
	res.dcs.clear();
	for (const auto &dc1: a.dcs) {
		res.dcs.emplace_back();
		divecomputer &newdc = res.dcs.back();
		copy_dive_computer(newdc, dc1);
		const divecomputer *match = find_matching_computer(dc1, b);
		if (match) {
			merge_events(res, newdc, dc1, *match, cylinders_map_a, cylinders_map_b, offset);
			merge_samples(newdc, dc1, *match, cylinders_map_a, cylinders_map_b, offset);
			merge_extra_data(newdc, dc1, *match);
			/* Use the diveid of the later dive! */
			if (offset > 0)
				newdc.diveid = match->diveid;
		} else {
			dc_cylinder_renumber(res, res.dcs.back(), cylinders_map_a);
		}
	}
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
static void join_dive_computers(struct dive &d,
				const struct dive &a, const struct dive &b,
				const int cylinders_map_a[], const int cylinders_map_b[],
				bool prefer_downloaded)
{
	d.dcs.clear();
	if (!a.dcs[0].model.empty() && b.dcs[0].model.empty()) {
		copy_dc_renumber(d, a, cylinders_map_a);
		return;
	}
	if (!b.dcs[0].model.empty() && a.dcs[0].model.empty()) {
		copy_dc_renumber(d, b, cylinders_map_b);
		return;
	}

	copy_dc_renumber(d, a, cylinders_map_a);
	copy_dc_renumber(d, b, cylinders_map_b);

	remove_redundant_dc(d, prefer_downloaded);
}

static bool has_dc_type(const struct dive &dive, bool dc_is_planner)
{
	return std::any_of(dive.dcs.begin(), dive.dcs.end(),
			   [dc_is_planner] (const divecomputer &dc)
			   { return is_dc_planner(&dc) == dc_is_planner; });
}

// Does this dive have a dive computer for which is_dc_planner has value planned
bool dive::is_planned() const
{
	return has_dc_type(*this, true);
}

bool dive::is_logged() const
{
	return has_dc_type(*this, false);
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
merge_result merge_dives(const struct dive &a_in, const struct dive &b_in, int offset, bool prefer_downloaded)
{
	merge_result res = { std::make_unique<dive>(), nullptr, nullptr };

	if (offset) {
		/*
		 * If "likely_same_dive()" returns true, that means that
		 * it is *not* the same dive computer, and we do not want
		 * to try to turn it into a single longer dive. So we'd
		 * join them as two separate dive computers at zero offset.
		 */
		if (likely_same_dive(a_in, b_in))
			offset = 0;
	}

	const dive *a = &a_in;
	const dive *b = &b_in;
	if (is_dc_planner(&a->dcs[0]))
		std::swap(a, b);

	res.dive->when = prefer_downloaded ? b->when : a->when;
	res.dive->selected = a->selected || b->selected;
	res.trip = get_preferred_trip(a, b);
	MERGE_TXT(res.dive, a, b, notes, "\n--\n");
	MERGE_TXT(res.dive, a, b, buddy, ", ");
	MERGE_TXT(res.dive, a, b, diveguide, ", ");
	MERGE_MAX(res.dive, a, b, rating);
	MERGE_TXT(res.dive, a, b, suit, ", ");
	MERGE_MAX(res.dive, a, b, number);
	MERGE_NONZERO(res.dive, a, b, visibility);
	MERGE_NONZERO(res.dive, a, b, wavesize);
	MERGE_NONZERO(res.dive, a, b, current);
	MERGE_NONZERO(res.dive, a, b, surge);
	MERGE_NONZERO(res.dive, a, b, chill);
	res.dive->pictures = !a->pictures.empty() ? a->pictures : b->pictures;
	res.dive->tags = taglist_merge(a->tags, b->tags);
	/* if we get dives without any gas / cylinder information in an import, make sure
	 * that there is at leatst one entry in the cylinder map for that dive */
	auto cylinders_map_a = std::make_unique<int[]>(std::max(size_t(1), a->cylinders.size()));
	auto cylinders_map_b = std::make_unique<int[]>(std::max(size_t(1), b->cylinders.size()));
	merge_cylinders(*res.dive, *a, *b, cylinders_map_a.get(), cylinders_map_b.get());
	merge_equipment(*res.dive, *a, *b);
	merge_temperatures(*res.dive, *a, *b);
	if (prefer_downloaded) {
		/* If we prefer downloaded, do those first, and get rid of "might be same" computers */
		join_dive_computers(*res.dive, *b, *a, cylinders_map_b.get(), cylinders_map_a.get(), true);
	} else if (offset && might_be_same_device(a->dcs[0], b->dcs[0])) {
		interleave_dive_computers(*res.dive, *a, *b, cylinders_map_a.get(), cylinders_map_b.get(), offset);
	} else {
		join_dive_computers(*res.dive, *a, *b, cylinders_map_a.get(), cylinders_map_b.get(), false);
	}

	/* The CNS values will be recalculated from the sample in fixup_dive() */
	res.dive->cns = res.dive->maxcns = 0;

	/* we take the first dive site, unless it's empty */
	res.site = a->dive_site && !a->dive_site->is_empty() ? a->dive_site : b->dive_site;
	if (!dive_site_has_gps_location(res.site) && dive_site_has_gps_location(b->dive_site)) {
		/* we picked the first dive site and that didn't have GPS data, but the new dive has
		 * GPS data (that could be a download from a GPS enabled dive computer).
		 * Keep the dive site, but add the GPS data */
		res.site->location = b->dive_site->location;
	}
	fixup_dive(res.dive.get());
	return res;
}

struct start_end_pressure {
	pressure_t start;
	pressure_t end;
};

static void force_fixup_dive(struct dive *d)
{
	struct divecomputer *dc = &d->dcs[0];
	int old_temp = dc->watertemp.mkelvin;
	int old_mintemp = d->mintemp.mkelvin;
	int old_maxtemp = d->maxtemp.mkelvin;
	duration_t old_duration = d->duration;
	std::vector<start_end_pressure> old_pressures(d->cylinders.size());

	d->maxdepth.mm = 0;
	dc->maxdepth.mm = 0;
	d->watertemp.mkelvin = 0;
	dc->watertemp.mkelvin = 0;
	d->duration.seconds = 0;
	d->maxtemp.mkelvin = 0;
	d->mintemp.mkelvin = 0;
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		old_pressures[i].start = cyl.start;
		old_pressures[i].end = cyl.end;
		cyl.start.mbar = 0;
		cyl.end.mbar = 0;
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
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (!cyl.start.mbar)
			cyl.start = old_pressures[i].start;
		if (!cyl.end.mbar)
			cyl.end = old_pressures[i].end;
	}
}

/*
 * Split a dive that has a surface interval from samples 'a' to 'b'
 * into two dives, but don't add them to the log yet.
 * Returns the nr of the old dive or <0 on failure.
 * Moreover, on failure both output dives are set to NULL.
 * On success, the newly allocated dives are returned in out1 and out2.
 */
static std::array<std::unique_ptr<dive>, 2> split_dive_at(const struct dive &dive, int a, int b)
{
	size_t nr = divelog.dives.get_idx(&dive);

	/* if we can't find the dive in the dive list, don't bother */
	if (nr == std::string::npos)
		return {};

	/* Splitting should leave at least 3 samples per dive */
	if (a < 3 || static_cast<size_t>(b + 4) > dive.dcs[0].samples.size())
		return {};

	/* We're not trying to be efficient here.. */
	auto d1 = std::make_unique<struct dive>(dive);
	auto d2 = std::make_unique<struct dive>(dive);
	d1->id = dive_getUniqID();
	d2->id = dive_getUniqID();
	d1->divetrip = d2->divetrip = nullptr;

	/* now unselect the first first segment so we don't keep all
	 * dives selected by mistake. But do keep the second one selected
	 * so the algorithm keeps splitting the dive further */
	d1->selected = false;

	struct divecomputer &dc1 = d1->dcs[0];
	struct divecomputer &dc2 = d2->dcs[0];
	/*
	 * Cut off the samples of d1 at the beginning
	 * of the interval.
	 */
	dc1.samples.resize(a);

	/* And get rid of the 'b' first samples of d2 */
	dc2.samples.erase(dc2.samples.begin(), dc2.samples.begin() + b);

	/* Now the secondary dive computers */
	int32_t t = dc2.samples[0].time.seconds;
	for (auto it1 = d1->dcs.begin() + 1; it1 != d1->dcs.end(); ++it1) {
		auto it = std::find_if(it1->samples.begin(), it1->samples.end(),
				       [t](auto &sample) { return sample.time.seconds >= t; });
		it1->samples.erase(it, it1->samples.end());
	}
	for (auto it2 = d2->dcs.begin() + 1; it2 != d2->dcs.end(); ++it2) {
		auto it = std::find_if(it2->samples.begin(), it2->samples.end(),
				       [t](auto &sample) { return sample.time.seconds >= t; });
		it2->samples.erase(it2->samples.begin(), it);
	}

	/*
	 * This is where we cut off events from d1,
	 * and shift everything in d2
	 */
	d2->when += t;
	auto it1 = d1->dcs.begin();
	auto it2 = d2->dcs.begin();
	while (it1 != d1->dcs.end() && it2 != d2->dcs.end()) {
		it2->when += t;
		for (auto &sample: it2->samples)
			sample.time.seconds -= t;

		/* Remove the events past 't' from d1 */
		auto it = std::lower_bound(it1->events.begin(), it1->events.end(), t,
					   [] (struct event &ev, int t)
					   { return ev.time.seconds < t; });
		it1->events.erase(it, it1->events.end());

		/* Remove the events before 't' from d2, and shift the rest */
		it = std::lower_bound(it2->events.begin(), it2->events.end(), t,
				      [] (struct event &ev, int t)
				      { return ev.time.seconds < t; });
		it2->events.erase(it2->events.begin(), it);
		for (auto &ev: it2->events)
			ev.time.seconds -= t;

		++it1;
		++it2;
	}

	force_fixup_dive(d1.get());
	force_fixup_dive(d2.get());

	/*
	 * Was the dive numbered? If it was the last dive, then we'll
	 * increment the dive number for the tail part that we split off.
	 * Otherwise the tail is unnumbered.
	 */
	if (d2->number) {
		if (divelog.dives.size() == nr + 1)
			d2->number++;
		else
			d2->number = 0;
	}

	return { std::move(d1), std::move(d2) };
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
std::array<std::unique_ptr<dive>, 2> split_dive(const struct dive &dive)
{
	const struct divecomputer *dc = &dive.dcs[0];
	bool at_surface = true;
	if (dc->samples.empty())
		return {};
	auto surface_start = dc->samples.begin();
	for (auto it = dc->samples.begin() + 1; it != dc->samples.end(); ++it) {
		bool surface_sample = it->depth.mm < SURFACE_THRESHOLD;

		/*
		 * We care about the transition from and to depth 0,
		 * not about the depth staying similar.
		 */
		if (at_surface == surface_sample)
			continue;
		at_surface = surface_sample;

		// Did it become surface after having been non-surface? We found the start
		if (at_surface) {
			surface_start = it;
			continue;
		}

		// Going down again? We want at least a minute from
		// the surface start.
		if (surface_start == dc->samples.begin())
			continue;
		if (!should_split(dc, surface_start->time.seconds, std::prev(it)->time.seconds))
			continue;

		return split_dive_at(dive, surface_start - dc->samples.begin(), it - dc->samples.begin() - 1);
	}
	return {};
}

std::array<std::unique_ptr<dive>, 2> split_dive_at_time(const struct dive &dive, duration_t time)
{
	auto it = std::find_if(dive.dcs[0].samples.begin(), dive.dcs[0].samples.end(),
			       [time](auto &sample) { return sample.time.seconds >= time.seconds; });
	if (it == dive.dcs[0].samples.end())
		return {};
	size_t idx = it - dive.dcs[0].samples.begin();
	if (idx < 1)
		return {};
	return split_dive_at(dive, static_cast<int>(idx), static_cast<int>(idx - 1));
}

/*
 * "dc_maxtime()" is how much total time this dive computer
 * has for this dive. Note that it can differ from "duration"
 * if there are surface events in the middle.
 *
 * Still, we do ignore all but the last surface samples from the
 * end, because some divecomputers just generate lots of them.
 */
static inline int dc_totaltime(const struct divecomputer &dc)
{
	int time = dc.duration.seconds;

	for (auto it = dc.samples.rbegin(); it != dc.samples.rend(); ++it) {
		time = it->time.seconds;
		if (it->depth.mm >= SURFACE_THRESHOLD)
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
duration_t dive::totaltime() const
{
	int time =  duration.seconds;

	bool logged = is_logged();
	for (auto &dc: dcs) {
		if (logged || !is_dc_planner(&dc)) {
			int dc_time = dc_totaltime(dc);
			if (dc_time > time)
				time = dc_time;
		}
	}
	return { time };
}

timestamp_t dive::endtime() const
{
	return when + totaltime().seconds;
}

bool time_during_dive_with_offset(const struct dive *dive, timestamp_t when, timestamp_t offset)
{
	timestamp_t start = dive->when;
	timestamp_t end = dive->endtime();
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
			git_prefs.units.length = units::METERS;
		if (strstr(units, "FEET"))
			git_prefs.units.length = units::FEET;
		if (strstr(units, "LITER"))
			git_prefs.units.volume = units::LITER;
		if (strstr(units, "CUFT"))
			git_prefs.units.volume = units::CUFT;
		if (strstr(units, "BAR"))
			git_prefs.units.pressure = units::BAR;
		if (strstr(units, "PSI"))
			git_prefs.units.pressure = units::PSI;
		if (strstr(units, "CELSIUS"))
			git_prefs.units.temperature = units::CELSIUS;
		if (strstr(units, "FAHRENHEIT"))
			git_prefs.units.temperature = units::FAHRENHEIT;
		if (strstr(units, "KG"))
			git_prefs.units.weight = units::KG;
		if (strstr(units, "LBS"))
			git_prefs.units.weight = units::LBS;
		if (strstr(units, "SECONDS"))
			git_prefs.units.vertical_speed_time = units::SECONDS;
		if (strstr(units, "MINUTES"))
			git_prefs.units.vertical_speed_time = units::MINUTES;
	}

}

/* clones a dive and moves given dive computer to front */
std::unique_ptr<dive> clone_make_first_dc(const struct dive &d, int dc_number)
{
	/* copy the dive */
	auto res = std::make_unique<dive>(d);

	/* make a new unique id, since we still can't handle two equal ids */
	res->id = dive_getUniqID();

	if (dc_number != 0)
		move_in_range(res->dcs, dc_number, dc_number + 1, 0);

	return res;
}

/* Clone a dive and delete given dive computer */
std::unique_ptr<dive> clone_delete_divecomputer(const struct dive &d, int dc_number)
{
	/* copy the dive */
	auto res = std::make_unique<dive>(d);

	/* make a new unique id, since we still can't handle two equal ids */
	res->id = dive_getUniqID();

	if (res->dcs.size() <= 1)
		return res;

	if (dc_number < 0 || static_cast<size_t>(dc_number) >= res->dcs.size())
		return res;

	res->dcs.erase(res->dcs.begin() + dc_number);

	force_fixup_dive(res.get());

	return res;
}

/*
 * This splits the dive src by dive computer. The first output dive has all
 * dive computers except num, the second only dive computer num.
 * The dives will not be associated with a trip.
 * On error, both output parameters are set to NULL.
 */
std::array<std::unique_ptr<dive>, 2> split_divecomputer(const struct dive &src, int num)
{
	if (num < 0 || src.dcs.size() < 2 || static_cast<size_t>(num) >= src.dcs.size())
		return {};

	// Copy the dive with full divecomputer list
	auto out1 = std::make_unique<dive>(src);

	// Remove all DCs, stash them and copy the dive again.
	// Then, we have to dives without DCs and a list of DCs.
	std::vector<divecomputer> dcs;
	std::swap(out1->dcs, dcs);
	auto out2 = std::make_unique<dive>(*out1);

	// Give the dives new unique ids and remove them from the trip.
	out1->id = dive_getUniqID();
	out2->id = dive_getUniqID();
	out1->divetrip = out2->divetrip = NULL;

	// Now copy the divecomputers
	out1->dcs.reserve(src.dcs.size() - 1);
	for (auto [idx, dc]: enumerated_range(dcs)) {
		auto &dcs = idx == num ? out2->dcs : out1->dcs;
		dcs.push_back(std::move(dc));
	}

	// Recalculate gas data, etc.
	fixup_dive(out1.get());
	fixup_dive(out2.get());

	return { std::move(out1), std::move(out2) };
}

//Calculate O2 in best mix
fraction_t best_o2(depth_t depth, const struct dive *dive, bool in_planner)
{
	fraction_t fo2;
	int po2 = in_planner ? prefs.bottompo2 : (int)(prefs.modpO2 * 1000.0);

	fo2.permille = (po2 * 100 / dive->depth_to_mbar(depth.mm)) * 10;	//use integer arithmetic to round down to nearest percent
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
	pnarcotic = dive->depth_to_mbar(prefs.bestmixend.mm);
	ambient = dive->depth_to_mbar(depth.mm);
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

int dive::depth_to_mbar(int depth) const
{
	return lrint(depth_to_mbarf(depth));
}

double dive::depth_to_mbarf(int depth) const
{
	// For downloaded and planned dives, use DC's values
	int salinity = dcs[0].salinity;
	pressure_t surface_pressure = dcs[0].surface_pressure;

	if (is_dc_manually_added_dive(&dcs[0])) { // For manual dives, salinity and pressure in another place...
		surface_pressure = this->surface_pressure;
		salinity = user_salinity;
	}
	return calculate_depth_to_mbarf(depth, surface_pressure, salinity);
}

double dive::depth_to_bar(int depth) const
{
	return depth_to_mbar(depth) / 1000.0;
}

double dive::depth_to_atm(int depth) const
{
	return mbar_to_atm(depth_to_mbar(depth));
}

/* for the inverse calculation we use just the relative pressure
 * (that's the one that some dive computers like the Uemis Zurich
 * provide - for the other models that do this libdivecomputer has to
 * take care of this, but the Uemis we support natively */
int dive::rel_mbar_to_depth(int mbar) const
{
	// For downloaded and planned dives, use DC's salinity. Manual dives, use user's salinity
	int salinity = is_dc_manually_added_dive(&dcs[0]) ? user_salinity : dcs[0].salinity;
	if (!salinity)
		salinity = SEAWATER_SALINITY;

	/* whole mbar gives us cm precision */
	double specific_weight = salinity_to_specific_weight(salinity);
	return (int)lrint(mbar / specific_weight);
}

int dive::mbar_to_depth(int mbar) const
{
	// For downloaded and planned dives, use DC's pressure. Manual dives, use user's pressure
	pressure_t surface_pressure = is_dc_manually_added_dive(&dcs[0])
		? this->surface_pressure
		: dcs[0].surface_pressure;

	if (!surface_pressure.mbar)
		surface_pressure.mbar = SURFACE_PRESSURE;
		
	return rel_mbar_to_depth(mbar - surface_pressure.mbar);
}

/* MOD rounded to multiples of roundto mm */
depth_t gas_mod(struct gasmix mix, pressure_t po2_limit, const struct dive *dive, int roundto)
{
	depth_t rounded_depth;

	double depth = (double) dive->mbar_to_depth(po2_limit.mbar * 1000 / get_o2(mix));
	rounded_depth.mm = (int)lrint(depth / roundto) * roundto;
	return rounded_depth;
}

/* Maximum narcotic depth rounded to multiples of roundto mm */
depth_t gas_mnd(struct gasmix mix, depth_t end, const struct dive *dive, int roundto)
{
	depth_t rounded_depth;
	pressure_t ppo2n2;
	ppo2n2.mbar = dive->depth_to_mbar(end.mm);

	int maxambient = prefs.o2narcotic ?
					(int)lrint(ppo2n2.mbar / (1 - get_he(mix) / 1000.0))
			      :
					get_n2(mix) > 0 ?
						(int)lrint(ppo2n2.mbar * N2_IN_AIR / get_n2(mix))
					:
						// Actually: Infinity
						1000000;
	rounded_depth.mm = (int)lrint(((double)dive->mbar_to_depth(maxambient)) / roundto) * roundto;
	return rounded_depth;
}

struct dive_site *get_dive_site_for_dive(const struct dive *dive)
{
	return dive->dive_site;
}

std::string get_dive_country(const struct dive *dive)
{
	struct dive_site *ds = dive->dive_site;
	return ds ? taxonomy_get_country(ds->taxonomy) : std::string();
}

std::string get_dive_location(const struct dive *dive)
{
	const struct dive_site *ds = dive->dive_site;
	return ds ? ds->name : std::string();
}

unsigned int number_of_computers(const struct dive *dive)
{
	return dive ? static_cast<int>(dive->dcs.size()) : 1;
}

struct divecomputer *get_dive_dc(struct dive *dive, int nr)
{
	if (!dive || dive->dcs.empty())
		return NULL;
	nr = std::max(0, nr);
	return &dive->dcs[static_cast<size_t>(nr) % dive->dcs.size()];
}

const struct divecomputer *get_dive_dc(const struct dive *dive, int nr)
{
	return get_dive_dc((struct dive *)dive, nr);
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
	location_t res;

	for (const auto &data: dc->extra_data) {
		if (data.key == "GPS1") {
			parse_location(data.value.c_str(), &res);
			/* If we found a valid GPS1 field exit early since
			 * it has priority over GPS2 */
			if (has_location(&res))
				break;
		} else if (data.key == "GPS2") {
			/* For GPS2 fields continue searching, as we might
			 * still find a GPS1 field */
			parse_location(data.value.c_str(), &res);
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
	for (const struct divecomputer &dc: d->dcs) {
		location_t res = dc_get_gps_location(&dc);
		if (has_location(&res))
			return res;
	}

	/* No libdivecomputer generated GPS data found.
	 * Let's use the location of the current dive site.
	 */
	if (d->dive_site)
		return d->dive_site->location;

	return location_t();
}

gasmix_loop::gasmix_loop(const struct dive &d, const struct divecomputer &dc) :
	dive(d), dc(dc), last(gasmix_air), loop("gaschange")
{
	/* if there is no cylinder, return air */
	if (dive.cylinders.empty())
		return;

	/* on first invocation, get initial gas mix and first event (if any) */
	int cyl = explicit_first_cylinder(&dive, &dc);
	last = get_cylinder(&dive, cyl)->gasmix;
	ev = loop.next(dc);
}

gasmix gasmix_loop::next(int time)
{
	/* if there is no cylinder, return air */
	if (dive.cylinders.empty())
		return last;

	while (ev && ev->time.seconds <= time) {
		last = get_gasmix_from_event(&dive, *ev);
		ev = loop.next(dc);
	}
	return last;
}

/* get the gas at a certain time during the dive */
/* If there is a gasswitch at that time, it returns the new gasmix */
struct gasmix get_gasmix_at_time(const struct dive &d, const struct divecomputer &dc, duration_t time)
{
	return gasmix_loop(d, dc).next(time.seconds);
}

/* Does that cylinder have any pressure readings? */
bool cylinder_with_sensor_sample(const struct dive *dive, int cylinder_id)
{
	for (const auto &dc: dive->dcs) {
		for (const auto &sample: dc.samples) {
			for (int j = 0; j < MAX_SENSORS; ++j) {
				if (!sample.pressure[j].mbar)
					continue;
				if (sample.sensor[j] == cylinder_id)
					return true;
			}
		}
	}
	return false;
}

/*
 * What do the dive computers say the water temperature is?
 * (not in the samples, but as dc property for dcs that support that)
 */
temperature_t dive::dc_watertemp() const
{
	int sum = 0, nr = 0;

	for (auto &dc: dcs) {
		if (dc.watertemp.mkelvin) {
			sum += dc.watertemp.mkelvin;
			nr++;
		}
	}
	if (!nr)
		return temperature_t();
	return temperature_t{ static_cast<uint32_t>((sum + nr / 2) / nr) };
}

/*
 * What do the dive computers say the air temperature is?
 */
temperature_t dive::dc_airtemp() const
{
	int sum = 0, nr = 0;

	for (auto &dc: dcs) {
		if (dc.airtemp.mkelvin) {
			sum += dc.airtemp.mkelvin;
			nr++;
		}
	}
	if (!nr)
		return temperature_t();
	return temperature_t{ static_cast<uint32_t>((sum + nr / 2) / nr) };
}
