// SPDX-License-Identifier: GPL-2.0
#ifndef PLANNER_H
#define PLANNER_H

#include "units.h"
#include "divemode.h"

/* this should be converted to use our types */
struct divedatapoint {
	int time;
	depth_t depth;
	int cylinderid;
	pressure_t minimum_gas;
	int setpoint;
	bool entered;
	struct divedatapoint *next;
	enum divemode_t divemode;
};

struct diveplan {
	timestamp_t when;
	int surface_pressure; /* mbar */
	int bottomsac;	/* ml/min */
	int decosac;	  /* ml/min */
	int salinity;
	short gflow;
	short gfhigh;
	short vpmb_conservatism;
	struct divedatapoint *dp;
	int eff_gflow, eff_gfhigh;
	int surface_interval;
};

struct deco_state_cache;

#ifdef __cplusplus
extern "C" {
#endif

extern int validate_gas(const char *text, struct gasmix *gas);
extern int validate_po2(const char *text, int *mbar_po2);
extern int get_cylinderid_at_time(struct dive *dive, struct divecomputer *dc, duration_t time);
extern bool diveplan_empty(struct diveplan *diveplan);
extern void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive, bool show_disclaimer, bool error);
extern const char *get_planner_disclaimer();

extern void free_dps(struct diveplan *diveplan);

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int cylinderid, int po2, bool entered, enum divemode_t divemode);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
struct decostop {
	int depth;
	int time;
};

#ifdef __cplusplus
}

#include <string>
extern std::string get_planner_disclaimer_formatted();
extern bool plan(struct deco_state *ds, struct diveplan *diveplan, struct dive *dive, int timestep, struct decostop *decostoptable, deco_state_cache &cache, bool is_planner, bool show_disclaimer);
#endif
#endif // PLANNER_H
