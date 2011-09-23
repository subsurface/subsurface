/* profile.c */
/* creates all the necessary data for drawing the dive profile 
 * uses cairo to draw it
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"

int selected_dive = 0;

typedef enum { STABLE, SLOW, MODERATE, FAST, CRAZY } velocity_t;
/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr;
	int maxtime;
	int meandepth, maxdepth;
	int minpressure, maxpressure;
	int mintemp, maxtemp;
	struct plot_data {
		int sec;
		int pressure, temperature;
		/* Depth info */
		int val;
		int smoothed;
		velocity_t velocity;
		struct plot_data *min[3];
		struct plot_data *max[3];
		int avg[3];
	} entry[];
};

/* convert velocity to colors */
typedef struct { double r, g, b; } rgb_t;
static const rgb_t rgb[] = {
	[STABLE]   = {0.0, 0.4, 0.0},
	[SLOW]     = {0.4, 0.8, 0.0},
	[MODERATE] = {0.8, 0.8, 0.0},
	[FAST]     = {0.8, 0.5, 0.0},
	[CRAZY]    = {1.0, 0.0, 0.0},
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

static void set_source_rgba(struct graphics_context *gc, double r, double g, double b, double a)
{
	/*
	 * For printers, we still honor 'a', but ignore colors
	 * for now. Black is white and white is black
	 */
	if (gc->printer) {
		double sum = r+g+b;
		if (sum > 0.8)
			r = g = b = 0;
		else
			r = g = b = 1;
	}
	cairo_set_source_rgba(gc->cr, r, g, b, a);
}

void set_source_rgb(struct graphics_context *gc, double r, double g, double b)
{
	set_source_rgba(gc, r, g, b, 1);
}

#define ROUND_UP(x,y) ((((x)+(y)-1)/(y))*(y))

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such.
 * we also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
static int get_maxtime(struct plot_info *pi)
{
	int seconds = pi->maxtime;
	/* min 30 minutes, rounded up to 5 minutes, with at least 2.5 minutes to spare */
	return MAX(30*60, ROUND_UP(seconds+150, 60*5));
}

static int get_maxdepth(struct plot_info *pi)
{
	unsigned mm = pi->maxdepth;
	/* Minimum 30m, rounded up to 10m, with at least 3m to spare */
	return MAX(30000, ROUND_UP(mm+3000, 10000));
}

typedef struct {
	int size;
	double r,g,b;
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

	cairo_set_font_size(cr, tro->size);
	cairo_font_extents(cr, &fe);
	cairo_text_extents(cr, buffer, &extents);
	dx = tro->hpos * extents.width + extents.x_bearing;
	dy = tro->vpos * extents.height + fe.descent;

	move_to(gc, x, y);
	cairo_rel_move_to(cr, dx, dy);

	cairo_text_path(cr, buffer);
	set_source_rgb(gc, 0, 0, 0);
	cairo_stroke(cr);

	move_to(gc, x, y);
	cairo_rel_move_to(cr, dx, dy);

	set_source_rgb(gc, tro->r, tro->g, tro->b);
	cairo_show_text(cr, buffer);
}

static void plot_one_event(struct graphics_context *gc, struct plot_info *pi, struct event *event, const text_render_options_t *tro)
{
	int i, depth = 0;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *data = pi->entry + i;
		if (event->time.seconds < data->sec)
			break;
		depth = data->val;
	}
	plot_text(gc, tro, event->time.seconds, depth, "%s", event->name);
}

static void plot_events(struct graphics_context *gc, struct plot_info *pi, struct dive *dive)
{
	static const text_render_options_t tro = {14, 1.0, 0.2, 0.2, CENTER, TOP};
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

	d = get_depth_units(entry->val, &decimals, NULL);

	plot_text(gc, tro, sec, entry->val, "%.*f", decimals, d);
}

