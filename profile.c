#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"

int selected_dive = 0;

#define ROUND_UP(x,y) ((((x)+(y)-1)/(y))*(y))

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such.
 */
static int round_seconds_up(int seconds)
{
	return MAX(30*60, ROUND_UP(seconds, 60*10));
}

static int round_feet_up(int feet)
{
	return MAX(90, ROUND_UP(feet+5, 15));
}

typedef struct {
    double r,g,b;
    enum {CENTER,LEFT} allign;
} text_render_options_t;

static void plot_text(cairo_t *cr, text_render_options_t *tro,
		      double x, double y, const char *fmt, ...)
{
	cairo_text_extents_t extents;
	char buffer[80];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	cairo_text_extents(cr, buffer, &extents);

	if (tro->allign == CENTER)
	    x -= extents.width/2 + extents.x_bearing;
	y += extents.height * 1.2;

	cairo_move_to(cr, x, y);
	cairo_text_path(cr, buffer);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);

	cairo_move_to(cr, x, y);
	cairo_set_source_rgb(cr, tro->r, tro->g, tro->b);
	cairo_show_text(cr, buffer);
}

/*
 * Find the next maximum point in a 10-minute window.
 *
 * We exit early if we hit "enough" of a depth reversal,
 * which is roughly 10 feet.
 */
static int next_minmax(struct dive *dive, int index, int minmax)
{
	const int enough = 3000;
	int timelimit, depthlimit, result;
	struct sample *sample = dive->sample + index;

	if (index >= dive->samples)
		return 0;

	timelimit = 24*60*60;
	depthlimit = sample->depth.mm;
	result = 0;

	for (;;) {
		int time, depth;

		index++;
		sample++;
		if (index >= dive->samples)
			break;
		time = sample->time.seconds;
		depth = sample->depth.mm;
		if (time > timelimit)
			break;

		if (minmax) {
			if (depth <= depthlimit) {
				if (depthlimit - depth > enough)
					break;
				continue;
			}
		} else {
			if (depth >= depthlimit) {
				if (depth - depthlimit > enough)
					break;
				continue;
			}
		}

		result = index;
		depthlimit = depth;
		/* Look up to ten minutes into the future */
		timelimit = time + 600;
	}
	return result;
}

/* Scale to 0,0 -> maxx,maxy */
#define SCALE(x,y) (x)*maxx/scalex+topx,(y)*maxy/scaley+topy

static void plot_depth_text(struct dive *dive, cairo_t *cr,
	double topx, double topy, double maxx, double maxy)
{
	double scalex, scaley;
	int maxtime, maxdepth;
	int i;

	/* Get plot scaling limits */
	maxtime = round_seconds_up(dive->duration.seconds);
	maxdepth = round_feet_up(to_feet(dive->maxdepth));

	scalex = maxtime;
	scaley = maxdepth;

	cairo_set_font_size(cr, 14);
	cairo_set_source_rgb(cr, 1, 0.2, 0.2);
	i = 0;
	while ((i = next_minmax(dive, i, 1)) != 0) {
		text_render_options_t tro = {1.0, 0.2, 0.2, CENTER};
		struct sample *sample = dive->sample+i;
		int sec = sample->time.seconds;
		int depth = to_feet(sample->depth);

		plot_text(cr, &tro, SCALE(sec, depth), "%d ft", depth);
		i = next_minmax(dive, i, 0);
		if (!i)
			break;
	}
}

static void plot_depth_profile(struct dive *dive, cairo_t *cr,
	double topx, double topy, double maxx, double maxy)
{
	double scalex, scaley;
	int begins, sec, depth;
	int i, samples;
	struct sample *sample;
	int maxtime, maxdepth;

	samples = dive->samples;
	if (!samples)
		return;

	cairo_set_line_width(cr, 2);

	/* Get plot scaling limits */
	maxtime = round_seconds_up(dive->duration.seconds);
	maxdepth = round_feet_up(to_feet(dive->maxdepth));

	/* Time markers: every 5 min */
	scalex = maxtime;
	scaley = 1.0;
	for (i = 5*60; i < maxtime; i += 5*60) {
		cairo_move_to(cr, SCALE(i, 0));
		cairo_line_to(cr, SCALE(i, 1));
	}

	/* Depth markers: every 15 ft */
	scalex = 1.0;
	scaley = maxdepth;
	cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
	for (i = 15; i < maxdepth; i += 15) {
		cairo_move_to(cr, SCALE(0, i));
		cairo_line_to(cr, SCALE(1, i));
	}
	cairo_stroke(cr);

	/* Show mean depth */
	cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.40);
	cairo_move_to(cr, SCALE(0, to_feet(dive->meandepth)));
	cairo_line_to(cr, SCALE(1, to_feet(dive->meandepth)));
	cairo_stroke(cr);

	scalex = maxtime;

	sample = dive->sample;
	cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.80);
	begins = sample->time.seconds;
	cairo_move_to(cr, SCALE(sample->time.seconds, to_feet(sample->depth)));
	for (i = 1; i < dive->samples; i++) {
		sample++;
		sec = sample->time.seconds;
		depth = to_feet(sample->depth);
		cairo_line_to(cr, SCALE(sec, depth));
	}
	scaley = 1.0;
	cairo_line_to(cr, SCALE(sec, 0));
	cairo_line_to(cr, SCALE(begins, 0));
	cairo_close_path(cr);
	cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.20);
	cairo_fill_preserve(cr);
	cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.80);
	cairo_stroke(cr);
}

