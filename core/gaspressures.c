// SPDX-License-Identifier: GPL-2.0
/*  gaspressures.c
 *  ---------------
 *  This file contains the routines to calculate the gas pressures in the cylinders.
 *  The functions below support the code in profile.cpp.
 *  The high-level function is populate_pressure_information(), called by function
 *  create_plot_info_new() in profile.c. The other functions below are, in turn,
 *  called by populate_pressure_information(). The calling sequence is as follows:
 *
 *  populate_pressure_information() -> calc_pressure_time()
 *                                  -> fill_missing_tank_pressures() -> fill_missing_segment_pressures()
 *                                                                   -> get_pr_interpolate_data()
 *
 *  The pr_track_t related functions below implement a linked list that is used by
 *  the majority of the functions below. The linked list covers a part of the dive profile
 *  for which there are no cylinder pressure data. Each element in the linked list
 *  represents a segment between two consecutive points on the dive profile.
 *  pr_track_t is defined in gaspressures.h
 */

#include "ssrf.h"
#include "dive.h"
#include "event.h"
#include "profile.h"
#include "gaspressures.h"
#include "pref.h"

#include <stdlib.h>

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

typedef struct pr_interpolate_struct pr_interpolate_t;
struct pr_interpolate_struct {
	int start;
	int end;
	int pressure_time;
	int acc_pressure_time;
};

enum interpolation_strategy {SAC, TIME, CONSTANT};

static pr_track_t *pr_track_alloc(int start, int t_start)
{
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

#ifdef DEBUG_PR_TRACK
static void dump_pr_track(int cyl, pr_track_t *track_pr)
{
	pr_track_t *list;

	printf("cyl%d:\n", cyl);
	list = track_pr;
	while (list) {
		printf("   start %f end %f t_start %d:%02d t_end %d:%02d pt %d\n",
		       mbar_to_PSI(list->start),
		       mbar_to_PSI(list->end),
		       FRACTION(list->t_start, 60),
		       FRACTION(list->t_end, 60),
		       list->pressure_time);
		list = list->next;
	}
}
#endif

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
static void fill_missing_segment_pressures(pr_track_t *list, enum interpolation_strategy strategy)
{
	double magic;

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
		switch (strategy) {
		case SAC:
			for (;;) {
				int pressure;
				pt += list->pressure_time;
				pressure = start;
				if (pt_sum)
					pressure -= lrint((start - end) * (double)pt / pt_sum);
				list->end = pressure;
				if (list == tmp)
					break;
				list = list->next;
				list->start = pressure;
			}
			break;
		case TIME:
			if (list->t_end && (tmp->t_start - tmp->t_end)) {
				magic = (list->t_start - tmp->t_end) / (tmp->t_start - tmp->t_end);
				list->end = lrint(start - (start - end) * magic);
			} else {
				list->end = start;
			}
			break;
		case CONSTANT:
			list->end = start;
		}

		/* Ok, we've done that set of segments */
		list = list->next;
	}
}

#ifdef DEBUG_PR_INTERPOLATE
void dump_pr_interpolate(int i, pr_interpolate_t interpolate_pr)
{
	printf("Interpolate for entry %d: start %d - end %d - pt %d - acc_pt %d\n", i,
	       interpolate_pr.start, interpolate_pr.end, interpolate_pr.pressure_time, interpolate_pr.acc_pressure_time);
}
#endif


static struct pr_interpolate_struct get_pr_interpolate_data(pr_track_t *segment, struct plot_info *pi, int cur)
{ // cur = index to pi->entry corresponding to t_end of segment;
	struct pr_interpolate_struct interpolate;
	int i;
	struct plot_data *entry;

	interpolate.start = segment->start;
	interpolate.end = segment->end;
	interpolate.acc_pressure_time = 0;
	interpolate.pressure_time = 0;

	for (i = 0; i < pi->nr; i++) {
		entry = pi->entry + i;

		if (entry->sec < segment->t_start)
			continue;
		interpolate.pressure_time += entry->pressure_time;
		if (entry->sec >= segment->t_end)
			break;
		if (i <= cur)
			interpolate.acc_pressure_time += entry->pressure_time;
	}
	return interpolate;
}

