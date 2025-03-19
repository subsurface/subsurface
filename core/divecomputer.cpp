// SPDX-License-Identifier: GPL-2.0

#include "divecomputer.h"
#include "errorhelper.h"
#include "event.h"
#include "extradata.h"
#include "pref.h"
#include "tanksensormapping.h"
#include "sample.h"
#include "subsurface-string.h"

#include <string.h>
#include <stdlib.h>
#include <tuple>

divecomputer::divecomputer() = default;
divecomputer::~divecomputer() = default;
divecomputer::divecomputer(const divecomputer &) = default;
divecomputer::divecomputer(divecomputer &&) = default;
divecomputer &divecomputer::operator=(const divecomputer &) = default;

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
 *	     max_t-t1-(1-d_frac)*(t4-t3) = avg_d*max_t/max_d
 *		   t1+(1-d_frac)*(t4-t3) = max_t*(1-avg_d/max_d)
 *
 * and descent slope must match ascent slopes:
 *
 *   t1 / max_d = (t3-t2) / (max_d*(1-d_frac))
 *	   t1 = (t3-t2)/(1-d_frac)
 *
 * and
 *
 *   t1 / max_d = (max_t-t4) / (max_d*d_frac)
 *	   t1 = (max_t-t4)/d_frac
 *
 * In general, we have more free variables than we have constraints,
 * but we can aim for certain basics, like a good ascent slope.
 */
static int fill_samples(std::vector<sample> &s, int max_d, int avg_d, int max_t, double slope, double d_frac)
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
static void fill_samples_no_avg(std::vector<sample> &s, int max_d, int max_t, double slope)
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
		s[3].depth = 5_m;
		s[4].time.seconds = max_t - lrint(5000 / slope);
		s[4].depth = 5_m;
	}
}

