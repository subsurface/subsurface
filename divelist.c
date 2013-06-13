/* divelist.c */
/* core logic for the dive list -
 * accessed through the following interfaces:
 *
 * dive_trip_t *dive_trip_list;
 * unsigned int amount_selected;
 * void dump_selection(void)
 * dive_trip_t *find_trip_by_idx(int idx)
 * int trip_has_selected_dives(dive_trip_t *trip)
 * void get_dive_gas(struct dive *dive, int *o2_p, int *he_p, int *o2low_p)
 * int total_weight(struct dive *dive)
 * int get_divenr(struct dive *dive)
 * double init_decompression(struct dive *dive)
 * void update_cylinder_related_info(struct dive *dive)
 * void get_location(struct dive *dive, char **str)
 * void get_cylinder(struct dive *dive, char **str)
 * void get_suit(struct dive *dive, char **str)
 * void dump_trip_list(void)
 * dive_trip_t *find_matching_trip(timestamp_t when)
 * void insert_trip(dive_trip_t **dive_trip_p)
 * void remove_dive_from_trip(struct dive *dive)
 * void add_dive_to_trip(struct dive *dive, dive_trip_t *trip)
 * dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive)
 * void autogroup_dives(void)
 * void clear_trip_indexes(void)
 * void delete_single_dive(int idx)
 * void add_single_dive(int idx, struct dive *dive)
 * void merge_dive_index(int i, struct dive *a)
 * void select_dive(int idx)
 * void deselect_dive(int idx)
 * void mark_divelist_changed(int changed)
 * int unsaved_changes()
 * void remove_autogen_trips()
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glib/gi18n.h>
#include <assert.h>
#include <zip.h>
#include <libxslt/transform.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "webservice.h"

static short dive_list_changed = FALSE;

short autogroup = FALSE;

dive_trip_t *dive_trip_list;

unsigned int amount_selected;

#if DEBUG_SELECTION_TRACKING
void dump_selection(void)
{
	int i;
	struct dive *dive;

	printf("currently selected are %u dives:", amount_selected);
	for_each_dive(i, dive) {
		if (dive->selected)
			printf(" %d", i);
	}
	printf("\n");
}
#endif

void set_autogroup(gboolean value)
{
	/* if we keep the UI paradigm, this needs to toggle
	 * the checkbox on the autogroup menu item */
	autogroup = value;
}

dive_trip_t *find_trip_by_idx(int idx)
{
	dive_trip_t *trip = dive_trip_list;

	if (idx >= 0)
		return NULL;
	idx = -idx;
	while (trip) {
		if (trip->index == idx)
			return trip;
		trip = trip->next;
	}
	return NULL;
}

int dive_nr_sort(int idx_a, int idx_b, timestamp_t when_a, timestamp_t when_b)
{
	struct dive *a, *b;
	dive_trip_t *tripa = NULL, *tripb = NULL;

	if (idx_a < 0) {
		a = NULL;
		tripa = find_trip_by_idx(idx_a);
	} else {
		a = get_dive(idx_a);
		if (a)
			tripa = a->divetrip;
	}

	if (idx_b < 0) {
		b = NULL;
		tripb = find_trip_by_idx(idx_b);
	} else {
		b = get_dive(idx_b);
		if (b)
			tripb = b->divetrip;
	}

	/*
	 * Compare dive dates within the same trip (or when there
	 * are no trips involved at all). But if we have two
	 * different trips use the trip dates for comparison
	 */
	if (tripa != tripb) {
		if (tripa)
			when_a = tripa->when;
		if (tripb)
			when_b = tripb->when;
	}
	return when_a - when_b;
}

int trip_has_selected_dives(dive_trip_t *trip)
{
	struct dive *dive;
	for (dive = trip->dives; dive; dive = dive->next) {
		if (dive->selected)
			return 1;
	}
	return 0;
}

/* Get the values as we want to show them. Whole feet. But meters with one decimal for
 * values less than 20m, without decimals for larger values */
