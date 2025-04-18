// SPDX-License-Identifier: GPL-2.0
/* profile.c */
/* creates all the necessary data for drawing the dive profile
 */
#include "gettext.h"
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "errorhelper.h"
#include "event.h"
#include "interpolate.h"
#include "sample.h"
#include "subsurface-string.h"
#include "tanksensormapping.h"

#include "planner.h"
#include "profile.h"
#include "gaspressures.h"
#include "deco.h"
#include "errorhelper.h"
#include "libdivecomputer/parser.h"
#include "libdivecomputer/version.h"
#include "membuffer.h"
#include "qthelper.h"
#include "range.h"
#include "format.h"

//#define DEBUG_GAS 1

#define MAX_PROFILE_DECO 7200


#ifdef DEBUG_PI
/* debugging tool - not normally used */
static void dump_pi(const struct plot_info &pi)
{
	int i;

	printf("pi:{nr:%d maxtime:%d meandepth:%d maxdepth:%d \n"
	       "    maxpressure:%d mintemp:%d maxtemp:%d\n",
	       pi.nr, pi.maxtime, pi.meandepth, pi.maxdepth,
	       pi.maxpressure, pi.mintemp, pi.maxtemp);
	for (i = 0; i < pi.nr; i++) {
		struct plot_data &entry = pi.entry[i];
		printf("    entry[%d]:{cylinderindex:%d sec:%d pressure:{%d,%d}\n"
		       "                time:%d:%02d temperature:%d depth:%d stopdepth:%d stoptime:%d ndl:%d smoothed:%d po2:%lf phe:%lf pn2:%lf sum-pp %lf}\n",
		       i, entry.sensor[0], entry.sec,
		       entry.pressure[0], entry.pressure[1],
		       entry.sec / 60, entry.sec % 60,
		       entry.temperature, entry.depth.mm, entry.stopdepth.mm, entry.stoptime, entry.ndl, entry.smoothed,
		       entry.pressures.o2, entry.pressures.he, entry.pressures.n2,
		       entry.pressures.o2 + entry.pressures.he + entry.pressures.n2);
	}
	printf("   }\n");
}
#endif

template<typename T>
static T round_up(T x, T y)
{
	return ((x + y - 1) / y) * y;
}

template<typename T>
static T div_up(T x, T y)
{
	return (x + y - 1) / y;
}

plot_info::plot_info()
{
}

plot_info::~plot_info()
{
}

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 */
int get_maxtime(const struct plot_info &pi)
{
	int seconds = pi.maxtime;
	int min = prefs.zoomed_plot ? 30 : 30 * 60;
	return std::max(min, seconds);
}

/* get the maximum depth to which we want to plot */
int get_maxdepth(const struct plot_info &pi)
{
	/* 3m to spare */
	int mm = pi.maxdepth + 3000;
	return prefs.zoomed_plot ? mm : std::max(30000, mm);
}

/* UNUSED! */
static int get_local_sac(struct plot_info &pi, int idx1, int idx2, struct dive *dive) __attribute__((unused));

