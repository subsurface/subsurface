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
#include "divecomputer.h"
#include "divelist.h"
#include "divesite.h"
#include "equipment.h"
#include "errorhelper.h"
#include "event.h"
#include "extradata.h"
#include "format.h"
#include "fulltext.h"
#include "interpolate.h"
#include "qthelper.h"
#include "membuffer.h"
#include "picture.h"
#include "range.h"
#include "sample.h"
#include "tag.h"
#include "tanksensormapping.h"
#include "trip.h"

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

/* get_cylinder_idx_by_use(): Find the index of the first cylinder with a particular CCR use type.
 * The index returned corresponds to that of the first cylinder with a cylinder_use that
 * equals the appropriate enum value [oxygen, diluent, bailout] given by cylinder_use_type.
 * A negative number returned indicates that a match could not be found.
 * Call parameters: dive = the dive being processed
 *		  cylinder_use_type = an enum, one of {oxygen, diluent, bailout} */
static int get_cylinder_idx_by_use(const struct dive &dive, enum cylinderuse cylinder_use_type)
{
	auto it = std::find_if(dive.cylinders.begin(), dive.cylinders.end(), [cylinder_use_type]
			       (auto &cyl) { return cyl.cylinder_use == cylinder_use_type; });
	return it != dive.cylinders.end() ? it - dive.cylinders.begin() : -1;
}

/* access to cylinders is controlled by two functions:
 * - get_cylinder() returns the cylinder of a dive and supposes that
 *   the cylinder with the given index exists. If it doesn't, an error
 *   message is printed and the "surface air" cylinder returned.
 *   (NOTE: this MUST not be written into!).
 * - get_or_create_cylinder() creates an empty cylinder if it doesn't exist.
 *   Multiple cylinders might be created if the index is bigger than the
 *   number of existing cylinders
 */
cylinder_t *dive::get_cylinder(int idx)
{
	return &cylinders[idx];
}

const cylinder_t *dive::get_cylinder(int idx) const
{
	return &cylinders[idx];
}

static int get_libdivecomputer_gasmix_value(const struct gasmix &mix)
{
	int o2 = get_o2(mix);
	int he = get_he(mix);

	/* Convert to odd libdivecomputer format */
	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;

	return o2 + (he << 16);
}

/* warning: does not test idx for validity */
struct event create_gas_switch_event(struct dive *dive, int seconds, int idx)
{
	/* The gas switch event format is insane for historical reasons */
	struct gasmix mix = dive->get_cylinder(idx)->gasmix;

	struct event ev(seconds, get_he(mix) ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE, 0, get_libdivecomputer_gasmix_value(mix), "gaschange");
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
	struct event ev = create_gas_switch_event(dive, seconds, idx);
	add_event_to_dc(dc, std::move(ev));
}

