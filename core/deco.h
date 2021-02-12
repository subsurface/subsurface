// SPDX-License-Identifier: GPL-2.0
#ifndef DECO_H
#define DECO_H

#include "units.h"
#include "gas.h"
#include "divemode.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dive;
struct divecomputer;
struct decostop;

struct deco_state {
	double tissue_n2_sat[16];
	double tissue_he_sat[16];
	double tolerated_by_tissue[16];
	double tissue_inertgas_saturation[16];
	double buehlmann_inertgas_a[16];
	double buehlmann_inertgas_b[16];

	double max_n2_crushing_pressure[16];
	double max_he_crushing_pressure[16];

	double crushing_onset_tension[16];            // total inert gas tension in the t* moment
	double n2_regen_radius[16];                   // rs
	double he_regen_radius[16];
	double max_ambient_pressure;                  // last moment we were descending

	double bottom_n2_gradient[16];
	double bottom_he_gradient[16];

	double initial_n2_gradient[16];
	double initial_he_gradient[16];
	pressure_t first_ceiling_pressure;
	pressure_t max_bottom_ceiling_pressure;
	int ci_pointing_to_guiding_tissue;
	double gf_low_pressure_this_dive;
	int deco_time;
	bool icd_warning;
	int  sum1;
	long sumx, sumxx;
	double sumy, sumxy;
	int plot_depth;
};

extern const double buehlmann_N2_t_halflife[];

extern int deco_allowed_depth(double tissues_tolerance, double surface_pressure, const struct dive *dive, bool smooth);

double get_gf(struct deco_state *ds, double ambpressure_bar, const struct dive *dive);
extern void clear_deco(struct deco_state *ds, double surface_pressure, bool in_planner);
extern void dump_tissues(struct deco_state *ds);
extern void set_gf(short gflow, short gfhigh);
extern void set_vpmb_conservatism(short conservatism);
extern void cache_deco_state(struct deco_state *source, struct deco_state **datap);
extern void restore_deco_state(struct deco_state *data, struct deco_state *target, bool keep_vpmb_state);
extern void nuclear_regeneration(struct deco_state *ds, double time);
extern void vpmb_start_gradient(struct deco_state *ds);
extern void vpmb_next_gradient(struct deco_state *ds, double deco_time, double surface_pressure, bool in_planner);
extern double tissue_tolerance_calc(struct deco_state *ds, const struct dive *dive, double pressure, bool in_planner);
extern void calc_crushing_pressure(struct deco_state *ds, double pressure);
extern void vpmb_start_gradient(struct deco_state *ds);
extern void clear_vpmb_state(struct deco_state *ds);
extern void add_segment(struct deco_state *ds, double pressure, struct gasmix gasmix, int period_in_seconds, int setpoint, enum divemode_t divemode, int sac, bool in_planner);

extern double regressiona(const struct deco_state *ds);
extern double regressionb(const struct deco_state *ds);
extern void reset_regression(struct deco_state *ds);
extern void update_regression(struct deco_state *ds, const struct dive *dive);

#ifdef __cplusplus
}
#endif

#endif // DECO_H