void get_depth_values(int depth, int *depth_int, int *depth_decimal, int *show_decimal)
{
	int integer, frac;

	*show_decimal = 1;
	switch (prefs.units.length) {
	case METERS:
		/* To tenths of meters */
		depth = (depth + 49) / 100;
		integer = depth / 10;
		frac = depth % 10;
		if (integer < 20)
			break;
		if (frac >= 5)
			integer++;
		*show_decimal = 0;
		break;
	case FEET:
		integer = mm_to_feet(depth) + 0.5;
		*show_decimal = 0;
		break;
	default:
		/* can't happen */
		return;
	}
	*depth_int = integer;
	if (*show_decimal)
		*depth_decimal = frac;
}

/*
 * Get "maximal" dive gas for a dive.
 * Rules:
 *  - Trimix trumps nitrox (highest He wins, O2 breaks ties)
 *  - Nitrox trumps air (even if hypoxic)
 * These are the same rules as the inter-dive sorting rules.
 */
void get_dive_gas(struct dive *dive, int *o2_p, int *he_p, int *o2low_p)
{
	int i;
	int maxo2 = -1, maxhe = -1, mino2 = 1000;


	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		struct gasmix *mix = &cyl->gasmix;
		int o2 = mix->o2.permille;
		int he = mix->he.permille;
		struct divecomputer *dc = &dive->dc;
		int used = 0;
		int first_gas_explicit = 0;

		while (dc) {
			struct event *event = dc->events;
			while (event) {
				if (event->value) {
					if (event->name && !strcmp(event->name, "gaschange")) {
						unsigned int event_he = event->value >> 16;
						unsigned int event_o2 = event->value & 0xffff;

						if (event->time.seconds < 30)
							first_gas_explicit = 1;
						if (is_air(o2, he)) {
							if (is_air(event_o2 * 10, event_he * 10))
								used = 1;
						}
						else {
							if (he == event_he*10 && o2 == event_o2*10)
								used = 1;
						}
					}
				}
				event = event->next;
			}
			dc = dc->next;
		}

		/* Unless explicity set, the first gas to use has index 0 */
		if (i == 0 && !first_gas_explicit)
			used = 1;

		if (!used)
			continue;

		if (cylinder_none(cyl))
			continue;
		if (!o2)
			o2 = O2_IN_AIR;
		if (o2 < mino2)
			mino2 = o2;
		if (he > maxhe)
			goto newmax;
		if (he < maxhe)
			continue;
		if (o2 <= maxo2)
			continue;
newmax:
		maxhe = he;
		maxo2 = o2;
	}
	/* All air? Show/sort as "air"/zero */
	if (!maxhe && maxo2 == O2_IN_AIR && mino2 == maxo2)
		maxo2 = mino2 = 0;
	*o2_p = maxo2;
	*he_p = maxhe;
	*o2low_p = mino2;
}

int total_weight(struct dive *dive)
{
	int i, total_grams = 0;

	if (dive)
		for (i=0; i< MAX_WEIGHTSYSTEMS; i++)
			total_grams += dive->weightsystem[i].weight.grams;
	return total_grams;
}

static int active_o2(struct dive *dive, struct divecomputer *dc, duration_t time)
{
	int o2permille = dive->cylinder[0].gasmix.o2.permille;
	struct event *event = dc->events;

	if (!o2permille)
		o2permille = O2_IN_AIR;

	for (event = dc->events; event; event = event->next) {
		if (event->time.seconds > time.seconds)
			break;
		if (strcmp(event->name, "gaschange"))
			continue;
		o2permille = 10*(event->value & 0xffff);
	}
	return o2permille;
}