std::pair<const struct gasmix, divemode_t> dive::get_gasmix_from_event(const struct event &ev, const struct divecomputer &dc) const
{
	if (ev.is_gaschange()) {
		int index = ev.gas.index;
		// FIXME: The planner uses one past cylinder-count to signify "surface air". Remove in due course.
		if (index >= 0 && static_cast<size_t>(index) < cylinders.size() + 1) {
			const cylinder_t *cylinder = get_cylinder(index);
			return std::make_pair(cylinder->gasmix, get_effective_divemode(dc, *cylinder));
		}
		return std::make_pair(ev.gas.mix, dc.divemode);
	}
	return std::make_pair(gasmix_air, dc.divemode);
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

void dive::clear()
{
	*this = dive();
}

/* make a true copy that is independent of the source dive;
 * all data structures are duplicated, so the copy can be modified without
 * any impact on the source */
void copy_dive(const struct dive *s, struct dive *d)
{
	/* simply copy things over, but then clear the dive cache. */
	*d = *s;
	d->invalidate_cache();
}

/* copies all events from the given dive computer before a given time
   this is used when editing a dive in the planner to preserve the events
   of the old dive */
void copy_events_until(const struct dive *sd, struct dive *dd, int dcNr, int time)
{
	if (!sd || !dd)
		return;

	const struct divecomputer *s = &sd->dcs[0];
	struct divecomputer *d = dd->get_dc(dcNr);

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
		if (!used_only || s->is_cylinder_used(i) || s->get_cylinder(i)->cylinder_use == NOT_USED)
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

/*
 * If the event has an explicit cylinder index,
 * we return that. If it doesn't, we return the best
 * match based on the gasmix.
 *
 * Some dive computers give cylinder indices, some
 * give just the gas mix.
 */
int dive::get_cylinder_index(const struct event &ev, const struct divecomputer &dc) const
{
	if (ev.gas.index >= 0)
		return ev.gas.index;

	/*
	 * This should no longer happen!
	 *
	 * We now match up gas change events with their cylinders at dive
	 * event fixup time.
	 */
	report_info("Still looking up cylinder based on gas mix in get_cylinder_index()!");

	gasmix mix = get_gasmix_from_event(ev, dc).first;
	int best = find_best_gasmix_match(mix, cylinders, nullptr);
	return best < 0 ? 0 : best;
}

cylinder_t *dive::get_or_create_cylinder(int idx)
{
	if (idx < 0) {
		report_info("Warning: accessing invalid cylinder %d", idx);
		return NULL;
	}
	while (static_cast<size_t>(idx) >= cylinders.size())
		cylinders.emplace_back();
	return &cylinders[idx];
}

/* Are there any used cylinders which we do not know usage about? */
static bool has_unknown_used_cylinders(const struct dive &dive, const struct divecomputer *dc,
				       const bool used_cylinders[], int num)
{
	int idx;
	auto used_and_unknown = std::make_unique<bool[]>(dive.cylinders.size());
	std::copy(used_cylinders, used_cylinders + dive.cylinders.size(), used_and_unknown.get());

	/* We know about using the O2 cylinder in a CCR dive */
	if (dc->divemode == CCR) {
		int o2_cyl = get_cylinder_idx_by_use(dive, OXYGEN);
		if (o2_cyl >= 0 && used_and_unknown[o2_cyl]) {
			used_and_unknown[o2_cyl] = false;
			num--;
		}
	}

	/* We know about the explicit first cylinder (or first) */
	/* And we have possible switches to other gases */
	gasmix_loop loop(dive, *dc);
	while (loop.has_next() && num > 0) {
		idx = loop.next_cylinder_index().first;
		if (idx >= 0 && used_and_unknown[idx]) {
			used_and_unknown[idx] = false;
			num--;
		}
	}

	return num > 0;
}

std::vector<dive::depth_duration> dive::per_cylinder_mean_depth_and_duration(int dc_nr) const
{
	const struct divecomputer *dc = get_dc(dc_nr);

	std::vector<depth_duration> res;
	if (cylinders.empty())
		return res;

	res.resize(cylinders.size());

	/*
	 * There is no point in doing per-cylinder information
	 * if we don't actually know about the usage of all the
	 * used cylinders.
	 */
	auto used_cylinders = std::make_unique<bool[]>(cylinders.size());
	int num_used_cylinders = get_cylinder_used(this, used_cylinders.get());
	if (has_unknown_used_cylinders(*this, dc, used_cylinders.get(), num_used_cylinders)) {
		/*
		 * If we had more than one used cylinder, but
		 * do not know usage of them, we simply cannot
		 * account mean depth to them.
		 */
		if (num_used_cylinders > 1)
			return res;

		/*
		 * For a single cylinder, use the overall mean
		 * and duration
		 */
		for (size_t i = 0; i < cylinders.size(); i++) {
			if (used_cylinders[i]) {
				res[i].depth = dc->meandepth;
				res[i].duration = dc->duration;
			}
		}

		return res;
	}

	// TODO: Wow: the old code was super sketchy: It would "fake" a dc here of all places if there were no samples.
	// Let's comment this out for now and fix the root-cause if this ever gives a problem.
	//if (dc->samples.empty())
	//	fake_dc(dc);

	// No samples - something is wrong. Give up.
	if (dc->samples.empty())
		return res;

	gasmix_loop loop(*this, *dc);
	std::vector<int32_t> depthtime(cylinders.size(), 0); // mm × seconds
	duration_t lasttime;
	depth_t lastdepth;
	int last_cylinder_index = -1;
	std::pair<int, int> gaschange_event;
	for (auto it = dc->samples.begin(); it != dc->samples.end(); ++it) {
		/* Make sure to move the event past 'lasttime' */
		gaschange_event = loop.cylinder_index_at(lasttime.seconds);

		/* Do we need to fake a midway sample? */
		if (last_cylinder_index >= 0 && last_cylinder_index != gaschange_event.first) {
			depth_t newdepth = interpolate(lastdepth, it->depth, gaschange_event.second - lasttime.seconds, (it->time - lasttime).seconds);
			if (newdepth.mm > SURFACE_THRESHOLD || lastdepth.mm > SURFACE_THRESHOLD) {
				res[gaschange_event.first].duration.seconds += gaschange_event.second - lasttime.seconds;
				depthtime[gaschange_event.first] += (gaschange_event.second - lasttime.seconds) * (newdepth.mm + lastdepth.mm) / 2;
			}

			lasttime.seconds = gaschange_event.second;
			lastdepth = newdepth;
		}

		/* We ignore segments at the surface */
		if (it->depth.mm > SURFACE_THRESHOLD || lastdepth.mm > SURFACE_THRESHOLD) {
			res[gaschange_event.first].duration += it->time - lasttime;
			depthtime[gaschange_event.first] += (it->time - lasttime).seconds * (it->depth + lastdepth).mm / 2;
		}

		lastdepth = it->depth;
		lasttime = it->time;
		last_cylinder_index = gaschange_event.first;
	}
	for (size_t i = 0; i < cylinders.size(); i++) {
		if (res[i].duration.seconds)
			res[i].depth.mm = (depthtime[i] + res[i].duration.seconds / 2) / res[i].duration.seconds;
	}

	return res;
}

static void update_min_max_temperatures(struct dive &dive, temperature_t temperature)
{
	if (temperature.mkelvin) {
		if (!dive.maxtemp.mkelvin || temperature.mkelvin > dive.maxtemp.mkelvin)
			dive.maxtemp = temperature;
		if (!dive.mintemp.mkelvin || temperature.mkelvin < dive.mintemp.mkelvin)
			dive.mintemp = temperature;
	}
}

/*
 * If the cylinder tank pressures are within half a bar
 * (about 8 PSI) of the sample pressures, we consider it
 * to be a rounding error, and throw them away as redundant.
 */
static int same_rounded_pressure(pressure_t a, pressure_t b)
{
	return abs((a - b).mbar) <= 500;
}

static double calculate_depth_to_mbarf(depth_t depth, pressure_t surface_pressure, int salinity);

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
		gasmix_loop loop(*dive, *dc);
		for (auto &sample: dc->samples) {
			struct gasmix gasmix = loop.at(sample.time.seconds).first;
			gas_pressures pressures = fill_pressures(lrint(calculate_depth_to_mbarf(sample.depth, dc->surface_pressure, 0)), gasmix ,0, dc->divemode);
			if (abs(sample.setpoint.mbar - (int)(1000 * pressures.o2)) <= 50)
				sample.setpoint = 0_baro2;
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
	type.description = format_string_std(fmt, int_cast<int>(cuft));
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

static void sanitize_cylinder_info(struct dive &dive)
{
	for (auto &cyl: dive.cylinders) {
		sanitize_gasmix(cyl.gasmix);
		sanitize_cylinder_type(cyl.type);
	}
}

pressure_t dive::calculate_surface_pressure() const
{
	pressure_t res;
	int sum = 0, nr = 0;

	bool logged = is_logged();
	for (auto &dc: dcs) {
		if ((logged || !is_dc_planner(&dc)) && dc.surface_pressure.mbar) {
			sum += dc.surface_pressure.mbar;
			nr++;
		}
	}
	res.mbar = nr ? (sum + nr / 2) / nr : 0;
	return res;
}

static void fixup_surface_pressure(struct dive &dive)
{
	dive.surface_pressure = dive.calculate_surface_pressure();
}

/* if the surface pressure in the dive data is redundant to the calculated
 * value (i.e., it was added by running fixup on the dive) return 0,
 * otherwise return the surface pressure given in the dive */
pressure_t dive::un_fixup_surface_pressure() const
{
	return surface_pressure.mbar == calculate_surface_pressure().mbar ?
		pressure_t() : surface_pressure;
}

static void fixup_water_salinity(struct dive &dive)
{
	int sum = 0, nr = 0;

	bool logged = dive.is_logged();
	for (auto &dc: dive.dcs) {
		if ((logged || !is_dc_planner(&dc)) && dc.salinity) {
			if (dc.salinity < 500)
				dc.salinity += FRESHWATER_SALINITY;
			sum += dc.salinity;
			nr++;
		}
	}
	if (nr)
		dive.salinity = (sum + nr / 2) / nr;
}

int dive::get_salinity() const
{
	return user_salinity ? user_salinity : salinity;
}

static void fixup_meandepth(struct dive &dive)
{
	int sum = 0, nr = 0;

	bool logged = dive.is_logged();
	for (auto &dc: dive.dcs) {
		if ((logged || !is_dc_planner(&dc)) && dc.meandepth.mm) {
			sum += dc.meandepth.mm;
			nr++;
		}
	}

	if (nr)
		dive.meandepth.mm = (sum + nr / 2) / nr;
}

static void fixup_duration(struct dive &dive)
{
	duration_t duration;

	bool logged = dive.is_logged();
	for (auto &dc: dive.dcs) {
		if (logged || !is_dc_planner(&dc))
			duration.seconds = std::max(duration.seconds, dc.duration.seconds);
	}
	dive.duration = duration;
}

static void fixup_watertemp(struct dive &dive)
{
	if (!dive.watertemp.mkelvin)
		dive.watertemp = dive.dc_watertemp();
}

static void fixup_airtemp(struct dive &dive)
{
	if (!dive.airtemp.mkelvin)
		dive.airtemp = dive.dc_airtemp();
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
 *
 * We also throw away CCR bailout events if they are
 * indicating a switch to a mode that is already defined by
 * the gas currently used, or to be used within the next minute.
 */
static void fixup_dc_events(struct dive &dive, struct divecomputer &dc)
{
	std::vector<int> to_delete;
	divemode_t current_divemode = dc.divemode;
	struct cylinder_t *current_cylinder = dive.get_cylinder(0);
	divemode_t new_divemode;
	for (auto [idx, event]: enumerated_range(dc.events)) {
		if (event.name == "bookmark" || event.name == "heading")
			continue;

		if (event.is_gaschange() && event.gas.index >= 0) {
			current_cylinder = dive.get_cylinder(event.gas.index);
			current_divemode = get_effective_divemode(dc, *current_cylinder);
		} else if (event.is_divemodechange()) {
			new_divemode = static_cast<divemode_t>(event.value);
			if (!((dc.divemode == CCR && prefs.allowOcGasAsDiluent && current_cylinder->cylinder_use == OC_GAS) || dc.divemode == PSCR)) {
				to_delete.push_back(idx);
				continue;
			} else if (new_divemode != current_divemode) {
				current_divemode = new_divemode;
			}
		}

		for (int idx2 = idx - 1; idx2 > 0; --idx2) {
			const auto &prev = dc.events[idx2];
			if ((event.time - prev.time).seconds > 60)
				break;

			if (range_contains(to_delete, idx2))
				continue;

			if ((event.time - prev.time).seconds <= 10 && event.is_gaschange() && prev.is_divemodechange()) {
				new_divemode = static_cast<divemode_t>(prev.value);
				if (new_divemode != current_divemode)
					current_divemode = new_divemode;
				else
					to_delete.push_back(idx2);
			}

			if (!event.is_gaschange() && prev.name == event.name && prev.flags == event.flags) {
				to_delete.push_back(idx);
				break;
			}
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

static void fixup_dc_depths(struct dive &dive, struct divecomputer &dc)
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
		if (sample.cns > dive.maxcns)
			dive.maxcns = sample.cns;
	}

	update_depth(&dc.maxdepth, maxdepth);
	if (!dive.is_logged() || !is_dc_planner(&dc))
		if (maxdepth > dive.maxdepth.mm)
			dive.maxdepth.mm = maxdepth;
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

static void fixup_dc_temp(struct dive &dive, struct divecomputer &dc)
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
				sample.temperature = 0_K;
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
	int lastindex[MAX_SENSORS] = { NO_SENSOR };
	int lastpressure[MAX_SENSORS] = { 0 };

	for (auto &sample: dc.samples) {
		int j;

		for (j = 0; j < MAX_SENSORS; j++) {
			int pressure = sample.pressure[j].mbar;
			int index = sample.sensor[j];

			if (index == lastindex[j]) {
				/* Remove duplicate redundant pressure information */
				if (pressure == lastpressure[j])
					sample.pressure[j] = 0_bar;
			}
			lastindex[j] = index;
			lastpressure[j] = pressure;
		}
	}
}

/* Do we need a sensor -> cylinder mapping? */
static void fixup_start_pressure(struct dive &dive, int idx, pressure_t p)
{
	if (idx >= 0 && static_cast<size_t>(idx) < dive.cylinders.size()) {
		cylinder_t &cyl = dive.cylinders[idx];
		if (p.mbar && !cyl.sample_start.mbar)
			cyl.sample_start = p;
	}
}

static void fixup_end_pressure(struct dive &dive, int idx, pressure_t p)
{
	if (idx >= 0 && static_cast<size_t>(idx) < dive.cylinders.size()) {
		cylinder_t &cyl = dive.cylinders[idx];
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
static void fixup_dive_pressures(struct dive &dive, struct divecomputer &dc)
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
static bool validate_gaschange(struct dive &dive, struct event &event)
{
	/* We'll get rid of the per-event gasmix, but for now sanitize it */
	if (gasmix_is_air(event.gas.mix))
		event.gas.mix.o2 = 0_percent;

	/* Do we already have a cylinder index for this gasmix? */
	if (event.gas.index >= 0)
		return true;

	enum cylinderuse use;
	enum cylinderuse *use_ptr = nullptr;
	if (event.flags == 1) {
		switch (event.value) {
		case CCR:
			use = DILUENT;

			break;
		case FREEDIVE:
			use = NOT_USED;

			break;
		default:
			use = OC_GAS;

			break;
		}
		use_ptr = &use;
	}
	int index = find_best_gasmix_match(event.gas.mix, dive.cylinders, use_ptr);

	if (index < 0 || static_cast<size_t>(index) >= dive.cylinders.size())
		return false;

	/* Fix up the event to have the right information */
	event.flags = 0;
	event.gas.index = index;
	event.gas.mix = dive.cylinders[index].gasmix;

	event.value = get_libdivecomputer_gasmix_value(event.gas.mix);

	if (get_he(event.gas.mix))
		event.type = SAMPLE_EVENT_GASCHANGE2;

	return true;
}

/* Clean up event, return true if event is ok, false if it should be dropped as bogus */
static bool validate_event(struct dive &dive, struct event &event)
{
	if (event.is_gaschange())
		return validate_gaschange(dive, event);
	return true;
}

static void fixup_dc_gasswitch(struct dive &dive, struct divecomputer &dc)
{
	// erase-remove idiom
	auto &events = dc.events;
	events.erase(std::remove_if(events.begin(), events.end(),
				    [&dive](auto &ev) { return !validate_event(dive, ev); }),
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

static void fixup_sensor_mappings(struct dive &dive, struct divecomputer &dc)
{
	if (dc.tank_sensor_mappings.size() == 0) {
		// Add the implied mappings
		for (auto sensor_id: get_tank_sensor_ids(dc))
			if (sensor_id >= 0 && (unsigned int)sensor_id < dive.cylinders.size())
				dc.tank_sensor_mappings.push_back(tank_sensor_mapping { sensor_id, (unsigned int)sensor_id });
	} else {
		// Remove mappings where the sensor id does not exist
		// or the cylinder index is out of range
		auto sensor_ids = get_tank_sensor_ids(dc);
		dc.tank_sensor_mappings.erase(std::remove_if(dc.tank_sensor_mappings.begin(), dc.tank_sensor_mappings.end(),
			[sensor_ids, &dive](const tank_sensor_mapping &mapping) {
				return sensor_ids.find(mapping.sensor_id) == sensor_ids.end()
					|| mapping.cylinder_index >= dive.cylinders.size();
			}), dc.tank_sensor_mappings.end());

		// Remove any mappings that have a duplicate sensor id
		dc.tank_sensor_mappings.erase(std::unique(dc.tank_sensor_mappings.begin(), dc.tank_sensor_mappings.end(),
			[](const tank_sensor_mapping &a, const tank_sensor_mapping &b) {
				return a.sensor_id == b.sensor_id;
			}), dc.tank_sensor_mappings.end());

		// Remove any mappings that have the same cylinder index
		dc.tank_sensor_mappings.erase(std::unique(dc.tank_sensor_mappings.begin(), dc.tank_sensor_mappings.end(),
			[](const tank_sensor_mapping &a, const tank_sensor_mapping &b) {
				return a.cylinder_index == b.cylinder_index;
			}), dc.tank_sensor_mappings.end());
	}
}

int check_dc_cylinder_use(struct dive &dive, struct divecomputer &dc)
{

	if (dc.divemode != OC && dc.divemode != CCR && dc.divemode != PSCR)
		return 1;

	// Check that we have at least one cylinder that is suitable for the dive

	// For OC we default to air if we don't have any cylinders

	if (dc.divemode == OC && dive.cylinders.empty())
		return 1;

	for (auto &cylinder: dive.cylinders)
		if ((dc.divemode == CCR && cylinder.cylinder_use == DILUENT) || ((dc.divemode == PSCR || dc.divemode == OC) && cylinder.cylinder_use == OC_GAS))
			return 1;

	return report_error("Dive: %u, dive computer: %s: %s dive, but no %s cylinder found. Please add or select the correct cylinder use.", dive.number, dc.model.c_str(), dc.divemode == OC ? "open circuit" : dc.divemode == CCR ? "CCR" : "PSCR", dc.divemode == OC ? "open circuit" : dc.divemode == CCR ? "diluent" : "drive gas");
}


void dive::fixup_dive_dc(struct divecomputer &dc)
{
	/* Fixup duration and mean depth */
	fixup_dc_duration(dc);

	/* Fix up sample depth data */
	fixup_dc_depths(*this, dc);

	/* Fix up first sample ndl data */
	fixup_dc_ndl(dc);

	/* Fix up dive temperatures based on dive computer samples */
	fixup_dc_temp(*this, dc);

	/* Fix up gas switch events */
	fixup_dc_gasswitch(*this, dc);

	/* Fix up cylinder ids in pressure sensors */
	fixup_dc_sample_sensors(dc);

	fixup_sensor_mappings(*this, dc);

	/* Fix up cylinder pressures based on DC info */
	fixup_dive_pressures(*this, dc);

	fixup_dc_events(*this, dc);

	/* Fixup CCR / PSCR dives with o2sensor values, but without no_o2sensors */
	fixup_no_o2sensors(dc);

	/* Check cylinder use */
	check_dc_cylinder_use(*this, dc);

	/* If there are no samples, generate a fake profile based on depth and time */
	if (dc.samples.empty())
		fake_dc(&dc);
}

void dive::fixup_dive()
{
	sanitize_cylinder_info(*this);
	maxcns = cns;

	/*
	 * Use the dive's temperatures for minimum and maximum in case
	 * we do not have temperatures recorded by DC.
	 */

	update_min_max_temperatures(*this, watertemp);

	for (auto &dc: dcs)
		fixup_dive_dc(dc);

	fixup_water_salinity(*this);
	if (!surface_pressure.mbar)
		fixup_surface_pressure(*this);
	fixup_meandepth(*this);
	fixup_duration(*this);
	fixup_watertemp(*this);
	fixup_airtemp(*this);
	for (auto &cyl: cylinders) {
		add_cylinder_description(cyl.type);
		if (same_rounded_pressure(cyl.sample_start, cyl.start))
			cyl.start = 0_bar;
		if (same_rounded_pressure(cyl.sample_end, cyl.end))
			cyl.end = 0_bar;
	}

	for (auto &ws: weightsystems)
		add_weightsystem_description(ws);
}

/* Don't pick a zero for MERGE_MIN() */
#define MERGE_MAX(res, a, b, n) res->n = std::max(a.n, b.n)
#define MERGE_MIN(res, a, b, n) res->n = (a.n) ? (b.n) ? std::min(a.n, b.n) : (a.n) : (b.n)
#define MERGE_TXT(res, a, b, n, sep) res->n = merge_text(a.n, b.n, sep)
#define MERGE_NONZERO(res, a, b, n) (res)->n = (a).n ? (a).n : (b).n

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
			surface.ndl = prev.ndl;
			surface.time.seconds = last_time + 20;

			append_sample(surface, &dc);

			surface.time = sample.time - 20_sec;
			append_sample(surface, &dc);
		}
	}
	append_sample(sample, &dc);
}

static void sensors_renumber_last_sample(struct divecomputer &dc, const int mapping[]);
static void sensors_renumber(struct sample &s, const struct sample *next, const int mapping[]);

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
			sensors_renumber_last_sample(res, cylinders_map_a);
			as++;
			continue;
		}

		/* Only samples from b? */
		if (at < 0) {
		add_sample_b:
			struct sample sample = *bs;
			sample.time.seconds += offset;
			merge_one_sample(sample, res);
			sensors_renumber_last_sample(res, cylinders_map_b);
			bs++;
			continue;
		}

		if (at < bt)
			goto add_sample_a;
		if (at > bt)
			goto add_sample_b;

		/* same-time sample: add a merged sample. Take the non-zero ones */
		struct sample sample = *bs;
		sample.time.seconds += offset;
		sensors_renumber(sample, nullptr, cylinders_map_b);
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

static void merge_tank_sensor_mappings(struct divecomputer &res,
                         const struct divecomputer &src1_in, const struct divecomputer &src2_in,
                         const int *cylinders_map1, const int *cylinders_map2,
			 int offset)
{
	/* If two cylinders are merged and both have a sensor assigned, use the sensor associated with the earliest part of the log */
	auto src1 = &src1_in;
	auto src2 = &src2_in;
	if (offset < 0) {
		std::swap(src1, src2);
		std::swap(cylinders_map1, cylinders_map2); // The pointers, not the contents are swapped.
	}

	res.tank_sensor_mappings.clear();

	for (auto &mapping: src2->tank_sensor_mappings)
		res.tank_sensor_mappings.push_back(tank_sensor_mapping {
			(int16_t)(mapping.sensor_id == NO_SENSOR ? NO_SENSOR : cylinders_map2[mapping.sensor_id]),
			(unsigned int)cylinders_map2[mapping.cylinder_index]
		});

	for (auto &mapping: src1->tank_sensor_mappings)
		res.tank_sensor_mappings.push_back(tank_sensor_mapping {
			(int16_t)(mapping.sensor_id == NO_SENSOR ? NO_SENSOR : cylinders_map1[mapping.sensor_id]),
			(unsigned int)cylinders_map1[mapping.cylinder_index]
		});
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

/* Force an initial gaschange event to the (old) gas #0 */
static void add_initial_gaschange(struct dive &dive, struct divecomputer &dc, int offset, int idx)
{
	/* if there is a gaschange event up to 30 sec after the initial event,
	 * refrain from adding the initial event */
	gasmix_loop loop(dive, dc);
	while (loop.has_next()) {
		int time = loop.next().second;
		if (time > offset + 30)
			break;
		else if (time > offset)
			return;
	}

	/* Old starting gas mix */
	add_gas_switch_event(&dive, &dc, offset, idx);
}

static void sensors_renumber(struct sample &s, const struct sample *prev, const int mapping[])
{
	for (int j = 0; j < MAX_SENSORS; j++) {
		int sensor = NO_SENSOR;

		if (s.sensor[j] != NO_SENSOR)
			sensor = mapping[s.sensor[j]];
		if (sensor == NO_SENSOR) {
			// Remove sensor and gas pressure info
			if (!prev) {
				s.sensor[j] = 0;
				s.pressure[j] = 0_bar;
			} else {
				s.sensor[j] = prev->sensor[j];
				s.pressure[j] = prev->pressure[j];
			}
		} else {
			s.sensor[j] = sensor;
		}
	}
}

static void sensors_renumber_last_sample(struct divecomputer &dc, const int mapping[])
{
	if (dc.samples.empty())
		return;
	sample *prev = dc.samples.size() > 1 ? &dc.samples[dc.samples.size() - 2] : nullptr;
	sensors_renumber(dc.samples.back(), prev, mapping);
}

static void event_renumber(struct event &ev, const int mapping[])
{
	if (!ev.is_gaschange())
		return;
	if (ev.gas.index < 0)
		return;
	ev.gas.index = mapping[ev.gas.index];
}

static void dc_tank_sensor_mappings_renumber(struct divecomputer &dc, const int mapping[])
{
	for (auto it = dc.tank_sensor_mappings.begin(); it != dc.tank_sensor_mappings.end();) {
		int cylinder_index = mapping[(*it).cylinder_index];
		if (cylinder_index == NO_SENSOR) {
			it = dc.tank_sensor_mappings.erase(it);

			break;
		}

		(*it).cylinder_index = cylinder_index;

		it++;
	}
}

static void dc_cylinder_renumber(struct dive &dive, struct divecomputer &dc, const int mapping[])
{
	dc_tank_sensor_mappings_renumber(dc, mapping);

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
		if (gasmix_distance(mygas, gas2) == 0 && (dive->is_cylinder_used(i) || check_unused))
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
		a->type.size = b->type.size;
	if (!a->type.workingpressure.mbar)
		a->type.workingpressure = b->type.workingpressure;
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
	a->gas_used += b->gas_used;
	a->deco_gas_used += b->deco_gas_used;
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
	if (dive->is_cylinder_used(idx) || prefs.include_unused_tanks)
		return true;

	/* This tests for typenames or gas contents */
	return cylinder_has_data(dive->cylinders[idx]);
}

bool is_cylinder_use_appropriate(const struct divecomputer &dc, const cylinder_t &cyl, bool allowNonUsable)
{
	switch (cyl.cylinder_use) {
	case OC_GAS:
		if (dc.divemode == FREEDIVE)
			return false;

		break;
	case OXYGEN:
		if (!allowNonUsable)
			return false;

	case DILUENT:
		if (dc.divemode != CCR)
			return false;

		break;
	case NOT_USED:
		if (!allowNonUsable)
			return false;

		break;
	default:
		return false;
	}

	return true;
}

divemode_t get_effective_divemode(const struct divecomputer &dc, const struct cylinder_t &cylinder)
{
	divemode_t divemode = dc.divemode;
	if (divemode == CCR && cylinder.cylinder_use == OC_GAS)
		divemode = OC;

	return divemode;
}

std::tuple<divemode_t, int, const struct gasmix *> get_dive_status_at(const struct dive &dive, const struct divecomputer &dc, int seconds, divemode_loop *loop_mode, gasmix_loop *loop_gas)
{
	if (!loop_mode)
		loop_mode = new divemode_loop(dc);
	if (!loop_gas)
		loop_gas = new gasmix_loop(dive, dc);
	auto [divemode, divemode_time] = loop_mode->at(seconds);
	auto [cylinder_index, gasmix_time] = loop_gas->cylinder_index_at(seconds);
	const struct gasmix *gasmix;
	if (cylinder_index == -1) {
		gasmix = &gasmix_air;
		if (gasmix_time >= divemode_time)
			divemode = OC;
	} else {
		const struct cylinder_t *cylinder = dive.get_cylinder(cylinder_index);
		gasmix = &cylinder->gasmix;
		if (gasmix_time >= divemode_time)
			divemode = get_effective_divemode(dc, *cylinder);
	}

	return std::make_tuple(divemode, cylinder_index, gasmix);
}

const std::vector<struct tank_sensor_mapping> get_tank_sensor_mappings_for_storage(const struct dive &dive, const struct divecomputer &dc)
{
	std::set<int16_t> sensor_ids = get_tank_sensor_ids(dc);
	// If we don't have any mappings, return a mapping between the first
	// cylinder and an invalid sensor id. This is used to distinguish
	// from the case of having a full 1 : 1 mapping which is encoded
	// as no mapping in order to retain backwards compatibility.
	if (dc.tank_sensor_mappings.empty() && !dive.cylinders.empty() && !sensor_ids.empty())
		return std::vector<struct tank_sensor_mapping>({ tank_sensor_mapping { NO_SENSOR, 0 } });

	// Check if any of the mappings are not 1 : 1
	for (const auto &mapping: dc.tank_sensor_mappings)
		if (mapping.sensor_id != (int16_t)mapping.cylinder_index)
			return dc.tank_sensor_mappings;

	// Check if there is a cylinder that does not have a mapping
	for (const auto &sensor_id: sensor_ids)
		if (sensor_id >= 0 && sensor_id < (int16_t)dive.cylinders.size()
			&& std::find_if(dc.tank_sensor_mappings.begin(), dc.tank_sensor_mappings.end(),
				[&sensor_id](const struct tank_sensor_mapping &mapping) {
					return mapping.sensor_id == sensor_id;
				}) == dc.tank_sensor_mappings.end())
			return dc.tank_sensor_mappings;

	// If all cylinders have a mapping, return an empty mapping
	return std::vector<struct tank_sensor_mapping>();
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

		j = match_cylinder(b.get_cylinder(i), res, try_to_match.get());

		/* No match? Add it to the result */
		if (j < 0) {
			size_t res_nr = res.cylinders.size();
			mapping_b[i] = static_cast<int>(res_nr);
			res.cylinders.push_back(b.cylinders[i]);
			continue;
		}

		/* Otherwise, merge the result to the one we found */
		mapping_b[i] = j;
		merge_one_cylinder(res.get_cylinder(j), b.get_cylinder(i));

		/* Don't match the same target more than once */
		try_to_match[j] = false;
	}
}

/* Check whether a weightsystem table contains a given weightsystem */
static bool has_weightsystem(const weightsystem_table &t, const weightsystem_t &w)
{
	return any_of(t.begin(), t.end(), [&w] (auto &w2) { return w == w2; });
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
	MERGE_NONZERO(&res, a, b, watertemp.mkelvin);
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
		unsigned int interval = (b.time - a.time).seconds;
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
bool dive::likely_same(const struct dive &b) const
{
	/* don't merge manually added dives with anything */
	if (is_dc_manually_added_dive(&dcs[0]) ||
	    is_dc_manually_added_dive(&b.dcs[0]))
		return false;

	/*
	 * Do some basic sanity testing of the values we
	 * have filled in during 'fixup_dive()'
	 */
	if (!similar(maxdepth.mm, b.maxdepth.mm, 1000) ||
	    (meandepth.mm && b.meandepth.mm && !similar(meandepth.mm, b.meandepth.mm, 1000)) ||
	    !duration.seconds || !b.duration.seconds ||
	    !similar(duration.seconds, b.duration.seconds, 5 * 60))
		return false;

	/* See if we can get an exact match on one dive computer */
	if (match_dc_dive(*this, b) > 0)
		return true;

	/*
	 * Allow a time difference due to dive computer time
	 * setting etc. Check if they overlap.
	 */
	int fuzz = std::max(duration.seconds, b.duration.seconds) / 2;
	fuzz = std::max(fuzz, 60);

	return (when <= b.when + fuzz) && (when >= b.when - fuzz);
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
				      const int cylinders_map_a[], const int cylinders_map_b[])
{
	res.dcs.clear();
	for (const auto &dc1: a.dcs) {
		res.dcs.emplace_back();
		divecomputer &newdc = res.dcs.back();
		copy_dive_computer(newdc, dc1);
		const divecomputer *match = find_matching_computer(dc1, b);
		if (match) {
			int offset = match->when - dc1.when;
			merge_events(res, newdc, dc1, *match, cylinders_map_a, cylinders_map_b, offset);
			merge_samples(newdc, dc1, *match, cylinders_map_a, cylinders_map_b, offset);
			merge_extra_data(newdc, dc1, *match);
			merge_tank_sensor_mappings(newdc, dc1, *match, cylinders_map_a, cylinders_map_b, offset);
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

std::unique_ptr<dive> dive::create_merged_dive(const struct dive &a, const struct dive &b, int offset, bool prefer_downloaded)
{
	auto res = std::make_unique<dive>();

	if (offset) {
		/*
		 * If "likely_same_dive()" returns true, that means that
		 * it is *not* the same dive computer, and we do not want
		 * to try to turn it into a single longer dive. So we'd
		 * join them as two separate dive computers at zero offset.
		 */
		if (a.likely_same(b))
			offset = 0;
	}

	res->when = prefer_downloaded ? b.when : a.when;
	res->selected = a.selected || b.selected;
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
	res->pictures = !a.pictures.empty() ? a.pictures : b.pictures;
	res->tags = taglist_merge(a.tags, b.tags);
	/* if we get dives without any gas / cylinder information in an import, make sure
	 * that there is at leatst one entry in the cylinder map for that dive */
	auto cylinders_map_a = std::make_unique<int[]>(std::max(size_t(1), a.cylinders.size()));
	auto cylinders_map_b = std::make_unique<int[]>(std::max(size_t(1), b.cylinders.size()));
	merge_cylinders(*res, a, b, cylinders_map_a.get(), cylinders_map_b.get());
	merge_equipment(*res, a, b);
	merge_temperatures(*res, a, b);
	if (prefer_downloaded) {
		/* If we prefer downloaded, do those first, and get rid of "might be same" computers */
		join_dive_computers(*res, b, a, cylinders_map_b.get(), cylinders_map_a.get(), true);
	} else if (offset && might_be_same_device(a.dcs[0], b.dcs[0])) {
		interleave_dive_computers(*res, a, b, cylinders_map_a.get(), cylinders_map_b.get());
	} else {
		join_dive_computers(*res, a, b, cylinders_map_a.get(), cylinders_map_b.get(), false);
	}

	return res;
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
	return { .seconds = time };
}

timestamp_t dive::endtime() const
{
	return when + totaltime().seconds;
}

bool dive::time_during_dive_with_offset(timestamp_t when, timestamp_t offset) const
{
	timestamp_t start = when;
	timestamp_t end = endtime();
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

//Calculate O2 in best mix
fraction_t dive::best_o2(depth_t depth, bool in_planner) const
{
	fraction_t fo2;
	int po2 = in_planner ? prefs.bottompo2 : (int)(prefs.modpO2 * 1000.0);

	fo2.permille = (po2 * 100 / depth_to_mbar(depth)) * 10;	//use integer arithmetic to round down to nearest percent
	// Don't permit >100% O2
	// TODO: use std::min, once we have comparison
	if (fo2.permille > 1000)
		fo2 = 100_percent;
	return fo2;
}

//Calculate He in best mix. O2 is considered narcopic
fraction_t dive::best_he(depth_t depth, bool o2narcotic, fraction_t fo2) const
{
	fraction_t fhe;
	int pnarcotic, ambient;
	pnarcotic = depth_to_mbar(prefs.bestmixend);
	ambient = depth_to_mbar(depth);
	if (o2narcotic) {
		fhe.permille = (100 - 100 * pnarcotic / ambient) * 10;	//use integer arithmetic to round up to nearest percent
	} else {
		fhe.permille = 1000 - fo2.permille - N2_IN_AIR * pnarcotic / ambient;
	}
	// TODO: use std::max, once we have comparison
	if (fhe.permille < 0)
		fhe = 0_percent;
	return fhe;
}

static constexpr std::array<unsigned char, 20> null_id = {};
void dive::invalidate_cache()
{
	git_id = null_id;
}

bool dive::cache_is_valid() const
{
	return git_id != null_id;
}

pressure_t dive::get_surface_pressure() const
{
	return surface_pressure.mbar > 0 ? surface_pressure : 1_atm;
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
static double calculate_depth_to_mbarf(depth_t depth, pressure_t surface_pressure, int salinity)
{
	if (!surface_pressure.mbar)
		surface_pressure = 1_atm;
	if (!salinity)
		salinity = SEAWATER_SALINITY;
	if (salinity < 500)
		salinity += FRESHWATER_SALINITY;
	double specific_weight = salinity_to_specific_weight(salinity);
	return surface_pressure.mbar + depth.mm * specific_weight;
}

int dive::depth_to_mbar(depth_t depth) const
{
	return lrint(depth_to_mbarf(depth));
}

double dive::depth_to_mbarf(depth_t depth) const
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

double dive::depth_to_bar(depth_t depth) const
{
	return depth_to_mbar(depth) / 1000.0;
}

double dive::depth_to_atm(depth_t depth) const
{
	return mbar_to_atm(depth_to_mbar(depth));
}

/* for the inverse calculation we use just the relative pressure
 * (that's the one that some dive computers like the Uemis Zurich
 * provide - for the other models that do this libdivecomputer has to
 * take care of this, but the Uemis we support natively */
depth_t dive::rel_mbar_to_depth(int mbar) const
{
	// For downloaded and planned dives, use DC's salinity. Manual dives, use user's salinity
	int salinity = is_dc_manually_added_dive(&dcs[0]) ? user_salinity : dcs[0].salinity;
	if (!salinity)
		salinity = SEAWATER_SALINITY;

	/* whole mbar gives us cm precision */
	double specific_weight = salinity_to_specific_weight(salinity);
	depth_t res { .mm = int_cast<int>(mbar / specific_weight) };
	return res;
}

depth_t dive::mbar_to_depth(int mbar) const
{
	// For downloaded and planned dives, use DC's pressure. Manual dives, use user's pressure
	pressure_t surface_pressure = is_dc_manually_added_dive(&dcs[0])
		? this->surface_pressure
		: dcs[0].surface_pressure;

	if (!surface_pressure.mbar)
		surface_pressure = 1_atm;

	return rel_mbar_to_depth(mbar - surface_pressure.mbar);
}

/* MOD rounded to multiples of roundto mm */
depth_t dive::gas_mod(struct gasmix mix, pressure_t po2_limit, depth_t roundto) const
{
	double depth = (double) mbar_to_depth(po2_limit.mbar * 1000 / get_o2(mix)).mm;
	// Rounding should be towards lower=safer depths but we give a bit
	// of fudge to all to switch to o2 at 6m. So from .9 we round up.
	return depth_t { .mm = (int)(depth / roundto.mm + 0.1) * roundto.mm };
}

/* Maximum narcotic depth rounded to multiples of roundto mm */
depth_t dive::gas_mnd(struct gasmix mix, depth_t end, depth_t roundto) const
{
	pressure_t ppo2n2 { .mbar = depth_to_mbar(end) };

	int maxambient = prefs.o2narcotic ?
					int_cast<int>(ppo2n2.mbar / (1 - get_he(mix) / 1000.0))
			      :
					get_n2(mix) > 0 ?
						int_cast<int>(ppo2n2.mbar * N2_IN_AIR / get_n2(mix))
					:
						// Actually: Infinity
						1000000;
	double depth = static_cast<double>(mbar_to_depth(maxambient).mm);
	return depth_t { .mm = int_cast<int>(depth / roundto.mm) * roundto.mm };
}

std::string dive::get_country() const
{
	return dive_site ? taxonomy_get_country(dive_site->taxonomy) : std::string();
}

std::string dive::get_location() const
{
	return dive_site ? dive_site->name : std::string();
}

int dive::number_of_computers() const
{
	return static_cast<int>(dcs.size());
}

struct divecomputer *dive::get_dc(int nr)
{
	if (dcs.empty()) // Can't happen!
		return NULL;
	nr = std::max(0, nr);
	return &dcs[static_cast<size_t>(nr) % dcs.size()];
}

const struct divecomputer *dive::get_dc(int nr) const
{
	return const_cast<dive &>(*this).get_dc(nr);
}

bool dive::dive_has_gps_location() const
{
	return dive_site && dive_site->has_gps_location();
}

/* Extract GPS location of a dive computer stored in the GPS1
 * or GPS2 extra data fields */
static location_t dc_get_gps_location(const struct divecomputer &dc)
{
	location_t res;

	for (const auto &data: dc.extra_data) {
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
location_t dive::get_gps_location() const
{
	for (const struct divecomputer &dc: dcs) {
		location_t res = dc_get_gps_location(dc);
		if (has_location(&res))
			return res;
	}

	/* No libdivecomputer generated GPS data found.
	 * Let's use the location of the current dive site.
	 */
	return dive_site ? dive_site->location : location_t();
}

gasmix_loop::gasmix_loop(const struct dive &d, const struct divecomputer &dc) :
	dive(d), dc(dc), first_run(true), loop("gaschange", dc)
{
}

/* Some dive computers (Cobalt) don't start the dive with cylinder 0 but explicitly
 * tell us what the first gas is with a gas change event in the first sample.
 * Sneakily we'll use a return value of 0 (or FALSE) when there is no explicit
 * first cylinder - in which case cylinder 0 is indeed the first cylinder.
 * We likewise return 0 if the event concerns a cylinder that doesn't exist.
 * If the dive has no cylinders, -1 is returned. */

std::pair<int, int> gasmix_loop::next_cylinder_index()
{
	if (first_run) {
		next_event = loop.next();
		first_run = false;

		if (dive.cylinders.empty()) {
			last_cylinder_index = -1;
			last_time = 0;
		} else {
			last_cylinder_index = 0; // default to first cylinder
			last_time = 0;
			if (next_event && ((!dc.samples.empty() && next_event->time.seconds == dc.samples[0].time.seconds) || next_event->time.seconds <= 1)) {
				last_cylinder_index = dive.get_cylinder_index(*next_event, dc);
				last_time = next_event->time.seconds;
				next_event = loop.next();
			} else if (dc.divemode == CCR) {
				last_cylinder_index = std::max(get_cylinder_idx_by_use(dive, DILUENT), last_cylinder_index);
			}
		}
	} else {
		if (next_event) {
			last_cylinder_index = dive.get_cylinder_index(*next_event, dc);
			last_time = next_event->time.seconds;
			next_event = loop.next();
		} else {
			last_cylinder_index = -1;
			last_time = INT_MAX;
		}
	}

	return std::make_pair(last_cylinder_index, last_time);
}

std::pair<gasmix, int> gasmix_loop::next()
{
	next_cylinder_index();

	return get_last_gasmix();
}

std::pair<int, int> gasmix_loop::cylinder_index_at(int time)
{
	if (first_run)
		next_cylinder_index();

	while (has_next() && next_event->time.seconds <= time)
		next_cylinder_index();

	return std::make_pair(last_cylinder_index, last_time);
}

std::pair<gasmix, int> gasmix_loop::at(int time)
{
	cylinder_index_at(time);

	return get_last_gasmix();
}

bool gasmix_loop::has_next() const
{
	return first_run || (!dive.cylinders.empty() && next_event);
}

std::pair<gasmix, int> gasmix_loop::get_last_gasmix()
{
	if (last_cylinder_index < 0 && last_time == 0)
		return std::make_pair(gasmix_air, 0);

	return std::make_pair(last_cylinder_index < 0 ? gasmix_invalid : dive.get_cylinder(last_cylinder_index)->gasmix, last_time);
}

/* get the gas at a certain time during the dive */
/* If there is a gasswitch at that time, it returns the new gasmix */
struct gasmix dive::get_gasmix_at_time(const struct divecomputer &dc, duration_t time) const
{
	return gasmix_loop(*this, dc).at(time.seconds).first;
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
	return temperature_t{ .mkelvin = static_cast<uint32_t>((sum + nr / 2) / nr) };
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
	return temperature_t{ .mkelvin = static_cast<uint32_t>((sum + nr / 2) / nr) };
}

/*
 * Get "maximal" dive gas for a dive.
 * Rules:
 *  - Trimix trumps nitrox (highest He wins, O2 breaks ties)
 *  - Nitrox trumps air (even if hypoxic)
 * These are the same rules as the inter-dive sorting rules.
 */
dive::get_maximal_gas_result dive::get_maximal_gas() const
{
	int maxo2 = -1, maxhe = -1, mino2 = 1000;

	for (auto [i, cyl]: enumerated_range(cylinders)) {
		int o2 = get_o2(cyl.gasmix);
		int he = get_he(cyl.gasmix);

		if (!is_cylinder_used(i))
			continue;
		if (cyl.cylinder_use == OXYGEN)
			continue;
		if (cyl.cylinder_use == NOT_USED)
			continue;
		if (o2 > maxo2)
			maxo2 = o2;
		if (o2 < mino2 && maxhe <= 0)
			mino2 = o2;
		if (he > maxhe) {
			maxhe = he;
			mino2 = o2;
		}
	}
	/* All air? Show/sort as "air"/zero */
	if ((!maxhe && maxo2 == O2_IN_AIR && mino2 == maxo2) ||
			(maxo2 == -1 && maxhe == -1 && mino2 == 1000))
		maxo2 = mino2 = 0;
	return { mino2, maxhe, maxo2 };
}

bool dive::has_gaschange_event(const struct divecomputer *dc, int idx) const
{
	gasmix_loop loop(*this, *dc);
	while (loop.has_next()) {
		if (loop.next_cylinder_index().first == idx)
			return true;
	}

	return false;
}

bool dive::is_cylinder_used(int idx) const
{
	if (idx < 0 || static_cast<size_t>(idx) >= cylinders.size())
		return false;

	const cylinder_t &cyl = cylinders[idx];
	if ((cyl.start.mbar - cyl.end.mbar) > SOME_GAS)
		return true;

	if ((cyl.sample_start.mbar - cyl.sample_end.mbar) > SOME_GAS)
		return true;

	for (auto &dc: dcs) {
		if (has_gaschange_event(&dc, idx))
			return true;
		else if (dc.divemode == CCR && idx == get_cylinder_idx_by_use(*this, OXYGEN))
			return true;
	}
	return false;
}

bool dive::is_cylinder_prot(int idx) const
{
	if (idx < 0 || static_cast<size_t>(idx) >= cylinders.size())
		return false;

	return std::any_of(dcs.begin(), dcs.end(),
			   [this, idx](auto &dc)
			   { return has_gaschange_event(&dc, idx); });
}

weight_t dive::total_weight() const
{
	// TODO: implement addition for units.h types
	return std::accumulate(weightsystems.begin(), weightsystems.end(), weight_t(),
			       [] (weight_t w, const weightsystem_t &ws)
			       { return weight_t{ .grams = w.grams + ws.weight.grams }; });
}