static void plot_text_samples(struct graphics_context *gc, struct plot_info *pi)
{
	static const text_render_options_t deep = {14, 1.0, 0.2, 0.2, CENTER, TOP};
	static const text_render_options_t shallow = {14, 1.0, 0.2, 0.2, CENTER, BOTTOM};
	int i;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;

		if (entry->val < 2000)
			continue;

		if (entry == entry->max[2])
			render_depth_sample(gc, entry, &deep);

		if (entry == entry->min[2])
			render_depth_sample(gc, entry, &shallow);
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

	set_source_rgba(gc, 1, 0.2, 0.2, 0.20);
	move_to(gc, entry->sec, entry->smoothed);
	for (i = 1; i < pi->nr; i++) {
		entry++;
		line_to(gc, entry->sec, entry->smoothed);
	}
	cairo_stroke(gc->cr);
}

static void plot_minmax_profile_minute(struct graphics_context *gc, struct plot_info *pi,
				int index, double a)
{
	int i;
	struct plot_data *entry = pi->entry;

	set_source_rgba(gc, 1, 0.2, 1, a);
	move_to(gc, entry->sec, entry->min[index]->val);
	for (i = 1; i < pi->nr; i++) {
		entry++;
		line_to(gc, entry->sec, entry->min[index]->val);
	}
	for (i = 1; i < pi->nr; i++) {
		line_to(gc, entry->sec, entry->max[index]->val);
		entry--;
	}
	cairo_close_path(gc->cr);
	cairo_fill(gc->cr);
}

static void plot_minmax_profile(struct graphics_context *gc, struct plot_info *pi)
{
	if (gc->printer)
		return;
	plot_minmax_profile_minute(gc, pi, 2, 0.1);
	plot_minmax_profile_minute(gc, pi, 1, 0.1);
	plot_minmax_profile_minute(gc, pi, 0, 0.1);
}

static void plot_depth_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	cairo_t *cr = gc->cr;
	int sec, depth;
	struct plot_data *entry;
	int maxtime, maxdepth, marker;

	/* Get plot scaling limits */
	maxtime = get_maxtime(pi);
	maxdepth = get_maxdepth(pi);

	/* Time markers: every 5 min */
	gc->leftx = 0; gc->rightx = maxtime;
	gc->topy = 0; gc->bottomy = 1.0;
	for (i = 5*60; i < maxtime; i += 5*60) {
		move_to(gc, i, 0);
		line_to(gc, i, 1);
	}

	/* Depth markers: every 30 ft or 10 m*/
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = maxdepth;
	switch (output_units.length) {
	case METERS: marker = 10000; break;
	case FEET: marker = 9144; break;	/* 30 ft */
	}

	set_source_rgba(gc, 1, 1, 1, 0.5);
	for (i = marker; i < maxdepth; i += marker) {
		move_to(gc, 0, i);
		line_to(gc, 1, i);
	}
	cairo_stroke(cr);

	/* Show mean depth */
	set_source_rgba(gc, 1, 0.2, 0.2, 0.40);
	move_to(gc, 0, pi->meandepth);
	line_to(gc, 1, pi->meandepth);
	cairo_stroke(cr);

	gc->leftx = 0; gc->rightx = maxtime;

	/*
	 * These are good for debugging text placement etc,
	 * but not for actual display..
	 */
	if (0) {
		plot_smoothed_profile(gc, pi);
		plot_minmax_profile(gc, pi);
	}

	set_source_rgba(gc, 1, 0.2, 0.2, 0.80);

	/* Do the depth profile for the neat fill */
	gc->topy = 0; gc->bottomy = maxdepth;
	set_source_rgba(gc, 1, 0.2, 0.2, 0.20);

	entry = pi->entry;
	move_to(gc, 0, 0);
	for (i = 0; i < pi->nr; i++, entry++)
		line_to(gc, entry->sec, entry->val);
	cairo_close_path(gc->cr);
	if (gc->printer) {
		set_source_rgba(gc, 1, 1, 1, 0.2);
		cairo_fill_preserve(cr);
		set_source_rgb(gc, 1, 1, 1);
		cairo_stroke(cr);
		return;
	}
	cairo_fill(gc->cr);

	/* Now do it again for the velocity colors */
	entry = pi->entry;
	for (i = 1; i < pi->nr; i++) {
		entry++;
		sec = entry->sec;
		/* we want to draw the segments in different colors
		 * representing the vertical velocity, so we need to
		 * chop this into short segments */
		rgb_t color = rgb[entry->velocity];
		depth = entry->val;
		set_source_rgb(gc, color.r, color.g, color.b);
		move_to(gc, entry[-1].sec, entry[-1].val);
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
	int deg;
	const char *unit;
	static const text_render_options_t tro = {12, 0.2, 0.2, 1.0, LEFT, TOP};
	temperature_t temperature = { mkelvin };

	if (output_units.temperature == FAHRENHEIT) {
		deg = to_F(temperature);
		unit = UTF8_DEGREE "F";
	} else {
		deg = to_C(temperature);
		unit = UTF8_DEGREE "C";
	}
	plot_text(gc, &tro, sec, temperature.mkelvin, "%d%s", deg, unit);
}

