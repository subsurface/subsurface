/* profile.c */
/* creates all the necessary data for drawing the dive profile
 * uses cairo to draw it
 */
#include <glib/gi18n.h>

#include "dive.h"
#include "display.h"
#if USE_GTK_UI
#include "display-gtk.h"
#endif
#include "divelist.h"

#include "profile.h"
#include "libdivecomputer/parser.h"
#include "libdivecomputer/version.h"

int selected_dive = 0;
char zoomed_plot = 0;
char dc_number = 0;


static struct plot_data *last_pi_entry = NULL;

#define cairo_set_line_width_scaled(cr, w) \
	cairo_set_line_width((cr), (w) * plot_scale);

#if USE_GTK_UI

/* keep the last used gc around so we can invert the SCALEX calculation in
 * order to calculate a time value for an x coordinate */
static struct graphics_context last_gc;
int x_to_time(double x)
{
	int seconds = (x - last_gc.drawing_area.x) / last_gc.maxx * (last_gc.rightx - last_gc.leftx) + last_gc.leftx;
	return (seconds > 0) ? seconds : 0;
}

/* x offset into the drawing area */
int x_abs(double x)
{
	return x - last_gc.drawing_area.x;
}

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

static void pattern_add_color_stop_rgba(struct graphics_context *gc, cairo_pattern_t *pat, double o, color_indice_t c)
{
	const color_t *col = &profile_color[c];
	struct rgba rgb = col->media[gc->printer];
	cairo_pattern_add_color_stop_rgba(pat, o, rgb.r, rgb.g, rgb.b, rgb.a);
}
#endif /* USE_GTK_UI */

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
		printf("    entry[%d]:{cylinderindex:%d sec:%d pressure:{%d,%d}\n"
			"                time:%d:%02d temperature:%d depth:%d stopdepth:%d stoptime:%d ndl:%d smoothed:%d po2:%lf phe:%lf pn2:%lf sum-pp %lf}\n",
			i, entry->cylinderindex, entry->sec,
			entry->pressure[0], entry->pressure[1],
			entry->sec / 60, entry->sec % 60,
			entry->temperature, entry->depth, entry->stopdepth, entry->stoptime, entry->ndl, entry->smoothed,
			entry->po2, entry->phe, entry->pn2,
			entry->po2 + entry->phe + entry->pn2);
	}
	printf("   }\n");
}

#define ROUND_UP(x,y) ((((x)+(y)-1)/(y))*(y))

/*
 * When showing dive profiles, we scale things to the
 * current dive. However, we don't scale past less than
 * 30 minutes or 90 ft, just so that small dives show
 * up as such unless zoom is enabled.
 * We also need to add 180 seconds at the end so the min/max
 * plots correctly
 */
