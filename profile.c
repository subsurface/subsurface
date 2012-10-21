/* profile.c */
/* creates all the necessary data for drawing the dive profile
 * uses cairo to draw it
 */
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"
#include "color.h"

int selected_dive = 0;
char zoomed_plot = 0;

static double plot_scale = SCALE_SCREEN;

#define cairo_set_line_width_scaled(cr, w) \
	cairo_set_line_width((cr), (w) * plot_scale);

typedef enum { STABLE, SLOW, MODERATE, FAST, CRAZY } velocity_t;

/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr;
	int maxtime;
	int meandepth, maxdepth;
	int maxpressure;
	int mintemp, maxtemp;
	struct plot_data {
		unsigned int same_cylinder:1;
		unsigned int cylinderindex;
		int sec;
		/* pressure[0] is sensor pressure
		 * pressure[1] is interpolated pressure */
		int pressure[2];
		int temperature;
		/* Depth info */
		int depth;
		int smoothed;
		velocity_t velocity;
		struct plot_data *min[3];
		struct plot_data *max[3];
		int avg[3];
	} entry[];
};

#define SENSOR_PR 0
#define INTERPOLATED_PR 1
#define SENSOR_PRESSURE(_entry) (_entry)->pressure[SENSOR_PR]
#define INTERPOLATED_PRESSURE(_entry) (_entry)->pressure[INTERPOLATED_PR]
#define GET_PRESSURE(_entry) (SENSOR_PRESSURE(_entry) ? : INTERPOLATED_PRESSURE(_entry))

#define SAC_COLORS_START_IDX SAC_1
#define SAC_COLORS 9
#define VELOCITY_COLORS_START_IDX VELO_STABLE
#define VELOCITY_COLORS 5

typedef enum {
	/* SAC colors. Order is important, the SAC_COLORS_START_IDX define above. */
	SAC_1, SAC_2, SAC_3, SAC_4, SAC_5, SAC_6, SAC_7, SAC_8, SAC_9,

	/* Velocity colors.  Order is still important, ref VELOCITY_COLORS_START_IDX. */
	VELO_STABLE, VELO_SLOW, VELO_MODERATE, VELO_FAST, VELO_CRAZY,

	/* Other colors */
	TEXT_BACKGROUND, ALERT_BG, ALERT_FG, EVENTS, SAMPLE_DEEP, SAMPLE_SHALLOW,
	SMOOTHED, MINUTE, TIME_GRID, TIME_TEXT, DEPTH_GRID, MEAN_DEPTH, DEPTH_TOP,
	DEPTH_BOTTOM, TEMP_TEXT, TEMP_PLOT, SAC_DEFAULT, BOUNDING_BOX, PRESSURE_TEXT, BACKGROUND
} color_indice_t;

typedef struct {
	/* media[0] is screen, and media[1] is printer */
	struct rgba {
		double r,g,b,a;
	} media[2];
} color_t;

/* [color indice] = {{screen color, printer color}} */
static const color_t profile_color[] = {
	[SAC_1]           = {{FUNGREEN1, BLACK1_LOW_TRANS}},
	[SAC_2]           = {{APPLE1, BLACK1_LOW_TRANS}},
	[SAC_3]           = {{ATLANTIS1, BLACK1_LOW_TRANS}},
	[SAC_4]           = {{ATLANTIS2, BLACK1_LOW_TRANS}},
	[SAC_5]           = {{EARLSGREEN1, BLACK1_LOW_TRANS}},
	[SAC_6]           = {{HOKEYPOKEY1, BLACK1_LOW_TRANS}},
	[SAC_7]           = {{TUSCANY1, BLACK1_LOW_TRANS}},
	[SAC_8]           = {{CINNABAR1, BLACK1_LOW_TRANS}},
	[SAC_9]           = {{REDORANGE1, BLACK1_LOW_TRANS}},

	[VELO_STABLE]     = {{CAMARONE1, BLACK1_LOW_TRANS}},
	[VELO_SLOW]       = {{LIMENADE1, BLACK1_LOW_TRANS}},
	[VELO_MODERATE]   = {{RIOGRANDE1, BLACK1_LOW_TRANS}},
	[VELO_FAST]       = {{PIRATEGOLD1, BLACK1_LOW_TRANS}},
	[VELO_CRAZY]      = {{RED1, BLACK1_LOW_TRANS}},

	[TEXT_BACKGROUND] = {{CONCRETE1_LOWER_TRANS, WHITE1}},
	[ALERT_BG]        = {{BROOM1_LOWER_TRANS, BLACK1_LOW_TRANS}},
	[ALERT_FG]        = {{BLACK1_LOW_TRANS, BLACK1_LOW_TRANS}},
	[EVENTS]          = {{REDORANGE1, BLACK1_LOW_TRANS}},
	[SAMPLE_DEEP]     = {{PERSIANRED1, BLACK1_LOW_TRANS}},
	[SAMPLE_SHALLOW]  = {{PERSIANRED1, BLACK1_LOW_TRANS}},
	[SMOOTHED]        = {{REDORANGE1_HIGH_TRANS, BLACK1_LOW_TRANS}},
	[MINUTE]          = {{MEDIUMREDVIOLET1_HIGHER_TRANS, BLACK1_LOW_TRANS}},
	[TIME_GRID]       = {{WHITE1, TUNDORA1_MED_TRANS}},
	[TIME_TEXT]       = {{FORESTGREEN1, BLACK1_LOW_TRANS}},
	[DEPTH_GRID]      = {{WHITE1, TUNDORA1_MED_TRANS}},
	[MEAN_DEPTH]      = {{REDORANGE1_MED_TRANS, BLACK1_LOW_TRANS}},
	[DEPTH_BOTTOM]    = {{GOVERNORBAY1_MED_TRANS, TUNDORA1_MED_TRANS}},
	[DEPTH_TOP]       = {{MERCURY1_MED_TRANS, WHITE1_MED_TRANS}},
	[TEMP_TEXT]       = {{GOVERNORBAY2, BLACK1_LOW_TRANS}},
	[TEMP_PLOT]       = {{ROYALBLUE2_LOW_TRANS, BLACK1_LOW_TRANS}},
	[SAC_DEFAULT]     = {{WHITE1, BLACK1_LOW_TRANS}},
	[BOUNDING_BOX]    = {{WHITE1, BLACK1_LOW_TRANS}},
	[PRESSURE_TEXT]   = {{KILLARNEY1, BLACK1_LOW_TRANS}},
	[BACKGROUND]      = {{SPRINGWOOD1, BLACK1_LOW_TRANS}},
};

