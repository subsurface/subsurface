// SPDX-License-Identifier: GPL-2.0
/* statistics.c
 *
 * core logic for the Info & Stats page -
 * char *get_minutes(int seconds);
 * void calculate_stats_summary(struct stats_summary *out, bool selected_only);
 * void calculate_stats_selected(stats_t *stats_selection);
 */

#include "statistics.h"
#include "dive.h"
#include "display.h"
#include "event.h"
#include "gettext.h"
#include "sample.h"
#include "subsurface-time.h"
#include "trip.h"
#include "units.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void process_temperatures(struct dive *dp, stats_t *stats)
{
	temperature_t min_temp, mean_temp, max_temp = {.mkelvin = 0};

	max_temp.mkelvin = dp->maxtemp.mkelvin;
	if (max_temp.mkelvin && (!stats->max_temp.mkelvin || max_temp.mkelvin > stats->max_temp.mkelvin))
		stats->max_temp.mkelvin = max_temp.mkelvin;

	min_temp.mkelvin = dp->mintemp.mkelvin;
	if (min_temp.mkelvin && (!stats->min_temp.mkelvin || min_temp.mkelvin < stats->min_temp.mkelvin))
		stats->min_temp.mkelvin = min_temp.mkelvin;

	if (min_temp.mkelvin || max_temp.mkelvin) {
		mean_temp.mkelvin = min_temp.mkelvin;
		if (mean_temp.mkelvin)
			mean_temp.mkelvin = (mean_temp.mkelvin + max_temp.mkelvin) / 2;
		else
			mean_temp.mkelvin = max_temp.mkelvin;
		stats->combined_temp.mkelvin += mean_temp.mkelvin;
		stats->combined_count++;
	}
}

static void process_dive(struct dive *dive, stats_t *stats)
{
	int old_tadt, sac_time = 0;
	int32_t duration = dive->duration.seconds;

	old_tadt = stats->total_average_depth_time.seconds;
	stats->total_time.seconds += duration;
	if (duration > stats->longest_time.seconds)
		stats->longest_time.seconds = duration;
	if (stats->shortest_time.seconds == 0 || duration < stats->shortest_time.seconds)
		stats->shortest_time.seconds = duration;
	if (dive->maxdepth.mm > stats->max_depth.mm)
		stats->max_depth.mm = dive->maxdepth.mm;
	if (stats->min_depth.mm == 0 || dive->maxdepth.mm < stats->min_depth.mm)
		stats->min_depth.mm = dive->maxdepth.mm;
	stats->combined_max_depth.mm += dive->maxdepth.mm;

	process_temperatures(dive, stats);

	/* Maybe we should drop zero-duration dives */
	if (!duration)
		return;
	if (dive->meandepth.mm) {
		stats->total_average_depth_time.seconds += duration;
		stats->avg_depth.mm = lrint((1.0 * old_tadt * stats->avg_depth.mm +
					duration * dive->meandepth.mm) /
					stats->total_average_depth_time.seconds);
	}
	if (dive->sac > 100) { /* less than .1 l/min is bogus, even with a pSCR */
		sac_time = stats->total_sac_time.seconds + duration;
		stats->avg_sac.mliter = lrint((1.0 * stats->total_sac_time.seconds * stats->avg_sac.mliter +
					 duration * dive->sac) /
					 sac_time);
		if (dive->sac > stats->max_sac.mliter)
			stats->max_sac.mliter = dive->sac;
		if (stats->min_sac.mliter == 0 || dive->sac < stats->min_sac.mliter)
			stats->min_sac.mliter = dive->sac;
		stats->total_sac_time.seconds = sac_time;
	}
}

char *get_minutes(int seconds)
{
	static char buf[80];
	snprintf(buf, sizeof(buf), "%d:%.2d", FRACTION(seconds, 60));
	return buf;
}

/*
 * Calculate a summary of the statistics and put in the stats_summary
 * structure provided in the first parameter.
 * Before first use, it should be initialized with init_stats_summary().
 * After use, memory must be released with free_stats_summary().
 */