int get_maxtime(struct plot_info *pi)
{
	int seconds = pi->maxtime;
	if (zoomed_plot) {
		/* Rounded up to one minute, with at least 2.5 minutes to
		 * spare.
		 * For dive times shorter than 10 minutes, we use seconds/4 to
		 * calculate the space dynamically.
		 * This is seamless since 600/4 = 150.
		 */
		if (seconds < 600)
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
int get_maxdepth(struct plot_info *pi)
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

/* collect all event names and whether we display them */
struct ev_select *ev_namelist;
int evn_allocated;
int evn_used;

int evn_foreach(void (*callback)(const char *, int *, void *), void *data)
{
	int i;

	for (i = 0; i < evn_used; i++) {
		/* here we display an event name on screen - so translate */
		callback(_(ev_namelist[i].ev_name), &ev_namelist[i].plot_ev, data);
	}
	return i;
}

void clear_events(void)
{
	evn_used = 0;
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

int setup_temperature_limits(struct graphics_context *gc, struct plot_info *pi)
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
	return maxtemp && maxtemp >= mintemp;
}

#if 0
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
	static const text_render_options_t tro = {DEPTH_TEXT_SIZE, SAMPLE_DEEP, RIGHT, MIDDLE};

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
	static const text_render_options_t tro = {PP_TEXT_SIZE, PP_LINES, LEFT, MIDDLE};

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




static void plot_single_temp_text(struct graphics_context *gc, int sec, int mkelvin)
{
	double deg;
	const char *unit;
	static const text_render_options_t tro = {TEMP_TEXT_SIZE, TEMP_TEXT, LEFT, TOP};

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
		sec = entry->sec;

		if (!mkelvin)
			continue;
		last_temperature = mkelvin;
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

/* gets both the actual start and end pressure as well as the scaling factors */

#endif /* USE_GTK_UI */

int get_cylinder_pressure_range(struct graphics_context *gc)
{
	gc->leftx = 0;
	gc->rightx = get_maxtime(&gc->pi);

	if (PP_GRAPHS_ENABLED)
		gc->bottomy = -gc->pi.maxpressure * 0.75;
	else
		gc->bottomy = 0;
	gc->topy = gc->pi.maxpressure * 1.5;
	if (!gc->pi.maxpressure)
		return FALSE;

	while (gc->pi.endtempcoord <= SCALEY(gc, gc->pi.minpressure - (gc->topy) * 0.1))
		gc->bottomy -=  gc->topy * 0.1;

	return TRUE;
}


/* Get local sac-rate (in ml/min) between entry1 and entry2 */
int get_local_sac(struct plot_data *entry1, struct plot_data *entry2, struct dive *dive)
{
	int index = entry1->cylinderindex;
	cylinder_t *cyl;
	int duration = entry2->sec - entry1->sec;
	int depth, airuse;
	pressure_t a, b;
	double atm;

	if (entry2->cylinderindex != index)
		return 0;
	if (duration <= 0)
		return 0;
	a.mbar = GET_PRESSURE(entry1);
	b.mbar = GET_PRESSURE(entry2);
	if (!a.mbar || !b.mbar)
		return 0;

	/* Mean pressure in ATM */
	depth = (entry1->depth + entry2->depth) / 2;
	atm = (double) depth_to_mbar(depth, dive) / SURFACE_PRESSURE;

	cyl = dive->cylinder + index;

	airuse = gas_volume(cyl, a) - gas_volume(cyl, b);

	/* milliliters per minute */
	return airuse / atm * 60 / duration;
}

#if USE_GTK_UI


static void plot_pressure_value(struct graphics_context *gc, int mbar, int sec,
				int xalign, int yalign)
{
	int pressure;
	const char *unit;

	pressure = get_pressure_units(mbar, &unit);
	text_render_options_t tro = {PRESSURE_TEXT_SIZE, PRESSURE_TEXT, xalign, yalign};
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

	cyl = -1;
	for (i = 0; i < pi->nr; i++) {
		entry = pi->entry + i;
		mbar = GET_PRESSURE(entry);

		if (!mbar)
			continue;
		if (cyl != entry->cylinderindex) {
			cyl = entry->cylinderindex;
			if (!seen_cyl[cyl]) {
				plot_pressure_value(gc, mbar, entry->sec, LEFT, BOTTOM);
				seen_cyl[cyl] = TRUE;
			}
		}
		last_pressure[cyl] = mbar;
		last_time[cyl] = entry->sec;
	}

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		if (last_time[cyl]) {
			plot_pressure_value(gc, last_pressure[cyl], last_time[cyl], CENTER, TOP);
		}
	}
}

static void plot_deco_text(struct graphics_context *gc, struct plot_info *pi)
{
	if (prefs.profile_calc_ceiling) {
		float x = gc->leftx + (gc->rightx - gc->leftx) / 2;
		float y = gc->topy = 1.0;
		text_render_options_t tro = {PRESSURE_TEXT_SIZE, PRESSURE_TEXT, CENTER, -0.2};
		gc->bottomy = 0.0;
		plot_text(gc, &tro, x, y, "GF %.0f/%.0f", prefs.gflow * 100, prefs.gfhigh * 100);
	}
}
#endif /* USE_GTK_UI */

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
		} else {
			entry->velocity = STABLE;
		}
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
	pt->end = 0;
	pt->t_start = pt->t_end = t_start;
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
static inline int pressure_time(struct dive *dive, struct divecomputer *dc, struct plot_data *a, struct plot_data *b)
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
		if (!track_pr[cyl])
			continue;
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
		cur_pt = pressure_time(dive, &dive->dc, entry-1, entry);
		pressure = cur_pr[cyl] + cur_pt * magic + 0.5;
		INTERPOLATED_PRESSURE(entry) = pressure;
		cur_pr[cyl] = pressure;
	}
}

