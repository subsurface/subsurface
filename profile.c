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
#include "display-gtk.h"
#include "divelist.h"
#include "color.h"
#include "libdivecomputer/parser.h"
#include "libdivecomputer/version.h"

int selected_dive = 0;
char zoomed_plot = 0;
char dc_number = 0;

static double plot_scale = SCALE_SCREEN;
static struct plot_data *last_pi_entry = NULL;

#define cairo_set_line_width_scaled(cr, w) \
	cairo_set_line_width((cr), (w) * plot_scale);

typedef enum { STABLE, SLOW, MODERATE, FAST, CRAZY } velocity_t;

struct plot_data {
	unsigned int same_cylinder:1, in_deco:1;
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

	/* gas colors */
	PO2, PO2_ALERT, PN2, PN2_ALERT, PHE, PHE_ALERT, PP_LINES,

	/* Other colors */
	TEXT_BACKGROUND, ALERT_BG, ALERT_FG, EVENTS, SAMPLE_DEEP, SAMPLE_SHALLOW,
	SMOOTHED, MINUTE, TIME_GRID, TIME_TEXT, DEPTH_GRID, MEAN_DEPTH, DEPTH_TOP,
	DEPTH_BOTTOM, TEMP_TEXT, TEMP_PLOT, SAC_DEFAULT, BOUNDING_BOX, PRESSURE_TEXT, BACKGROUND,
	CEILING_SHALLOW, CEILING_DEEP, CALC_CEILING_SHALLOW, CALC_CEILING_DEEP
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

	[PO2]             = {{APPLE1, APPLE1_MED_TRANS}},
	[PO2_ALERT]       = {{RED1, APPLE1_MED_TRANS}},
	[PN2]             = {{BLACK1_LOW_TRANS, BLACK1_LOW_TRANS}},
	[PN2_ALERT]       = {{RED1, BLACK1_LOW_TRANS}},
	[PHE]             = {{PEANUT, PEANUT_MED_TRANS}},
	[PHE_ALERT]       = {{RED1, PEANUT_MED_TRANS}},
	[PP_LINES]        = {{BLACK1_HIGH_TRANS, BLACK1_HIGH_TRANS}},

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
	[CEILING_SHALLOW] = {{REDORANGE1_HIGH_TRANS, REDORANGE1_HIGH_TRANS}},
	[CEILING_DEEP]    = {{RED1_MED_TRANS, RED1_MED_TRANS}},
	[CALC_CEILING_SHALLOW] = {{FUNGREEN1_HIGH_TRANS, FUNGREEN1_HIGH_TRANS}},
	[CALC_CEILING_DEEP]    = {{APPLE1_HIGH_TRANS, APPLE1_HIGH_TRANS}},

};

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
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = &pi->entry[i];
		printf("    entry[%d]:{same_cylinder:%d cylinderindex:%d sec:%d pressure:{%d,%d}\n"
			"                time:%d:%02d temperature:%d depth:%d stopdepth:%d stoptime:%d ndl:%d smoothed:%d po2:%lf phe:%lf pn2:%lf sum-pp %lf}\n",
			i, entry->same_cylinder, entry->cylinderindex, entry->sec,
			entry->pressure[0], entry->pressure[1],
			entry->sec / 60, entry->sec % 60,
			entry->temperature, entry->depth, entry->stopdepth, entry->stoptime, entry->ndl, entry->smoothed,
			entry->po2, entry->phe, entry->pn2,
			entry->po2 + entry->phe + entry->pn2);
	}
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

/* get the maximum depth to which we want to plot
 * take into account the additional verical space needed to plot
 * partial pressure graphs */
static int get_maxdepth(struct plot_info *pi)
{
	unsigned mm = pi->maxdepth;
	int md;

	if (zoomed_plot) {
		/* Rounded up to 10m, with at least 3m to spare */
		md = ROUND_UP(mm+3000, 10000);
	} else {
		/* Minimum 30m, rounded up to 10m, with at least 3m to spare */
		md = MAX(30000, ROUND_UP(mm+3000, 10000));
	}
	md += pi->maxpp * 9000;
	return md;
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
	dx = tro->hpos * (extents.width + extents.x_bearing);
	dy = tro->vpos * (extents.height + fe.descent);
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

/* collect all event names and whether we display them */
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
	int i = 0, len;

	if (!eventname || (len = strlen(eventname)) == 0)
		return;
	while (i < evn_used) {
		if (!strncmp(eventname, ev_namelist[i].ev_name, len))
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
	char buffer[80];

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
	if (event->time.seconds < 30 && !strcmp(event->name, "gaschange"))
		/* a gas change in the first 30 seconds is the way of some dive computers
		 * to tell us the gas that is used; let's not plot a marker for that */
		return;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *data = pi->entry + i;
		if (event->time.seconds < data->sec)
			break;
		depth = data->depth;
	}
	/* draw a little triangular marker and attach tooltip */
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
	if (event->value) {
		if (event->name && !strcmp(event->name, "gaschange")) {
			unsigned int he = event->value >> 16;
			unsigned int o2 = event->value & 0xffff;
			if (he) {
				snprintf(buffer, sizeof(buffer), "%s: (%d/%d)",
					_(event->name), o2, he);
			} else {
				if (o2 == 21)
					snprintf(buffer, sizeof(buffer), "%s: %s",
						_(event->name), _("air"));
				else
					snprintf(buffer, sizeof(buffer), "%s: %d%% %s",
						_(event->name), o2, _("O" UTF8_SUBSCRIPT_2));
			}
		} else {
			snprintf(buffer, sizeof(buffer), "%s: %d", _(event->name), event->value);
		}
	} else {
		snprintf(buffer, sizeof(buffer), "%s", _(event->name));
	}
	attach_tooltip(x-15, y-6, 12, 12, buffer);
}

