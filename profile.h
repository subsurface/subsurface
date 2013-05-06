#ifndef PROFILE_H
#define PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dive.h"

typedef enum { STABLE, SLOW, MODERATE, FAST, CRAZY } velocity_t;

struct divecomputer;
struct graphics_context;
struct plot_info;
struct plot_data {
	unsigned int in_deco:1;
	unsigned int cylinderindex;
	int sec;
	/* pressure[0] is sensor pressure
	 * pressure[1] is interpolated pressure */
	int pressure[2];
	int temperature;
	/* Depth info */
	int depth;
	int ceiling;
	int ndl;
	int stoptime;
	int stopdepth;
	int cns;
	int smoothed;
	double po2, pn2, phe;
	double mod, ead, end, eadd;
	velocity_t velocity;
	struct plot_data *min[3];
	struct plot_data *max[3];
	int avg[3];
};

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