static void plot_temperature_text(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	int last = 0, sec = 0;
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
		if (sec < last + 300)
			continue;
		last = sec;
		plot_single_temp_text(gc,sec,mkelvin);
		last_printed_temp = mkelvin;
	}
	/* it would be nice to print the end temperature, if it's different */
	if (abs(last_temperature - last_printed_temp) > 500)
		plot_single_temp_text(gc, sec, last_temperature);
}

static void plot_temperature_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i;
	cairo_t *cr = gc->cr;
	int last = 0;

	if (!setup_temperature_limits(gc, pi))
		return;

	set_source_rgba(gc, 0.2, 0.2, 1.0, 0.8);
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

static void plot_cylinder_pressure(struct graphics_context *gc, struct plot_info *pi)
{
	int i;

	if (!get_cylinder_pressure_range(gc, pi))
		return;

	set_source_rgba(gc, 0.2, 1.0, 0.2, 0.80);

	move_to(gc, 0, pi->maxpressure);
	for (i = 1; i < pi->nr; i++) {
		int mbar;
		struct plot_data *entry = pi->entry + i;

		mbar = entry->pressure;
		if (!mbar)
			continue;
		line_to(gc, entry->sec, mbar);
	}
	line_to(gc, pi->maxtime, pi->minpressure);
	cairo_stroke(gc->cr);
}

static int mbar_to_PSI(int mbar)
{
	pressure_t p = {mbar};
	return to_PSI(p);
}