static void plot_events(struct graphics_context *gc, struct plot_info *pi, struct divecomputer *dc)
{
	static const text_render_options_t tro = {14, EVENTS, CENTER, TOP};
	struct event *event = dc->events;

	if (gc->printer)
		return;

	while (event) {
		if (event->flags != SAMPLE_FLAGS_BEGIN && event->flags != SAMPLE_FLAGS_END)
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

static void plot_depth_scale(struct graphics_context *gc, struct plot_info *pi)
{
	int i, maxdepth, marker;
	static const text_render_options_t tro = {10, SAMPLE_DEEP, RIGHT, MIDDLE};

	/* Depth markers: every 30 ft or 10 m*/
	maxdepth = get_maxdepth(pi);
	gc->topy = 0; gc->bottomy = maxdepth;

	switch (prefs.units.length) {
	case METERS: marker = 10000; break;
	case FEET: marker = 9144; break;	/* 30 ft */
	}
	set_source_rgba(gc, DEPTH_GRID);
	/* don't write depth labels all the way to the bottom as
	 * there may be other graphs below the depth plot (like
	 * partial pressure graphs) where this would look out
	 * of place - so we only make sure that we print the next
	 * marker below the actual maxdepth of the dive */
	for (i = marker; i <= pi->maxdepth + marker; i += marker) {
		double d = get_depth_units(i, NULL, NULL);
		plot_text(gc, &tro, -0.002, i, "%.0f", d);
	}
}

static void setup_pp_limits(struct graphics_context *gc, struct plot_info *pi)
{
	int maxdepth;

	gc->leftx = 0;
	gc->rightx = get_maxtime(pi);

	/* the maxdepth already includes extra vertical space - and if
	 * we use 1.5 times the corresponding pressure as maximum partial
	 * pressure the graph seems to look fine*/
	maxdepth = get_maxdepth(pi);
	gc->topy = 1.5 * (maxdepth + 10000) / 10000.0 * SURFACE_PRESSURE / 1000;
	gc->bottomy = -gc->topy / 20;
}

static void plot_pp_text(struct graphics_context *gc, struct plot_info *pi)
{
	double pp, dpp, m;
	int hpos;
	static const text_render_options_t tro = {11, PP_LINES, LEFT, MIDDLE};

	setup_pp_limits(gc, pi);
	pp = floor(pi->maxpp * 10.0) / 10.0 + 0.2;
	dpp = pp > 4 ? 1.0 : 0.5;
	hpos = pi->entry[pi->nr - 1].sec;
	set_source_rgba(gc, PP_LINES);
	for (m = 0.0; m <= pp; m += dpp) {
		move_to(gc, 0, m);
		line_to(gc, hpos, m);
		cairo_stroke(gc->cr);
		plot_text(gc, &tro, hpos + 30, m, "%.1f", m);
	}
}

static void plot_pp_gas_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	struct plot_data *entry;

	setup_pp_limits(gc, pi);

	if (prefs.pp_graphs.pn2) {
		set_source_rgba(gc, PN2);
		entry = pi->entry;
		move_to(gc, entry->sec, entry->pn2);
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->pn2 < prefs.pp_graphs.pn2_threshold)
				line_to(gc, entry->sec, entry->pn2);
			else
				move_to(gc, entry->sec, entry->pn2);
		}
		cairo_stroke(gc->cr);

		set_source_rgba(gc, PN2_ALERT);
		entry = pi->entry;
		move_to(gc, entry->sec, entry->pn2);
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->pn2 >= prefs.pp_graphs.pn2_threshold)
				line_to(gc, entry->sec, entry->pn2);
			else
				move_to(gc, entry->sec, entry->pn2);
		}
		cairo_stroke(gc->cr);
	}
	if (prefs.pp_graphs.phe) {
		set_source_rgba(gc, PHE);
		entry = pi->entry;
		move_to(gc, entry->sec, entry->phe);
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->phe < prefs.pp_graphs.phe_threshold)
				line_to(gc, entry->sec, entry->phe);
			else
				move_to(gc, entry->sec, entry->phe);
		}
		cairo_stroke(gc->cr);

		set_source_rgba(gc, PHE_ALERT);
		entry = pi->entry;
		move_to(gc, entry->sec, entry->phe);
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->phe >= prefs.pp_graphs.phe_threshold)
				line_to(gc, entry->sec, entry->phe);
			else
				move_to(gc, entry->sec, entry->phe);
		}
		cairo_stroke(gc->cr);
	}
	if (prefs.pp_graphs.po2) {
		set_source_rgba(gc, PO2);
		entry = pi->entry;
		move_to(gc, entry->sec, entry->po2);
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->po2 < prefs.pp_graphs.po2_threshold)
				line_to(gc, entry->sec, entry->po2);
			else
				move_to(gc, entry->sec, entry->po2);
		}
		cairo_stroke(gc->cr);

		set_source_rgba(gc, PO2_ALERT);
		entry = pi->entry;
		move_to(gc, entry->sec, entry->po2);
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->po2 >= prefs.pp_graphs.po2_threshold)
				line_to(gc, entry->sec, entry->po2);
			else
				move_to(gc, entry->sec, entry->po2);
		}
		cairo_stroke(gc->cr);
	}
}