/* Get local sac-rate (in ml/min) between entry1 and entry2 */
static int get_local_sac(struct plot_info &pi, int idx1, int idx2, struct dive *dive)
{
	int index = 0;
	cylinder_t *cyl;
	struct plot_data &entry1 = pi.entry[idx1];
	struct plot_data &entry2 = pi.entry[idx2];
	int duration = entry2.sec - entry1.sec;
	pressure_t a, b;
	double atm;

	if (duration <= 0)
		return 0;
	a.mbar = get_plot_pressure(pi, idx1, 0);
	b.mbar = get_plot_pressure(pi, idx2, 0);
	if (!b.mbar || a.mbar <= b.mbar)
		return 0;

	/* Mean pressure in ATM */
	depth_t depth = (entry1.depth + entry2.depth) / 2;
	atm = dive->depth_to_atm(depth);

	cyl = dive->get_cylinder(index);

	volume_t airuse = cyl->gas_volume(a) - cyl->gas_volume(b);

	/* milliliters per minute */
	return lrint(airuse.mliter / atm * 60 / duration);
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

static void analyze_plot_info(struct plot_info &pi)
{
	/* Smoothing function: 5-point triangular smooth */
	for (size_t i = 2; i < pi.entry.size(); i++) {
		struct plot_data &entry = pi.entry[i];
		depth_t depth;

		if (i + 2 < pi.entry.size()) {
			depth = pi.entry[i-2].depth + pi.entry[i-1].depth * 2 + pi.entry[i].depth * 3 + pi.entry[i+1].depth * 2 + pi.entry[i+2].depth;
			entry.smoothed = (depth + 4_mm) / 9;
		}
		/* vertical velocity in mm/sec */
		/* Linus wants to smooth this - let's at least look at the samples that aren't FAST or CRAZY */
		if (pi.entry[i].sec - pi.entry[i-1].sec) {
			entry.speed = (pi.entry[i+0].depth - pi.entry[i-1].depth).mm / (pi.entry[i].sec - pi.entry[i-1].sec);
			entry.velocity = velocity(entry.speed);
			/* if our samples are short and we aren't too FAST*/
			if (pi.entry[i].sec - pi.entry[i-1].sec < 15 && entry.velocity < FAST) {
				int past = -2;
				while (i + past > 0 && pi.entry[i].sec - pi.entry[i+past].sec < 15)
					past--;
				entry.velocity = velocity((pi.entry[i].depth - pi.entry[i+past].depth).mm /
							   (pi.entry[i].sec - pi.entry[i+past].sec));
			}
		} else {
			entry.velocity = STABLE;
			entry.speed = 0;
		}
	}
}

static size_t set_setpoint(struct plot_info &pi, size_t i, int setpoint, int end)
{
	while (i < pi.entry.size()) {
		struct plot_data &entry = pi.entry[i];
		if (entry.sec > end)
			break;
		entry.o2pressure.mbar = setpoint;
		i++;
	}
	return i;
}

static void check_setpoint_events(const struct dive *, const struct divecomputer *dc, struct plot_info &pi)
{
	size_t i = 0;
	pressure_t setpoint;

	event_loop loop("SP change", *dc);
	bool found = false;
	while (auto ev = loop.next()) {
		i = set_setpoint(pi, i, setpoint.mbar, ev->time.seconds);
		setpoint.mbar = ev->value;
		found = true;
	}
	if (found) // Fill the last setpoint until the end of the dive
		set_setpoint(pi, i, setpoint.mbar, INT_MAX);
}

static void calculate_max_limits_new(const struct dive *dive, const struct divecomputer *given_dc, struct plot_info &pi, bool in_planner)
{
	bool seen = false;
	bool found_sample_beyond_last_event = false;
	int maxdepth = dive->maxdepth.mm;
	int maxtime = 0;
	int maxpressure = 0, minpressure = INT_MAX;
	int maxhr = 0, minhr = INT_MAX;
	int mintemp = dive->mintemp.mkelvin;
	int maxtemp = dive->maxtemp.mkelvin;

	/* Get the per-cylinder maximum pressure if they are manual */
	for (auto &cyl: dive->cylinders) {
		int mbar_start = cyl.start.mbar;
		int mbar_end = cyl.end.mbar;
		if (mbar_start > maxpressure)
			maxpressure = mbar_start;
		if (mbar_end && mbar_end < minpressure)
			minpressure = mbar_end;
	}

	auto process_dc = [&] (const divecomputer &dc) {
		int lastdepth = 0;

		/* Make sure we can fit all events */
		if (!dc.events.empty())
			maxtime = std::max(maxtime, dc.events.back().time.seconds);

		for (auto &s: dc.samples) {
			int depth = s.depth.mm;
			int temperature = s.temperature.mkelvin;
			int heartbeat = s.heartbeat;

			for (int sensor = 0; sensor < MAX_SENSORS; ++sensor) {
				int pressure = s.pressure[sensor].mbar;
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
				maxdepth = s.depth.mm;
			/* Make sure that we get the first sample beyond the last event.
			 * If maxtime is somewhere in the middle of the last segment,
			 * populate_plot_entries() gets confused leading to display artifacts. */
			if ((depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD || in_planner || !found_sample_beyond_last_event) &&
			    s.time.seconds > maxtime) {
				found_sample_beyond_last_event = true;
				maxtime = s.time.seconds;
			}
			lastdepth = depth;
		}
	};

	/* Then do all the samples from all the dive computers */
	for (auto &dc: dive->dcs) {
		if (&dc == given_dc)
			seen = true;
		process_dc(dc);
	}
	if (!seen)
		process_dc(*given_dc);

	if (minpressure > maxpressure)
		minpressure = 0;
	if (minhr > maxhr)
		minhr = maxhr;

	pi.maxdepth = maxdepth;
	pi.maxtime = maxtime;
	pi.maxpressure = maxpressure;
	pi.minpressure = minpressure;
	pi.minhr = minhr;
	pi.maxhr = maxhr;
	pi.mintemp = mintemp;
	pi.maxtemp = maxtemp;
}

static plot_data &add_entry(struct plot_info &pi)
{
	pi.entry.emplace_back();
	pi.pressures.resize(pi.pressures.size() + pi.nr_cylinders);
	return pi.entry.back();
}

/* copy the previous entry (we know this exists), update time and depth
 * and zero out the sensor pressure (since this is a synthetic entry)
 * increment the entry pointer and the count of synthetic entries. */
static void insert_entry(struct plot_info &pi, int time, depth_t depth, int sac)
{
	struct plot_data &entry = add_entry(pi);
	struct plot_data &prev = pi.entry[pi.entry.size() - 2];
	entry = prev;
	entry.sec = time;
	entry.depth = depth;
	entry.running_sum = prev.running_sum + (depth + prev.depth) * (time - prev.sec) / 2;
	entry.sac = sac;
	entry.ndl = -1;
	entry.bearing = -1;
}

static void populate_plot_entries(const struct dive *dive, const struct divecomputer *dc, struct plot_info &pi)
{
	pi.nr_cylinders = static_cast<int>(dive->cylinders.size());

	/*
	 * To avoid continuous reallocation, allocate the expected number of entries.
	 * We want to have a plot_info event at least every 10s (so "maxtime/10+1"),
	 * but samples could be more dense than that (so add in dc->samples). We also
	 * need to have one for every event (so count events and add that) and
	 * additionally we want two surface events around the whole thing (thus the
	 * additional 4).  There is also one extra space for a final entry
	 * that has time > maxtime (because there can be surface samples
	 * past "maxtime" in the original sample data)
	 */
	size_t nr = dc->samples.size() + 6 + pi.maxtime / 10 + dc->events.size();
	pi.entry.reserve(nr);
	pi.pressures.reserve(nr * pi.nr_cylinders);

	// The two extra events at the start
	pi.entry.resize(2);
	pi.pressures.resize(pi.nr_cylinders * 2);

	depth_t lastdepth;
	int lasttime = 0;
	int lasttemp = 0;
	/* skip events at time = 0 */
	auto evit = dc->events.begin();
	while (evit != dc->events.end() && evit->time.seconds == 0)
		++evit;
	for (const auto &sample: dc->samples) {
		int time = sample.time.seconds;
		int offset, delta;
		int sac = sample.sac.mliter;

		/* Add intermediate plot entries if required */
		delta = time - lasttime;
		if (delta <= 0) {
			time = lasttime;
			delta = 1; // avoid divide by 0
		}
		for (offset = 10; offset < delta; offset += 10) {
			if (lasttime + offset > pi.maxtime)
				break;

			/* Add events if they are between plot entries */
			while (evit != dc->events.end() && static_cast<int>(evit->time.seconds) < lasttime + offset) {
				insert_entry(pi, evit->time.seconds, interpolate(lastdepth, sample.depth, evit->time.seconds - lasttime, delta), sac);
				++evit;
			}

			/* now insert the time interpolated entry */
			insert_entry(pi, lasttime + offset, interpolate(lastdepth, sample.depth, offset, delta), sac);

			/* skip events that happened at this time */
			while (evit != dc->events.end() && static_cast<int>(evit->time.seconds) == lasttime + offset)
				++evit;
		}

		/* Add events if they are between plot entries */
		while (evit != dc->events.end() && static_cast<int>(evit->time.seconds) < time) {
			insert_entry(pi, evit->time.seconds, interpolate(lastdepth, sample.depth, evit->time.seconds - lasttime, delta), sac);
			++evit;
		}

		plot_data &entry = add_entry(pi);
		plot_data &prev = pi.entry[pi.entry.size() - 2];
		entry.sec = time;
		entry.depth = sample.depth;

		entry.running_sum = prev.running_sum + (sample.depth + prev.depth) * (time - prev.sec) / 2;
		entry.stopdepth = sample.stopdepth;
		entry.stoptime = sample.stoptime.seconds;
		entry.ndl = sample.ndl.seconds;
		entry.tts = sample.tts.seconds;
		entry.in_deco = sample.in_deco;
		entry.cns = sample.cns;
		if (dc->divemode == CCR || (dc->divemode == PSCR && dc->no_o2sensors)) {
			entry.o2pressure.mbar = entry.o2setpoint.mbar = sample.setpoint.mbar;     // for rebreathers
			int i;
			for (i = 0; i < MAX_O2_SENSORS; i++)
				entry.o2sensor[i].mbar = sample.o2sensor[i].mbar;
		} else {
			entry.pressures.o2 = sample.setpoint.mbar / 1000.0;
		}

		for (auto &mapping: dc->tank_sensor_mappings)
			for (int i = 0; i < MAX_SENSORS; i++)
				if (sample.sensor[i] == mapping.sensor_id && sample.pressure[i].mbar) {
					set_plot_pressure_data(pi, pi.entry.size() - 1, SENSOR_PR, mapping.cylinder_index, sample.pressure[i].mbar);

					break;
				}

		if (sample.temperature.mkelvin)
			entry.temperature = lasttemp = sample.temperature.mkelvin;
		else
			entry.temperature = lasttemp;
		entry.heartbeat = sample.heartbeat;
		entry.bearing = sample.bearing.degrees;
		entry.sac = sample.sac.mliter;
		if (sample.rbt.seconds)
			entry.rbt = sample.rbt.seconds;
		/* skip events that happened at this time */
		while (evit != dc->events.end() && static_cast<int>(evit->time.seconds) == time)
			++evit;
		lasttime = time;
		lastdepth = entry.depth;

		if (time > pi.maxtime)
			break;
	}

	/* Add any remaining events */
	while (evit != dc->events.end()) {
		int time = evit->time.seconds;

		if (time > lasttime) {
			insert_entry(pi, evit->time.seconds, 0_m, 0);
			lasttime = time;
		}
		++evit;
	}

	/* Add two final surface events */
	add_entry(pi).sec = lasttime + 1;
	add_entry(pi).sec = lasttime + 2;
	pi.nr = (int)pi.entry.size();
}

/*
 * Calculate the sac rate between the two plot entries 'first' and 'last'.
 *
 * Everything in between has a cylinder pressure for at least some of the cylinders.
 */
static int sac_between(const struct dive *dive, const struct plot_info &pi, int first, int last, const char gases[])
{
	if (first == last)
		return 0;

	/* Get airuse for the set of cylinders over the range */
	volume_t airuse;
	for (int i = 0; i < pi.nr_cylinders; i++) {
		pressure_t a, b;

		if (!gases[i])
			continue;

		a.mbar = get_plot_pressure(pi, first, i);
		b.mbar = get_plot_pressure(pi, last, i);
		const cylinder_t *cyl = dive->get_cylinder(i);
		// TODO: Implement addition/subtraction on units.h types
		volume_t cyluse = cyl->gas_volume(a) - cyl->gas_volume(b);
		if (cyluse.mliter > 0)
			airuse += cyluse;
	}
	if (!airuse.mliter)
		return 0;

	/* Calculate depthpressure integrated over time */
	double pressuretime = 0.0;
	do {
		const struct plot_data &entry = pi.entry[first];
		const struct plot_data &next = pi.entry[first + 1];
		depth_t depth = (entry.depth + next.depth) / 2;
		int time = next.sec - entry.sec;
		double atm = dive->depth_to_atm(depth);

		pressuretime += atm * time;
	} while (++first < last);

	/* Turn "atmseconds" into "atmminutes" */
	pressuretime /= 60;

	/* SAC = mliter per minute */
	return lrint(airuse.mliter / pressuretime);
}

/* Is there pressure data for all gases? */
static bool all_pressures(const struct plot_info &pi, int idx, const char gases[])
{
	int i;

	for (i = 0; i < pi.nr_cylinders; i++) {
		if (gases[i] && !get_plot_pressure(pi, idx, i))
			return false;
	}

	return true;
}

/* Which of the set of gases have pressure data? Returns false if none of them. */
static bool filter_pressures(const struct plot_info &pi, int idx, const char gases_in[], char gases_out[])
{
	int i;
	bool has_pressure = false;

	for (i = 0; i < pi.nr_cylinders; i++) {
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
static void fill_sac(const struct dive *dive, struct plot_info &pi, int idx, const char gases_in[], char gases[])
{
	struct plot_data &entry = pi.entry[idx];
	int first, last;
	int time;

	if (entry.sac)
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
	time = entry.sec - 30;
	while (idx > 0) {
		const struct plot_data &entry = pi.entry[idx];
		const struct plot_data &prev = pi.entry[idx - 1];

		if (prev.depth.mm < SURFACE_THRESHOLD && entry.depth.mm < SURFACE_THRESHOLD)
			break;
		if (prev.sec < time)
			break;
		if (!all_pressures(pi, idx - 1, gases))
			break;
		idx--;
		first = idx;
	}

	/* Now find an entry a minute after the first one */
	last = first;
	time = pi.entry[first].sec + 60;
	while (++idx < pi.nr) {
		const struct plot_data &entry = pi.entry[last];
		const struct plot_data &next = pi.entry[last + 1];
		if (next.depth.mm < SURFACE_THRESHOLD && entry.depth.mm < SURFACE_THRESHOLD)
			break;
		if (next.sec > time)
			break;
		if (!all_pressures(pi, idx + 1, gases))
			break;
		last = idx;
	}

	/* Ok, now calculate the SAC between 'first' and 'last' */
	entry.sac = sac_between(dive, pi, first, last, gases);
}

/*
 * Create a bitmap of cylinders that match our current gasmix
 */
static void matching_gases(const struct dive *dive, struct gasmix gasmix, char gases[])
{
	for (auto [i, cyl]: enumerated_range(dive->cylinders))
		gases[i] = same_gasmix(gasmix, cyl.gasmix);
}

static void calculate_sac(const struct dive *dive, const struct divecomputer *dc, struct plot_info &pi)
{
	std::vector<char> gases(pi.nr_cylinders, false);

	/* This might be premature optimization, but let's allocate the gas array for
	 * the fill_sac function only once an not once per sample */
	std::vector<char> gases_scratch(pi.nr_cylinders);

	struct gasmix gasmix = gasmix_invalid;
	gasmix_loop loop(*dive, *dc);
	for (int i = 0; i < pi.nr; i++) {
		const struct plot_data &entry = pi.entry[i];
		struct gasmix newmix = loop.at(entry.sec).first;
		if (!same_gasmix(newmix, gasmix)) {
			gasmix = newmix;
			matching_gases(dive, newmix, gases.data());
		}

		fill_sac(dive, pi, i, gases.data(), gases_scratch.data());
	}
}

static void populate_secondary_sensor_data(const struct divecomputer &dc, struct plot_info &pi)
{
	std::vector<int> seen(pi.nr_cylinders, 0);
	for (int idx = 0; idx < pi.nr; ++idx)
		for (int c = 0; c < pi.nr_cylinders; ++c)
			if (get_plot_pressure_data(pi, idx, SENSOR_PR, c))
				++seen[c]; // Count instances so we can differentiate a real sensor from just start and end pressure
	int idx = 0;
	/* We should try to see if it has interesting pressure data here */
	for (const auto &sample: dc.samples) {
		if (idx >= pi.nr)
			break;
		for (; idx < pi.nr; ++idx) {
			if (idx == pi.nr - 1 || pi.entry[idx].sec >= sample.time.seconds)
				// We've either found the entry at or just after the sample's time,
				// or this is the last entry so use for the last sensor readings if there are any.
				break;
		}

		for (auto &mapping: dc.tank_sensor_mappings)
			// Copy sensor data if available, but don't add if this dc already has sensor data
			for (int i = 0; i < MAX_SENSORS; i++)
				if (sample.sensor[i] == mapping.sensor_id && sample.pressure[i].mbar && seen[mapping.cylinder_index] < 3) {
					set_plot_pressure_data(pi, pi.entry.size() - 1, SENSOR_PR, mapping.cylinder_index, sample.pressure[i].mbar);

					break;
				}
	}
}

/*
 * This adds a pressure entry to the plot_info based on the gas change
 * information and the manually filled in pressures.
 */
static void add_plot_pressure(struct plot_info &pi, int time, int cyl, pressure_t p)
{
	for (int i = 0; i < pi.nr; i++) {
		if (i == pi.nr - 1 || pi.entry[i].sec >= time) {
			set_plot_pressure_data(pi, i, SENSOR_PR, cyl, p.mbar);
			return;
		}
	}
}

static void setup_gas_sensor_pressure(const struct dive *dive, const struct divecomputer *dc, struct plot_info &pi)
{
	if (pi.nr_cylinders == 0)
		return;

	/* FIXME: The planner uses a dummy one-past-end cylinder for surface air! */
	int num_cyl = pi.nr_cylinders + 1;
	std::vector<int> seen(num_cyl, false);
	std::vector<int> first(num_cyl, 0);
	std::vector<int> last(num_cyl, INT_MAX);

	int prev = -1;
	gasmix_loop loop(*dive, *dc);
	while (loop.has_next()) {
		auto [cylinder_index, time] = loop.next_cylinder_index();

		if (cylinder_index < 0)
			continue; // unknown cylinder
		if (cylinder_index >= num_cyl) {
			report_info("setup_gas_sensor_pressure(): invalid cylinder idx %d", cylinder_index);
			continue;
		}

		if (prev >= 0) {
			last[prev] = time;

			if (!seen[cylinder_index])
				first[cylinder_index] = time;
		}

		seen[cylinder_index] = true;

		prev = cylinder_index;
	}
	last[prev] = INT_MAX;

	// Fill in "seen[]" array - mark cylinders we're not interested
	// in as negative.
	for (int i = 0; i < pi.nr_cylinders; i++) {
		const cylinder_t *cyl = dive->get_cylinder(i);
		int start = cyl->start.mbar;
		int end = cyl->end.mbar;

		/*
		 * Fundamentally uninteresting?
		 *
		 * A dive computer with no pressure data isn't interesting
		 * to plot pressures for even if we've seen it..
		 */
		if (!start || !end || start == end) {
			seen[i] = false;

			continue;
		}

		/* If we've seen it, we're definitely interested */
		if (seen[i])
			continue;

		/* If it's mentioned by other dcs, ignore it */
		bool other_divecomputer = false;
		for (auto &secondary: dive->dcs)
			if (dive->has_gaschange_event(&secondary, i))
				other_divecomputer = true;

		if (!other_divecomputer)
			seen[i] = true;
	}

	for (int i = 0; i < pi.nr_cylinders; i++) {
		if (seen[i]) {
			const cylinder_t *cyl = dive->get_cylinder(i);

			add_plot_pressure(pi, first[i], i, cyl->start);
			add_plot_pressure(pi, last[i], i, cyl->end);
		}
	}

	/*
	 * Here, we should try to walk through all the dive computers,
	 * and try to see if they have sensor data different from the
	 * current dive computer (dc).
	 */
	for (auto &secondary: dive->dcs) {
		if (&secondary == dc)
			continue;
		populate_secondary_sensor_data(secondary, pi);
	}
}

/* calculate DECO STOP / TTS / NDL */
static void calculate_ndl_tts(struct deco_state *ds, const struct dive *dive, struct plot_data &entry, struct gasmix gasmix,
			      double surface_pressure, enum divemode_t divemode, bool in_planner)
{
	/* should this be configurable? */
	/* ascent speed up to first deco stop */
	const int ascent_s_per_step = 1;
	const int ascent_s_per_deco_step = 1;
	/* how long time steps in deco calculations? */
	const int time_stepsize = 60;
	const depth_t deco_stepsize = m_or_ft(3, 10);
	/* at what depth is the current deco-step? */
	depth_t ascent_depth = entry.depth;
	depth_t next_stop { .mm = round_up(deco_allowed_depth(
					 tissue_tolerance_calc(ds, dive, dive->depth_to_bar(ascent_depth), in_planner),
					 surface_pressure, dive, 1).mm, deco_stepsize.mm)};
	/* at what time should we give up and say that we got enuff NDL? */
	/* If iterating through a dive, entry.tts_calc needs to be reset */
	entry.tts_calc = 0;

	/* If we don't have a ceiling yet, calculate ndl. Don't try to calculate
	 * a ndl for lower values than 3m it would take forever */
	if (next_stop.mm <= 0) {
		if (entry.depth.mm < 3000) {
			entry.ndl = MAX_PROFILE_DECO;
			return;
		}
		/* stop if the ndl is above max_ndl seconds, and call it plenty of time */
		while (entry.ndl_calc < MAX_PROFILE_DECO &&
		       deco_allowed_depth(tissue_tolerance_calc(ds, dive, dive->depth_to_bar(ascent_depth), in_planner),
					  surface_pressure, dive, 1).mm <= 0
		       ) {
			entry.ndl_calc += time_stepsize;
			add_segment(ds, dive->depth_to_bar(ascent_depth),
				    gasmix, time_stepsize, entry.o2pressure.mbar, divemode, prefs.bottomsac, in_planner);
		}
		/* we don't need to calculate anything else */
		return;
	}

	/* We are in deco */
	entry.in_deco_calc = true;

	/* Add segments for movement to stopdepth */
	for (; ascent_depth.mm > next_stop.mm; ascent_depth.mm -= ascent_s_per_step * ascent_velocity(ascent_depth, entry.running_sum / entry.sec, 0), entry.tts_calc += ascent_s_per_step) {
		add_segment(ds, dive->depth_to_bar(ascent_depth),
			    gasmix, ascent_s_per_step, entry.o2pressure.mbar, divemode, prefs.decosac, in_planner);
		next_stop.mm = round_up(deco_allowed_depth(tissue_tolerance_calc(ds, dive, dive->depth_to_bar(ascent_depth), in_planner),
							surface_pressure, dive, 1).mm, deco_stepsize.mm);
	}
	ascent_depth = next_stop;

	/* And how long is the current deco-step? */
	entry.stoptime_calc = 0;
	entry.stopdepth_calc = next_stop;
	next_stop -= deco_stepsize;

	/* And how long is the total TTS */
	while (next_stop.mm >= 0) {
		/* save the time for the first stop to show in the graph */
		if (ascent_depth.mm == entry.stopdepth_calc.mm)
			entry.stoptime_calc += time_stepsize;

		entry.tts_calc += time_stepsize;
		if (entry.tts_calc > MAX_PROFILE_DECO)
			break;
		add_segment(ds, dive->depth_to_bar(ascent_depth),
			    gasmix, time_stepsize, entry.o2pressure.mbar, divemode, prefs.decosac, in_planner);

		if (deco_allowed_depth(tissue_tolerance_calc(ds, dive, dive->depth_to_bar(ascent_depth), in_planner), surface_pressure, dive, 1).mm <= next_stop.mm) {
			/* move to the next stop and add the travel between stops */
			for (; ascent_depth.mm > next_stop.mm; ascent_depth.mm -= ascent_s_per_deco_step * ascent_velocity(ascent_depth, entry.running_sum / entry.sec, 0), entry.tts_calc += ascent_s_per_deco_step)
				add_segment(ds, dive->depth_to_bar(ascent_depth),
					    gasmix, ascent_s_per_deco_step, entry.o2pressure.mbar, divemode, prefs.decosac, in_planner);
			ascent_depth = next_stop;
			next_stop -= deco_stepsize;
		}
	}
}

/* Let's try to do some deco calculations.
 */
static void calculate_deco_information(struct deco_state *ds, const struct deco_state *planner_ds, const struct dive *dive,
				       const struct divecomputer *dc, struct plot_info &pi)
{
	int i, count_iteration = 0;
	double surface_pressure = (dc->surface_pressure.mbar ? dc->surface_pressure.mbar : dive->get_surface_pressure().mbar) / 1000.0;
	bool first_iteration = true;
	int prev_deco_time = 10000000, time_deep_ceiling = 0;
	bool in_planner = planner_ds != NULL;

	if (!in_planner) {
		ds->deco_time = 0;
		ds->first_ceiling_pressure = 0_bar;
	} else {
		ds->deco_time = planner_ds->deco_time;
		ds->first_ceiling_pressure = planner_ds->first_ceiling_pressure;
	}
	deco_state_cache cache_data_initial;
	lock_planner();
	/* For VPM-B outside the planner, cache the initial deco state for CVA iterations */
	if (decoMode(in_planner) == VPMB) {
		cache_data_initial.cache(ds);
	}
	/* For VPM-B outside the planner, iterate until deco time converges (usually one or two iterations after the initial)
	 * Set maximum number of iterations to 10 just in case */

	while ((abs(prev_deco_time - ds->deco_time) >= 30) && (count_iteration < 10)) {
		depth_t first_ceiling, current_ceiling, last_ceiling;
		int last_ndl_tts_calc_time = 0, final_tts = 0 , time_clear_ceiling = 0;
		if (decoMode(in_planner) == VPMB)
			ds->first_ceiling_pressure.mbar = dive->depth_to_mbar(first_ceiling);

		gasmix_loop loop_gas(*dive, *dc);
		divemode_loop loop_mode(*dc);
		for (i = 1; i < pi.nr; i++) {
			struct plot_data &entry = pi.entry[i];
			struct plot_data &prev = pi.entry[i - 1];
			int t0 = prev.sec, t1 = entry.sec;
			int time_stepsize = 20;
			depth_t max_ceiling;

			[[maybe_unused]] auto [current_divemode, _cylinder_index, gasmix] = get_dive_status_at(*dive, *dc, entry.sec, &loop_mode, &loop_gas);

			entry.ambpressure = dive->depth_to_bar(entry.depth);
			entry.gfline = get_gf(ds, entry.ambpressure, dive) * (100.0 - AMB_PERCENTAGE) + AMB_PERCENTAGE;
			if (t0 > t1) {
				report_info("non-monotonous dive stamps %d %d", t0, t1);
				std::swap(t0, t1);
			}
			if (t0 != t1 && t1 - t0 < time_stepsize)
				time_stepsize = t1 - t0;
			for (int j = t0 + time_stepsize; j <= t1; j += time_stepsize) {
				depth_t new_depth = interpolate(prev.depth, entry.depth, j - t0, t1 - t0);
				add_segment(ds, dive->depth_to_bar(new_depth),
					    *gasmix, time_stepsize, entry.o2pressure.mbar, current_divemode, entry.sac, in_planner);
				entry.icd_warning = ds->icd_warning;
				if ((t1 - j < time_stepsize) && (j < t1))
					time_stepsize = t1 - j;
			}
			if (t0 == t1) {
				entry.ceiling = prev.ceiling;
			} else {
				/* Keep updating the VPM-B gradients until the start of the ascent phase of the dive. */
				if (decoMode(in_planner) == VPMB && last_ceiling.mm >= first_ceiling.mm && first_iteration == true) {
					nuclear_regeneration(ds, t1);
					vpmb_start_gradient(ds);
					/* For CVA iterations, calculate next gradient */
					if (!first_iteration || in_planner)
						vpmb_next_gradient(ds, ds->deco_time, surface_pressure / 1000.0, in_planner);
				}
				entry.ceiling = deco_allowed_depth(tissue_tolerance_calc(ds, dive, dive->depth_to_bar(entry.depth), in_planner), surface_pressure, dive, !prefs.calcceiling3m);
				if (prefs.calcceiling3m)
					current_ceiling = deco_allowed_depth(tissue_tolerance_calc(ds, dive, dive->depth_to_bar(entry.depth), in_planner), surface_pressure, dive, true);
				else
					current_ceiling = entry.ceiling;
				last_ceiling = current_ceiling;
				/* If using VPM-B, take first_ceiling_pressure as the deepest ceiling */
				if (decoMode(in_planner) == VPMB) {
					if  (current_ceiling.mm >= first_ceiling.mm ||
					     (time_deep_ceiling == t0 && entry.depth.mm == prev.depth.mm)) {
						time_deep_ceiling = t1;
						first_ceiling = current_ceiling;
						ds->first_ceiling_pressure.mbar = dive->depth_to_mbar(first_ceiling);
						if (first_iteration) {
							nuclear_regeneration(ds, t1);
							vpmb_start_gradient(ds);
							/* For CVA calculations, deco time = dive time remaining is a good guess,
							   but we want to over-estimate deco_time for the first iteration so it
							   converges correctly, so add 30min*/
							if (!in_planner)
								ds->deco_time = pi.maxtime - t1 + 1800;
							vpmb_next_gradient(ds, ds->deco_time, surface_pressure / 1000.0, in_planner);
						}
					}
					// Use the point where the ceiling clears as the end of deco phase for CVA calculations
					if (current_ceiling.mm > 0)
						time_clear_ceiling = 0;
					else if (time_clear_ceiling == 0 && t1 > time_deep_ceiling)
						time_clear_ceiling = t1;
				}
			}
			entry.surface_gf = 0.0;
			entry.current_gf = 0.0;
			for (int j = 0; j < 16; j++) {
				double m_value = ds->buehlmann_inertgas_a[j] + entry.ambpressure / ds->buehlmann_inertgas_b[j];
				double surface_m_value = ds->buehlmann_inertgas_a[j] + surface_pressure / ds->buehlmann_inertgas_b[j];
				entry.ceilings[j] = deco_allowed_depth(ds->tolerated_by_tissue[j], surface_pressure, dive, 1);
				if (entry.ceilings[j].mm > max_ceiling.mm)
					max_ceiling = entry.ceilings[j];
				double current_gf = (ds->tissue_inertgas_saturation[j] - entry.ambpressure) / (m_value - entry.ambpressure);
				entry.percentages[j] = ds->tissue_inertgas_saturation[j] < entry.ambpressure ?
					lrint(ds->tissue_inertgas_saturation[j] / entry.ambpressure * AMB_PERCENTAGE) :
					lrint(AMB_PERCENTAGE + current_gf * (100.0 - AMB_PERCENTAGE));
				if (current_gf > entry.current_gf)
					entry.current_gf = current_gf;
				double surface_gf = 100.0 * (ds->tissue_inertgas_saturation[j] - surface_pressure) / (surface_m_value - surface_pressure);
				if (surface_gf > entry.surface_gf)
					entry.surface_gf = surface_gf;
			}

			// In the planner, if the ceiling is violated, add an event.
			// TODO: This *really* shouldn't be done here. This is a contract
			// between the planner and the profile that the planner uses a dive
			// that can be trampled upon. But ultimately, the ceiling-violation
			// marker should be handled differently!
			// Don't scream if we violate the ceiling by a few cm.
			if (in_planner && !pi.waypoint_above_ceiling &&
			    entry.depth.mm < max_ceiling.mm - 100 && entry.sec > 0) {
				struct dive *non_const_dive = (struct dive *)dive; // cast away const!
				add_event(&non_const_dive->dcs[0], entry.sec, SAMPLE_EVENT_CEILING, -1, max_ceiling.mm / 1000,
					  translate("gettextFromC", "planned waypoint above ceiling"));
				pi.waypoint_above_ceiling = true;
			}

			/* should we do more calculations?
			* We don't for print-mode because this info doesn't show up there
			* If the ceiling hasn't cleared by the last data point, we need tts for VPM-B CVA calculation
			* It is not necessary to do these calculation on the first VPMB iteration, except for the last data point */
			if ((prefs.calcndltts && (decoMode(in_planner) != VPMB || in_planner || !first_iteration)) ||
			    (decoMode(in_planner) == VPMB && !in_planner && i == pi.nr - 1)) {
				/* only calculate ndl/tts on every 30 seconds */
				if ((entry.sec - last_ndl_tts_calc_time) < 30 && i != pi.nr - 1) {
					struct plot_data &prev_entry = pi.entry[i - 1];
					entry.stoptime_calc = prev_entry.stoptime_calc;
					entry.stopdepth_calc = prev_entry.stopdepth_calc;
					entry.tts_calc = prev_entry.tts_calc;
					entry.ndl_calc = prev_entry.ndl_calc;
					continue;
				}
				last_ndl_tts_calc_time = entry.sec;

				/* We are going to mess up deco state, so store it for later restore */
				deco_state_cache cache_data;
				cache_data.cache(ds);
				calculate_ndl_tts(ds, dive, entry, *gasmix, surface_pressure, current_divemode, in_planner);
				if (decoMode(in_planner) == VPMB && !in_planner && i == pi.nr - 1)
					final_tts = entry.tts_calc;
				/* Restore "real" deco state for next real time step */
				cache_data.restore(ds, decoMode(in_planner) == VPMB);
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
					ds->deco_time = round_up(time_clear_ceiling - time_deep_ceiling + 20, 60) + 20;
			vpmb_next_gradient(ds, ds->deco_time, surface_pressure / 1000.0, in_planner);
			final_tts = 0;
			last_ndl_tts_calc_time = 0;
			first_ceiling = 0_m;
			first_iteration = false;
			count_iteration ++;
			this_deco_time = ds->deco_time;
			cache_data_initial.restore(ds, true);
			ds->deco_time = this_deco_time;
		} else {
			// With Buhlmann iterating isn't needed.  This makes the while condition false.
			prev_deco_time = ds->deco_time = 0;
		}
	}

#if DECO_CALC_DEBUG & 1
	dump_tissues(ds);
#endif
	unlock_planner();
}

/* Sort the o2 pressure values. There are so few that a simple bubble sort
 * will do */

static void sort_o2_pressures(int *sensorn, int np, const struct plot_data &entry)
{
	int smallest, position, old;

	for (int i = 0; i < np - 1; i++) {
		position = i;
		smallest = entry.o2sensor[sensorn[i]].mbar;
		for (int j = i+1; j < np; j++)
			if (entry.o2sensor[sensorn[j]].mbar < smallest) {
				position = j;
				smallest = entry.o2sensor[sensorn[j]].mbar;
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
static int calculate_ccr_po2(struct plot_data &entry, const struct divecomputer *dc)
{
	int sump = 0, minp = 0, maxp = 0;
	int sensorn[MAX_O2_SENSORS];
	int i, np = 0;

	for (i = 0; i < dc->no_o2sensors && i < MAX_O2_SENSORS; i++)
		if (entry.o2sensor[i].mbar) { // Valid reading
			sensorn[np++] = i;
			sump += entry.o2sensor[i].mbar;
		}
	if (np == 0)
		return entry.o2pressure.mbar;
	else if (np == 1)
		return entry.o2sensor[sensorn[0]].mbar;

	maxp = np - 1;
	sort_o2_pressures(sensorn, np, entry);

	// This is the Shearwater voting logic: If there are still at least three sensors and one
	// differs by more than 20% from the closest it is voted out.
	while (maxp - minp > 1) {
		if (entry.o2sensor[sensorn[minp + 1]].mbar - entry.o2sensor[sensorn[minp]].mbar >
		    sump / (maxp - minp + 1) / 5) {
			sump -= entry.o2sensor[sensorn[minp]].mbar;
			++minp;
			continue;
		}
		if (entry.o2sensor[sensorn[maxp]].mbar - entry.o2sensor[sensorn[maxp - 1]].mbar >
		    sump / (maxp - minp +1) / 5) {
			sump -= entry.o2sensor[sensorn[maxp]].mbar;
			--maxp;
			continue;
		}
		break;
	}

	return sump / (maxp - minp + 1);

}

static double gas_density(const struct gas_pressures &pressures)
{
	return (pressures.o2 * O2_DENSITY + pressures.he * HE_DENSITY + pressures.n2 * N2_DENSITY) / 1000.0;
}

static void calculate_gas_information_new(const struct dive *dive, const struct divecomputer *dc, struct plot_info &pi)
{
	int i;
	double amb_pressure;

	gasmix_loop loop_gas(*dive, *dc);
	divemode_loop loop_mode(*dc);
	for (i = 1; i < pi.nr; i++) {
		double fn2, fhe;
		struct plot_data &entry = pi.entry[i];

		[[maybe_unused]] auto [current_divemode, _cylinder_index, gasmix] = get_dive_status_at(*dive, *dc, entry.sec, &loop_mode, &loop_gas);
		amb_pressure = dive->depth_to_bar(entry.depth);
		entry.pressures = fill_pressures(amb_pressure, *gasmix, (current_divemode == OC) ? 0.0 : entry.o2pressure.mbar / 1000.0, current_divemode);
		fn2 = 1000.0 * entry.pressures.n2 / amb_pressure;
		fhe = 1000.0 * entry.pressures.he / amb_pressure;
		if (dc->divemode == PSCR) { // OC pO2 is calulated for PSCR with or without external PO2 monitoring.
			entry.scr_OC_pO2.mbar = (int) dive->depth_to_mbar(entry.depth) * get_o2(*gasmix) / 1000;
		}

		/* Calculate MOD, EAD, END and EADD based on partial pressures calculated before
		 * so there is no difference in calculating between OC and CC
		 * END takes O₂ + N₂ (air) into account ("Narcotic" for trimix dives)
		 * EAD just uses N₂ ("Air" for nitrox dives) */
		pressure_t modpO2 = { .mbar = (int)(prefs.modpO2 * 1000) };
		entry.mod = dive->gas_mod(*gasmix, modpO2, 1_mm);
		entry.end = dive->mbar_to_depth(lrint(dive->depth_to_mbarf(entry.depth) * (1000 - fhe) / 1000.0));
		entry.ead = dive->mbar_to_depth(lrint(dive->depth_to_mbarf(entry.depth) * fn2 / (double)N2_IN_AIR));
		entry.eadd = dive->mbar_to_depth(lrint(dive->depth_to_mbarf(entry.depth) *
				      (entry.pressures.o2 / amb_pressure * O2_DENSITY +
				       entry.pressures.n2 / amb_pressure * N2_DENSITY +
				       entry.pressures.he / amb_pressure * HE_DENSITY) /
				      (O2_IN_AIR * O2_DENSITY + N2_IN_AIR * N2_DENSITY) * 1000));
		entry.density = gas_density(entry.pressures);
		if (entry.mod.mm < 0)
			entry.mod = 0_m;
		if (entry.ead.mm < 0)
			entry.ead = 0_m;
		if (entry.end.mm < 0)
			entry.end = 0_m;
		if (entry.eadd.mm < 0)
			entry.eadd = 0_m;
	}
}

static void fill_o2_values(const struct dive *dive, const struct divecomputer *dc, struct plot_info &pi)
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

	for (i = 0; i < pi.nr; i++) {
		struct plot_data &entry = pi.entry[i];

		if (dc->divemode == CCR || (dc->divemode == PSCR && dc->no_o2sensors)) {
			if (i == 0) { // For 1st iteration, initialise the last_sensor values
				for (j = 0; j < dc->no_o2sensors; j++)
					last_sensor[j] = entry.o2sensor[j];
			} else { // Now re-insert the missing oxygen pressure values
				for (j = 0; j < dc->no_o2sensors; j++)
					if (entry.o2sensor[j].mbar)
						last_sensor[j] = entry.o2sensor[j];
					else
						entry.o2sensor[j] = last_sensor[j];
			} // having initialised the empty o2 sensor values for this point on the profile,
			amb_pressure.mbar = dive->depth_to_mbar(entry.depth);
			o2pressure.mbar = calculate_ccr_po2(entry, dc); // ...calculate the po2 based on the sensor data
			entry.o2pressure.mbar = std::min(o2pressure.mbar, amb_pressure.mbar);
		} else {
			entry.o2pressure = 0_bar; // initialise po2 to zero for dctype = OC
		}
	}
}

#ifdef DEBUG_GAS
/* A CCR debug function that writes the cylinder pressure and the oxygen values to the file debug_print_profiledata.dat:
 * Called in create_plot_info_new()
 */
static void debug_print_profiledata(struct plot_info &pi)
{
	FILE *f1;
	struct plot_data *entry;
	int i;
	if (!(f1 = fopen("debug_print_profiledata.dat", "w"))) {
		printf("File open error for: debug_print_profiledata.dat\n");
	} else {
		fprintf(f1, "id t1 gas gasint t2 t3 dil dilint t4 t5 setpoint sensor1 sensor2 sensor3 t6 po2 fo2\n");
		for (i = 0; i < pi.nr; i++) {
			struct plot_data &entry = pi.entry[i];
			fprintf(f1, "%d gas=%8d %8d ; dil=%8d %8d ; o2_sp= %d %d %d %d %d %d %d PO2= %f\n", i, get_plot_sensor_pressure(pi, i),
				get_plot_interpolated_pressure(pi, i), O2CYLINDER_PRESSURE(entry), INTERPOLATED_O2CYLINDER_PRESSURE(entry),
				entry.o2pressure.mbar, entry.o2sensor[0].mbar, entry.o2sensor[1].mbar, entry.o2sensor[2].mbar, entry.o2sensor[3].mbar, entry.o2sensor[4].mbar, entry.o2sensor[5].mbar, entry.pressures.o2);
		}
		fclose(f1);
	}
}
#endif

/*
 * Create a plot-info with smoothing and ranged min/max
 *
 * This also makes sure that we have extra empty events on both
 * sides, so that you can do end-points without having to worry
 * about it.
 *
 * The old data will be freed.
 */
struct plot_info create_plot_info_new(const struct dive *dive, const struct divecomputer *dc, const struct deco_state *planner_ds)
{
	struct deco_state plot_deco_state;
	bool in_planner = planner_ds != NULL;
	divelog.dives.init_decompression(&plot_deco_state, dive, in_planner);
	plot_info pi;
	calculate_max_limits_new(dive, dc, pi, in_planner);
	auto [o2, he, o2max ] = dive->get_maximal_gas();
	if (dc->divemode == FREEDIVE) {
		pi.dive_type = plot_info::FREEDIVING;
	} else if (he > 0) {
		pi.dive_type = plot_info::TRIMIX;
	} else {
		if (o2)
			pi.dive_type = plot_info::NITROX;
		else
			pi.dive_type = plot_info::AIR;
	}

	populate_plot_entries(dive, dc, pi);

	check_setpoint_events(dive, dc, pi);     /* Populate setpoints */
	setup_gas_sensor_pressure(dive, dc, pi); /* Try to populate our gas pressure knowledge */
	for (int cyl = 0; cyl < pi.nr_cylinders; cyl++)
		populate_pressure_information(dive, dc, pi, cyl);
	fill_o2_values(dive, dc, pi);			 /* .. and insert the O2 sensor data having 0 values. */
	calculate_sac(dive, dc, pi);			 /* Calculate sac */

	calculate_deco_information(&plot_deco_state, planner_ds, dive, dc, pi); /* and ceiling information, using gradient factor values in Preferences) */

	calculate_gas_information_new(dive, dc, pi);	 /* Calculate gas partial pressures */

#ifdef DEBUG_GAS
	debug_print_profiledata(pi);
#endif

	pi.meandepth = dive->dcs[0].meandepth.mm;
	analyze_plot_info(pi);
	return pi;
}

static std::vector<std::string> plot_string(const struct dive *d, const struct plot_info &pi, int idx)
{
	int pressurevalue;
	depth_t mod, ead, end, eadd;
	const char *depth_unit, *pressure_unit, *temp_unit, *vertical_speed_unit;
	double depthvalue, tempvalue, speedvalue, sacvalue;
	int decimals, cyl;
	const char *unit;
	const struct plot_data &entry = pi.entry[idx];
	std::vector<std::string> res;

	depthvalue = get_depth_units(entry.depth, NULL, &depth_unit);
	res.push_back(casprintf_loc(translate("gettextFromC", "@: %d:%02d"), FRACTION_TUPLE(entry.sec, 60)));
	res.push_back(casprintf_loc(translate("gettextFromC", "D: %.1f%s"), depthvalue, depth_unit));
	for (cyl = 0; cyl < pi.nr_cylinders; cyl++) {
		int mbar = get_plot_pressure(pi, idx, cyl);
		if (!mbar)
			continue;
		struct gasmix mix = d->get_cylinder(cyl)->gasmix;
		pressurevalue = get_pressure_units(mbar, &pressure_unit);
		res.push_back(casprintf_loc(translate("gettextFromC", "P: %d%s (%s)"), pressurevalue, pressure_unit, mix.name().c_str()));
	}
	if (entry.temperature) {
		tempvalue = get_temp_units(entry.temperature, &temp_unit);
		res.push_back(casprintf_loc(translate("gettextFromC", "T: %.1f%s"), tempvalue, temp_unit));
	}
	speedvalue = get_vertical_speed_units(abs(entry.speed), NULL, &vertical_speed_unit);
	/* Ascending speeds are positive, descending are negative */
	if (entry.speed > 0)
		speedvalue *= -1;
	res.push_back(casprintf_loc(translate("gettextFromC", "V: %.1f%s"), speedvalue, vertical_speed_unit));
	sacvalue = get_volume_units(entry.sac, &decimals, &unit);
	if (entry.sac && prefs.show_sac)
		res.push_back(casprintf_loc(translate("gettextFromC", "SAC: %.*f%s/min"), decimals, sacvalue, unit));
	if (entry.cns)
		res.push_back(casprintf_loc(translate("gettextFromC", "CNS: %u%%"), entry.cns));
	if (prefs.pp_graphs.po2 && entry.pressures.o2 > 0) {
		res.push_back(casprintf_loc(translate("gettextFromC", "pO₂: %.2fbar"), entry.pressures.o2));
		if (entry.scr_OC_pO2.mbar)
			res.push_back(casprintf_loc(translate("gettextFromC", "SCR ΔpO₂: %.2fbar"), entry.scr_OC_pO2.mbar/1000.0 - entry.pressures.o2));
	}
	if (prefs.pp_graphs.pn2 && entry.pressures.n2 > 0)
		res.push_back(casprintf_loc(translate("gettextFromC", "pN₂: %.2fbar"), entry.pressures.n2));
	if (prefs.pp_graphs.phe && entry.pressures.he > 0)
		res.push_back(casprintf_loc(translate("gettextFromC", "pHe: %.2fbar"), entry.pressures.he));
	if (prefs.mod && entry.mod.mm > 0) {
		mod.mm = lrint(get_depth_units(entry.mod, NULL, &depth_unit));
		res.push_back(casprintf_loc(translate("gettextFromC", "MOD: %d%s"), mod.mm, depth_unit));
	}
	eadd.mm = lrint(get_depth_units(entry.eadd, NULL, &depth_unit));

	if (prefs.ead) {
		switch (pi.dive_type) {
		case plot_info::NITROX:
			if (entry.ead.mm > 0) {
				ead.mm = lrint(get_depth_units(entry.ead, NULL, &depth_unit));
				res.push_back(casprintf_loc(translate("gettextFromC", "EAD: %d%s"), ead.mm, depth_unit));
				res.push_back(casprintf_loc(translate("gettextFromC", "EADD: %d%s / %.1fg/L"), eadd.mm, depth_unit, entry.density));
				break;
			}
		case plot_info::TRIMIX:
			if (entry.end.mm > 0) {
				end.mm = lrint(get_depth_units(entry.end, NULL, &depth_unit));
				res.push_back(casprintf_loc(translate("gettextFromC", "END: %d%s"), end.mm, depth_unit));
				res.push_back(casprintf_loc(translate("gettextFromC", "EADD: %d%s / %.1fg/L"), eadd.mm, depth_unit, entry.density));
				break;
			}
		case plot_info::AIR:
			if (entry.density > 0) {
				res.push_back(casprintf_loc(translate("gettextFromC", "Density: %.1fg/L"), entry.density));
			}
		case plot_info::FREEDIVING:
			/* nothing */
			break;
		}
	}
	if (entry.stopdepth.mm > 0) {
		depthvalue = get_depth_units(entry.stopdepth, NULL, &depth_unit);
		if (entry.ndl > 0) {
			/* this is a safety stop as we still have ndl */
			if (entry.stoptime)
				res.push_back(casprintf_loc(translate("gettextFromC", "Safety stop: %umin @ %.0f%s"), div_up(entry.stoptime, 60),
					       depthvalue, depth_unit));
			else
				res.push_back(casprintf_loc(translate("gettextFromC", "Safety stop: unknown time @ %.0f%s"),
					       depthvalue, depth_unit));
		} else {
			/* actual deco stop */
			if (entry.stoptime)
				res.push_back(casprintf_loc(translate("gettextFromC", "Deco: %umin @ %.0f%s"), div_up(entry.stoptime, 60),
					       depthvalue, depth_unit));
			else
				res.push_back(casprintf_loc(translate("gettextFromC", "Deco: unknown time @ %.0f%s"),
					       depthvalue, depth_unit));
		}
	} else if (entry.in_deco) {
		res.push_back(translate("gettextFromC", "In deco"));
	} else if (entry.ndl >= 0) {
		res.push_back(casprintf_loc(translate("gettextFromC", "NDL: %umin"), div_up(entry.ndl, 60)));
	}
	if (entry.tts)
		res.push_back(casprintf_loc(translate("gettextFromC", "TTS: %umin"), div_up(entry.tts, 60)));
	if (entry.stopdepth_calc.mm > 0 && entry.stoptime_calc) {
		depthvalue = get_depth_units(entry.stopdepth_calc, NULL, &depth_unit);
		res.push_back(casprintf_loc(translate("gettextFromC", "Deco: %umin @ %.0f%s (calc)"), div_up(entry.stoptime_calc, 60),
				  depthvalue, depth_unit));
	} else if (entry.in_deco_calc) {
		/* This means that we have no NDL left,
		 * and we have no deco stop,
		 * so if we just accend to the surface slowly
		 * (ascent_mm_per_step / ascent_s_per_step)
		 * everything will be ok. */
		res.push_back(translate("gettextFromC", "In deco (calc)"));
	} else if (prefs.calcndltts && entry.ndl_calc != 0) {
		if(entry.ndl_calc < MAX_PROFILE_DECO)
			res.push_back(casprintf_loc(translate("gettextFromC", "NDL: %umin (calc)"), div_up(entry.ndl_calc, 60)));
		else
			res.push_back(translate("gettextFromC", "NDL: >2h (calc)"));
	}
	if (entry.tts_calc) {
		if (entry.tts_calc < MAX_PROFILE_DECO)
			res.push_back(casprintf_loc(translate("gettextFromC", "TTS: %umin (calc)"), div_up(entry.tts_calc, 60)));
		else
			res.push_back(translate("gettextFromC", "TTS: >2h (calc)"));
	}
	if (entry.rbt)
		res.push_back(casprintf_loc(translate("gettextFromC", "RBT: %umin"), div_up(entry.rbt, 60)));
	if (prefs.decoinfo) {
		if (entry.current_gf > 0.0)
			res.push_back(casprintf_loc(translate("gettextFromC", "GF %d%%"), (int)(100.0 * entry.current_gf)));
		if (entry.surface_gf > 0.0)
			res.push_back(casprintf_loc(translate("gettextFromC", "Surface GF %.0f%%"), entry.surface_gf));
		if (entry.ceiling.mm > 0) {
			depthvalue = get_depth_units(entry.ceiling, NULL, &depth_unit);
			res.push_back(casprintf_loc(translate("gettextFromC", "Calculated ceiling %.1f%s"), depthvalue, depth_unit));
			if (prefs.calcalltissues) {
				int k;
				for (k = 0; k < 16; k++) {
					if (entry.ceilings[k].mm > 0) {
						depthvalue = get_depth_units(entry.ceilings[k], NULL, &depth_unit);
						res.push_back(casprintf_loc(translate("gettextFromC", "Tissue %.0fmin: %.1f%s"), buehlmann_N2_t_halflife[k], depthvalue, depth_unit));
					}
				}
			}
		}
	}
	if (entry.icd_warning)
		res.push_back(translate("gettextFromC", "ICD in leading tissue"));
	if (entry.heartbeat && prefs.hrgraph)
		res.push_back(casprintf_loc(translate("gettextFromC", "heart rate: %d"), entry.heartbeat));
	if (entry.bearing >= 0)
		res.push_back(casprintf_loc(translate("gettextFromC", "bearing: %d"), entry.bearing));
	if (entry.running_sum.mm > 0) {
		depthvalue = get_depth_units(entry.running_sum / entry.sec, NULL, &depth_unit);
		res.push_back(casprintf_loc(translate("gettextFromC", "mean depth to here %.1f%s"), depthvalue, depth_unit));
	}

	return res;
}

std::pair<int, std::vector<std::string>> get_plot_details_new(const struct dive *d, const struct plot_info &pi, int time)
{
	/* The two first and the two last plot entries do not have useful data */
	if (pi.entry.size() <= 4)
		return { 0, {} };

	// binary search for sample index
	auto it = std::lower_bound(pi.entry.begin() + 2, pi.entry.end() - 3, time,
				   [] (const plot_data &d, int time)
				   { return d.sec < time; });
	int idx = it - pi.entry.begin();

	auto strings = plot_string(d, pi, idx);
	return std::make_pair(idx, strings);
}

/* Compare two plot_data entries and writes the results into a set of strings */
std::vector<std::string> compare_samples(const struct dive *d, const struct plot_info &pi, int idx1, int idx2, bool sum)
{
	std::string space(" ");
	const char *depth_unit, *pressure_unit, *vertical_speed_unit;
	double depthvalue, speedvalue;

	std::vector<std::string> res;
	if (idx1 < 0 || idx2 < 0)
		return res;

	if (pi.entry[idx1].sec > pi.entry[idx2].sec) {
		int tmp = idx2;
		idx2 = idx1;
		idx1 = tmp;
	} else if (pi.entry[idx1].sec == pi.entry[idx2].sec) {
		return res;
	}
	const struct plot_data &start = pi.entry[idx1];
	const struct plot_data &stop = pi.entry[idx2];

	int avg_speed = 0;
	int max_asc_speed = 0;
	int max_desc_speed = 0;

	depth_t delta_depth = start.depth - stop.depth;
	delta_depth.mm = std::abs(delta_depth.mm);
	int delta_time = abs(start.sec - stop.sec);
	depth_t avg_depth;
	depth_t max_depth;
	depth_t min_depth { .mm = INT_MAX };

	int last_sec = start.sec;

	volume_t cylinder_volume;
	std::vector<int> start_pressures(pi.nr_cylinders, 0);
	std::vector<int> last_pressures(pi.nr_cylinders, 0);
	std::vector<int> bar_used(pi.nr_cylinders, 0);
	std::vector<int> volumes_used(pi.nr_cylinders, 0);
	std::vector<char> cylinder_is_used(pi.nr_cylinders, false);

	for (int i = idx1; i < idx2; ++i) {
		const struct plot_data &data = pi.entry[i];
		if (sum)
			avg_speed += abs(data.speed) * (data.sec - last_sec);
		else
			avg_speed += data.speed * (data.sec - last_sec);
		avg_depth += data.depth * (data.sec - last_sec);

		if (data.speed > max_desc_speed)
			max_desc_speed = data.speed;
		if (data.speed < max_asc_speed)
			max_asc_speed = data.speed;

		if (data.depth.mm < min_depth.mm)
			min_depth = data.depth;
		if (data.depth.mm > max_depth.mm)
			max_depth = data.depth;

		for (int cylinder_index = 0; cylinder_index < pi.nr_cylinders; cylinder_index++) {
			int next_pressure = get_plot_pressure(pi, i, cylinder_index);
			if (next_pressure && !start_pressures[cylinder_index])
				start_pressures[cylinder_index] = next_pressure;

			if (start_pressures[cylinder_index]) {
				if (last_pressures[cylinder_index]) {
					bar_used[cylinder_index] += last_pressures[cylinder_index] - next_pressure;

					const cylinder_t *cyl = d->get_cylinder(cylinder_index);

					// TODO: Implement addition/subtraction on units.h types
					volumes_used[cylinder_index] += (cyl->gas_volume((pressure_t){ .mbar = last_pressures[cylinder_index] }) -
									 cyl->gas_volume((pressure_t){ .mbar = next_pressure })).mliter;
				}

				// check if the gas in this cylinder is being used
				if (next_pressure < start_pressures[cylinder_index] - 1000 && !cylinder_is_used[cylinder_index]) {
					cylinder_is_used[cylinder_index] = true;
				}
			}

			last_pressures[cylinder_index] = next_pressure;
		}

		last_sec = data.sec;
	}

	avg_depth /= stop.sec - start.sec;
	avg_speed /= stop.sec - start.sec;

	std::string l = casprintf_loc(translate("gettextFromC", "ΔT:%d:%02dmin"), delta_time / 60, delta_time % 60);

	depthvalue = get_depth_units(delta_depth, NULL, &depth_unit);
	l += space + casprintf_loc(translate("gettextFromC", "ΔD:%.1f%s"), depthvalue, depth_unit);

	depthvalue = get_depth_units(min_depth, NULL, &depth_unit);
	l += space + casprintf_loc(translate("gettextFromC", "↓D:%.1f%s"), depthvalue, depth_unit);

	depthvalue = get_depth_units(max_depth, NULL, &depth_unit);
	l += space + casprintf_loc(translate("gettextFromC", "↑D:%.1f%s"), depthvalue, depth_unit);

	depthvalue = get_depth_units(avg_depth, NULL, &depth_unit);
	l += space + casprintf_loc(translate("gettextFromC", "øD:%.1f%s"), depthvalue, depth_unit);
	res.push_back(l);

	speedvalue = get_vertical_speed_units(abs(max_desc_speed), NULL, &vertical_speed_unit);
	l = casprintf_loc(translate("gettextFromC", "↓V:%.2f%s"), speedvalue, vertical_speed_unit);

	speedvalue = get_vertical_speed_units(abs(max_asc_speed), NULL, &vertical_speed_unit);
	l += space + casprintf_loc(translate("gettextFromC", "↑V:%.2f%s"), speedvalue, vertical_speed_unit);

	speedvalue = get_vertical_speed_units(abs(avg_speed), NULL, &vertical_speed_unit);
	l += space + casprintf_loc(translate("gettextFromC", "øV:%.2f%s"), speedvalue, vertical_speed_unit);

	int total_bar_used = 0;
	int total_volume_used = 0;
	bool cylindersizes_are_identical = true;
	bool sac_is_determinable = true;
	for (int cylinder_index = 0; cylinder_index < pi.nr_cylinders; cylinder_index++) {
		if (cylinder_is_used[cylinder_index]) {
			total_bar_used += bar_used[cylinder_index];
			total_volume_used += volumes_used[cylinder_index];

			const cylinder_t *cyl = d->get_cylinder(cylinder_index);
			if (cyl->type.size.mliter) {
				if (cylinder_volume.mliter && cylinder_volume.mliter != cyl->type.size.mliter) {
					cylindersizes_are_identical = false;
				} else {
					cylinder_volume = cyl->type.size;
				}
			} else {
				sac_is_determinable = false;
			}
		}
	}

	// No point printing 'bar used' if we know it's meaningless because cylinders of different size were used
	if (cylindersizes_are_identical && total_bar_used) {
		int pressurevalue = get_pressure_units(total_bar_used, &pressure_unit);
		l += space + casprintf_loc(translate("gettextFromC", "ΔP:%d%s"), pressurevalue, pressure_unit);
	}

	// We can't calculate the SAC if the volume for some of the cylinders used is unknown
	if (sac_is_determinable && total_volume_used) {
		int volume_precision;
		const char *volume_unit;

		/* Mean pressure in ATM */
		double atm = d->depth_to_atm(avg_depth);

		/* milliliters per minute */
		int sac = lrint(total_volume_used / atm * 60 / delta_time);
		double volume_value = get_volume_units(sac, &volume_precision, &volume_unit);
		l += space + casprintf_loc(translate("gettextFromC", "SAC:%.*f%s/min"), volume_precision, volume_value, volume_unit);
	}
	res.push_back(l);

	return res;
}
