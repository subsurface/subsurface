// SPDX-License-Identifier: GPL-2.0
#ifndef PLANNER_H
#define PLANNER_H

#define LONGDECO 1
#define NOT_RECREATIONAL 2

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

#ifdef __cplusplus
extern "C" {
#endif

extern int validate_gas(const char *text, struct gasmix *gas);
extern int validate_po2(const char *text, int *mbar_po2);
extern timestamp_t current_time_notz(void);
extern void set_last_stop(bool last_stop_6m);
extern void set_verbatim(bool verbatim);
extern void set_display_runtime(bool display);
extern void set_display_duration(bool display);
extern void set_display_transitions(bool display);
extern int get_cylinderid_at_time(struct dive *dive, struct divecomputer *dc, duration_t time);
extern int get_gasidx(struct dive *dive, struct gasmix mix);
extern bool diveplan_empty(struct diveplan *diveplan);
extern void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive, bool show_disclaimer, int error);
extern const char *get_planner_disclaimer();
extern char *get_planner_disclaimer_formatted();

extern void free_dps(struct diveplan *diveplan);
extern struct dive *planned_dive;
extern char *cache_data;

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int cylinderid, int po2, bool entered, enum divemode_t divemode);
struct divedatapoint *create_dp(int time_incr, int depth, int cylinderid, int po2);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
struct decostop {
	int depth;
	int time;
};
extern bool plan(struct deco_state *ds, struct diveplan *diveplan, struct dive *dive, int timestep, struct decostop *decostoptable, struct deco_state **cached_datap, bool is_planner, bool show_disclaimer);

#ifdef __cplusplus
}
#endif
#endif // PLANNER_H