static void plot_depth_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i, incr;
	cairo_t *cr = gc->cr;
	int sec, depth;
	struct plot_data *entry;
	int maxtime, maxdepth, marker, maxline;
	int increments[8] = { 10, 20, 30, 60, 5*60, 10*60, 15*60, 30*60 };

	/* Get plot scaling limits */
	maxtime = get_maxtime(pi);
	maxdepth = get_maxdepth(pi);

	gc->maxtime = maxtime;

	/* Time markers: at most every 10 seconds, but no more than 12 markers.
	 * We start out with 10 seconds and increment up to 30 minutes,
	 * depending on the dive time.
	 * This allows for 6h dives - enough (I hope) for even the craziest
	 * divers - but just in case, for those 8h depth-record-breaking dives,
	 * we double the interval if this still doesn't get us to 12 or fewer
	 * time markers */
	i = 0;
	while (maxtime / increments[i] > 12 && i < 7)
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
	switch (prefs.units.length) {
	case METERS: marker = 10000; break;
	case FEET: marker = 9144; break;	/* 30 ft */
	}
	maxline = MAX(pi->maxdepth + marker, maxdepth * 2 / 3);
	set_source_rgba(gc, DEPTH_GRID);
	for (i = marker; i < maxline; i += marker) {
		move_to(gc, 0, i);
		line_to(gc, 1, i);
	}
	cairo_stroke(cr);

	gc->leftx = 0; gc->rightx = maxtime;

	/* Show mean depth */
	if (! gc->printer) {
		set_source_rgba(gc, MEAN_DEPTH);
		move_to(gc, 0, pi->meandepth);
		line_to(gc, pi->entry[pi->nr - 1].sec, pi->meandepth);
		cairo_stroke(cr);
	}

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

	/* Show any ceiling we may have encountered */
	for (i = pi->nr - 1; i >= 0; i--, entry--) {
		if (entry->ndl) {
			/* non-zero NDL implies this is a safety stop, no ceiling */
			line_to(gc, entry->sec, 0);
		} else if (entry->stopdepth < entry->depth) {
				line_to(gc, entry->sec, entry->stopdepth);
		} else {
			line_to(gc, entry->sec, entry->depth);
		}
	}
	cairo_close_path(gc->cr);
	cairo_fill(gc->cr);

	/* if the user wants the deco ceiling more visible, do that here (this
	 * basically draws over the background that we had allowed to shine
	 * through so far) */
	if (prefs.profile_red_ceiling) {
		pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0 * plot_scale);
		pattern_add_color_stop_rgba (gc, pat, 0, CEILING_SHALLOW);
		pattern_add_color_stop_rgba (gc, pat, 1, CEILING_DEEP);
		cairo_set_source(gc->cr, pat);
		cairo_pattern_destroy(pat);
		entry = pi->entry;
		move_to(gc, 0, 0);
		for (i = 0; i < pi->nr; i++, entry++) {
			if (entry->ndl == 0 && entry->stopdepth) {
				if (entry->ndl == 0 && entry->stopdepth < entry->depth) {
					line_to(gc, entry->sec, entry->stopdepth);
				} else {
					line_to(gc, entry->sec, entry->depth);
				}
			} else {
				line_to(gc, entry->sec, 0);
			}
		}
		cairo_close_path(gc->cr);
		cairo_fill(gc->cr);
	}
	/* finally, plot the calculated ceiling over all this */
	if (prefs.profile_calc_ceiling) {
		pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0 * plot_scale);
		pattern_add_color_stop_rgba (gc, pat, 0, CALC_CEILING_SHALLOW);
		pattern_add_color_stop_rgba (gc, pat, 1, CALC_CEILING_DEEP);
		cairo_set_source(gc->cr, pat);
		cairo_pattern_destroy(pat);
		entry = pi->entry;
		move_to(gc, 0, 0);
		for (i = 0; i < pi->nr; i++, entry++) {
			if (entry->ceiling)
				line_to(gc, entry->sec, entry->ceiling);
			else
				line_to(gc, entry->sec, 0);
		}
		line_to(gc, (entry-1)->sec, 0); /* make sure we end at 0 */
		cairo_close_path(gc->cr);
		cairo_fill(gc->cr);
	}
	/* next show where we have been bad and crossed the ceiling */
	pat = cairo_pattern_create_linear (0.0, 0.0,  0.0, 256.0 * plot_scale);
	pattern_add_color_stop_rgba (gc, pat, 0, CEILING_SHALLOW);
	pattern_add_color_stop_rgba (gc, pat, 1, CEILING_DEEP);
	cairo_set_source(gc->cr, pat);
	cairo_pattern_destroy(pat);
	entry = pi->entry;
	move_to(gc, 0, 0);
	for (i = 0; i < pi->nr; i++, entry++)
		line_to(gc, entry->sec, entry->depth);

	for (i = pi->nr - 1; i >= 0; i--, entry--) {
		if (entry->ndl == 0 && entry->stopdepth > entry->depth) {
			line_to(gc, entry->sec, entry->stopdepth);
		} else {
			line_to(gc, entry->sec, entry->depth);
		}
	}
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
	if (delta < 3000) /* less than 3K in fluctuation */
		delta = 3000;
	gc->topy = maxtemp + delta*2;

	if (PP_GRAPHS_ENABLED)
		gc->bottomy = mintemp - delta * 2;
	else
		gc->bottomy = mintemp - delta / 3;

	pi->endtempcoord = SCALEY(gc, pi->mintemp);
	return maxtemp > mintemp;
}

