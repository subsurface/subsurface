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

#ifdef __cplusplus
}
#endif
#endif
