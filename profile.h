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
	int cylinderindex;
	int sec;
	/* pressure[0] is sensor pressure
	 * pressure[1] is interpolated pressure */
	int pressure[2];
	int temperature;
	/* Depth info */
	int depth;
	int ceiling;
	int ceilings[16];
	int ndl;
	int tts;
	int stoptime;
	int stopdepth;
	int cns;
	int smoothed;
	double po2, pn2, phe;
	double mod, ead, end, eadd;
	velocity_t velocity;
	int speed;
	struct plot_data *min[3];
	struct plot_data *max[3];
	int avg[3];
};

void calculate_max_limits(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc);
struct plot_info *create_plot_info(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc);
int setup_temperature_limits(struct graphics_context *gc);
int get_cylinder_pressure_range(struct graphics_context *gc);
void compare_samples(struct plot_data *e1, struct plot_data *e2, char *buf, int bufsize, int sum);

struct ev_select {
	char *ev_name;
	bool plot_ev;
};

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

int get_local_sac(struct plot_data *entry1, struct plot_data *entry2, struct dive *dive);

void setup_pp_limits(struct graphics_context *gc);


#define ALIGN_LEFT 1
#define ALIGN_RIGHT 2
#define INVISIBLE 4
#define UNSORTABLE 8
#define EDITABLE 16

#ifndef TEXT_SCALE
#define TEXT_SCALE 1.0
#endif

#define DEPTH_TEXT_SIZE (10 * TEXT_SCALE)
#define PRESSURE_TEXT_SIZE (10 * TEXT_SCALE)
#define DC_TEXT_SIZE (10.5 * TEXT_SCALE)
#define PP_TEXT_SIZE (11 * TEXT_SCALE)
#define TEMP_TEXT_SIZE (12 * TEXT_SCALE)

#define RIGHT (-1.0)
#define CENTER (-0.5)
#define LEFT (0.0)

#define LINE_DOWN (1)
#define TOP (0)
#define MIDDLE (-0.5)
#define BOTTOM (-1)

#define SCALEXGC(x)  (((x) - gc.leftx) / (gc.rightx - gc.leftx) * gc.maxx)
#define SCALEYGC(y)  (((y) - gc.topy) / (gc.bottomy - gc.topy) * gc.maxy)
#define SCALEGC(x,y) SCALEXGC(x),SCALEYGC(y)

#define SCALEX(gc,x)  (((x)-gc->leftx)/(gc->rightx-gc->leftx)*gc->maxx)
#define SCALEY(gc,y)  (((y)-gc->topy)/(gc->bottomy-gc->topy)*gc->maxy)
#define SCALE(gc,x,y) SCALEX(gc,x),SCALEY(gc,y)

#define SENSOR_PR 0
#define INTERPOLATED_PR 1
#define SENSOR_PRESSURE(_entry) (_entry)->pressure[SENSOR_PR]
#define INTERPOLATED_PRESSURE(_entry) (_entry)->pressure[INTERPOLATED_PR]
#define GET_PRESSURE(_entry) (SENSOR_PRESSURE(_entry) ? SENSOR_PRESSURE(_entry) : INTERPOLATED_PRESSURE(_entry))

#define SAC_WINDOW 45	/* sliding window in seconds for current SAC calculation */

#ifdef __cplusplus
}
#endif
#endif