static void plot_single_temp_text(struct graphics_context *gc, int sec, int mkelvin)
{
	double deg;
	const char *unit;
	static const text_render_options_t tro = {12, TEMP_TEXT, LEFT, TOP};

	deg = get_temp_units(mkelvin, &unit);

	plot_text(gc, &tro, sec, mkelvin, "%.2g%s", deg, unit);
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

	if (PP_GRAPHS_ENABLED)
		gc->bottomy = -pi->maxpressure * 0.75;
	else
		gc->bottomy = 0;
	gc->topy = pi->maxpressure * 1.5;
	if (!pi->maxpressure)
		return FALSE;

	while (pi->endtempcoord <= SCALEY(gc, pi->minpressure - (gc->topy) * 0.1))
		gc->bottomy -=  gc->topy * 0.1;

	return TRUE;
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
		depth_to_mbar(((_entry1)->depth + (_entry2)->depth) / 2.0, (_dive)))

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
	int pressure_time;
	pr_track_t *next;
};

static pr_track_t *pr_track_alloc(int start, int t_start) {
	pr_track_t *pt = malloc(sizeof(pr_track_t));
	pt->start = start;
	pt->t_start = t_start;
	pt->end = 0;
	pt->t_end = 0;
	pt->pressure_time = 0;
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
			printf("cyl%d: start %d end %d t_start %d t_end %d pt %d\n", cyl,
				list->start, list->end, list->t_start, list->t_end, list->pressure_time);
			list = list->next;
		}
	}
}

/*
 * This looks at the pressures for one cylinder, and
 * calculates any missing beginning/end pressures for
 * each segment by taking the over-all SAC-rate into
 * account for that cylinder.
 *
 * NOTE! Many segments have full pressure information
 * (both beginning and ending pressure). But if we have
 * switched away from a cylinder, we will have the
 * beginning pressure for the first segment with a
 * missing end pressure. We may then have one or more
 * segments without beginning or end pressures, until
 * we finally have a segment with an end pressure.
 *
 * We want to spread out the pressure over these missing
 * segments according to how big of a time_pressure area
 * they have.
 */
static void fill_missing_segment_pressures(pr_track_t *list)
{
	while (list) {
		int start = list->start, end;
		pr_track_t *tmp = list;
		int pt_sum = 0, pt = 0;

		for (;;) {
			pt_sum += tmp->pressure_time;
			end = tmp->end;
			if (end)
				break;
			end = start;
			if (!tmp->next)
				break;
			tmp = tmp->next;
		}

		if (!start)
			start = end;

		/*
		 * Now 'start' and 'end' contain the pressure values
		 * for the set of segments described by 'list'..'tmp'.
		 * pt_sum is the sum of all the pressure-times of the
		 * segments.
		 *
		 * Now dole out the pressures relative to pressure-time.
		 */
		list->start = start;
		tmp->end = end;
		for (;;) {
			int pressure;
			pt += list->pressure_time;
			pressure = start;
			if (pt_sum)
				pressure -= (start-end)*(double)pt/pt_sum;
			list->end = pressure;
			if (list == tmp)
				break;
			list = list->next;
			list->start = pressure;
		}

		/* Ok, we've done that set of segments */
		list = list->next;
	}
}

/*
 * What's the pressure-time between two plot data entries?
 * We're calculating the integral of pressure over time by
 * adding these up.
 *
 * The units won't matter as long as everybody agrees about
 * them, since they'll cancel out - we use this to calculate
 * a constant SAC-rate-equivalent, but we only use it to
 * scale pressures, so it ends up being a unitless scaling
 * factor.
 */
static inline int pressure_time(struct dive *dive, struct plot_data *a, struct plot_data *b)
{
	int time = b->sec - a->sec;
	int depth = (a->depth + b->depth)/2;

	return depth_to_mbar(depth, dive) * time;
}

static void fill_missing_tank_pressures(struct dive *dive, struct plot_info *pi, pr_track_t **track_pr)
{
	int cyl, i;
	struct plot_data *entry;
	int cur_pr[MAX_CYLINDERS];

	if (0) {
		/* another great debugging tool */
		dump_pr_track(track_pr);
	}
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		fill_missing_segment_pressures(track_pr[cyl]);
		cur_pr[cyl] = track_pr[cyl]->start;
	}

	/* The first two are "fillers", but in case we don't have a sample
	 * at time 0 we need to process the second of them here */
	for (i = 1; i < pi->nr; i++) {
		double magic, cur_pt;
		pr_track_t *segment;
		int pressure;

		entry = pi->entry + i;
		cyl = entry->cylinderindex;

		if (SENSOR_PRESSURE(entry)) {
			cur_pr[cyl] = SENSOR_PRESSURE(entry);
			continue;
		}

		/* Find the right pressure segment for this entry.. */
		segment = track_pr[cyl];
		while (segment && segment->t_end < entry->sec)
			segment = segment->next;

		/* No (or empty) segment? Just use our current pressure */
		if (!segment || !segment->pressure_time) {
			SENSOR_PRESSURE(entry) = cur_pr[cyl];
			continue;
		}

		/* Overall pressure change over total pressure-time for this segment*/
		magic = (segment->end - segment->start) / (double) segment->pressure_time;

		/* Use that overall pressure change to update the current pressure */
		cur_pt = pressure_time(dive, entry-1, entry);
		pressure = cur_pr[cyl] + cur_pt * magic + 0.5;
		INTERPOLATED_PRESSURE(entry) = pressure;
		cur_pr[cyl] = pressure;
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
	 *
	 * There are two different gas change events that can get
	 * us here - GASCHANGE2 has the He value in the high 16
	 * bits; looking at the possible values we can actually
	 * handle them with the same code since the high 16 bits
	 * will be 0 with the GASCHANGE event - and that means no He
	 */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder+i;
		int o2 = (cyl->gasmix.o2.permille + 5) / 10;
		int he = (cyl->gasmix.he.permille + 5) / 10;
		if (o2 == (ev->value & 0xFFFF) && he == (ev->value >> 16))
			return i;
	}
	return 0;
}