#define plot_info_size(nr) (sizeof(struct plot_info) + (nr)*sizeof(struct plot_data))

/* Scale to 0,0 -> maxx,maxy */
#define SCALEX(gc,x)  (((x)-gc->leftx)/(gc->rightx-gc->leftx)*gc->maxx)
#define SCALEY(gc,y)  (((y)-gc->topy)/(gc->bottomy-gc->topy)*gc->maxy)
#define SCALE(gc,x,y) SCALEX(gc,x),SCALEY(gc,y)

static void move_to(struct graphics_context *gc, double x, double y)
{
	cairo_move_to(gc->cr, SCALE(gc, x, y));
}

static void line_to(struct graphics_context *gc, double x, double y)
{
	cairo_line_to(gc->cr, SCALE(gc, x, y));
}

static void set_source_rgba(struct graphics_context *gc, color_indice_t c)
{
	const color_t *col = &profile_color[c];
	struct rgba rgb = col->media[gc->printer];
	double r = rgb.r;
	double g = rgb.g;
	double b = rgb.b;
	double a = rgb.a;

	cairo_set_source_rgba(gc->cr, r, g, b, a);
}

void init_profile_background(struct graphics_context *gc)
{
	set_source_rgba(gc, BACKGROUND);
}

void pattern_add_color_stop_rgba(struct graphics_context *gc, cairo_pattern_t *pat, double o, color_indice_t c)
{
	const color_t *col = &profile_color[c];
	struct rgba rgb = col->media[gc->printer];
	cairo_pattern_add_color_stop_rgba(pat, o, rgb.r, rgb.g, rgb.b, rgb.a);
}

#define ROUND_UP(x,y) ((((x)+(y)-1)/(y))*(y))

/* debugging tool - not normally used */
static void dump_pi (struct plot_info *pi)
{
	int i;

	printf("pi:{nr:%d maxtime:%d meandepth:%d maxdepth:%d \n"
		"    maxpressure:%d mintemp:%d maxtemp:%d\n",
		pi->nr, pi->maxtime, pi->meandepth, pi->maxdepth,
		pi->maxpressure, pi->mintemp, pi->maxtemp);
	for (i = 0; i < pi->nr; i++)
		printf("    entry[%d]:{same_cylinder:%d cylinderindex:%d sec:%d pressure:{%d,%d}\n"
			"                time:%d:%02d temperature:%d depth:%d smoothed:%d}\n",
			i, pi->entry[i].same_cylinder, pi->entry[i].cylinderindex, pi->entry[i].sec,
			pi->entry[i].pressure[0], pi->entry[i].pressure[1],
			pi->entry[i].sec / 60, pi->entry[i].sec % 60,
			pi->entry[i].temperature, pi->entry[i].depth, pi->entry[i].smoothed);
	printf("   }\n");
}

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 * We also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
static int get_maxtime(struct plot_info *pi)
{
	int seconds = pi->maxtime;
	if (zoomed_plot) {
		/* Rounded up to one minute, with at least 2.5 minutes to
		 * spare.
		 * For dive times shorter than 10 minutes, we use seconds/4 to
		 * calculate the space dynamically.
		 * This is seamless since 600/4 = 150.
		 */
		if ( seconds < 600 )
			return ROUND_UP(seconds+seconds/4, 60);
		else
			return ROUND_UP(seconds+150, 60);
	} else {
		/* min 30 minutes, rounded up to 5 minutes, with at least 2.5 minutes to spare */
		return MAX(30*60, ROUND_UP(seconds+150, 60*5));
	}
}

static int get_maxdepth(struct plot_info *pi)
{
	unsigned mm = pi->maxdepth;
	if (zoomed_plot) {
		/* Rounded up to 10m, with at least 3m to spare */
		return ROUND_UP(mm+3000, 10000);
	} else {
		/* Minimum 30m, rounded up to 10m, with at least 3m to spare */
		return MAX(30000, ROUND_UP(mm+3000, 10000));
	}
}

typedef struct {
	int size;
	color_indice_t color;
	double hpos, vpos;
} text_render_options_t;

#define RIGHT (-1.0)
#define CENTER (-0.5)
#define LEFT (0.0)

#define TOP (1)
#define MIDDLE (0)
#define BOTTOM (-1)

static void plot_text(struct graphics_context *gc, const text_render_options_t *tro,
		      double x, double y, const char *fmt, ...)
{
	cairo_t *cr = gc->cr;
	cairo_font_extents_t fe;
	cairo_text_extents_t extents;
	double dx, dy;
	char buffer[80];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	cairo_set_font_size(cr, tro->size * plot_scale);
	cairo_font_extents(cr, &fe);
	cairo_text_extents(cr, buffer, &extents);
	dx = tro->hpos * extents.width + extents.x_bearing;
	dy = tro->vpos * extents.height + fe.descent;

	move_to(gc, x, y);
	cairo_rel_move_to(cr, dx, dy);

	cairo_text_path(cr, buffer);
	set_source_rgba(gc, TEXT_BACKGROUND);
	cairo_stroke(cr);

	move_to(gc, x, y);
	cairo_rel_move_to(cr, dx, dy);

	set_source_rgba(gc, tro->color);
	cairo_show_text(cr, buffer);
}

struct ev_select {
	char *ev_name;
	gboolean plot_ev;
};
static struct ev_select *ev_namelist;
static int evn_allocated;
static int evn_used;

void evn_foreach(void (*callback)(const char *, int *, void *), void *data)
{
	int i;

	for (i = 0; i < evn_used; i++) {
		/* here we display an event name on screen - so translate */
		callback(_(ev_namelist[i].ev_name), &ev_namelist[i].plot_ev, data);
	}
}

void remember_event(const char *eventname)
{
	int i=0, len;

	if (!eventname || (len = strlen(eventname)) == 0)
		return;
	while (i < evn_used) {
		if (!strncmp(eventname,ev_namelist[i].ev_name,len))
			return;
		i++;
	}
	if (evn_used == evn_allocated) {
		evn_allocated += 10;
		ev_namelist = realloc(ev_namelist, evn_allocated * sizeof(struct ev_select));
		if (! ev_namelist)
			/* we are screwed, but let's just bail out */
			return;
	}
	ev_namelist[evn_used].ev_name = strdup(eventname);
	ev_namelist[evn_used].plot_ev = TRUE;
	evn_used++;
}

