// SPDX-License-Identifier: GPL-2.0
#ifndef PLANNER_H
#define PLANNER_H

#include "units.h"
#include "divemode.h"
#include <string>
#include <vector>

/* this should be converted to use our types */
struct divedatapoint {
	int time;
	depth_t depth;
	int cylinderid;
	pressure_t minimum_gas;
	int setpoint;
	bool entered;
	enum divemode_t divemode;
};

struct diveplan {
	diveplan();
	~diveplan();

	timestamp_t when = 0;
	int surface_pressure = 0; /* mbar */
	int bottomsac = 0;	/* ml/min */
	int decosac = 0;	  /* ml/min */
	int salinity = 0;
	short gflow = 0;
	short gfhigh = 0;
	short vpmb_conservatism = 0;
	std::vector<divedatapoint> dp;
	int eff_gflow = 0, eff_gfhigh = 0;
	int surface_interval = 0;
};

struct deco_state_cache;

typedef enum {
	PLAN_OK,
	PLAN_ERROR_TIMEOUT,
	PLAN_ERROR_INAPPROPRIATE_GAS,
} planner_error_t;

extern int get_cylinderid_at_time(struct dive *dive, struct divecomputer *dc, duration_t time);
extern bool diveplan_empty(const struct diveplan &diveplan);
extern void add_plan_to_notes(struct diveplan &diveplan, struct dive *dive, bool show_disclaimer, planner_error_t error);
extern const char *get_planner_disclaimer();

void plan_add_segment(struct diveplan &diveplan, int duration, int depth, int cylinderid, int po2, bool entered, enum divemode_t divemode);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
struct decostop {
	int depth;
	int time;
};

extern std::string get_planner_disclaimer_formatted();
extern bool plan(struct deco_state *ds, struct diveplan &diveplan, struct dive *dive, int dcNr, int timestep, struct decostop *decostoptable, deco_state_cache &cache, bool is_planner, bool show_disclaimer);
#endif // PLANNER_H
