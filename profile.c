#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"

int selected_dive = 0;

/* Plot info with smoothing and one-, two- and three-minute minimums and maximums */
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
		struct plot_data *min[3];
		struct plot_data *max[3];
		int avg[3];
	} entry[];
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
	if (gc->printer) {
		/* Black is white and white is black */
		double sum = r+g+b;
		if (sum > 2)
			r = g = b = 0;
		else if (sum < 1)
			r = g = b = 1;
	}
	cairo_set_source_rgba(gc->cr, r, g, b, a);
}

static void set_source_rgb(struct graphics_context *gc, double r, double g, double b)
{
	set_source_rgba(gc, r, g, b, 1);
}

#define ROUND_UP(x,y) ((((x)+(y)-1)/(y))*(y))

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such.
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

static void render_depth_sample(struct graphics_context *gc, struct plot_data *entry, const text_render_options_t *tro)
{
	int sec = entry->sec;
	depth_t depth = { entry->val };
	const char *fmt;
	double d;

	switch (output_units.length) {
	case METERS:
		d = depth.mm / 1000.0;
		fmt = "%.1f";
		break;
	case FEET:
		d = to_feet(depth);
		fmt = "%.0f";
		break;
	}
	plot_text(gc, tro, sec, depth.mm, fmt, d);
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

	cairo_set_source_rgba(gc->cr, 1, 0.2, 0.2, 0.20);
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

	cairo_set_source_rgba(gc->cr, 1, 0.2, 1, a);
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
	int begins, sec, depth;
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

	plot_smoothed_profile(gc, pi);
	plot_minmax_profile(gc, pi);

	entry = pi->entry;
	set_source_rgba(gc, 1, 0.2, 0.2, 0.80);
	begins = entry->sec;
	move_to(gc, entry->sec, entry->val);
	for (i = 1; i < pi->nr; i++) {
		entry++;
		sec = entry->sec;
		if (sec <= maxtime) {
			depth = entry->val;
			line_to(gc, sec, depth);
		}
	}
	gc->topy = 0; gc->bottomy = 1.0;
	line_to(gc, MIN(sec,maxtime), 0);
	line_to(gc, begins, 0);
	cairo_close_path(cr);
	set_source_rgba(gc, 1, 0.2, 0.2, 0.20);
	cairo_fill_preserve(cr);
	set_source_rgba(gc, 1, 0.2, 0.2, 0.80);
	cairo_stroke(cr);
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
		unit = "F";
	} else {
		deg = to_C(temperature);
		unit = "C";
	}
	plot_text(gc, &tro, sec, temperature.mkelvin, "%d %s", deg, unit);
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

	gc->topy = 0; gc->bottomy = pi->maxpressure * 1.5;
	return pi->maxpressure != 0;
}

static void plot_cylinder_pressure(struct graphics_context *gc, struct plot_info *pi)
{
	int i;

	if (!get_cylinder_pressure_range(gc, pi))
		return;

	cairo_set_source_rgba(gc->cr, 0.2, 1.0, 0.2, 0.80);

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

/*
 * Return air usage (in liters).
 */
static double calculate_airuse(struct dive *dive)
{
	double airuse = 0;
	int i;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		int size = cyl->type.size.mliter;
		double kilo_atm;

		if (!size)
			continue;

		kilo_atm = (cyl->start.mbar - cyl->end.mbar) / 1013250.0;

		/* Liters of air at 1 atm == milliliters at 1k atm*/
		airuse += kilo_atm * size;
	}
	return airuse;
}

static void plot_info(struct dive *dive, struct graphics_context *gc)
{
	text_render_options_t tro = {10, 0.2, 1.0, 0.2, RIGHT, BOTTOM};
	const double liters_per_cuft = 28.317;
	const char *unit, *format, *desc;
	double airuse;
	char buffer1[80];
	char buffer2[80];
	int len;

	airuse = calculate_airuse(dive);
	if (!airuse) {
		update_air_info("");
		return;
	}
	switch (output_units.volume) {
	case LITER:
		unit = "l";
		format = "vol: %4.0f %s";
		break;
	case CUFT:
		unit = "cuft";
		format = "vol: %4.2f %s";
		airuse /= liters_per_cuft;
		break;
	}
	tro.vpos = -1.0;
	plot_text(gc, &tro, 0.98, 0.98, format, airuse, unit);
	len = snprintf(buffer1, sizeof(buffer1), format, airuse, unit);
	tro.vpos = -2.2;
	if (dive->duration.seconds) {
		double pressure = 1 + (dive->meandepth.mm / 10000.0);
		double sac = airuse / pressure * 60 / dive->duration.seconds;
		plot_text(gc, &tro, 0.98, 0.98, "SAC: %4.2f %s/min", sac, unit);
		snprintf(buffer1+len, sizeof(buffer1)-len, 
				"\nSAC: %4.2f %s/min", sac, unit);
	}
	len = 0;
	tro.vpos = -3.4;
	desc = dive->cylinder[0].type.description;
	if (desc || dive->cylinder[0].gasmix.o2.permille) {
		int o2 = dive->cylinder[0].gasmix.o2.permille / 10;
		if (!desc)
			desc = "";
		if (!o2)
			o2 = 21;
		plot_text(gc, &tro, 0.98, 0.98, "%s (%d%%)", desc, o2);
		len = snprintf(buffer2, sizeof(buffer2), "%s (%d%%): used ", desc, o2);
	}
	snprintf(buffer2+len, sizeof(buffer2)-len, buffer1); 
	update_air_info(buffer2);
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
	for (i = 2; i < nr-2; i++) {
		struct plot_data *entry = pi->entry+i;
		int val;

		val = entry[-2].val + 2*entry[-1].val + 3*entry[0].val + 2*entry[1].val + entry[2].val;
		entry->smoothed = (val+4) / 9;
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

	/* Text on top of all graphs.. */
	plot_temperature_text(gc, pi);
	plot_depth_text(gc, pi);
	plot_cylinder_pressure_text(gc, pi);

	/* And info box in the lower right corner.. */
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = 1.0;
	plot_info(dive, gc);

	/* Bounding box last */
	set_source_rgb(gc, 1, 1, 1);
	move_to(gc, 0, 0);
	line_to(gc, 0, 1);
	line_to(gc, 1, 1);
	line_to(gc, 1, 0);
	cairo_close_path(gc->cr);
	cairo_stroke(gc->cr);

}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = current_dive;
	struct graphics_context gc = { .printer = 0 };
	int w,h;

	w = widget->allocation.width;
	h = widget->allocation.height;

	gc.cr = gdk_cairo_create(widget->window);
	set_source_rgb(&gc, 0, 0, 0);
	cairo_paint(gc.cr);

	if (dive)
		plot(&gc, w, h, dive);

	cairo_destroy(gc.cr);

	return FALSE;
}

GtkWidget *dive_profile_widget(void)
{
	GtkWidget *da;

	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 350, 250);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);

	return da;
}