static void plot_one_event(struct graphics_context *gc, struct plot_info *pi, struct event *event, const text_render_options_t *tro)
{
	int i, depth = 0;
	int x,y;

	/* is plotting this event disabled? */
	if (event->name) {
		for (i = 0; i < evn_used; i++) {
			if (! strcmp(event->name, ev_namelist[i].ev_name)) {
				if (ev_namelist[i].plot_ev)
					break;
				else
					return;
			}
		}
	}
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *data = pi->entry + i;
		if (event->time.seconds < data->sec)
			break;
		depth = data->depth;
	}
	/* draw a little tirangular marker and attach tooltip */
	x = SCALEX(gc, event->time.seconds);
	y = SCALEY(gc, depth);
	set_source_rgba(gc, ALERT_BG);
	cairo_move_to(gc->cr, x-15, y+6);
	cairo_line_to(gc->cr, x-3  , y+6);
	cairo_line_to(gc->cr, x-9, y-6);
	cairo_line_to(gc->cr, x-15, y+6);
	cairo_stroke_preserve(gc->cr);
	cairo_fill(gc->cr);
	set_source_rgba(gc, ALERT_FG);
	cairo_move_to(gc->cr, x-9, y-3);
	cairo_line_to(gc->cr, x-9, y+1);
	cairo_move_to(gc->cr, x-9, y+4);
	cairo_line_to(gc->cr, x-9, y+4);
	cairo_stroke(gc->cr);
	/* we display the event on screen - so translate */
	attach_tooltip(x-15, y-6, 12, 12, _(event->name));
}

static void plot_events(struct graphics_context *gc, struct plot_info *pi, struct dive *dive)
{
	static const text_render_options_t tro = {14, EVENTS, CENTER, TOP};
	struct event *event = dive->events;

	if (gc->printer)
		return;

	while (event) {
		plot_one_event(gc, pi, event, &tro);
		event = event->next;
	}
}

static void render_depth_sample(struct graphics_context *gc, struct plot_data *entry, const text_render_options_t *tro)
{
	int sec = entry->sec, decimals;
	double d;

	d = get_depth_units(entry->depth, &decimals, NULL);

	plot_text(gc, tro, sec, entry->depth, "%.*f", decimals, d);
}

static void plot_text_samples(struct graphics_context *gc, struct plot_info *pi)
{
	static const text_render_options_t deep = {14, SAMPLE_DEEP, CENTER, TOP};
	static const text_render_options_t shallow = {14, SAMPLE_SHALLOW, CENTER, BOTTOM};
	int i;
	int last = -1;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;

		if (entry->depth < 2000)
			continue;

		if ((entry == entry->max[2]) && entry->depth != last) {
			render_depth_sample(gc, entry, &deep);
			last = entry->depth;
		}

		if ((entry == entry->min[2]) && entry->depth != last) {
			render_depth_sample(gc, entry, &shallow);
			last = entry->depth;
		}

		if (entry->depth != last)
			last = -1;
	}
}

static void plot_depth_text(struct graphics_context *gc, struct plot_info *pi)
{
	int maxtime, maxdepth;

	/* Get plot scaling limits */
	maxtime = get_maxtime(pi);
	maxdepth = get_maxdepth(pi);

	gc->leftx = 0; gc->rightx = maxtime;
	gc->topy = 0; gc->bottomy = maxdepth;

	plot_text_samples(gc, pi);
}

static void plot_smoothed_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	struct plot_data *entry = pi->entry;

	set_source_rgba(gc, SMOOTHED);
	move_to(gc, entry->sec, entry->smoothed);
	for (i = 1; i < pi->nr; i++) {
		entry++;
		line_to(gc, entry->sec, entry->smoothed);
	}
	cairo_stroke(gc->cr);
}

static void plot_minmax_profile_minute(struct graphics_context *gc, struct plot_info *pi,
				int index)
{
	int i;
	struct plot_data *entry = pi->entry;

	set_source_rgba(gc, MINUTE);
	move_to(gc, entry->sec, entry->min[index]->depth);
	for (i = 1; i < pi->nr; i++) {
		entry++;
		line_to(gc, entry->sec, entry->min[index]->depth);
	}
	for (i = 1; i < pi->nr; i++) {
		line_to(gc, entry->sec, entry->max[index]->depth);
		entry--;
	}
	cairo_close_path(gc->cr);
	cairo_fill(gc->cr);
}

static void plot_minmax_profile(struct graphics_context *gc, struct plot_info *pi)
{
	if (gc->printer)
		return;
	plot_minmax_profile_minute(gc, pi, 2);
	plot_minmax_profile_minute(gc, pi, 1);
	plot_minmax_profile_minute(gc, pi, 0);
}

