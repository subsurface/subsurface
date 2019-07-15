// SPDX-License-Identifier: GPL-2.0
#ifndef DECO_H
#define DECO_H

#ifdef __cplusplus
extern "C" {
#endif

struct dive;
struct deco_state;
struct decostop;

extern const double buehlmann_N2_t_halflife[];

extern int deco_allowed_depth(double tissues_tolerance, double surface_pressure, const struct dive *dive, bool smooth);

double get_gf(struct deco_state *ds, double ambpressure_bar, const struct dive *dive);
extern void clear_deco(struct deco_state *ds, double surface_pressure);
extern void dump_tissues(struct deco_state *ds);
extern void set_gf(short gflow, short gfhigh);
extern void set_vpmb_conservatism(short conservatism);
extern void cache_deco_state(struct deco_state *source, struct deco_state **datap);
extern void restore_deco_state(struct deco_state *data, struct deco_state *target, bool keep_vpmb_state);
extern void nuclear_regeneration(struct deco_state *ds, double time);
extern void vpmb_start_gradient(struct deco_state *ds);
extern void vpmb_next_gradient(struct deco_state *ds, double deco_time, double surface_pressure);
extern double tissue_tolerance_calc(struct deco_state *ds, const struct dive *dive, double pressure);
extern void calc_crushing_pressure(struct deco_state *ds, double pressure);
extern void vpmb_start_gradient(struct deco_state *ds);
extern void clear_vpmb_state(struct deco_state *ds);
extern void printdecotable(struct decostop *table);

extern double regressiona();
extern double regressionb();
extern void reset_regression();

#ifdef __cplusplus
}
#endif

#endif // DECO_H