/* calculate OTU for a dive - this only takes the first divecomputer into account */
static int calculate_otu(struct dive *dive)
{
	int i;
	double otu = 0.0;
	struct divecomputer *dc = &dive->dc;

	for (i = 1; i < dc->samples; i++) {
		int t;
		int po2;
		struct sample *sample = dc->sample + i;
		struct sample *psample = sample - 1;
		t = sample->time.seconds - psample->time.seconds;
		if (sample->po2) {
			po2 = sample->po2;
		} else {
			int o2 = active_o2(dive, dc, sample->time);
			po2 = o2 / 1000.0 * depth_to_mbar(sample->depth.mm, dive);
		}
		if (po2 >= 500)
			otu += pow((po2 - 500) / 1000.0, 0.83) * t / 30.0;
	}
	return otu + 0.5;
}
/* calculate CNS for a dive - this only takes the first divecomputer into account */
int const cns_table[][3] = {
/* po2, Maximum Single Exposure, Maximum 24 hour Exposure */
	{1600,  45 * 60, 150 * 60},
	{1500, 120 * 60, 180 * 60},
	{1400, 150 * 60, 180 * 60},
	{1300, 180 * 60, 210 * 60},
	{1200, 210 * 60, 240 * 60},
	{1100, 240 * 60, 270 * 60},
	{1000, 300 * 60, 300 * 60},
	{ 900, 360 * 60, 360 * 60},
	{ 800, 450 * 60, 450 * 60},
	{ 700, 570 * 60, 570 * 60},
	{ 600, 720 * 60, 720 * 60}
};

/* this only gets called if dive->maxcns == 0 which means we know that
 * none of the divecomputers has tracked any CNS for us
 * so we calculated it "by hand" */
static int calculate_cns(struct dive *dive)
{
	int i, j, divenr;
	double cns = 0.0;
	struct divecomputer *dc = &dive->dc;
	struct dive *prev_dive;
	timestamp_t endtime;

	/* shortcut */
	if (dive->cns)
		return dive->cns;
	/*
	 * Do we start with a cns loading from a privious dive?
	 * Check if we did a dive 12 hours prior, and what cns we had from that.
	 * Then apply ha 90min halftime to see whats left.
	 */
	divenr = get_divenr(dive);
	if (divenr) {
		prev_dive = get_dive(divenr -1 );
		endtime = prev_dive->when + prev_dive->duration.seconds;
		if (prev_dive && dive->when < (endtime + 3600 * 12)) {
			cns = calculate_cns(prev_dive);
			cns = cns * 1/pow(2, (dive->when - endtime) / (90.0 * 60.0));
		}
	}
	/* Caclulate the cns for each sample in this dive and sum them */
	for (i = 1; i < dc->samples; i++) {
		int t;
		int po2;
		struct sample *sample = dc->sample + i;
		struct sample *psample = sample - 1;
		t = sample->time.seconds - psample->time.seconds;
		if (sample->po2) {
			po2 = sample->po2;
		} else {
			int o2 = active_o2(dive, dc, sample->time);
			po2 = o2 / 1000.0 * depth_to_mbar(sample->depth.mm, dive);
		}
		/* Find what table-row we should calculate % for */
		for (j = 1; j < sizeof(cns_table)/(sizeof(int) * 3); j++)
			if (po2 > cns_table[j][0])
				break;
		j--;
		cns += ((double)t)/((double)cns_table[j][1]) * 100;
	}
	/* save calculated cns in dive struct */
	dive->cns = cns;
	return dive->cns;
}
/*
 * Return air usage (in liters).
 */
static double calculate_airuse(struct dive *dive)
{
	int airuse = 0;
	int i;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		pressure_t start, end;
		cylinder_t *cyl = dive->cylinder + i;

		start = cyl->start.mbar ? cyl->start : cyl->sample_start;
		end = cyl->end.mbar ? cyl->end : cyl->sample_end;

		airuse += gas_volume(cyl, start) - gas_volume(cyl, end);
	}
	return airuse / 1000.0;
}

/* this only uses the first divecomputer to calculate the SAC rate */
static int calculate_sac(struct dive *dive)
{
	struct divecomputer *dc = &dive->dc;
	double airuse, pressure, sac;
	int duration, meandepth;

	airuse = calculate_airuse(dive);
	if (!airuse)
		return 0;

	duration = dc->duration.seconds;
	if (!duration)
		return 0;

	meandepth = dc->meandepth.mm;
	if (!meandepth)
		return 0;

	/* Mean pressure in ATM (SAC calculations are in atm*l/min) */
	pressure = (double) depth_to_mbar(meandepth, dive) / SURFACE_PRESSURE;
	sac = airuse / pressure * 60 / duration;

	/* milliliters per minute.. */
	return sac * 1000;
}