static void plot_depth_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i, incr;
	cairo_t *cr = gc->cr;
	int sec, depth;
	struct plot_data *entry;
	int maxtime, maxdepth, marker;
	int increments[8] = { 10, 20, 30, 60, 5*60, 10*60, 15*60, 30*60 };

	/* Get plot scaling limits */
	maxtime = get_maxtime(pi);
	maxdepth = get_maxdepth(pi);

	/* Time markers: at most every 10 seconds, but no more than 12 markers.
	 * We start out with 10 seconds and increment up to 30 minutes,
	 * depending on the dive time.
	 * This allows for 6h dives - enough (I hope) for even the craziest
	 * divers - but just in case, for those 8h depth-record-breaking dives,
	 * we double the interval if this still doesn't get us to 12 or fewer
	 * time markers */
	i = 0;
	while (maxtime / increments[i] > 12 && i < 8)
		i++;
	incr = increments[i];
	while (maxtime / incr > 12)
		incr *= 2;

	gc->leftx = 0; gc->rightx = maxtime;
	gc->topy = 0; gc->bottomy = 1.0;
	set_source_rgba(gc, TIME_GRID);
	cairo_set_line_width_scaled(gc->cr, 2);

	for (i = incr; i < maxtime; i += incr) {
		move_to(gc, i, 0);
		line_to(gc, i, 1);
	}
	cairo_stroke(cr);

	/* now the text on the time markers */
	text_render_options_t tro = {10, TIME_TEXT, CENTER, TOP};
	if (maxtime < 600) {
		/* Be a bit more verbose with shorter dives */
		for (i = incr; i < maxtime; i += incr)
			plot_text(gc, &tro, i, 1, "%02d:%02d", i/60, i%60);
	} else {
		/* Only render the time on every second marker for normal dives */
		for (i = incr; i < maxtime; i += 2 * incr)
			plot_text(gc, &tro, i, 1, "%d", i/60);
	}
	/* Depth markers: every 30 ft or 10 m*/
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = maxdepth;
	switch (output_units.length) {
	case METERS: marker = 10000; break;
	case FEET: marker = 9144; break;	/* 30 ft */
	}

	set_source_rgba(gc, DEPTH_GRID);
	for (i = marker; i < maxdepth; i += marker) {
		move_to(gc, 0, i);
		line_to(gc, 1, i);
	}
	cairo_stroke(cr);

	/* Show mean depth */
	if (! gc->printer) {
		set_source_rgba(gc, MEAN_DEPTH);
		move_to(gc, 0, pi->meandepth);
		line_to(gc, 1, pi->meandepth);
		cairo_stroke(cr);
	}

	gc->leftx = 0; gc->rightx = maxtime;

	/*
	 * These are good for debugging text placement etc,
	 * but not for actual display..
	 */
	if (0) {
		plot_smoothed_profile(gc, pi);
		plot_minmax_profile(gc, pi);
	}

	/* Do the depth profile for the neat fill */
	gc->topy = 0; gc->bottomy = maxdepth;

	cairo_pattern_t *pat;
	pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0 * plot_scale);
	pattern_add_color_stop_rgba (gc, pat, 1, DEPTH_BOTTOM);
	pattern_add_color_stop_rgba (gc, pat, 0, DEPTH_TOP);

	cairo_set_source(gc->cr, pat);
	cairo_pattern_destroy(pat);
	cairo_set_line_width_scaled(gc->cr, 2);

	entry = pi->entry;
	move_to(gc, 0, 0);
	for (i = 0; i < pi->nr; i++, entry++)
		line_to(gc, entry->sec, entry->depth);
	cairo_close_path(gc->cr);

	cairo_fill(gc->cr);

	/* Now do it again for the velocity colors */
	entry = pi->entry;
	for (i = 1; i < pi->nr; i++) {
		entry++;
		sec = entry->sec;
		/* we want to draw the segments in different colors
		 * representing the vertical velocity, so we need to
		 * chop this into short segments */
		depth = entry->depth;
		set_source_rgba(gc, VELOCITY_COLORS_START_IDX + entry->velocity);
		move_to(gc, entry[-1].sec, entry[-1].depth);
		line_to(gc, sec, depth);
		cairo_stroke(cr);
	}
}

static int setup_temperature_limits(struct graphics_context *gc, struct plot_info *pi)
{
	int maxtime, mintemp, maxtemp, delta;

	/* Get plot scaling limits */
	maxtime = get_maxtime(pi);
	mintemp = pi->mintemp;
	maxtemp = pi->maxtemp;

	gc->leftx = 0; gc->rightx = maxtime;
	/* Show temperatures in roughly the lower third, but make sure the scale
	   is at least somewhat reasonable */
	delta = maxtemp - mintemp;
	if (delta > 3000) { /* more than 3K in fluctuation */
		gc->topy = maxtemp + delta*2;
		gc->bottomy = mintemp - delta/2;
	} else {
		gc->topy = maxtemp + 1500 + delta*2;
		gc->bottomy = mintemp - delta/2;
	}

	return maxtemp > mintemp;
}

static void plot_single_temp_text(struct graphics_context *gc, int sec, int mkelvin)
{
	double deg;
	const char *unit;
	static const text_render_options_t tro = {12, TEMP_TEXT, LEFT, TOP};

	deg = get_temp_units(mkelvin, &unit);

	plot_text(gc, &tro, sec, mkelvin, "%d%s", (int)(deg + 0.5), unit);
}

static void plot_temperature_text(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	int last = -300, sec = 0;
	int last_temperature = 0, last_printed_temp = 0;

	if (!setup_temperature_limits(gc, pi))
		return;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry+i;
		int mkelvin = entry->temperature;

		if (!mkelvin)
			continue;
		last_temperature = mkelvin;
		sec = entry->sec;
		/* don't print a temperature
		 * if it's been less than 5min and less than a 2K change OR
		 * if it's been less than 2min OR if the change from the
		 * last print is less than .4K (and therefore less than 1F */
		if (((sec < last + 300) && (abs(mkelvin - last_printed_temp) < 2000)) ||
			(sec < last + 120) ||
			(abs(mkelvin - last_printed_temp) < 400))
			continue;
		last = sec;
		plot_single_temp_text(gc,sec,mkelvin);
		last_printed_temp = mkelvin;
	}
	/* it would be nice to print the end temperature, if it's
	 * different or if the last temperature print has been more
	 * than a quarter of the dive back */
	if ((abs(last_temperature - last_printed_temp) > 500) ||
		((double)last / (double)sec < 0.75))
		plot_single_temp_text(gc, sec, last_temperature);
}

static void plot_temperature_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	cairo_t *cr = gc->cr;
	int last = 0;

	if (!setup_temperature_limits(gc, pi))
		return;

	cairo_set_line_width_scaled(gc->cr, 2);
	set_source_rgba(gc, TEMP_PLOT);
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		int mkelvin = entry->temperature;
		int sec = entry->sec;
		if (!mkelvin) {
			if (!last)
				continue;
			mkelvin = last;
		}
		if (last)
			line_to(gc, sec, mkelvin);
		else
			move_to(gc, sec, mkelvin);
		last = mkelvin;
	}
	cairo_stroke(cr);
}

/* gets both the actual start and end pressure as well as the scaling factors */
static int get_cylinder_pressure_range(struct graphics_context *gc, struct plot_info *pi)
{
	gc->leftx = 0;
	gc->rightx = get_maxtime(pi);

	gc->bottomy = 0; gc->topy = pi->maxpressure * 1.5;
	return pi->maxpressure != 0;
}

/* set the color for the pressure plot according to temporary sac rate
 * as compared to avg_sac; the calculation simply maps the delta between
 * sac and avg_sac to indexes 0 .. (SAC_COLORS - 1) with everything
 * more than 6000 ml/min below avg_sac mapped to 0 */