void fake_dc(struct divecomputer *dc)
{
	/* The dive has no samples, so create a few fake ones */
	int max_t = dc->duration.seconds;
	int max_d = dc->maxdepth.mm;
	int avg_d = dc->meandepth.mm;

	if (!max_t || !max_d) {
		dc->samples.clear();
		return;
	}

	std::vector<struct sample> &fake = dc->samples;
	fake.resize(6);

	fake[5].time.seconds = max_t;
	for (int i = 0; i < 6; i++) {
		fake[i].bearing.degrees = -1;
		fake[i].ndl.seconds = -1;
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
		fill_samples_no_avg(fake, max_d, max_t, std::max(2.0 * max_d / max_t, (double)prefs.ascratelast6m));
		if (fake[3].time.seconds == 0) { // just a 4 point profile
			dc->samples.resize(4);
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

divemode_loop::divemode_loop(const struct divecomputer &dc) :
	last(dc.divemode), last_time(0), loop("modechange", dc)
{
	/* on first invocation, get first event (if any) */
	ev = loop.next();
}

std::pair<divemode_t, int> divemode_loop::at(int time)
{
	while (ev && ev->time.seconds <= time) {
		last = static_cast<divemode_t>(ev->value);
		last_time = ev->time.seconds;
		ev = loop.next();
	}
	return std::make_pair(last, last_time);
}

/* helper function to make it easier to work with our structures
 * we don't interpolate here, just use the value from the last sample up to that time */
int get_depth_at_time(const struct divecomputer *dc, unsigned int time)
{
	int depth = 0;
	if (dc) {
		for (const auto &sample: dc->samples) {
			if (sample.time.seconds > (int)time)
				break;
			depth = sample.depth.mm;
		}
	}
	return depth;
}

struct sample *prepare_sample(struct divecomputer *dc)
{
	if (dc) {
		dc->samples.emplace_back();
		auto &sample = dc->samples.back();

		// Copy the sensor numbers - but not the pressure values
		// from the previous sample if any.
		if (dc->samples.size() >= 2) {
			auto &prev = dc->samples[dc->samples.size() - 2];
			for (int idx = 0; idx < MAX_SENSORS; idx++)
				sample.sensor[idx] = prev.sensor[idx];
		}
		// Init some values with -1
		sample.bearing.degrees = -1;
		sample.ndl.seconds = -1;

		return &sample;
	}
	return NULL;
}

void append_sample(const struct sample &sample, struct divecomputer *dc)
{
	dc->samples.push_back(sample);
}

std::set<int16_t> get_tank_sensor_ids(const struct divecomputer &dc)
{
	auto result = std::set<int16_t>();
	for (const auto &sample: dc.samples)
		for (int i = 0; i < MAX_SENSORS; i++)
			if (sample.pressure[i].mbar)
				result.insert(sample.sensor[i]);

	return result;
}

void fixup_dc_sample_sensors(struct divecomputer &dc)
{
	for (auto &sample: dc.samples)
		for (int j = 0; j < MAX_SENSORS; j++) {
			int sensor = sample.sensor[j];

			// No invalid sensor ID's, please
			if (sensor < 0) {
				sample.sensor[j] = NO_SENSOR;
				sample.pressure[j] = 0_bar;
				continue;
			}

			// Don't bother tracking sensors with no data
			if (!sample.pressure[j].mbar)
				sample.sensor[j] = NO_SENSOR;
		}
}

/*
 * Calculate how long we were actually under water, and the average
 * depth while under water.
 *
 * This ignores any surface time in the middle of the dive.
 */
void fixup_dc_duration(struct divecomputer &dc)
{
	int duration = 0;
	int lasttime = 0, lastdepth = 0, depthtime = 0;

	for (const auto &sample: dc.samples) {
		int time = sample.time.seconds;
		int depth = sample.depth.mm;

		/* Do we *have* a depth? */
		if (depth < 0)
			continue;

		/* We ignore segments at the surface */
		if (depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD) {
			duration += time - lasttime;
			depthtime += (time - lasttime) * (depth + lastdepth) / 2;
		}
		lastdepth = depth;
		lasttime = time;
	}
	if (duration) {
		dc.duration.seconds = duration;
		dc.meandepth.mm = (depthtime + duration / 2) / duration;
	}
}

static bool operator<(const event &ev1, const event &ev2)
{
	return std::tie(ev1.time.seconds, ev1.name) <
	       std::tie(ev2.time.seconds, ev2.name);
}

int add_event_to_dc(struct divecomputer *dc, struct event ev)
{
	// Do a binary search for insertion point
	auto it = std::lower_bound(dc->events.begin(), dc->events.end(), ev);
	int idx = it - dc->events.begin();
	dc->events.insert(it, ev);
	return idx;
}

struct event *add_event(struct divecomputer *dc, unsigned int time, int type, int flags, int value, const std::string &name)
{
	struct event ev(time, type, flags, value, name);
	int idx = add_event_to_dc(dc, std::move(ev));

	return &dc->events[idx];
}

/* Remove given event from dive computer. Returns the removed event. */
struct event remove_event_from_dc(struct divecomputer *dc, int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) > dc->events.size()) {
		report_info("removing invalid event %d", idx);
		return event();
	}
	event res = std::move(dc->events[idx]);
	dc->events.erase(dc->events.begin() + idx);
	return res;
}

struct event *get_event(struct divecomputer *dc, int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) > dc->events.size()) {
		report_info("accessing invalid event %d", idx);
		return nullptr;
	}
	return &dc->events[idx];
}

void add_extra_data(struct divecomputer *dc, const std::string &key, const std::string &value)
{
	if (key == "Serial") {
		dc->deviceid = calculate_string_hash(value.c_str());
		dc->serial = value;
	}
	if (key == "FW Version")
		dc->fw_version = value;

	dc->extra_data.push_back(extra_data { key, value });
}

/*
 * Match two dive computer entries against each other, and
 * tell if it's the same dive. Return 0 if "don't know",
 * positive for "same dive" and negative for "definitely
 * not the same dive"
 */
int match_one_dc(const struct divecomputer &a, const struct divecomputer &b)
{
	/* Not same model? Don't know if matching.. */
	if (a.model.empty() || b.model.empty())
		return 0;
	if (strcasecmp(a.model.c_str(), b.model.c_str()))
		return 0;

	/* Different device ID's? Don't know */
	if (a.deviceid != b.deviceid)
		return 0;

	/* Do we have dive IDs? */
	if (!a.diveid || !b.diveid)
		return 0;

	/*
	 * If they have different dive ID's on the same
	 * dive computer, that's a definite "same or not"
	 */
	return a.diveid == b.diveid && a.when == b.when ? 1 : -1;
}

static const char *planner_dc_name = "planned dive";

bool is_dc_planner(const struct divecomputer *dc)
{
	return dc->model == planner_dc_name;
}

void make_planner_dc(struct divecomputer *dc)
{
	dc->model = planner_dc_name;
}

const char *manual_dc_name = "manually added dive";

bool is_dc_manually_added_dive(const struct divecomputer *dc)
{
	return dc->model == manual_dc_name;
}

void make_manually_added_dive_dc(struct divecomputer *dc)
{
	dc->model = manual_dc_name;
}