static void fill_missing_tank_pressures(const struct dive *dive, struct plot_info *pi, pr_track_t *track_pr, int cyl)
{
	int i;
	struct plot_data *entry;
	pr_interpolate_t interpolate = { 0, 0, 0, 0 };
	pr_track_t *last_segment = NULL;
	int cur_pr;
	enum interpolation_strategy strategy;

	/* no segment where this cylinder is used */
	if (!track_pr)
		return;

	if (get_cylinder(dive, cyl)->cylinder_use == OC_GAS)
		strategy = SAC;
	else
		strategy = TIME;
	fill_missing_segment_pressures(track_pr, strategy); // Interpolate the missing tank pressure values ..
	cur_pr = track_pr->start;			       // in the pr_track_t lists of structures
							       // and keep the starting pressure for each cylinder.
#ifdef DEBUG_PR_TRACK
	dump_pr_track(cyl, track_pr);
#endif

	/* Transfer interpolated cylinder pressures from pr_track strucktures to plotdata
	 * Go down the list of tank pressures in plot_info. Align them with the start &
	 * end times of each profile segment represented by a pr_track_t structure. Get
	 * the accumulated pressure_depths from the pr_track_t structures and then
	 * interpolate the pressure where these do not exist in the plot_info pressure
	 * variables. Pressure values are transferred from the pr_track_t structures
	 * to the plot_info structure, allowing us to plot the tank pressure.
	 *
	 * The first two pi structures are "fillers", but in case we don't have a sample
	 * at time 0 we need to process the second of them here, therefore i=1 */
	for (i = 1; i < pi->nr; i++) { // For each point on the profile:
		double magic;
		pr_track_t *segment;
		int pressure;

		entry = pi->entry + i;

		pressure = get_plot_pressure(pi, i, cyl);

		if (pressure) {			// If there is a valid pressure value,
			last_segment = NULL;	// get rid of interpolation data,
			cur_pr = pressure; // set current pressure
			continue;		// and skip to next point.
		}
		// If there is NO valid pressure value..
		// Find the pressure segment corresponding to this entry..
		segment = track_pr;
		while (segment && segment->t_end < entry->sec) // Find the track_pr with end time..
			segment = segment->next;	       // ..that matches the plot_info time (entry->sec)

		// After last segment? All done.
		if (!segment)
			break;

		// Before first segment, or between segments.. Go on, no interpolation.
		if (segment->t_start > entry->sec)
			continue;

		if (!segment->pressure_time) {		// Empty segment?
			set_plot_pressure_data(pi, i, SENSOR_PR, cyl, cur_pr);
							// Just use our current pressure
			continue;			// and skip to next point.
		}

		// If there is a valid segment but no tank pressure ..
		if (segment == last_segment) {
			interpolate.acc_pressure_time += entry->pressure_time;
		} else {
			// Set up an interpolation structure
			interpolate = get_pr_interpolate_data(segment, pi, i);
			last_segment = segment;
		}

		if(get_cylinder(dive, cyl)->cylinder_use == OC_GAS) {

			/* if this segment has pressure_time, then calculate a new interpolated pressure */
			if (interpolate.pressure_time) {
				/* Overall pressure change over total pressure-time for this segment*/
				magic = (interpolate.end - interpolate.start) / (double)interpolate.pressure_time;

				/* Use that overall pressure change to update the current pressure */
				cur_pr = lrint(interpolate.start + magic * interpolate.acc_pressure_time);
			}
		} else {
			magic = (interpolate.end - interpolate.start) /  (segment->t_end - segment->t_start);
			cur_pr = lrint(segment->start + magic * (entry->sec - segment->t_start));
		}
		set_plot_pressure_data(pi, i, INTERPOLATED_PR, cyl, cur_pr); // and store the interpolated data in plot_info
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
static inline int calc_pressure_time(const struct dive *dive, struct plot_data *a, struct plot_data *b)
{
	int time = b->sec - a->sec;
	int depth = (a->depth + b->depth) / 2;

	if (depth <= SURFACE_THRESHOLD)
		return 0;

	return depth_to_mbar(depth, dive) * time;
}

#ifdef PRINT_PRESSURES_DEBUG
// A CCR debugging tool that prints the gas pressures in cylinder 0 and in the diluent cylinder, used in populate_pressure_information():
static void debug_print_pressures(struct plot_info *pi)
{
	int i;
	for (i = 0; i < pi->nr; i++)
		printf("%5d |%9d | %9d |\n", i, get_plot_sensor_pressure(pi, i), get_plot_interpolated_pressure(pi, i));
}
#endif

/* This function goes through the list of tank pressures, of structure plot_info for the dive profile where each
 * item in the list corresponds to one point (node) of the profile. It finds values for which there are no tank
 * pressures (pressure==0). For each missing item (node) of tank pressure it creates a pr_track_alloc structure
 * that represents a segment on the dive profile and that contains tank pressures. There is a linked list of
 * pr_track_alloc structures for each cylinder. These pr_track_alloc structures ultimately allow for filling
 * the missing tank pressure values on the dive profile using the depth_pressure of the dive. To do this, it
 * calculates the summed pressure-time value for the duration of the dive and stores these * in the pr_track_alloc
 * structures. This function is called by create_plot_info_new() in profile.c
 */
void populate_pressure_information(const struct dive *dive, const struct divecomputer *dc, struct plot_info *pi, int sensor)
{
	UNUSED(dc);
	int first, last, cyl;
	cylinder_t *cylinder = get_cylinder(dive, sensor);
	pr_track_t *track = NULL;
	pr_track_t *current = NULL;
	const struct event *ev, *b_ev;
	int missing_pr = 0, dense = 1;
	enum divemode_t dmode = dc->divemode;
	const double gasfactor[5] = {1.0, 0.0, prefs.pscr_ratio/1000.0, 1.0, 1.0 };

	if (sensor < 0 || sensor >= dive->cylinders.nr)
		return;

	/* if we have no pressure data whatsoever, this is pointless, so let's just return */
	if (!cylinder->start.mbar && !cylinder->end.mbar &&
	    !cylinder->sample_start.mbar && !cylinder->sample_end.mbar)
		return;

	/* Get a rough range of where we have any pressures at all */
	first = last = -1;
	for (int i = 0; i < pi->nr; i++) {
		int pressure = get_plot_sensor_pressure(pi, i, sensor);

		if (!pressure)
			continue;
		if (first < 0)
			first = i;
		last = i;
	}

	/* No sensor data at all? */
	if (first == last)
		return;

	/*
	 * Split the range:
	 *  - missing pressure data
	 *  - gas change events to other cylinders
	 *
	 * Note that we only look at gas switches if this cylinder
	 * itself has a gas change event.
	 */
	cyl = sensor;
	ev = NULL;
	if (has_gaschange_event(dive, dc, sensor))
		ev = get_next_event(dc->events, "gaschange");
	b_ev = get_next_event(dc->events, "modechange");

	for (int i = first; i <= last; i++) {
		struct plot_data *entry = pi->entry + i;
		int pressure = get_plot_sensor_pressure(pi, i, sensor);
		int time = entry->sec;

		while (ev && ev->time.seconds <= time) {   // Find 1st gaschange event after 
			cyl = get_cylinder_index(dive, ev); // the current gas change.
			if (cyl < 0)
				cyl = sensor;
			ev = get_next_event(ev->next, "gaschange");
		}

		while (b_ev && b_ev->time.seconds <= time) { // Keep existing divemode, then
			dmode = b_ev->value; // find 1st divemode change event after the current 
			b_ev = get_next_event(b_ev->next, "modechange"); // divemode change.
		}

		if (current) { // calculate pressure-time, taking into account the dive mode for this specific segment.
			entry->pressure_time = (int)(calc_pressure_time(dive, entry - 1, entry) * gasfactor[dmode] + 0.5);
			current->pressure_time += entry->pressure_time;
			current->t_end = entry->sec;
			if (pressure)
				current->end = pressure;
		}

		// We have a final pressure for 'current'
		// If a gas switch has occurred, finish the
		// current pressure track entry and continue
		// until we get back to this cylinder.
		if (cyl != sensor) {
			current = NULL;
			set_plot_pressure_data(pi, i, SENSOR_PR, sensor, 0);
			continue;
		}

		// If we have no pressure information, we will need to
		// continue with or without a tracking entry. Mark any
		// existing tracking entry as non-dense, and remember
		// to fill in interpolated data.
		if (current && !pressure) {
			missing_pr = 1;
			dense = 0;
			continue;
		}

		// If we already have a pressure tracking entry, and
		// it has not had any missing samples, just continue
		// using it - there's nothing to interpolate yet.
		if (current && dense)
			continue;

		// We need to start a new tracking entry, either
		// because the previous was interrupted by a gas
		// switch event, or because the previous one has
		// missing entries that need to be interpolated.
		// Or maybe we didn't have a previous one at all,
		// and this is the first pressure entry.
		current = pr_track_alloc(pressure, entry->sec);
		track = list_add(track, current);
		dense = 1;
	}

	if (missing_pr) {
		fill_missing_tank_pressures(dive, pi, track, sensor);
	}

#ifdef PRINT_PRESSURES_DEBUG
	debug_print_pressures(pi);
#endif

	list_free(track);
}