static void set_sac_color(struct graphics_context *gc, int sac, int avg_sac)
{
	int sac_index = 0;
	int delta = sac - avg_sac + 7000;

	if (!gc->printer) {
		sac_index = delta / 2000;
		if (sac_index < 0)
			sac_index = 0;
		if (sac_index > SAC_COLORS - 1)
			sac_index = SAC_COLORS - 1;
		set_source_rgba(gc, SAC_COLORS_START_IDX + sac_index);
	} else {
		set_source_rgba(gc, SAC_DEFAULT);
	}
}

/* calculate the current SAC in ml/min and convert to int */
#define GET_LOCAL_SAC(_entry1, _entry2, _dive)	(int)				\
	((GET_PRESSURE((_entry1)) - GET_PRESSURE((_entry2))) *			\
		(_dive)->cylinder[(_entry1)->cylinderindex].type.size.mliter /	\
		(((_entry2)->sec - (_entry1)->sec) / 60.0) /			\
		(1 + ((_entry1)->depth + (_entry2)->depth) / 20000.0) /		\
		1000.0)

#define SAC_WINDOW 45	/* sliding window in seconds for current SAC calculation */

static void plot_cylinder_pressure(struct graphics_context *gc, struct plot_info *pi,
				struct dive *dive)
{
	int i;
	int last = -1;
	int lift_pen = FALSE;
	int first_plot = TRUE;
	int sac = 0;
	struct plot_data *last_entry = NULL;

	if (!get_cylinder_pressure_range(gc, pi))
		return;

	cairo_set_line_width_scaled(gc->cr, 2);

	for (i = 0; i < pi->nr; i++) {
		int mbar;
		struct plot_data *entry = pi->entry + i;

		mbar = GET_PRESSURE(entry);
		if (!entry->same_cylinder) {
			lift_pen = TRUE;
			last_entry = NULL;
		}
		if (!mbar) {
			lift_pen = TRUE;
			continue;
		}
		if (!last_entry) {
			last = i;
			last_entry = entry;
			sac = GET_LOCAL_SAC(entry, pi->entry + i + 1, dive);
		} else {
			int j;
			sac = 0;
			for (j = last; j < i; j++)
				sac += GET_LOCAL_SAC(pi->entry + j, pi->entry + j + 1, dive);
			sac /= (i - last);
			if (entry->sec - last_entry->sec >= SAC_WINDOW) {
				last++;
				last_entry = pi->entry + last;
			}
		}
		set_sac_color(gc, sac, dive->sac);
		if (lift_pen) {
			if (!first_plot && entry->same_cylinder) {
				/* if we have a previous event from the same tank,
				 * draw at least a short line */
				int prev_pr;
				prev_pr = GET_PRESSURE(entry - 1);
				move_to(gc, (entry-1)->sec, prev_pr);
				line_to(gc, entry->sec, mbar);
			} else {
				first_plot = FALSE;
				move_to(gc, entry->sec, mbar);
			}
			lift_pen = FALSE;
		} else {
			line_to(gc, entry->sec, mbar);
		}
		cairo_stroke(gc->cr);
		move_to(gc, entry->sec, mbar);
	}
}

static void plot_pressure_value(struct graphics_context *gc, int mbar, int sec,
				int xalign, int yalign)
{
	int pressure;
	const char *unit;

	pressure = get_pressure_units(mbar, &unit);
	text_render_options_t tro = {10, PRESSURE_TEXT, xalign, yalign};
	plot_text(gc, &tro, sec, mbar, "%d %s", pressure, unit);
}

static void plot_cylinder_pressure_text(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	int mbar, cyl;
	int seen_cyl[MAX_CYLINDERS] = { FALSE, };
	int last_pressure[MAX_CYLINDERS] = { 0, };
	int last_time[MAX_CYLINDERS] = { 0, };
	struct plot_data *entry;

	if (!get_cylinder_pressure_range(gc, pi))
		return;

	/* only loop over the actual events from the dive computer
	 * plus the second synthetic event at the start (to make sure
	 * we get "time=0" right)
	 * sadly with a recent change that first entry may no longer
	 * have any pressure reading - in that case just grab the
	 * pressure from the second entry */
	if (GET_PRESSURE(pi->entry + 1) == 0 && GET_PRESSURE(pi->entry + 2) !=0)
		INTERPOLATED_PRESSURE(pi->entry + 1) = GET_PRESSURE(pi->entry + 2);
	for (i = 1; i < pi->nr; i++) {
		entry = pi->entry + i;

		if (!entry->same_cylinder) {
			cyl = entry->cylinderindex;
			if (!seen_cyl[cyl]) {
				mbar = GET_PRESSURE(entry);
				plot_pressure_value(gc, mbar, entry->sec, LEFT, BOTTOM);
				seen_cyl[cyl] = TRUE;
			}
			if (i > 2) {
				/* remember the last pressure and time of
				 * the previous cylinder */
				cyl = (entry - 1)->cylinderindex;
				last_pressure[cyl] = GET_PRESSURE(entry - 1);
				last_time[cyl] = (entry - 1)->sec;
			}
		}
	}
	cyl = entry->cylinderindex;
	if (GET_PRESSURE(entry))
		last_pressure[cyl] = GET_PRESSURE(entry);
	last_time[cyl] = entry->sec;

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		if (last_time[cyl]) {
			plot_pressure_value(gc, last_pressure[cyl], last_time[cyl], CENTER, TOP);
		}
	}
}

static void analyze_plot_info_minmax_minute(struct plot_data *entry, struct plot_data *first, struct plot_data *last, int index)
{
	struct plot_data *p = entry;
	int time = entry->sec;
	int seconds = 90*(index+1);
	struct plot_data *min, *max;
	int avg, nr;

	/* Go back 'seconds' in time */
	while (p > first) {
		if (p[-1].sec < time - seconds)
			break;
		p--;
	}

	/* Then go forward until we hit an entry past the time */
	min = max = p;
	avg = p->depth;
	nr = 1;
	while (++p < last) {
		int depth = p->depth;
		if (p->sec > time + seconds)
			break;
		avg += depth;
		nr ++;
		if (depth < min->depth)
			min = p;
		if (depth > max->depth)
			max = p;
	}
	entry->min[index] = min;
	entry->max[index] = max;
	entry->avg[index] = (avg + nr/2) / nr;
}

static void analyze_plot_info_minmax(struct plot_data *entry, struct plot_data *first, struct plot_data *last)
{
	analyze_plot_info_minmax_minute(entry, first, last, 0);
	analyze_plot_info_minmax_minute(entry, first, last, 1);
	analyze_plot_info_minmax_minute(entry, first, last, 2);
}