/* for now we do this based on the first divecomputer */
static void add_dive_to_deco(struct dive *dive)
{
	struct divecomputer *dc = &dive->dc;
	int i;

	if (!dc)
		return;
	for (i = 1; i < dc->samples; i++) {
		struct sample *psample = dc->sample + i - 1;
		struct sample *sample = dc->sample + i;
		int t0 = psample->time.seconds;
		int t1 = sample->time.seconds;
		int j;

		for (j = t0; j < t1; j++) {
			int depth = interpolate(psample->depth.mm, sample->depth.mm, j - t0, t1 - t0);
			(void) add_segment(depth_to_mbar(depth, dive) / 1000.0,
					   &dive->cylinder[sample->sensor].gasmix, 1, sample->po2, dive);
		}
	}
}

int get_divenr(struct dive *dive)
{
	int divenr = -1;
	while (++divenr < dive_table.nr && get_dive(divenr) != dive)
		;
	return divenr;
}

static struct gasmix air = { .o2.permille = O2_IN_AIR };

/* take into account previous dives until there is a 48h gap between dives */
double init_decompression(struct dive *dive)
{
	int i, divenr = -1;
	unsigned int surface_time;
	timestamp_t when, lasttime = 0;
	gboolean deco_init = FALSE;
	double tissue_tolerance, surface_pressure;

	if (!dive)
		return 0.0;
	divenr = get_divenr(dive);
	when = dive->when;
	i = divenr;
	while (i && --i) {
		struct dive* pdive = get_dive(i);
		/* we don't want to mix dives from different trips as we keep looking
		 * for how far back we need to go */
		if (dive->divetrip && pdive->divetrip != dive->divetrip)
			continue;
		if (!pdive || pdive->when > when || pdive->when + pdive->duration.seconds + 48 * 60 * 60 < when)
			break;
		when = pdive->when;
		lasttime = when + pdive->duration.seconds;
	}
	while (++i < divenr) {
		struct dive* pdive = get_dive(i);
		/* again skip dives from different trips */
		if (dive->divetrip && dive->divetrip != pdive->divetrip)
			continue;
		surface_pressure = get_surface_pressure_in_mbar(pdive, TRUE) / 1000.0;
		if (!deco_init) {
			clear_deco(surface_pressure);
			deco_init = TRUE;
#if DECO_CALC_DEBUG & 2
			dump_tissues();
#endif
		}
		add_dive_to_deco(pdive);
#if DECO_CALC_DEBUG & 2
		printf("added dive #%d\n", pdive->number);
		dump_tissues();
#endif
		if (pdive->when > lasttime) {
			surface_time = pdive->when - lasttime;
			lasttime = pdive->when + pdive->duration.seconds;
			tissue_tolerance = add_segment(surface_pressure, &air, surface_time, 0, dive);
#if DECO_CALC_DEBUG & 2
			printf("after surface intervall of %d:%02u\n", FRACTION(surface_time,60));
			dump_tissues();
#endif
		}
	}
	/* add the final surface time */
	if (lasttime && dive->when > lasttime) {
		surface_time = dive->when - lasttime;
		surface_pressure = get_surface_pressure_in_mbar(dive, TRUE) / 1000.0;
		tissue_tolerance = add_segment(surface_pressure, &air, surface_time, 0, dive);
#if DECO_CALC_DEBUG & 2
		printf("after surface intervall of %d:%02u\n", FRACTION(surface_time,60));
		dump_tissues();
#endif
	}
	if (!deco_init) {
		double surface_pressure = get_surface_pressure_in_mbar(dive, TRUE) / 1000.0;
		clear_deco(surface_pressure);
#if DECO_CALC_DEBUG & 2
		printf("no previous dive\n");
		dump_tissues();
#endif
	}
	return tissue_tolerance;
}

