#ifndef PROFILE_H
#define PROFILE_H

#ifdef __cplusplus
extern "C" {
#else
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef int bool;
#endif
#endif

struct dive;
struct divecomputer;
struct graphics_context;
struct plot_info;

void calculate_max_limits(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc);
struct plot_info *create_plot_info(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc);

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 * We also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
int get_maxtime(struct plot_info *pi);

/* get the maximum depth to which we want to plot
 * take into account the additional verical space needed to plot
 * partial pressure graphs */
int get_maxdepth(struct plot_info *pi);

#ifdef __cplusplus
}
#endif
#endif