static velocity_t velocity(int speed)
{
	velocity_t v;

	if (speed < -304) /* ascent faster than -60ft/min */
		v = CRAZY;
	else if (speed < -152) /* above -30ft/min */
		v = FAST;
	else if (speed < -76) /* -15ft/min */
		v = MODERATE;
	else if (speed < -25) /* -5ft/min */
		v = SLOW;
	else if (speed < 25) /* very hard to find data, but it appears that the recommendations
				for descent are usually about 2x ascent rate; still, we want
				stable to mean stable */
		v = STABLE;
	else if (speed < 152) /* between 5 and 30ft/min is considered slow */
		v = SLOW;
	else if (speed < 304) /* up to 60ft/min is moderate */
		v = MODERATE;
	else if (speed < 507) /* up to 100ft/min is fast */
		v = FAST;
	else /* more than that is just crazy - you'll blow your ears out */
		v = CRAZY;

	return v;
}
static struct plot_info *analyze_plot_info(struct plot_info *pi)
{
	int i;
	int nr = pi->nr;

	/* Do pressure min/max based on the non-surface data */
	for (i = 0; i < nr; i++) {
		struct plot_data *entry = pi->entry+i;
		int pressure = GET_PRESSURE(entry);
		int temperature = entry->temperature;

		if (pressure) {
			if (pressure > pi->maxpressure)
				pi->maxpressure = pressure;
		}

		if (temperature) {
			if (!pi->mintemp || temperature < pi->mintemp)
				pi->mintemp = temperature;
			if (temperature > pi->maxtemp)
				pi->maxtemp = temperature;
		}
	}

	/* Smoothing function: 5-point triangular smooth */
	for (i = 2; i < nr; i++) {
		struct plot_data *entry = pi->entry+i;
		int depth;

		if (i < nr-2) {
			depth = entry[-2].depth + 2*entry[-1].depth + 3*entry[0].depth + 2*entry[1].depth + entry[2].depth;
			entry->smoothed = (depth+4) / 9;
		}
		/* vertical velocity in mm/sec */
		/* Linus wants to smooth this - let's at least look at the samples that aren't FAST or CRAZY */
		if (entry[0].sec - entry[-1].sec) {
			entry->velocity = velocity((entry[0].depth - entry[-1].depth) / (entry[0].sec - entry[-1].sec));
                        /* if our samples are short and we aren't too FAST*/
			if (entry[0].sec - entry[-1].sec < 15 && entry->velocity < FAST) {
				int past = -2;
				while (i+past > 0 && entry[0].sec - entry[past].sec < 15)
					past--;
				entry->velocity = velocity((entry[0].depth - entry[past].depth) /
							(entry[0].sec - entry[past].sec));
			}
		} else
			entry->velocity = STABLE;
	}

	/* One-, two- and three-minute minmax data */
	for (i = 0; i < nr; i++) {
		struct plot_data *entry = pi->entry +i;
		analyze_plot_info_minmax(entry, pi->entry, pi->entry+nr);
	}

	return pi;
}

/*
 * simple structure to track the beginning and end tank pressure as
 * well as the integral of depth over time spent while we have no
 * pressure reading from the tank */
typedef struct pr_track_struct pr_track_t;
struct pr_track_struct {
	int start;
	int end;
	int t_start;
	int t_end;
	double pressure_time;
	pr_track_t *next;
};

static pr_track_t *pr_track_alloc(int start, int t_start) {
	pr_track_t *pt = malloc(sizeof(pr_track_t));
	pt->start = start;
	pt->t_start = t_start;
	pt->end = 0;
	pt->t_end = 0;
	pt->pressure_time = 0.0;
	pt->next = NULL;
	return pt;
}

/* poor man's linked list */
static pr_track_t *list_last(pr_track_t *list)
{
	pr_track_t *tail = list;
	if (!tail)
		return NULL;
	while (tail->next) {
		tail = tail->next;
	}
	return tail;
}

static pr_track_t *list_add(pr_track_t *list, pr_track_t *element)
{
	pr_track_t *tail = list_last(list);
	if (!tail)
		return element;
	tail->next = element;
	return list;
}

static void list_free(pr_track_t *list)
{
	if (!list)
		return;
	list_free(list->next);
	free(list);
}

static void dump_pr_track(pr_track_t **track_pr)
{
	int cyl;
	pr_track_t *list;

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		list = track_pr[cyl];
		while (list) {
			printf("cyl%d: start %d end %d t_start %d t_end %d pt %6.3f\n", cyl,
				list->start, list->end, list->t_start, list->t_end, list->pressure_time);
			list = list->next;
		}
	}
}

static void fill_missing_tank_pressures(struct plot_info *pi, pr_track_t **track_pr)
{
	pr_track_t *list = NULL;
	pr_track_t *nlist = NULL;
	double pt, magic;
	int cyl, i;
	struct plot_data *entry;
	int cur_pr[MAX_CYLINDERS];

	if (0) {
		/* another great debugging tool */
		dump_pr_track(track_pr);
	}
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		cur_pr[cyl] = track_pr[cyl]->start;
	}

	/* The first two are "fillers", but in case we don't have a sample
	 * at time 0 we need to process the second of them here */
	for (i = 1; i < pi->nr; i++) {
		entry = pi->entry + i;
		if (SENSOR_PRESSURE(entry)) {
			cur_pr[entry->cylinderindex] = SENSOR_PRESSURE(entry);
		} else {
			if(!list || list->t_end < entry->sec) {
				nlist = track_pr[entry->cylinderindex];
				list = NULL;
				while (nlist && nlist->t_start <= entry->sec) {
					list = nlist;
					nlist = list->next;
				}
				/* there may be multiple segments - so
				 * let's assemble the length */
				nlist = list;
				if (list) {
					pt = list->pressure_time;
					while (!nlist->end) {
						nlist = nlist->next;
						if (!nlist) {
							/* oops - we have no end pressure,
							 * so this means this is a tank without
							 * gas consumption information */
							break;
						}
						pt += nlist->pressure_time;
					}
				}
				if (!nlist) {
					/* just continue without calculating
					 * interpolated values */
					INTERPOLATED_PRESSURE(entry) = cur_pr[entry->cylinderindex];
					list = NULL;
					continue;
				}
				magic = (nlist->end - cur_pr[entry->cylinderindex]) / pt;
			}
			if (pt != 0.0) {
				double cur_pt = (entry->sec - (entry-1)->sec) *
					(1 + (entry->depth + (entry-1)->depth) / 20000.0);
				INTERPOLATED_PRESSURE(entry) =
					cur_pr[entry->cylinderindex] + cur_pt * magic + 0.5;
				cur_pr[entry->cylinderindex] = INTERPOLATED_PRESSURE(entry);
			} else
				INTERPOLATED_PRESSURE(entry) = cur_pr[entry->cylinderindex];
		}
	}
}