void update_cylinder_related_info(struct dive *dive)
{
	if (dive != NULL) {
		dive->sac = calculate_sac(dive);
		dive->otu = calculate_otu(dive);
		if (dive->maxcns == 0)
			dive->maxcns = calculate_cns(dive);
	}
}

static void get_string(char **str, const char *s)
{
	int len;
	char *n;

	if (!s)
		s = "";
	len = g_utf8_strlen(s, -1);
	if (len > 60)
		len = 60;
	n = malloc(len * sizeof(gunichar) + 1);
	g_utf8_strncpy(n, s, len);
	*str = n;
}

void get_location(struct dive *dive, char **str)
{
	get_string(str, dive->location);
}

void get_cylinder(struct dive *dive, char **str)
{
	get_string(str, dive->cylinder[0].type.description);
}

void get_suit(struct dive *dive, char **str)
{
	get_string(str, dive->suit);
}

#define MAX_DATE_STRING 256
/* caller needs to free the string */
char *get_dive_date_string(timestamp_t when)
{
	char *buffer = malloc(MAX_DATE_STRING);
	if (buffer) {
		struct tm tm;
		utc_mkdate(when, &tm);
		snprintf(buffer, MAX_DATE_STRING,
			/*++GETTEXT 60 char buffer weekday, monthname, day of month, year, hour:min */
			_("%1$s, %2$s %3$d, %4$d %5$02d:%6$02d"),
			weekday(tm.tm_wday),
			monthname(tm.tm_mon),
			tm.tm_mday, tm.tm_year + 1900,
			tm.tm_hour, tm.tm_min);
	}
	return buffer;
}

char *get_short_dive_date_string(timestamp_t when)
{
	char *buffer = malloc(MAX_DATE_STRING);
	if (buffer) {
		struct tm tm;
		utc_mkdate(when, &tm);
		snprintf(buffer, MAX_DATE_STRING,
			/*++GETTEXT 40 char buffer monthname, day of month, year, hour:min */
			_("%1$s %2$d, %3$d\n%4$02d:%5$02d"),
			monthname(tm.tm_mon),
			tm.tm_mday, tm.tm_year +1900,
			tm.tm_hour, tm.tm_min);
	}
	return buffer;
}

/* caller needs to free the string */
char *get_trip_date_string(timestamp_t when, int nr)
{
	char *buffer = malloc(MAX_DATE_STRING);
	if (buffer) {
		struct tm tm;
		utc_mkdate(when, &tm);
		snprintf(buffer, MAX_DATE_STRING,
			/*++GETTEXT 60 char buffer monthname, year, nr dives */
			ngettext("%1$s %2$d (%3$d dive)",
				 "%1$s %2$d (%3$d dives)", nr),
			monthname(tm.tm_mon),
			tm.tm_year + 1900,
			nr);
	}
	return buffer;
}

#define MAX_NITROX_STRING 80
#define UTF8_ELLIPSIS "\xE2\x80\xA6"

/* callers needs to free the string */
char *get_nitrox_string(struct dive *dive)
{
	int o2, he, o2low;
	char *buffer = malloc(MAX_NITROX_STRING);

	if (buffer) {
		get_dive_gas(dive, &o2, &he, &o2low);
		o2 = (o2 + 5) / 10;
		he = (he + 5) / 10;
		o2low = (o2low + 5) / 10;

		if (he)
			snprintf(buffer, MAX_NITROX_STRING, "%d/%d", o2, he);
		else if (o2)
			if (o2 == o2low)
				snprintf(buffer, MAX_NITROX_STRING, "%d", o2);
			else
				snprintf(buffer, MAX_NITROX_STRING, "%d" UTF8_ELLIPSIS "%d", o2low, o2);
		else
			strcpy(buffer, _("air"));
	}
	return buffer;
}

/*
 * helper functions for dive_trip handling
 */