static int get_cylinder_index(struct dive *dive, struct event *ev)
{
	int i;
	int best = 0, score = INT_MAX;
	int target_o2, target_he;

	/*
	 * Crazy gas change events give us odd encoded o2/he in percent.
	 * Decode into our internal permille format.
	 */
	target_o2 = (ev->value & 0xFFFF) * 10;
	target_he = (ev->value >> 16) * 10;

	/*
	 * Try to find a cylinder that best matches the target gas
	 * mix.
	 */
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder+i;
		int delta_o2, delta_he, distance;

		if (cylinder_nodata(cyl))
			continue;

		delta_o2 = get_o2(&cyl->gasmix) - target_o2;
		delta_he = get_he(&cyl->gasmix) - target_he;
		distance = delta_o2 * delta_o2 + delta_he * delta_he;
		if (distance >= score)
			continue;
		score = distance;
		best = i;
	}
	return best;
}

struct event *get_next_event(struct event *event, char *name)
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

void calculate_max_limits(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc)
{
	struct plot_info *pi;
	int maxdepth;
	int maxtime = 0;
	int maxpressure = 0, minpressure = INT_MAX;
	int mintemp, maxtemp;
	int cyl;

	/* The plot-info is embedded in the graphics context */
	pi = &gc->pi;
	memset(pi, 0, sizeof(*pi));

	maxdepth = dive->maxdepth.mm;
	mintemp = dive->mintemp.mkelvin;
	maxtemp = dive->maxtemp.mkelvin;

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
			if ((depth > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD) &&
			    s->time.seconds > maxtime)
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

	maxtime = pi->maxtime;

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
	for (i = pi->nr; --i >= 0; /* nothing */) {
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

	cylinderindex = -1;
	current = NULL;
	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;
		unsigned pressure = SENSOR_PRESSURE(entry);

		/* discrete integration of pressure over time to get the SAC rate equivalent */
		if (current) {
			current->pressure_time += pressure_time(dive, dc, entry-1, entry);
			current->t_end = entry->sec;
		}

		/* track the segments per cylinder and their pressure/time integral */
		if (entry->cylinderindex != cylinderindex) {
			cylinderindex = entry->cylinderindex;
			current = pr_track_alloc(pressure, entry->sec);
			track_pr[cylinderindex] = list_add(track_pr[cylinderindex], current);
			continue;
		}

		if (!pressure) {
			missing_pr = 1;
			continue;
		}

		current->end = pressure;

		/* Was it continuous? */
		if (SENSOR_PRESSURE(entry-1))
			continue;

		/* transmitter changed its working status */
		current = pr_track_alloc(pressure, entry->sec);
		track_pr[cylinderindex] = list_add(track_pr[cylinderindex], current);
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
	double surface_pressure = (dc->surface_pressure.mbar ? dc->surface_pressure.mbar : get_surface_pressure_in_mbar(dive, TRUE)) / 1000.0;

	for (i = 1; i < pi->nr; i++) {
		int fo2, fhe, j, t0, t1;
		double tissue_tolerance;
		struct plot_data *entry = pi->entry + i;
		int cylinderindex = entry->cylinderindex;

		amb_pressure = depth_to_mbar(entry->depth, dive) / 1000.0;
		fo2 = get_o2(&dive->cylinder[cylinderindex].gasmix);
		fhe = get_he(&dive->cylinder[cylinderindex].gasmix);
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
		if (entry->mod < 0)
			entry->mod = 0;
		if (entry->ead < 0)
			entry->ead = 0;
		if (entry->end < 0)
			entry->end = 0;
		if (entry->eadd < 0)
			entry->eadd = 0;

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
							&dive->cylinder[cylinderindex].gasmix, 1, entry->po2 * 1000, dive);
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
struct plot_info *create_plot_info(struct dive *dive, struct divecomputer *dc, struct graphics_context *gc)
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

#if USE_GTK_UI
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
#endif

/* make sure you pass this the FIRST dc - it just walks the list */
static int nr_dcs(struct divecomputer *main)
{
	int i = 1;
	struct divecomputer *dc = main;

	while ((dc = dc->next) != NULL)
		i++;
	return i;
}

struct divecomputer *select_dc(struct divecomputer *main)
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

static void plot_string(struct plot_data *entry, char *buf, int bufsize,
			int depth, int pressure, int temp, gboolean has_ndl)
{
	int pressurevalue, mod, ead, end, eadd;
	const char *depth_unit, *pressure_unit, *temp_unit;
	char *buf2 = malloc(bufsize);
	double depthvalue, tempvalue;

	depthvalue = get_depth_units(depth, NULL, &depth_unit);
	snprintf(buf, bufsize, _("D:%.1f %s"), depthvalue, depth_unit);
	if (pressure) {
		pressurevalue = get_pressure_units(pressure, &pressure_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nP:%d %s"), buf2, pressurevalue, pressure_unit);
	}
	if (temp) {
		tempvalue = get_temp_units(temp, &temp_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nT:%.1f %s"), buf2, tempvalue, temp_unit);
	}
	if (entry->ceiling) {
		depthvalue = get_depth_units(entry->ceiling, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nCalculated ceiling %.0f %s"), buf2, depthvalue, depth_unit);
	}
	if (entry->stopdepth) {
		depthvalue = get_depth_units(entry->stopdepth, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		if (entry->ndl) {
			/* this is a safety stop as we still have ndl */
			if (entry->stoptime)
				snprintf(buf, bufsize, _("%s\nSafetystop:%umin @ %.0f %s"), buf2, entry->stoptime / 60,
					depthvalue, depth_unit);
			else
				snprintf(buf, bufsize, _("%s\nSafetystop:unkn time @ %.0f %s"), buf2,
					depthvalue, depth_unit);
		} else {
			/* actual deco stop */
			if (entry->stoptime)
				snprintf(buf, bufsize, _("%s\nDeco:%umin @ %.0f %s"), buf2, entry->stoptime / 60,
					depthvalue, depth_unit);
			else
				snprintf(buf, bufsize, _("%s\nDeco:unkn time @ %.0f %s"), buf2,
					depthvalue, depth_unit);
		}
	} else if (entry->in_deco) {
		/* this means we had in_deco set but don't have a stop depth */
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nIn deco"), buf2);
	} else if (has_ndl) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nNDL:%umin"), buf2, entry->ndl / 60);
	}
	if (entry->cns) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nCNS:%u%%"), buf2, entry->cns);
	}
	if (prefs.pp_graphs.po2) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\npO%s:%.2fbar"), buf2, UTF8_SUBSCRIPT_2, entry->po2);
	}
	if (prefs.pp_graphs.pn2) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\npN%s:%.2fbar"), buf2, UTF8_SUBSCRIPT_2, entry->pn2);
	}
	if (prefs.pp_graphs.phe) {
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\npHe:%.2fbar"), buf2, entry->phe);
	}
	if (prefs.mod) {
		mod = (int)get_depth_units(entry->mod, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nMOD:%d%s"), buf2, mod, depth_unit);
	}
	if (prefs.ead) {
		ead = (int)get_depth_units(entry->ead, NULL, &depth_unit);
		end = (int)get_depth_units(entry->end, NULL, &depth_unit);
		eadd = (int)get_depth_units(entry->eadd, NULL, &depth_unit);
		memcpy(buf2, buf, bufsize);
		snprintf(buf, bufsize, _("%s\nEAD:%d%s\nEND:%d%s\nEADD:%d%s"), buf2, ead, depth_unit, end, depth_unit, eadd, depth_unit);
	}
	free(buf2);
}

void get_plot_details(struct graphics_context *gc, int time, char *buf, int bufsize)
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