static int get_cylinder_index(struct dive *dive, struct event *ev)
{
	int i;

	/*
	 * Try to find a cylinder that matches the O2 percentage
	 * in the gas change event 'value' field.
	 *
	 * Crazy suunto gas change events. We really should do
	 * this in libdivecomputer or something.
	 */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder+i;
		int o2 = (cyl->gasmix.o2.permille + 5) / 10;
		if (o2 == ev->value)
			return i;
	}

	return 0;
}

static struct event *get_next_gaschange(struct event *event)
{
	while (event) {
		if (!strcmp(event->name, "gaschange"))
			return event;
		event = event->next;
	}
	return event;
}

static int set_cylinder_index(struct plot_info *pi, int i, int cylinderindex, unsigned int end)
{
	while (i < pi->nr) {
		struct plot_data *entry = pi->entry+i;
		if (entry->sec > end)
			break;
		if (entry->cylinderindex != cylinderindex) {
			entry->cylinderindex = cylinderindex;
			entry->pressure[0] = 0;
		}
		i++;
	}
	return i;
}

static void check_gas_change_events(struct dive *dive, struct plot_info *pi)
{
	int i = 0, cylinderindex = 0;
	struct event *ev = get_next_gaschange(dive->events);

	if (!ev)
		return;

	do {
		i = set_cylinder_index(pi, i, cylinderindex, ev->time.seconds);
		cylinderindex = get_cylinder_index(dive, ev);
		ev = get_next_gaschange(ev->next);
	} while (ev);
	set_cylinder_index(pi, i, cylinderindex, ~0u);
}

/* for computers that track gas changes through events */
static int count_gas_change_events(struct dive *dive)
{
	int count = 0;
	struct event *ev = get_next_gaschange(dive->events);

	while (ev) {
		count++;
		ev = get_next_gaschange(ev->next);
	}
	return count;
}

/*
 * Create a plot-info with smoothing and ranged min/max
 *
 * This also makes sure that we have extra empty events on both
 * sides, so that you can do end-points without having to worry
 * about it.
 */
static struct plot_info *create_plot_info(struct dive *dive, int nr_samples, struct sample *dive_sample)
{
	int cylinderindex = -1;
	int lastdepth, lastindex;
	int i, pi_idx, nr, sec, cyl;
	size_t alloc_size;
	struct plot_info *pi;
	pr_track_t *track_pr[MAX_CYLINDERS] = {NULL, };
	pr_track_t *pr_track, *current;
	gboolean missing_pr = FALSE;
	struct plot_data *entry = NULL;
	struct event *ev;