static struct event *get_next_event(struct event *event, char *name)
{
	if (!name || !*name)
		return NULL;
	while (event) {
		if (!strcmp(event->name, name))
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

static void check_gas_change_events(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i = 0, cylinderindex = 0;
	struct event *ev = get_next_event(dc->events, "gaschange");

	if (!ev)
		return;

	do {
		i = set_cylinder_index(pi, i, cylinderindex, ev->time.seconds);
		cylinderindex = get_cylinder_index(dive, ev);
		ev = get_next_event(ev->next, "gaschange");
	} while (ev);
	set_cylinder_index(pi, i, cylinderindex, ~0u);
}

static void calculate_max_limits(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc)
{
	struct plot_info *pi;
	int maxdepth = 0;
	int maxtime = 0;
	int maxpressure = 0, minpressure = INT_MAX;
	int mintemp = 0, maxtemp = 0;
	int cyl;

	/* The plot-info is embedded in the graphics context */
	pi = &gc->pi;
	memset(pi, 0, sizeof(*pi));

	/* This should probably have been per-dive-computer */
	maxdepth = dive->dc.maxdepth.mm;
	mintemp = maxtemp = dive->dc.watertemp.mkelvin;

	/* Get the per-cylinder maximum pressure if they are manual */
	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		unsigned int mbar = dive->cylinder[cyl].start.mbar;
		if (mbar > maxpressure)
			maxpressure = mbar;
	}

	/* Then do all the samples from all the dive computers */
	do {
		int i = dc->samples;
		int lastdepth = 0;
		struct sample *s = dc->sample;

		while (--i >= 0) {
			int depth = s->depth.mm;
			int pressure = s->cylinderpressure.mbar;
			int temperature = s->temperature.mkelvin;

			if (!mintemp && temperature < mintemp)
				mintemp = temperature;
			if (temperature > maxtemp)
				maxtemp = temperature;

			if (pressure && pressure < minpressure)
				minpressure = pressure;
			if (pressure > maxpressure)
				maxpressure = pressure;

			if (depth > maxdepth)
				maxdepth = s->depth.mm;
			if ((depth || lastdepth) && s->time.seconds > maxtime)
				maxtime = s->time.seconds;
			lastdepth = depth;
			s++;
		}
	} while ((dc = dc->next) != NULL);

	if (minpressure > maxpressure)
		minpressure = 0;

	pi->maxdepth = maxdepth;
	pi->maxtime = maxtime;
	pi->maxpressure = maxpressure;
	pi->minpressure = minpressure;
	pi->mintemp = mintemp;
	pi->maxtemp = maxtemp;
}

static struct plot_data *populate_plot_entries(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int idx, maxtime, nr, i;
	int lastdepth, lasttime;
	struct plot_data *plot_data;

	maxtime = get_maxtime(pi);
	if (dive->end > 0)
		maxtime = dive->end;

	/*
	 * We want to have a plot_info event at least every 10s (so "maxtime/10+1"),
	 * but samples could be more dense than that (so add in dc->samples), and
	 * additionally we want two surface events around the whole thing (thus the
	 * additional 4).
	 */
	nr = dc->samples + 5 + maxtime / 10;
	plot_data = calloc(nr, sizeof(struct plot_data));
	pi->entry = plot_data;
	if (!plot_data)
		return NULL;
	pi->nr = nr;
	idx = 2; /* the two extra events at the start */

	lastdepth = 0;
	lasttime = 0;
	for (i = 0; i < dc->samples; i++) {
		struct plot_data *entry = plot_data + idx;
		struct sample *sample = dc->sample+i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;
		int offset, delta;

		/* Add intermediate plot entries if required */
		delta = time - lasttime;
		if (delta < 0) {
			time = lasttime;
			delta = 0;
		}
		for (offset = 10; offset < delta; offset += 10) {
			if (lasttime + offset > maxtime)
				break;

			/* Use the data from the previous plot entry */
			*entry = entry[-1];

			/* .. but update depth and time, obviously */
			entry->sec = lasttime + offset;
			entry->depth = interpolate(lastdepth, depth, offset, delta);

			/* And clear out the sensor pressure, since we'll interpolate */
			SENSOR_PRESSURE(entry) = 0;

			idx++; entry++;
		}

		if (time > maxtime)
			break;

		entry->sec = time;
		entry->depth = depth;

		entry->stopdepth = sample->stopdepth.mm;
		entry->stoptime = sample->stoptime.seconds;
		entry->ndl = sample->ndl.seconds;
		pi->has_ndl |= sample->ndl.seconds;
		entry->in_deco = sample->in_deco;
		entry->cns = sample->cns;
		entry->po2 = sample->po2 / 1000.0;
		/* FIXME! sensor index -> cylinder index translation! */
		entry->cylinderindex = sample->sensor;
		SENSOR_PRESSURE(entry) = sample->cylinderpressure.mbar;
		entry->temperature = sample->temperature.mkelvin;

		lasttime = time;
		lastdepth = depth;
		idx++;
	}

	/* Add two final surface events */
	plot_data[idx++].sec = lasttime+10;
	plot_data[idx++].sec = lasttime+20;
	pi->nr = idx;

	return plot_data;
}

static void populate_cylinder_pressure_data(int idx, int start, int end, struct plot_info *pi)
{
	int i;

	/* First: check that none of the entries has sensor pressure for this cylinder index */
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry+i;
		if (entry->cylinderindex != idx)
			continue;
		if (SENSOR_PRESSURE(entry))
			return;
	}