#ifdef DEBUG_TRIP
void dump_trip_list(void)
{
	dive_trip_t *trip;
	int i=0;
	timestamp_t last_time = 0;

	for (trip = dive_trip_list; trip; trip = trip->next) {
		struct tm tm;
		utc_mkdate(trip->when, &tm);
		if (trip->when < last_time)
			printf("\n\ndive_trip_list OUT OF ORDER!!!\n\n\n");
		printf("%s trip %d to \"%s\" on %04u-%02u-%02u %02u:%02u:%02u (%d dives - %p)\n",
			trip->autogen ? "autogen " : "",
			++i, trip->location,
			tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
			trip->nrdives, trip);
		last_time = trip->when;
	}
	printf("-----\n");
}
#endif

/* this finds the last trip that at or before the time given */
dive_trip_t *find_matching_trip(timestamp_t when)
{
	dive_trip_t *trip = dive_trip_list;

	if (!trip || trip->when > when) {
#ifdef DEBUG_TRIP
		printf("no matching trip\n");
#endif
		return NULL;
	}
	while (trip->next && trip->next->when <= when)
		trip = trip->next;
#ifdef DEBUG_TRIP
	{
		struct tm tm;
		utc_mkdate(trip->when, &tm);
		printf("found trip %p @ %04d-%02d-%02d %02d:%02d:%02d\n",
			trip,
			tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
#endif
	return trip;
}

/* insert the trip into the dive_trip_list - but ensure you don't have
 * two trips for the same date; but if you have, make sure you don't
 * keep the one with less information */
void insert_trip(dive_trip_t **dive_trip_p)
{
	dive_trip_t *dive_trip = *dive_trip_p;
	dive_trip_t **p = &dive_trip_list;
	dive_trip_t *trip;
	struct dive *divep;

	/* Walk the dive trip list looking for the right location.. */
	while ((trip = *p) != NULL && trip->when < dive_trip->when)
		p = &trip->next;

	if (trip && trip->when == dive_trip->when) {
		if (! trip->location)
			trip->location = dive_trip->location;
		if (! trip->notes)
			trip->notes = dive_trip->notes;
		divep = dive_trip->dives;
		while (divep) {
			add_dive_to_trip(divep, trip);
			divep = divep->next;
		}
		*dive_trip_p = trip;
	} else {
		dive_trip->next = trip;
		*p = dive_trip;
	}
#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

static void delete_trip(dive_trip_t *trip)
{
	dive_trip_t **p, *tmp;

	assert(!trip->dives);

	/* Remove the trip from the list of trips */
	p = &dive_trip_list;
	while ((tmp = *p) != NULL) {
		if (tmp == trip) {
			*p = trip->next;
			break;
		}
		p = &tmp->next;
	}

	/* .. and free it */
	if (trip->location)
		free(trip->location);
	if (trip->notes)
		free(trip->notes);
	free(trip);
}

static void find_new_trip_start_time(dive_trip_t *trip)
{
	struct dive *dive = trip->dives;
	timestamp_t when = dive->when;

	while ((dive = dive->next) != NULL) {
		if (dive->when < when)
			when = dive->when;
	}
	trip->when = when;
}

void remove_dive_from_trip(struct dive *dive)
{
	struct dive *next, **pprev;
	dive_trip_t *trip = dive->divetrip;

	if (!trip)
		return;

	/* Remove the dive from the trip's list of dives */
	next = dive->next;
	pprev = dive->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;

	dive->divetrip = NULL;
	dive->tripflag = NO_TRIP;
	assert(trip->nrdives > 0);
	if (!--trip->nrdives)
		delete_trip(trip);
	else if (trip->when == dive->when)
		find_new_trip_start_time(trip);
}

void add_dive_to_trip(struct dive *dive, dive_trip_t *trip)
{
	if (dive->divetrip == trip)
		return;
	assert(trip->when);
	remove_dive_from_trip(dive);
	trip->nrdives++;
	dive->divetrip = trip;
	dive->tripflag = ASSIGNED_TRIP;

	/* Add it to the trip's list of dives*/
	dive->next = trip->dives;
	if (dive->next)
		dive->next->pprev = &dive->next;
	trip->dives = dive;
	dive->pprev = &trip->dives;

	if (dive->when && trip->when > dive->when)
		trip->when = dive->when;
}

dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive)
{
	dive_trip_t *dive_trip = calloc(sizeof(dive_trip_t),1);
	dive_trip->when = dive->when;
	if (dive->location)
		dive_trip->location = strdup(dive->location);
	insert_trip(&dive_trip);

	dive->tripflag = IN_TRIP;
	add_dive_to_trip(dive, dive_trip);
	return dive_trip;
}

/*
 * Walk the dives from the oldest dive, and see if we can autogroup them
 */
void autogroup_dives(void)
{
	int i;
	struct dive *dive, *lastdive = NULL;

	for_each_dive(i, dive) {
		dive_trip_t *trip;

		if (dive->divetrip) {
			lastdive = dive;
			continue;
		}

		if (!DIVE_NEEDS_TRIP(dive)) {
			lastdive = NULL;
			continue;
		}

		/* Do we have a trip we can combine this into? */
		if (lastdive && dive->when < lastdive->when + TRIP_THRESHOLD) {
			dive_trip_t *trip = lastdive->divetrip;
			add_dive_to_trip(dive, trip);
			if (dive->location && !trip->location)
				trip->location = strdup(dive->location);
			lastdive = dive;
			continue;
		}

		lastdive = dive;
		trip = create_and_hookup_trip_from_dive(dive);
		trip->autogen = 1;
	}

#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

void clear_trip_indexes(void)
{
	dive_trip_t *trip;

	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;
}

/* this implements the mechanics of removing the dive from the table,
 * but doesn't deal with updating dive trips, etc */
void delete_single_dive(int idx)
{
	int i;
	struct dive *dive = get_dive(idx);
	if (!dive)
		return; /* this should never happen */
	remove_dive_from_trip(dive);
	for (i = idx; i < dive_table.nr - 1; i++)
		dive_table.dives[i] = dive_table.dives[i+1];
	dive_table.dives[--dive_table.nr] = NULL;
	if (dive->selected)
		amount_selected--;
	/* free all allocations */
	free(dive->dc.sample);
	if (dive->location)
		free((void *)dive->location);
	if (dive->notes)
		free((void *)dive->notes);
	if (dive->divemaster)
		free((void *)dive->divemaster);
	if (dive->buddy)
		free((void *)dive->buddy);
	if (dive->suit)
		free((void *)dive->suit);
	free(dive);
}

void add_single_dive(int idx, struct dive *dive)
{
	int i;
	dive_table.nr++;
	if (dive->selected)
		amount_selected++;
	for (i = idx; i < dive_table.nr ; i++) {
		struct dive *tmp = dive_table.dives[i];
		dive_table.dives[i] = dive;
		dive = tmp;
	}
}

void merge_dive_index(int i, struct dive *a)
{
	struct dive *b = get_dive(i+1);
	struct dive *res;

	res = merge_dives(a, b, b->when - a->when, FALSE);
	if (!res)
		return;

	add_single_dive(i, res);
	delete_single_dive(i+1);
	delete_single_dive(i+1);
#if USE_GTK_UI
	dive_list_update_dives();
#endif
	mark_divelist_changed(TRUE);
}

void select_dive(int idx)
{
	struct dive *dive = get_dive(idx);
	if (dive) {
		/* never select an invalid dive that isn't displayed */
		if (dive->dive_tags & DTAG_INVALID && !prefs.display_invalid_dives)
			return;
		if (!dive->selected) {
			dive->selected = 1;
			amount_selected++;
		}
		selected_dive = idx;
	}
}

void deselect_dive(int idx)
{
	struct dive *dive = get_dive(idx);
	if (dive && dive->selected) {
		dive->selected = 0;
		amount_selected--;
		if (selected_dive == idx && amount_selected > 0) {
			/* pick a different dive as selected */
			while (--selected_dive >= 0) {
				dive = get_dive(selected_dive);
				if (dive && dive->selected)
					return;
			}
			selected_dive = idx;
			while (++selected_dive < dive_table.nr) {
				dive = get_dive(selected_dive);
				if (dive && dive->selected)
					return;
			}
		}
		if (amount_selected == 0)
			selected_dive = -1;
	}
}

void mark_divelist_changed(int changed)
{
	dive_list_changed = changed;
}

int unsaved_changes()
{
	return dive_list_changed;
}

void remove_autogen_trips()
{
	int i;
	struct dive *dive;

	for_each_dive(i, dive) {
		dive_trip_t *trip = dive->divetrip;

		if (trip && trip->autogen)
			remove_dive_from_trip(dive);
	}
}

/*
 * When adding dives to the dive table, we try to renumber
 * the new dives based on any old dives in the dive table.
 *
 * But we only do it if:
 *
 *  - there are no dives in the dive table
 *
 *  OR
 *
 *  - the last dive in the old dive table was numbered
 *
 *  - all the new dives are strictly at the end (so the
 *    "last dive" is at the same location in the dive table
 *    after re-sorting the dives.
 *
 *  - none of the new dives have any numbers
 *
 * This catches the common case of importing new dives from
 * a dive computer, and gives them proper numbers based on
 * your old dive list. But it tries to be very conservative
 * and not give numbers if there is *any* question about
 * what the numbers should be - in which case you need to do
 * a manual re-numbering.
 */
static void try_to_renumber(struct dive *last, int preexisting)
{
	int i, nr;

	/*
	 * If the new dives aren't all strictly at the end,
	 * we're going to expect the user to do a manual
	 * renumbering.
	 */
	if (preexisting && get_dive(preexisting-1) != last)
		return;

	/*
	 * If any of the new dives already had a number,
	 * we'll have to do a manual renumbering.
	 */
	for (i = preexisting; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		if (dive->number)
			return;
	}

	/*
	 * Ok, renumber..
	 */
	if (last)
		nr = last->number;
	else
		nr = 0;
	for (i = preexisting; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		dive->number = ++nr;
	}
}

void process_dives(bool is_imported, bool prefer_imported)
{
		int i;
	int preexisting = dive_table.preexisting;
	struct dive *last;

	/* check if we need a nickname for the divecomputer for newly downloaded dives;
	 * since we know they all came from the same divecomputer we just check for the
	 * first one */
	if (preexisting < dive_table.nr && dive_table.dives[preexisting]->downloaded)
		set_dc_nickname(dive_table.dives[preexisting]);
	else
		/* they aren't downloaded, so record / check all new ones */
		for (i = preexisting; i < dive_table.nr; i++)
			set_dc_nickname(dive_table.dives[i]);

	/* This does the right thing for -1: NULL */
	last = get_dive(preexisting-1);

	sort_table(&dive_table);

	for (i = 1; i < dive_table.nr; i++) {
		struct dive **pp = &dive_table.dives[i-1];
		struct dive *prev = pp[0];
		struct dive *dive = pp[1];
		struct dive *merged;

		/* only try to merge overlapping dives - or if one of the dives has
		 * zero duration (that might be a gps marker from the webservice) */
		if (prev->duration.seconds && dive->duration.seconds &&
		    prev->when + prev->duration.seconds < dive->when)
			continue;

		merged = try_to_merge(prev, dive, prefer_imported);
		if (!merged)
			continue;

		/* careful - we might free the dive that last points to. Oops... */
		if (last == prev || last == dive)
			last = merged;

		/* Redo the new 'i'th dive */
		i--;
		add_single_dive(i, merged);
		delete_single_dive(i+1);
		delete_single_dive(i+1);
	}
	/* make sure no dives are still marked as downloaded */
	for (i = 1; i < dive_table.nr; i++)
		dive_table.dives[i]->downloaded = FALSE;

	if (is_imported) {
		/* If there are dives in the table, are they numbered */
		if (!last || last->number)
			try_to_renumber(last, preexisting);

		/* did we add dives to the dive table? */
		if (preexisting != dive_table.nr)
			mark_divelist_changed(TRUE);
	}
}