void calculate_stats_summary(struct stats_summary *out, bool selected_only)
{
	int idx;
	int t_idx, d_idx, r;
	struct dive *dp;
	struct tm tm;
	int current_year = 0;
	int current_month = 0;
	int year_iter = 0;
	int month_iter = 0;
	int prev_month = 0, prev_year = 0;
	int trip_iter = 0;
	dive_trip_t *trip_ptr = 0;
	size_t size, tsize, dsize, tmsize;
	stats_t stats = { 0 };

	if (dive_table.nr > 0) {
		stats.shortest_time.seconds = dive_table.dives[0]->duration.seconds;
		stats.min_depth.mm = dive_table.dives[0]->maxdepth.mm;
		stats.selection_size = dive_table.nr;
	}

	/* allocate sufficient space to hold the worst
	 * case (one dive per year or all dives during
	 * one month) for yearly and monthly statistics*/

	size = sizeof(stats_t) * (dive_table.nr + 1);
	tsize = sizeof(stats_t) * (NUM_DIVEMODE + 1);
	dsize = sizeof(stats_t) * ((STATS_MAX_DEPTH / STATS_DEPTH_BUCKET) + 1);
	tmsize = sizeof(stats_t) * ((STATS_MAX_TEMP / STATS_TEMP_BUCKET) + 1);
	free_stats_summary(out);
	out->stats_yearly = malloc(size);
	out->stats_monthly = malloc(size);
	out->stats_by_trip = malloc(size);
	out->stats_by_type = malloc(tsize);
	out->stats_by_depth = malloc(dsize);
	out->stats_by_temp = malloc(tmsize);
	if (!out->stats_yearly || !out->stats_monthly || !out->stats_by_trip ||
		  !out->stats_by_type || !out->stats_by_depth || !out->stats_by_temp)
		return;
	memset(out->stats_yearly, 0, size);
	memset(out->stats_monthly, 0, size);
	memset(out->stats_by_trip, 0, size);
	memset(out->stats_by_type, 0, tsize);
	memset(out->stats_by_depth, 0, dsize);
	memset(out->stats_by_temp, 0, tmsize);
	out->stats_yearly[0].is_year = true;

	/* Setting the is_trip to true to show the location as first
	 * field in the statistics window */
	out->stats_by_type[0].location = strdup(translate("gettextFromC", "All (by type stats)"));
	out->stats_by_type[0].is_trip = true;
	out->stats_by_type[1].location = strdup(translate("gettextFromC", divemode_text_ui[OC]));
	out->stats_by_type[1].is_trip = true;
	out->stats_by_type[2].location = strdup(translate("gettextFromC", divemode_text_ui[CCR]));
	out->stats_by_type[2].is_trip = true;
	out->stats_by_type[3].location = strdup(translate("gettextFromC", divemode_text_ui[PSCR]));
	out->stats_by_type[3].is_trip = true;
	out->stats_by_type[4].location = strdup(translate("gettextFromC", divemode_text_ui[FREEDIVE]));
	out->stats_by_type[4].is_trip = true;

	out->stats_by_depth[0].location = strdup(translate("gettextFromC", "All (by max depth stats)"));
	out->stats_by_depth[0].is_trip = true;

	out->stats_by_temp[0].location = strdup(translate("gettextFromC", "All (by min. temp stats)"));
	out->stats_by_temp[0].is_trip = true;

	/* this relies on the fact that the dives in the dive_table
	 * are in chronological order */
	for_each_dive (idx, dp) {
		if (selected_only && !dp->selected)
			continue;
		if (dp->invalid)
			continue;
		process_dive(dp, &stats);

		/* yearly statistics */
		utc_mkdate(dp->when, &tm);
		if (current_year == 0)
			current_year = tm.tm_year;

		if (current_year != tm.tm_year) {
			current_year = tm.tm_year;
			process_dive(dp, &(out->stats_yearly[++year_iter]));
			out->stats_yearly[year_iter].is_year = true;
		} else {
			process_dive(dp, &(out->stats_yearly[year_iter]));
		}
		out->stats_yearly[year_iter].selection_size++;
		out->stats_yearly[year_iter].period = current_year;

		/* stats_by_type[0] is all the dives combined */
		out->stats_by_type[0].selection_size++;
		process_dive(dp, &(out->stats_by_type[0]));

		process_dive(dp, &(out->stats_by_type[dp->dc.divemode + 1]));
		out->stats_by_type[dp->dc.divemode + 1].selection_size++;

		/* stats_by_depth[0] is all the dives combined */
		out->stats_by_depth[0].selection_size++;
		process_dive(dp, &(out->stats_by_depth[0]));

		d_idx = dp->maxdepth.mm / (STATS_DEPTH_BUCKET * 1000);
		if (d_idx < 0)
			d_idx = 0;
		if (d_idx >= STATS_MAX_DEPTH / STATS_DEPTH_BUCKET)
			d_idx = STATS_MAX_DEPTH / STATS_DEPTH_BUCKET - 1;
		process_dive(dp, &(out->stats_by_depth[d_idx + 1]));
		out->stats_by_depth[d_idx + 1].selection_size++;

		/* stats_by_temp[0] is all the dives combined */
		out->stats_by_temp[0].selection_size++;
		process_dive(dp, &(out->stats_by_temp[0]));

		t_idx = ((int)mkelvin_to_C(dp->mintemp.mkelvin)) / STATS_TEMP_BUCKET;
		if (t_idx < 0)
			t_idx = 0;
		if (t_idx >= STATS_MAX_TEMP / STATS_TEMP_BUCKET)
			t_idx = STATS_MAX_TEMP / STATS_TEMP_BUCKET - 1;
		process_dive(dp, &(out->stats_by_temp[t_idx + 1]));
		out->stats_by_temp[t_idx + 1].selection_size++;

		if (dp->divetrip != NULL) {
			if (trip_ptr != dp->divetrip) {
				trip_ptr = dp->divetrip;
				trip_iter++;
			}

			/* stats_by_trip[0] is all the dives combined */
			out->stats_by_trip[0].selection_size++;
			process_dive(dp, &(out->stats_by_trip[0]));
			out->stats_by_trip[0].is_trip = true;
			out->stats_by_trip[0].location = strdup(translate("gettextFromC", "All (by trip stats)"));

			process_dive(dp, &(out->stats_by_trip[trip_iter]));
			out->stats_by_trip[trip_iter].selection_size++;
			out->stats_by_trip[trip_iter].is_trip = true;
			out->stats_by_trip[trip_iter].location = dp->divetrip->location;
		}

		/* monthly statistics */
		if (current_month == 0) {
			current_month = tm.tm_mon + 1;
		} else {
			if (current_month != tm.tm_mon + 1)
				current_month = tm.tm_mon + 1;
			if (prev_month != current_month || prev_year != current_year)
				month_iter++;
		}
		process_dive(dp, &(out->stats_monthly[month_iter]));
		out->stats_monthly[month_iter].selection_size++;
		out->stats_monthly[month_iter].period = current_month;
		prev_month = current_month;
		prev_year = current_year;
	}

	/* add labels for depth ranges up to maximum depth seen */
	if (out->stats_by_depth[0].selection_size) {
		d_idx = out->stats_by_depth[0].max_depth.mm;
		if (d_idx > STATS_MAX_DEPTH * 1000)
			d_idx = STATS_MAX_DEPTH * 1000;
		for (r = 0; r * (STATS_DEPTH_BUCKET * 1000) < d_idx; ++r)
			out->stats_by_depth[r+1].is_trip = true;
	}

	/* add labels for depth ranges up to maximum temperature seen */
	if (out->stats_by_temp[0].selection_size) {
		t_idx = (int)mkelvin_to_C(out->stats_by_temp[0].max_temp.mkelvin);
		if (t_idx > STATS_MAX_TEMP)
			t_idx = STATS_MAX_TEMP;
		for (r = 0; r * STATS_TEMP_BUCKET < t_idx; ++r)
			out->stats_by_temp[r+1].is_trip = true;
	}
}