static void plot_cylinder_pressure_text(struct graphics_context *gc, struct plot_info *pi)
{
	if (get_cylinder_pressure_range(gc, pi)) {
		int start, end;
		const char *unit = "bar";

		switch (output_units.pressure) {
		case PASCAL:
			start = pi->maxpressure * 100;
			end = pi->minpressure * 100;
			unit = "pascal";
			break;
		case BAR:
			start = (pi->maxpressure + 500) / 1000;
			end = (pi->minpressure + 500) / 1000;
			unit = "bar";
			break;
		case PSI:
			start = mbar_to_PSI(pi->maxpressure);
			end = mbar_to_PSI(pi->minpressure);
			unit = "psi";
			break;
		}

		text_render_options_t tro = {10, 0.2, 1.0, 0.2, LEFT, TOP};
		plot_text(gc, &tro, 0, pi->maxpressure, "%d %s", start, unit);
		plot_text(gc, &tro, pi->maxtime, pi->minpressure,
			  "%d %s", end, unit);
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
	avg = p->val;
	nr = 1;
	while (++p < last) {
		int val = p->val;
		if (p->sec > time + seconds)
			break;
		avg += val;
		nr ++;
		if (val < min->val)
			min = p;
		if (val > max->val)
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
		int pressure = entry->pressure;
		int temperature = entry->temperature;

		if (pressure) {
			if (!pi->minpressure || pressure < pi->minpressure)
				pi->minpressure = pressure;
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
	for (i = 2; i < nr-1; i++) {
		struct plot_data *entry = pi->entry+i;
		int val;

		if (i < nr-2) {
			val = entry[-2].val + 2*entry[-1].val + 3*entry[0].val + 2*entry[1].val + entry[2].val;
			entry->smoothed = (val+4) / 9;
		}
		/* vertical velocity in mm/sec */
		/* Linus wants to smooth this - let's at least look at the samples that aren't FAST or CRAZY */
		if (entry[0].sec - entry[-1].sec) {
			entry->velocity = velocity((entry[0].val - entry[-1].val) / (entry[0].sec - entry[-1].sec));
                        /* if our samples are short and we aren't too FAST*/
			if (entry[0].sec - entry[-1].sec < 30 && entry->velocity < FAST) { 
				int past = -2;
				while (i+past > 0 && entry[0].sec - entry[past].sec < 30)
					past--;
				entry->velocity = velocity((entry[0].val - entry[past].val) / 
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
 * Create a plot-info with smoothing and ranged min/max
 *
 * This also makes sure that we have extra empty events on both
 * sides, so that you can do end-points without having to worry
 * about it.
 */
static struct plot_info *create_plot_info(struct dive *dive)
{
	int lastdepth, lastindex;
	int i, nr = dive->samples + 4, sec;
	size_t alloc_size = plot_info_size(nr);
	struct plot_info *pi;

	pi = malloc(alloc_size);
	if (!pi)
		return pi;
	memset(pi, 0, alloc_size);
	pi->nr = nr;
	sec = 0;
	lastindex = 0;
	lastdepth = -1;
	for (i = 0; i < dive->samples; i++) {
		int depth;
		struct sample *sample = dive->sample+i;
		struct plot_data *entry = pi->entry + i + 2;

		sec = entry->sec = sample->time.seconds;
		depth = entry->val = sample->depth.mm;
		entry->pressure = sample->cylinderpressure.mbar;
		entry->temperature = sample->temperature.mkelvin;

		if (depth || lastdepth)
			lastindex = i+2;

		lastdepth = depth;
		if (depth > pi->maxdepth)
			pi->maxdepth = depth;
	}
	if (lastdepth)
		lastindex = i + 2;
	/* Fill in the last two entries with empty values but valid times */
	i = dive->samples + 2;
	pi->entry[i].sec = sec + 20;
	pi->entry[i+1].sec = sec + 40;

	pi->nr = lastindex+1;
	pi->maxtime = pi->entry[lastindex].sec;

	pi->minpressure = dive->cylinder[0].end.mbar;
	pi->maxpressure = dive->cylinder[0].start.mbar;

	pi->meandepth = dive->meandepth.mm;

	return analyze_plot_info(pi);
}

void plot(struct graphics_context *gc, int w, int h, struct dive *dive)
{
	double topx, topy;
	struct plot_info *pi = create_plot_info(dive);

	topx = w / 20.0;
	topy = h / 20.0;
	cairo_translate(gc->cr, topx, topy);
	cairo_set_line_width(gc->cr, 2);
	cairo_set_line_cap(gc->cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(gc->cr, CAIRO_LINE_JOIN_ROUND);

	/*
	 * We can use "cairo_translate()" because that doesn't
	 * scale line width etc. But the actual scaling we need
	 * do set up ourselves..
	 *
	 * Snif. What a pity.
	 */
	gc->maxx = (w - 2*topx);
	gc->maxy = (h - 2*topy);

	/* Temperature profile */
	plot_temperature_profile(gc, pi);

	/* Cylinder pressure plot */
	plot_cylinder_pressure(gc, pi);

	/* Depth profile */
	plot_depth_profile(gc, pi);
	plot_events(gc, pi, dive);

	/* Text on top of all graphs.. */
	plot_temperature_text(gc, pi);
	plot_depth_text(gc, pi);
	plot_cylinder_pressure_text(gc, pi);

	/* Bounding box last */
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = 1.0;

	set_source_rgb(gc, 1, 1, 1);
	move_to(gc, 0, 0);
	line_to(gc, 0, 1);
	line_to(gc, 1, 1);
	line_to(gc, 1, 0);
	cairo_close_path(gc->cr);
	cairo_stroke(gc->cr);

	free(pi);
}