/* gets both the actual start and end pressure as well as the scaling factors */
static int get_cylinder_pressure_range(struct dive *dive, double *scalex, double *scaley,
				       double *startp, double *endp)
{
	int i;
	double min, max;
	double bar;

	*scalex = round_seconds_up(dive->duration.seconds);

	max = 0;
	min = 5000;
	if (startp)
	    *startp = *endp = 0.0;

	for (i = 0; i < dive->samples; i++) {
		struct sample *sample = dive->sample + i;

		/* FIXME! We only track cylinder 0 right now */
		if (sample->cylinderindex)
			continue;
		if (!sample->cylinderpressure.mbar)
			continue;
		bar = sample->cylinderpressure.mbar;
		if (bar != 0.0 && startp && *startp == 0.0)
		    *startp = bar;
		if (bar < min)
			min = bar;
		if (bar > max)
			max = bar;
	}
	if (endp)
	    *endp = bar;
	if (!max)
		return 0;
	*scaley = max * 1.5;
	return 1;
}

static void plot_cylinder_pressure(struct dive *dive, cairo_t *cr,
	double topx, double topy, double maxx, double maxy)
{
	int i, sec = -1;
	double scalex, scaley;

	if (!get_cylinder_pressure_range(dive, &scalex, &scaley, NULL, NULL))
		return;

	cairo_set_source_rgba(cr, 0.2, 1.0, 0.2, 0.80);

	cairo_move_to(cr, SCALE(0, dive->cylinder[0].start.mbar));
	for (i = 1; i < dive->samples; i++) {
		int mbar;
		struct sample *sample = dive->sample + i;

		mbar = sample->cylinderpressure.mbar;
		if (!mbar)
			continue;
		sec = sample->time.seconds;
		cairo_line_to(cr, SCALE(sec, mbar));
	}
	/*
	 * We may have "surface time" events, in which case we don't go
	 * back to dive duration
	 */
	if (sec < dive->duration.seconds)
		cairo_line_to(cr, SCALE(dive->duration.seconds, dive->cylinder[0].end.mbar));
	cairo_stroke(cr);
}


static void plot_cylinder_pressure_text(struct dive *dive, cairo_t *cr,
	double topx, double topy, double maxx, double maxy)
{
	double scalex, scaley;
	double startp,endp;

	cairo_set_font_size(cr, 10);

	if (get_cylinder_pressure_range(dive, &scalex, &scaley,
					&startp, &endp)) {
		text_render_options_t tro = {0.2, 1.0, 0.2, LEFT};
		plot_text(cr, &tro, SCALE(0, startp), "%3.0f bar", startp/1000.0);
		plot_text(cr, &tro, SCALE(dive->duration.seconds, endp),
			  "%3.0f bar", endp/1000.0);
	}
}

static void plot(cairo_t *cr, int w, int h, struct dive *dive)
{
	double topx, topy, maxx, maxy;
	double scalex, scaley;

	topx = w / 20.0;
	topy = h / 20.0;
	maxx = (w - 2*topx);
	maxy = (h - 2*topy);

	/* Cylinder pressure plot */
	plot_cylinder_pressure(dive, cr, topx, topy, maxx, maxy);

	/* Depth profile */
	plot_depth_profile(dive, cr, topx, topy, maxx, maxy);

	/* Text on top of all graphs.. */
	plot_depth_text(dive, cr, topx, topy, maxx, maxy);
	plot_cylinder_pressure_text(dive, cr, topx, topy, maxx, maxy);

	/* Bounding box last */
	scalex = scaley = 1.0;
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_move_to(cr, SCALE(0,0));
	cairo_line_to(cr, SCALE(0,1));
	cairo_line_to(cr, SCALE(1,1));
	cairo_line_to(cr, SCALE(1,0));
	cairo_close_path(cr);
	cairo_stroke(cr);

}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = current_dive;
	cairo_t *cr;
	int w,h;

	w = widget->allocation.width;
	h = widget->allocation.height;

	cr = gdk_cairo_create(widget->window);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_paint(cr);

	if (dive)
		plot(cr, w, h, dive);

	cairo_destroy(cr);

	return FALSE;
}

GtkWidget *dive_profile_widget(void)
{
	GtkWidget *da;

	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 450, 350);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);

	return da;
}
