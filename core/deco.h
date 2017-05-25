// SPDX-License-Identifier: GPL-2.0
#ifndef DECO_H
#define DECO_H

#ifdef __cplusplus
extern "C" {
#endif

extern double buehlmann_N2_t_halflife[];
extern struct deco_state *deco_state;

extern int deco_allowed_depth(double tissues_tolerance, double surface_pressure, struct dive *dive, bool smooth);

double get_gf(double ambpressure_bar, const struct dive *dive);

#ifdef __cplusplus
}
#endif

#endif // DECO_H
