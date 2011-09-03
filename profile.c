#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

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

/* Scale to 0,0 -> maxx,maxy */
#define SCALE(x,y) (x)*maxx/scalex+topx,(y)*maxy/scaley+topy

static void plot_profile(struct dive *dive, cairo_t *cr,
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

	/* Depth markers: every 15 ft */
	scalex = 1.0;
	scaley = maxdepth;
	cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
	for (i = 15; i < maxdepth; i += 15) {
		cairo_move_to(cr, SCALE(0, i));
		cairo_line_to(cr, SCALE(1, i));
	}

	/* Time markers: every 5 min */
	scalex = maxtime;
	scaley = 1.0;
	for (i = 5*60; i < maxtime; i += 5*60) {
		cairo_move_to(cr, SCALE(i, 0));
		cairo_line_to(cr, SCALE(i, 1));
	}
	cairo_stroke(cr);

	scaley = maxdepth;

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

static int get_tank_pressure_range(struct dive *dive, double *scalex, double *scaley)
{
	int i;
	double min, max;

	*scalex = round_seconds_up(dive->duration.seconds);

	max = 0;
	min = 5000;
	for (i = 0; i < dive->samples; i++) {
		struct sample *sample = dive->sample + i;
		double bar;

		if (!sample->tankpressure.mbar)
			continue;
		bar = sample->tankpressure.mbar;
		if (bar < min)
			min = bar;
		if (bar > max)
			max = bar;
	}
	if (!max)
		return 0;
	*scaley = max * 1.5;
	return 1;
}

static void plot_tank_pressure(struct dive *dive, cairo_t *cr,
	double topx, double topy, double maxx, double maxy)
{
	int i;
	double scalex, scaley;

	if (!get_tank_pressure_range(dive, &scalex, &scaley))
		return;

	cairo_set_source_rgba(cr, 0.2, 1.0, 0.2, 0.80);

	cairo_move_to(cr, SCALE(0, dive->beginning_pressure.mbar));
	for (i = 1; i < dive->samples; i++) {
		int sec, mbar;
		struct sample *sample = dive->sample + i;

		sec = sample->time.seconds;
		mbar = sample->tankpressure.mbar;
		if (!mbar)
			continue;
		cairo_line_to(cr, SCALE(sec, mbar));
	}
	cairo_line_to(cr, SCALE(dive->duration.seconds, dive->end_pressure.mbar));
	cairo_stroke(cr);
}

static void plot(cairo_t *cr, int w, int h, struct dive *dive)
{
	double topx, topy, maxx, maxy;
	double scalex, scaley;

	topx = w / 20.0;
	topy = h / 20.0;
	maxx = (w - 2*topx);
	maxy = (h - 2*topy);

	/* Depth profile */
	plot_profile(dive, cr, topx, topy, maxx, maxy);

	/* Tank pressure plot? */
	plot_tank_pressure(dive, cr, topx, topy, maxx, maxy);

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

GtkWidget *dive_profile_frame(void)
{
	GtkWidget *frame;
	GtkWidget *da;

	frame = gtk_frame_new("Dive profile");
	gtk_widget_show(frame);
	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 450, 350);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);
	gtk_container_add(GTK_CONTAINER(frame), da);

	return frame;
}