	/* Then: populate the first entry with the beginning cylinder pressure */
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry+i;
		if (entry->cylinderindex != idx)
			continue;
		SENSOR_PRESSURE(entry) = start;
		break;
	}

	/* .. and the last entry with the ending cylinder pressure */
	for (i = pi->nr; --i >= 0; ) {
		struct plot_data *entry = pi->entry+i;
		if (entry->cylinderindex != idx)
			continue;
		SENSOR_PRESSURE(entry) = end;
		break;
	}
}

static void populate_secondary_sensor_data(struct divecomputer *dc, struct plot_info *pi)
{
	/* We should try to see if it has interesting pressure data here */
}

static void setup_gas_sensor_pressure(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i;
	struct divecomputer *secondary;

	/* First, populate the pressures with the manual cylinder data.. */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder+i;
		int start = cyl->start.mbar ? : cyl->sample_start.mbar;
		int end = cyl->end.mbar ? : cyl->sample_end.mbar;

		if (!start || !end)
			continue;

		populate_cylinder_pressure_data(i, start, end, pi);
	}

	/*
	 * Here, we should try to walk through all the dive computers,
	 * and try to see if they have sensor data different from the
	 * primary dive computer (dc).
	 */
	secondary = &dive->dc;
	do {
		if (secondary == dc)
			continue;
		populate_secondary_sensor_data(dc, pi);
	} while ((secondary = secondary->next) != NULL);
}

static void populate_pressure_information(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i, cylinderindex;
	pr_track_t *track_pr[MAX_CYLINDERS] = {NULL, };
	pr_track_t *current;
	gboolean missing_pr = FALSE;

	/* Set up the pressure tracking data structures */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		int mbar = cyl->start.mbar ? : cyl->sample_start.mbar;
		track_pr[i] = pr_track_alloc(mbar, 0);
	}

	cylinderindex = pi->entry[0].cylinderindex;
	current = track_pr[cylinderindex];
	for (i = 1; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;

		entry = pi->entry + i;

		entry->same_cylinder = entry->cylinderindex == cylinderindex;
		cylinderindex = entry->cylinderindex;

		/* discrete integration of pressure over time to get the SAC rate equivalent */
		current->pressure_time += pressure_time(dive, entry-1, entry);

		/* track the segments per cylinder and their pressure/time integral */
		if (!entry->same_cylinder) {
			current = pr_track_alloc(SENSOR_PRESSURE(entry), entry->sec);
			track_pr[cylinderindex] = list_add(track_pr[cylinderindex], current);
		} else { /* same cylinder */
			if (SENSOR_PRESSURE(entry) && !SENSOR_PRESSURE(entry-1)) {
				/* transmitter changed its working status */
				current->end = SENSOR_PRESSURE(entry);
				current->t_end = entry->sec;
				current = pr_track_alloc(SENSOR_PRESSURE(entry), entry->sec);
				track_pr[cylinderindex] =
					list_add(track_pr[cylinderindex], current);
			}
		}
		/* finally, do the discrete integration to get the SAC rate equivalent */
		if (SENSOR_PRESSURE(entry)) {
			current->end = SENSOR_PRESSURE(entry);
			current->t_end = entry->sec;
		}
		missing_pr |= !SENSOR_PRESSURE(entry);
	}

	/* initialize the end pressures */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		int pr = cyl->end.mbar ? : cyl->sample_end.mbar;
		if (pr && track_pr[i]) {
			pr_track_t *pr_track = list_last(track_pr[i]);
			pr_track->end = pr;
		}
	}

	if (missing_pr) {
		fill_missing_tank_pressures(dive, pi, track_pr);
	}
	for (i = 0; i < MAX_CYLINDERS; i++)
		list_free(track_pr[i]);
}