	/* we want to potentially add synthetic plot_info elements for the gas changes */
	nr = nr_samples + 4 + 2 * count_gas_change_events(dive);
	alloc_size = plot_info_size(nr);
	pi = malloc(alloc_size);
	if (!pi)
		return pi;
	memset(pi, 0, alloc_size);
	pi->nr = nr;
	pi_idx = 2; /* the two extra events at the start */
	/* check for gas changes before the samples start */
	ev = get_next_gaschange(dive->events);
	while (ev && ev->time.seconds < dive_sample->time.seconds) {
		entry = pi->entry + pi_idx;
		entry->sec = ev->time.seconds;
		entry->depth = 0; /* is that always correct ? */
		pi_idx++;
		ev = get_next_gaschange(ev->next);
	}
	if (ev && ev->time.seconds == dive_sample->time.seconds) {
		/* we already have a sample at the time of the event */
		ev = get_next_gaschange(ev->next);
	}
	sec = 0;
	lastindex = 0;
	lastdepth = -1;
	for (i = 0; i < nr_samples; i++) {
		int depth;
		int delay = 0;
		struct sample *sample = dive_sample+i;

		entry = pi->entry + i + pi_idx;
		while (ev && ev->time.seconds < sample->time.seconds) {
			/* insert two fake plot info structures for the end of
			 * the old tank and the start of the new tank */
			if (ev->time.seconds == sample->time.seconds - 1) {
				entry->sec = ev->time.seconds - 1;
				(entry+1)->sec = ev->time.seconds;
			} else {
				entry->sec = ev->time.seconds;
				(entry+1)->sec = ev->time.seconds + 1;
			}
			/* we need a fake depth - let's interpolate */
			if (i) {
				entry->depth = sample->depth.mm -
					(sample->depth.mm - (sample-1)->depth.mm) / 2;
			} else
				entry->depth = sample->depth.mm;
			(entry+1)->depth = entry->depth;
			pi_idx += 2;
			entry = pi->entry + i + pi_idx;
			ev = get_next_gaschange(ev->next);
		}
		if (ev && ev->time.seconds == sample->time.seconds) {
			/* we already have a sample at the time of the event
			 * just add a new one for the old tank and delay the
			 * real even by one second (to keep time monotonous) */
			entry->sec = ev->time.seconds;
			entry->depth = sample->depth.mm;
			pi_idx++;
			entry = pi->entry + i + pi_idx;
			ev = get_next_gaschange(ev->next);
			delay = 1;
		}
		sec = entry->sec = sample->time.seconds + delay;
		depth = entry->depth = sample->depth.mm;
		entry->cylinderindex = sample->cylinderindex;
		SENSOR_PRESSURE(entry) = sample->cylinderpressure.mbar;
		entry->temperature = sample->temperature.mkelvin;

		if (depth || lastdepth)
			lastindex = i + pi_idx;

		lastdepth = depth;
		if (depth > pi->maxdepth)
			pi->maxdepth = depth;
	}
	entry = pi->entry + i + pi_idx;
	/* are there still unprocessed gas changes? that would be very strange */
	while (ev) {
		entry->sec = ev->time.seconds;
		entry->depth = 0; /* why are there gas changes after the dive is over? */
		pi_idx++;
		entry = pi->entry + i + pi_idx;
		ev = get_next_gaschange(ev->next);
	}
	nr = nr_samples + pi_idx - 2;
	check_gas_change_events(dive, pi);

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) /* initialize the start pressures */
		track_pr[cyl] = pr_track_alloc(dive->cylinder[cyl].start.mbar, -1);
	current = track_pr[pi->entry[2].cylinderindex];
	for (i = 0; i < nr + 1; i++) {
		entry = pi->entry + i + 1;

		entry->same_cylinder = entry->cylinderindex == cylinderindex;
		cylinderindex = entry->cylinderindex;

		/* track the segments per cylinder and their pressure/time integral */
		if (!entry->same_cylinder) {
			current->end = SENSOR_PRESSURE(entry-1);
			current->t_end = (entry-1)->sec;
			current = pr_track_alloc(SENSOR_PRESSURE(entry), entry->sec);
			track_pr[cylinderindex] = list_add(track_pr[cylinderindex], current);
		} else { /* same cylinder */
			if ((!SENSOR_PRESSURE(entry) && SENSOR_PRESSURE(entry-1)) ||
				(SENSOR_PRESSURE(entry) && !SENSOR_PRESSURE(entry-1))) {
				/* transmitter changed its working status */
				current->end = SENSOR_PRESSURE(entry-1);
				current->t_end = (entry-1)->sec;
				current = pr_track_alloc(SENSOR_PRESSURE(entry), entry->sec);
				track_pr[cylinderindex] =
					list_add(track_pr[cylinderindex], current);
			}
		}
		/* finally, do the discrete integration to get the SAC rate equivalent */
		current->pressure_time += (entry->sec - (entry-1)->sec) *
			(1 + (entry->depth + (entry-1)->depth) / 20000.0);
		missing_pr |= !SENSOR_PRESSURE(entry);
	}

	if (entry)
		current->t_end = entry->sec;

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) { /* initialize the end pressures */
		int pr = dive->cylinder[cyl].end.mbar;
		if (pr && track_pr[cyl]) {
			pr_track = list_last(track_pr[cyl]);
			pr_track->end = pr;
		}
	}
	/* Fill in the last two entries with empty values but valid times
	 * without creating a false cylinder change event */
	i = nr + 2;
	pi->entry[i].sec = sec + 20;
	pi->entry[i].same_cylinder = 1;
	pi->entry[i].cylinderindex = pi->entry[i-1].cylinderindex;
	INTERPOLATED_PRESSURE(pi->entry + i) = GET_PRESSURE(pi->entry + i - 1);
	pi->entry[i+1].sec = sec + 40;
	pi->entry[i+1].same_cylinder = 1;
	pi->entry[i+1].cylinderindex = pi->entry[i-1].cylinderindex;
	INTERPOLATED_PRESSURE(pi->entry + i + 1) = GET_PRESSURE(pi->entry + i - 1);
	/* the number of actual entries - some computers have lots of
	 * depth 0 samples at the end of a dive, we want to make sure
	 * we have exactly one of them at the end */
	pi->nr = lastindex+1;
	while (pi->nr <= i+2 && pi->entry[pi->nr-1].depth > 0)
		pi->nr++;
	pi->maxtime = pi->entry[lastindex].sec;

	/* Analyze_plot_info() will do the sample max pressures,
	 * this handles the manual pressures
	 */
	pi->maxpressure = 0;
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		unsigned int mbar = dive->cylinder[cyl].start.mbar;
		if (mbar > pi->maxpressure)
			pi->maxpressure = mbar;
	}

	pi->meandepth = dive->meandepth.mm;

	if (missing_pr) {
		fill_missing_tank_pressures(pi, track_pr);
	}
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++)
		list_free(track_pr[cyl]);
	if (0) /* awesome for debugging - not useful otherwise */
		dump_pi(pi);
	return analyze_plot_info(pi);
}

static void plot_set_scale(scale_mode_t scale)
{
	switch (scale) {
	default:
	case SC_SCREEN:
		plot_scale = SCALE_SCREEN;
		break;
	case SC_PRINT:
		plot_scale = SCALE_PRINT;
		break;
	}
}

void plot(struct graphics_context *gc, cairo_rectangle_t *drawing_area, struct dive *dive, scale_mode_t scale)
{
	struct plot_info *pi;
	static struct sample fake[4];
	struct sample *sample = dive->sample;
	int nr = dive->samples;

	plot_set_scale(scale);

	if (!nr) {
		/* The dive has no samples, so create a few fake ones.  This assumes an
		ascent/descent rate of 9 m/min, which is just below the limit for FAST. */
		int duration = dive->duration.seconds;
		int maxdepth = dive->maxdepth.mm;
		int asc_desc_time = dive->maxdepth.mm*60/9000;
		if (asc_desc_time * 2 >= duration)
			asc_desc_time = duration / 2;
		sample = fake;
		fake[1].time.seconds = asc_desc_time;
		fake[1].depth.mm = maxdepth;
		fake[2].time.seconds = duration - asc_desc_time;
		fake[2].depth.mm = maxdepth;
		fake[3].time.seconds = duration * 1.00;
		nr = 4;
	}

	pi = create_plot_info(dive, nr, sample);

	cairo_translate(gc->cr, drawing_area->x, drawing_area->y);
	cairo_set_line_width_scaled(gc->cr, 1);
	cairo_set_line_cap(gc->cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(gc->cr, CAIRO_LINE_JOIN_ROUND);

	/*
	 * We can use "cairo_translate()" because that doesn't
	 * scale line width etc. But the actual scaling we need
	 * do set up ourselves..
	 *
	 * Snif. What a pity.
	 */
	gc->maxx = (drawing_area->width - 2*drawing_area->x);
	gc->maxy = (drawing_area->height - 2*drawing_area->y);

	/* Depth profile */
	plot_depth_profile(gc, pi);
	plot_events(gc, pi, dive);

	/* Temperature profile */
	plot_temperature_profile(gc, pi);

	/* Cylinder pressure plot */
	plot_cylinder_pressure(gc, pi, dive);

	/* Text on top of all graphs.. */
	plot_temperature_text(gc, pi);
	plot_depth_text(gc, pi);
	plot_cylinder_pressure_text(gc, pi);

	/* Bounding box last */
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = 1.0;

	set_source_rgba(gc, BOUNDING_BOX);
	cairo_set_line_width_scaled(gc->cr, 1);
	move_to(gc, 0, 0);
	line_to(gc, 0, 1);
	line_to(gc, 1, 1);
	line_to(gc, 1, 0);
	cairo_close_path(gc->cr);
	cairo_stroke(gc->cr);

	free(pi);
}
