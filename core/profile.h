// SPDX-License-Identifier: GPL-2.0
#ifndef PROFILE_H
#define PROFILE_H

#include "dive.h"
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	STABLE,
	SLOW,
	MODERATE,
	FAST,
	CRAZY
} velocity_t;

enum plot_pressure {
	SENSOR_PR = 0,
	INTERPOLATED_PR = 1,
	NUM_PLOT_PRESSURES = 2
};

struct membuffer;
struct deco_state;
struct divecomputer;
struct plot_info;

/*
 * sensor data for a given cylinder
 */
struct plot_pressure_data {
	int data[NUM_PLOT_PRESSURES];
};

struct plot_data {
	unsigned int in_deco : 1;
	int sec;
	int temperature;
	/* Depth info */
	int depth;
	int ceiling;
	int ceilings[16];
	int percentages[16];
	int ndl;
	int tts;
	int rbt;
	int stoptime;
	int stopdepth;
	int cns;
	int smoothed;
	int sac;
	int running_sum;
	struct gas_pressures pressures;
	pressure_t o2pressure;  // for rebreathers, this is consensus measured po2, or setpoint otherwise. 0 for OC.
	pressure_t o2sensor[3]; //for rebreathers with up to 3 PO2 sensors
	pressure_t o2setpoint;
	pressure_t scr_OC_pO2;
	double mod, ead, end, eadd;
	velocity_t velocity;
	int speed;
	// stats over 9 minute window:
	int min, max;	// indices into pi->entry[]
	/* values calculated by us */
	unsigned int in_deco_calc : 1;
	int ndl_calc;
	int tts_calc;
	int stoptime_calc;
	int stopdepth_calc;
	int pressure_time;
	int heartbeat;
	int bearing;
	double ambpressure;
	double gfline;
	double surface_gf;
	double current_gf;
	double density;
	bool icd_warning;
};

extern void compare_samples(const struct dive *d, const struct plot_info *pi, int idx1, int idx2, char *buf, int bufsize, bool sum);
extern void init_plot_info(struct plot_info *pi);
/* when planner_dc is non-null, this is called in planner mode. */
extern void create_plot_info_new(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi, bool fast, const struct deco_state *planner_ds);
extern int get_plot_details_new(const struct dive *d, const struct plot_info *pi, int time, struct membuffer *);
extern void free_plot_info_data(struct plot_info *pi);

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 * We also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
extern int get_maxtime(const struct plot_info *pi);

/* get the maximum depth to which we want to plot
 * take into account the additional verical space needed to plot
 * partial pressure graphs */
extern int get_maxdepth(const struct plot_info *pi);

static inline int get_plot_pressure_data(const struct plot_info *pi, int idx, enum plot_pressure sensor, int cylinder)
{
	return pi->pressures[cylinder + idx * pi->nr_cylinders].data[sensor];
}

static inline void set_plot_pressure_data(struct plot_info *pi, int idx, enum plot_pressure sensor, int cylinder, int value)
{
	pi->pressures[cylinder + idx * pi->nr_cylinders].data[sensor] = value;
}

static inline int get_plot_sensor_pressure(const struct plot_info *pi, int idx, int cylinder)
{
	return get_plot_pressure_data(pi, idx, SENSOR_PR, cylinder);
}

static inline int get_plot_interpolated_pressure(const struct plot_info *pi, int idx, int cylinder)
{
	return get_plot_pressure_data(pi, idx, INTERPOLATED_PR, cylinder);
}

static inline int get_plot_pressure(const struct plot_info *pi, int idx, int cylinder)
{
	int res = get_plot_sensor_pressure(pi, idx, cylinder);
	return res ? res : get_plot_interpolated_pressure(pi, idx, cylinder);
}

#ifdef __cplusplus
}
#endif
#endif // PROFILE_H