static void calculate_deco_information(struct dive *dive, struct divecomputer *dc, struct plot_info *pi)
{
	int i;
	double amb_pressure;
	double surface_pressure = (dive->dc.surface_pressure.mbar ? dive->dc.surface_pressure.mbar : SURFACE_PRESSURE) / 1000.0;

	for (i = 1; i < pi->nr; i++) {
		int fo2, fhe, j, t0, t1;
		double tissue_tolerance;
		struct plot_data *entry = pi->entry + i;
		int cylinderindex = entry->cylinderindex;

		amb_pressure = depth_to_mbar(entry->depth, dive) / 1000.0;
		fo2 = dive->cylinder[cylinderindex].gasmix.o2.permille ? : O2_IN_AIR;
		fhe = dive->cylinder[cylinderindex].gasmix.he.permille;
		double ratio = (double)fhe / (1000.0 - fo2);

		if (entry->po2) {
			/* we have an O2 partial pressure in the sample - so this
			 * is likely a CC dive... use that instead of the value
			 * from the cylinder info */
			double po2 = entry->po2 > amb_pressure ? amb_pressure : entry->po2;
			entry->po2 = po2;
			entry->phe = (amb_pressure - po2) * ratio;
			entry->pn2 = amb_pressure - po2 - entry->phe;
		} else {
			entry->po2 = fo2 / 1000.0 * amb_pressure;
			entry->phe = fhe / 1000.0 * amb_pressure;
			entry->pn2 = (1000 - fo2 - fhe) / 1000.0 * amb_pressure;
		}

                /* Calculate MOD, EAD, END and EADD based on partial pressures calculated before
                 * so there is no difference in calculating between OC and CC
                 * EAD takes O2 + N2 (air) into account
                 * END just uses N2 */
		entry->mod = (prefs.mod_ppO2 / fo2 * 1000 - 1) * 10000;
		entry->ead = (entry->depth + 10000) *
			(entry->po2 + (amb_pressure - entry->po2) * (1 - ratio)) / amb_pressure - 10000;
		entry->end = (entry->depth + 10000) *
			(amb_pressure - entry->po2) * (1 - ratio) / amb_pressure / N2_IN_AIR * 1000 - 10000;
		entry->eadd = (entry->depth + 10000) *
			(entry->po2 / amb_pressure * O2_DENSITY + entry->pn2 / amb_pressure *
                                N2_DENSITY + entry->phe / amb_pressure * HE_DENSITY) /
				(O2_IN_AIR * O2_DENSITY + N2_IN_AIR * N2_DENSITY) * 1000 -10000;
		if(entry->mod <0)
			entry->mod=0;
		if(entry->ead <0)
			entry->ead=0;
		if(entry->end <0)
			entry->end=0;
		if(entry->eadd <0)
			entry->eadd=0;

		if (entry->po2 > pi->maxpp && prefs.pp_graphs.po2)
			pi->maxpp = entry->po2;
		if (entry->phe > pi->maxpp && prefs.pp_graphs.phe)
			pi->maxpp = entry->phe;
		if (entry->pn2 > pi->maxpp && prefs.pp_graphs.pn2)
			pi->maxpp = entry->pn2;

		/* and now let's try to do some deco calculations */
		t0 = (entry - 1)->sec;
		t1 = entry->sec;
		tissue_tolerance = 0;
		for (j = t0+1; j <= t1; j++) {
			int depth = interpolate(entry[-1].depth, entry[0].depth, j - t0, t1 - t0);
			double min_pressure = add_segment(depth_to_mbar(depth, dive) / 1000.0,
							&dive->cylinder[cylinderindex].gasmix, 1, entry->po2, dive);
			tissue_tolerance = min_pressure;
		}
		if (t0 == t1)
			entry->ceiling = (entry - 1)->ceiling;
		else
			entry->ceiling = deco_allowed_depth(tissue_tolerance, surface_pressure, dive, !prefs.calc_ceiling_3m_incr);
	}

#if DECO_CALC_DEBUG & 1
	dump_tissues();
#endif
}

/*
 * Create a plot-info with smoothing and ranged min/max
 *
 * This also makes sure that we have extra empty events on both
 * sides, so that you can do end-points without having to worry
 * about it.
 */
static struct plot_info *create_plot_info(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc)
{
	struct plot_info *pi;

	/* The plot-info is embedded in the graphics context */
	pi = &gc->pi;

	/* reset deco information to start the calculation */
	init_decompression(dive);

	/* Create the new plot data */
	if (last_pi_entry)
		free((void *)last_pi_entry);
	last_pi_entry = populate_plot_entries(dive, dc, pi);

	/* Populate the gas index from the gas change events */
	check_gas_change_events(dive, dc, pi);

	/* Try to populate our gas pressure knowledge */
	setup_gas_sensor_pressure(dive, dc, pi);

	/* .. calculate missing pressure entries */
	populate_pressure_information(dive, dc, pi);

	/* Then, calculate partial pressures and deco information */
	calculate_deco_information(dive, dc, pi);
	pi->meandepth = dive->dc.meandepth.mm;

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

/* make sure you pass this the FIRST dc - it just walks the list */
static int nr_dcs(struct divecomputer *main)
{
	int i = 1;
	struct divecomputer *dc = main;

	while ((dc = dc->next) != NULL)
		i++;
	return i;
}

static struct divecomputer *select_dc(struct divecomputer *main)
{
	int i = dc_number;
	struct divecomputer *dc = main;

	while (i < 0)
		i += nr_dcs(main);
	do {
		if (--i < 0)
			return dc;
	} while ((dc = dc->next) != NULL);

