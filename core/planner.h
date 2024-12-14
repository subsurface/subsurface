// SPDX-License-Identifier: GPL-2.0
#ifndef PLANNER_H
#define PLANNER_H

#include "units.h"
#include "divemode.h"
#include <string>
#include <vector>

/* this should be converted to use our types */
struct divedatapoint {
	int time = 0;
	depth_t depth;
	int cylinderid = 0;
	pressure_t minimum_gas;
	int setpoint = 0;
	bool entered = false;
	enum divemode_t divemode = OC;

	divedatapoint() = default;
	divedatapoint(const divedatapoint &) = default;
	divedatapoint(divedatapoint &&) = default;
	divedatapoint &operator=(const divedatapoint &) = default;
	divedatapoint(int time_incr, depth_t depth, int cylinderid, int po2, bool entered);
};

typedef enum {
	PLAN_OK,
	PLAN_ERROR_TIMEOUT,
	PLAN_ERROR_INAPPROPRIATE_GAS,
} planner_error_t;

struct diveplan {
	diveplan();
	~diveplan();
	diveplan(const diveplan &) = default;
	diveplan(diveplan &&) = default;
	diveplan &operator=(const diveplan &) = default;
	diveplan &operator=(diveplan &&) = default;

	timestamp_t when = 0;
	pressure_t surface_pressure;
	int bottomsac = 0;	/* ml/min */
	int decosac = 0;	  /* ml/min */
	int salinity = 0;
	short gflow = 0;
	short gfhigh = 0;
	short vpmb_conservatism = 0;
	std::vector<divedatapoint> dp;
	int eff_gflow = 0, eff_gfhigh = 0;
	int surface_interval = 0;

	bool is_empty() const;
	void add_plan_to_notes(struct dive &dive, bool show_disclaimer, planner_error_t error);
	int duration() const;
};

struct deco_state_cache;

extern int get_cylinderid_at_time(struct dive *dive, struct divecomputer *dc, duration_t time);
extern int ascent_velocity(depth_t depth, depth_t avg_depth, int);
extern const char *get_planner_disclaimer();

void plan_add_segment(struct diveplan &diveplan, int duration, depth_t depth, int cylinderid, int po2, bool entered, enum divemode_t divemode);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
struct decostop {
	int depth;
	int time;
};

extern std::string get_planner_disclaimer_formatted();
extern std::vector<decostop> plan(struct deco_state *ds, struct diveplan &diveplan, struct dive *dive, int dcNr, int timestep, deco_state_cache &cache, bool is_planner, bool show_disclaimer);
#endif // PLANNER_H
