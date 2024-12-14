// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILE_H
#define PROFILE_H

#include "gas.h" // gas_pressures
#include "sample.h" // MAX_O2_SENSORS

#include <memory>
#include <string>
#include <array>
#include <vector>

enum velocity_t {
	STABLE,
	SLOW,
	MODERATE,
	FAST,
	CRAZY
};

enum plot_pressure {
	SENSOR_PR = 0,
	INTERPOLATED_PR = 1,
	NUM_PLOT_PRESSURES = 2
};

struct membuffer;
struct deco_state;
struct dive;
struct divecomputer;

/*
 * sensor data for a given cylinder
 */
struct plot_pressure_data {
	std::array<int, NUM_PLOT_PRESSURES> data;
};

struct plot_data {
	bool in_deco = false;
	int sec = 0;
	int temperature = 0;
	/* Depth info */
	depth_t depth;
	depth_t ceiling;
	std::array<depth_t, 16> ceilings;
	std::array<int, 16> percentages;
	int ndl = 0;
	int tts = 0;
	int rbt = 0;
	int stoptime = 0;
	depth_t stopdepth;
	int cns = 0;
	depth_t smoothed;
	int sac = 0;
	depth_t running_sum; // strictly speaking not a depth, but depth × time. Might define a custom type for that based on a longer integer.
	struct gas_pressures pressures;
	// TODO: make pressure_t default to 0
	pressure_t o2pressure;  // for rebreathers, this is consensus measured po2, or setpoint otherwise. 0 for OC.
	std::array<pressure_t, MAX_O2_SENSORS> o2sensor; //for rebreathers with several sensors
	pressure_t o2setpoint;
	pressure_t scr_OC_pO2;
	depth_t mod, ead, end, eadd;
	velocity_t velocity = STABLE;
	int speed = 0;
	/* values calculated by us */
	bool in_deco_calc = false;
	int ndl_calc = 0;
	int tts_calc = 0;
	int stoptime_calc = 0;
	depth_t stopdepth_calc;
	int pressure_time = 0;
	int heartbeat = 0;
	int bearing = 0;
	double ambpressure = 0.0;
	double gfline = 0.0;
	double surface_gf = 0.0;
	double current_gf = 0.0;
	double density = 0.0;
	bool icd_warning = false;
};

/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr = 0; // TODO: remove - redundant with entry.size()
	int nr_cylinders = 0;
	int maxtime = 0;
	int meandepth = 0, maxdepth = 0;
	int minpressure = 0, maxpressure = 0;
	int minhr = 0, maxhr = 0;
	int mintemp = 0, maxtemp = 0;
	enum {AIR, NITROX, TRIMIX, FREEDIVING} dive_type = AIR;
	double endtempcoord = 0.0;
	double maxpp = 0.0;
	bool waypoint_above_ceiling = false;
	std::vector<plot_data> entry;
	std::vector<plot_pressure_data> pressures; /* cylinders.size() blocks of nr entries. */

	plot_info();
	~plot_info();
	plot_info(const plot_info &) = default;
	plot_info(plot_info &&) = default;
	plot_info &operator=(const plot_info &) = default;
	plot_info &operator=(plot_info &&) = default;
};

#define AMB_PERCENTAGE 50.0

/* when planner_dc is non-null, this is called in planner mode. */
extern struct plot_info create_plot_info_new(const struct dive *dive, const struct divecomputer *dc, const struct deco_state *planner_ds);

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 * We also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
extern int get_maxtime(const struct plot_info &pi);

/* get the maximum depth to which we want to plot */
extern int get_maxdepth(const struct plot_info &pi);

static inline int get_plot_pressure_data(const struct plot_info &pi, int idx, enum plot_pressure sensor, int cylinder)
{
	return pi.pressures[cylinder + idx * pi.nr_cylinders].data[sensor];
}

static inline void set_plot_pressure_data(struct plot_info &pi, int idx, enum plot_pressure sensor, int cylinder, int value)
{
	pi.pressures[cylinder + idx * pi.nr_cylinders].data[sensor] = value;
}

static inline int get_plot_sensor_pressure(const struct plot_info &pi, int idx, int cylinder)
{
	return get_plot_pressure_data(pi, idx, SENSOR_PR, cylinder);
}

static inline int get_plot_interpolated_pressure(const struct plot_info &pi, int idx, int cylinder)
{
	return get_plot_pressure_data(pi, idx, INTERPOLATED_PR, cylinder);
}

static inline int get_plot_pressure(const struct plot_info &pi, int idx, int cylinder)
{
	int res = get_plot_sensor_pressure(pi, idx, cylinder);
	return res ? res : get_plot_interpolated_pressure(pi, idx, cylinder);
}

// Returns index of sample and array of strings describing the dive details at given time
std::pair<int, std::vector<std::string>> get_plot_details_new(const struct dive *d, const struct plot_info &pi, int time);
std::vector<std::string> compare_samples(const struct dive *d, const struct plot_info &pi, int idx1, int idx2, bool sum);

#endif // PROFILE_H