void free_stats_summary(struct stats_summary *stats)
{
	free(stats->stats_yearly);
	free(stats->stats_monthly);
	free(stats->stats_by_trip);
	free(stats->stats_by_type);
	free(stats->stats_by_depth);
	free(stats->stats_by_temp);
}

void init_stats_summary(struct stats_summary *stats)
{
	stats->stats_yearly = NULL;
	stats->stats_monthly = NULL;
	stats->stats_by_trip = NULL;
	stats->stats_by_type = NULL;
	stats->stats_by_depth = NULL;
	stats->stats_by_temp = NULL;
}

/* make sure we skip the selected summary entries */
void calculate_stats_selected(stats_t *stats_selection)
{
	struct dive *dive;
	unsigned int i, nr;

	memset(stats_selection, 0, sizeof(*stats_selection));

	nr = 0;
	for_each_dive(i, dive) {
		if (dive->selected && !dive->invalid) {
			process_dive(dive, stats_selection);
			nr++;
		}
	}
	stats_selection->selection_size = nr;
}

#define SOME_GAS 5000 // 5bar drop in cylinder pressure makes cylinder used

bool has_gaschange_event(const struct dive *dive, const struct divecomputer *dc, int idx)
{
	bool first_gas_explicit = false;
	const struct event *event = get_next_event(dc->events, "gaschange");
	while (event) {
		if (dc->sample && (event->time.seconds == 0 ||
				   (dc->samples && dc->sample[0].time.seconds == event->time.seconds)))
			first_gas_explicit = true;
		if (get_cylinder_index(dive, event) == idx)
			return true;
		event = get_next_event(event->next, "gaschange");
	}
	if (dc->divemode == CCR) {
		if (idx == get_cylinder_idx_by_use(dive, DILUENT))
			return true;
		if (idx == get_cylinder_idx_by_use(dive, OXYGEN))
			return true;
	}
	return !first_gas_explicit && idx == 0;
}