	/* If we switched dives to one with fewer DC's, reset the dive computer counter */
	dc_number = 0;
	return main;
}

void plot(struct graphics_context *gc, struct dive *dive, scale_mode_t scale)
{
	struct plot_info *pi;
	struct divecomputer *dc = &dive->dc;
	cairo_rectangle_t *drawing_area = &gc->drawing_area;
	const char *nickname;

	plot_set_scale(scale);

	if (!dc->samples) {
		static struct sample fake[4];
		static struct divecomputer fakedc = {
			.sample = fake,
			.samples = 4
		};

		/* The dive has no samples, so create a few fake ones.  This assumes an
		ascent/descent rate of 9 m/min, which is just below the limit for FAST. */
		int duration = dive->dc.duration.seconds;
		int maxdepth = dive->dc.maxdepth.mm;
		int asc_desc_time = dive->dc.maxdepth.mm*60/9000;
		if (asc_desc_time * 2 >= duration)
			asc_desc_time = duration / 2;
		fake[1].time.seconds = asc_desc_time;
		fake[1].depth.mm = maxdepth;
		fake[2].time.seconds = duration - asc_desc_time;
		fake[2].depth.mm = maxdepth;
		fake[3].time.seconds = duration * 1.00;
		fakedc.events = dc->events;
		dc = &fakedc;
	}

	/*
	 * Set up limits that are independent of
	 * the dive computer
	 */
	calculate_max_limits(dive, dc, gc);

	/* shift the drawing area so we have a nice margin around it */
	cairo_translate(gc->cr, drawing_area->x, drawing_area->y);
	cairo_set_line_width_scaled(gc->cr, 1);
	cairo_set_line_cap(gc->cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(gc->cr, CAIRO_LINE_JOIN_ROUND);

	/*
	 * We don't use "cairo_translate()" because that doesn't
	 * scale line width etc. But the actual scaling we need
	 * do set up ourselves..
	 *
	 * Snif. What a pity.
	 */
	gc->maxx = (drawing_area->width - 2*drawing_area->x);
	gc->maxy = (drawing_area->height - 2*drawing_area->y);

	dc = select_dc(dc);

	/* This is per-dive-computer. Right now we just do the first one */
	pi = create_plot_info(dive, dc, gc);

	/* Depth profile */
	plot_depth_profile(gc, pi);
	plot_events(gc, pi, dc);

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

	/* Put the dive computer name in the lower left corner */
	nickname = get_dc_nickname(dc->model, dc->deviceid);
	if (!nickname || *nickname == '\0')
		nickname = dc->model;
	if (nickname) {
		static const text_render_options_t computer = {10, TIME_TEXT, LEFT, MIDDLE};
		plot_text(gc, &computer, 0, 1, "%s", nickname);
	}

	if (PP_GRAPHS_ENABLED) {
		plot_pp_gas_profile(gc, pi);
		plot_pp_text(gc, pi);
	}

	/* now shift the translation back by half the margin;
	 * this way we can draw the vertical scales on both sides */
	cairo_translate(gc->cr, -drawing_area->x / 2.0, 0);
	gc->maxx += drawing_area->x;
	gc->leftx = -(drawing_area->x / drawing_area->width) / 2.0;
	gc->rightx = 1.0 - gc->leftx;

	plot_depth_scale(gc, pi);

	if (gc->printer) {
		free(pi->entry);
		last_pi_entry = pi->entry = NULL;
		pi->nr = 0;
	}
}

static void plot_string(struct plot_data *entry, char *buf, size_t bufsize,
			int depth, int pressure, int temp, gboolean has_ndl)
{
	int pressurevalue, mod, ead, end, eadd;
	const char *depth_unit, *pressure_unit, *temp_unit;
	char *buf2 = malloc(bufsize);
	double depthvalue, tempvalue;

	depthvalue = get_depth_units(depth, NULL, &depth_unit);
	snprintf(buf, bufsize, "D:%.1f %s", depthvalue, depth_unit);
	if (pressure) {
		pressurevalue = get_pressure_units(pressure, &pressure_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nP:%d %s", buf2, pressurevalue, pressure_unit);
	}
	if (temp) {
		tempvalue = get_temp_units(temp, &temp_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nT:%.1f %s", buf2, tempvalue, temp_unit);
	}
	if (entry->ceiling) {
		depthvalue = get_depth_units(entry->ceiling, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nCalculated ceiling %.0f %s", buf2, depthvalue, depth_unit);
	}
	if (entry->stopdepth) {
		depthvalue = get_depth_units(entry->stopdepth, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		if (entry->ndl) {
			/* this is a safety stop as we still have ndl */
			if (entry->stoptime)
				snprintf(buf, bufsize, "%s\nSafetystop:%umin @ %.0f %s", buf2, entry->stoptime / 60,
					depthvalue, depth_unit);
			else
				snprintf(buf, bufsize, "%s\nSafetystop:unkn time @ %.0f %s", buf2,
					depthvalue, depth_unit);
		} else {
			/* actual deco stop */
			if (entry->stoptime)
				snprintf(buf, bufsize, "%s\nDeco:%umin @ %.0f %s", buf2, entry->stoptime / 60,
					depthvalue, depth_unit);
			else
				snprintf(buf, bufsize, "%s\nDeco:unkn time @ %.0f %s", buf2,
					depthvalue, depth_unit);
		}
	} else if (entry->in_deco) {
		/* this means we had in_deco set but don't have a stop depth */
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nIn deco", buf2);
	} else if (has_ndl) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nNDL:%umin", buf2, entry->ndl / 60);
	}
	if (entry->cns) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nCNS:%u%%", buf2, entry->cns);
	}
	if (prefs.pp_graphs.po2) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\npO" UTF8_SUBSCRIPT_2 ":%.2fbar", buf2, entry->po2);
	}
	if (prefs.pp_graphs.pn2) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\npN" UTF8_SUBSCRIPT_2 ":%.2fbar", buf2, entry->pn2);
	}
	if (prefs.pp_graphs.phe) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\npHe:%.2fbar", buf2, entry->phe);
	}
	if (prefs.mod) {
		mod = (int)get_depth_units(entry->mod, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nMOD:%d%s", buf2, mod, depth_unit);
	}
	if (prefs.ead) {
		ead = (int)get_depth_units(entry->ead, NULL, &depth_unit);
		end = (int)get_depth_units(entry->end, NULL, &depth_unit);
		eadd = (int)get_depth_units(entry->eadd, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, "%s\nEAD:%d%s\nEND:%d%s\nEADD:%d%s", buf2, ead, depth_unit, end, depth_unit, eadd, depth_unit);
	}
	free(buf2);
}

void get_plot_details(struct graphics_context *gc, int time, char *buf, size_t bufsize)
{
	struct plot_info *pi = &gc->pi;
	int pressure = 0, temp = 0;
	struct plot_data *entry = NULL;
	int i;

	for (i = 0; i < pi->nr; i++) {
		entry = pi->entry + i;
		if (entry->temperature)
			temp = entry->temperature;
		if (GET_PRESSURE(entry))
			pressure = GET_PRESSURE(entry);
		if (entry->sec >= time)
			break;
	}
	if (entry)
		plot_string(entry, buf, bufsize, entry->depth, pressure, temp, pi->has_ndl);
}