bool is_cylinder_used(const struct dive *dive, int idx)
{
	const struct divecomputer *dc;
	cylinder_t *cyl;
	if (idx < 0 || idx >= dive->cylinders.nr)
		return false;

	cyl = get_cylinder(dive, idx);
	if ((cyl->start.mbar - cyl->end.mbar) > SOME_GAS)
		return true;

	if ((cyl->sample_start.mbar - cyl->sample_end.mbar) > SOME_GAS)
		return true;

	for_each_dc(dive, dc) {
		if (has_gaschange_event(dive, dc, idx))
			return true;
	}
	return false;
}

bool is_cylinder_prot(const struct dive *dive, int idx)
{
	const struct divecomputer *dc;
	if (idx < 0 || idx >= dive->cylinders.nr)
		return false;

	for_each_dc(dive, dc) {
		if (has_gaschange_event(dive, dc, idx))
			return true;
	}
	return false;
}

/* Returns a dynamically allocated array with dive->cylinders.nr entries,
 * which has to be freed by the caller */
volume_t *get_gas_used(struct dive *dive)
{
	int idx;

	volume_t *gases = malloc(dive->cylinders.nr * sizeof(volume_t));
	for (idx = 0; idx < dive->cylinders.nr; idx++) {
		cylinder_t *cyl = get_cylinder(dive, idx);
		pressure_t start, end;

		start = cyl->start.mbar ? cyl->start : cyl->sample_start;
		end = cyl->end.mbar ? cyl->end : cyl->sample_end;
		if (end.mbar && start.mbar > end.mbar)
			gases[idx].mliter = gas_volume(cyl, start) - gas_volume(cyl, end);
		else
			gases[idx].mliter = 0;
	}

	return gases;
}

/* Quite crude reverse-blender-function, but it produces a approx result */
static void get_gas_parts(struct gasmix mix, volume_t vol, int o2_in_topup, volume_t *o2, volume_t *he)
{
	volume_t air = {};

	if (gasmix_is_air(mix)) {
		o2->mliter = 0;
		he->mliter = 0;
		return;
	}

	air.mliter = lrint(((double)vol.mliter * get_n2(mix)) / (1000 - o2_in_topup));
	he->mliter = lrint(((double)vol.mliter * get_he(mix)) / 1000.0);
	o2->mliter += vol.mliter - he->mliter - air.mliter;
}

void selected_dives_gas_parts(volume_t *o2_tot, volume_t *he_tot)
{
	int i, j;
	struct dive *d;
	for_each_dive (i, d) {
		if (!d->selected || d->invalid)
			continue;
		volume_t *diveGases = get_gas_used(d);
		for (j = 0; j < d->cylinders.nr; j++) {
			if (diveGases[j].mliter) {
				volume_t o2 = {}, he = {};
				get_gas_parts(get_cylinder(d, j)->gasmix, diveGases[j], O2_IN_AIR, &o2, &he);
				o2_tot->mliter += o2.mliter;
				he_tot->mliter += he.mliter;
			}
		}
		free(diveGases);
	}
}
