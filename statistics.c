/* statistics.c
 *
 * core logic for the Info & Stats page -
 * char *get_time_string(int seconds, int maxdays);
 * char *get_minutes(int seconds);
 * void process_all_dives(struct dive *dive, struct dive **prev_dive);
 * void get_selected_dives_text(char *buffer, int size);
 */
#include "gettext.h"
#include <string.h>
#include <ctype.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"
#include "statistics.h"

static stats_t stats;
stats_t stats_selection;
stats_t *stats_monthly = NULL;
stats_t *stats_yearly = NULL;
stats_t *stats_by_trip = NULL;

static void process_temperatures(struct dive *dp, stats_t *stats)
{
	int min_temp, mean_temp, max_temp = 0;

	max_temp = dp->maxtemp.mkelvin;
	if (max_temp && (!stats->max_temp || max_temp > stats->max_temp))
		stats->max_temp = max_temp;

	min_temp = dp->mintemp.mkelvin;
	if (min_temp && (!stats->min_temp || min_temp < stats->min_temp))
		stats->min_temp = min_temp;

	if (min_temp || max_temp) {
		mean_temp = min_temp;
		if (mean_temp)
			mean_temp = (mean_temp + max_temp) / 2;
		else
			mean_temp = max_temp;
		stats->combined_temp += get_temp_units(mean_temp, NULL);
		stats->combined_count++;
	}
}

static void process_dive(struct dive *dp, stats_t *stats)
{
	int old_tt, sac_time = 0;
	int duration = dp->duration.seconds;

	old_tt = stats->total_time.seconds;
	stats->total_time.seconds += duration;
	if (duration > stats->longest_time.seconds)
		stats->longest_time.seconds = duration;
	if (stats->shortest_time.seconds == 0 || duration < stats->shortest_time.seconds)
		stats->shortest_time.seconds = duration;
	if (dp->maxdepth.mm > stats->max_depth.mm)
		stats->max_depth.mm = dp->maxdepth.mm;
	if (stats->min_depth.mm == 0 || dp->maxdepth.mm < stats->min_depth.mm)
		stats->min_depth.mm = dp->maxdepth.mm;

	process_temperatures(dp, stats);

	/* Maybe we should drop zero-duration dives */
	if (!duration)
		return;
	stats->avg_depth.mm = (1.0 * old_tt * stats->avg_depth.mm +
			       duration * dp->meandepth.mm) /
			      stats->total_time.seconds;
	if (dp->sac > 100) { /* less than .1 l/min is bogus, even with a pSCR */
		sac_time = stats->total_sac_time + duration;
		stats->avg_sac.mliter = (1.0 * stats->total_sac_time * stats->avg_sac.mliter +
					 duration * dp->sac) /
					sac_time;
		if (dp->sac > stats->max_sac.mliter)
			stats->max_sac.mliter = dp->sac;
		if (stats->min_sac.mliter == 0 || dp->sac < stats->min_sac.mliter)
			stats->min_sac.mliter = dp->sac;
		stats->total_sac_time = sac_time;
	}
}

char *get_minutes(int seconds)
{
	static char buf[80];
	snprintf(buf, sizeof(buf), "%d:%.2d", FRACTION(seconds, 60));
	return buf;
}

void process_all_dives(struct dive *dive, struct dive **prev_dive)
{
	int idx;
	struct dive *dp;
	struct tm tm;
	int current_year = 0;
	int current_month = 0;
	int year_iter = 0;
	int month_iter = 0;
	int prev_month = 0, prev_year = 0;
	int trip_iter = 0;
	dive_trip_t *trip_ptr = 0;
	unsigned int size;

	*prev_dive = NULL;
	memset(&stats, 0, sizeof(stats));
	if (dive_table.nr > 0) {
		stats.shortest_time.seconds = dive_table.dives[0]->duration.seconds;
		stats.min_depth.mm = dive_table.dives[0]->maxdepth.mm;
		stats.selection_size = dive_table.nr;
	}

	/* allocate sufficient space to hold the worst
	 * case (one dive per year or all dives during
	 * one month) for yearly and monthly statistics*/

	free(stats_yearly);
	free(stats_monthly);
	free(stats_by_trip);

	size = sizeof(stats_t) * (dive_table.nr + 1);
	stats_yearly = malloc(size);
	stats_monthly = malloc(size);
	stats_by_trip = malloc(size);
	if (!stats_yearly || !stats_monthly || !stats_by_trip)
		return;
	memset(stats_yearly, 0, size);
	memset(stats_monthly, 0, size);
	memset(stats_by_trip, 0, size);
	stats_yearly[0].is_year = true;

	/* this relies on the fact that the dives in the dive_table
	 * are in chronological order */
	for_each_dive (idx, dp) {
		if (dive && dp->when == dive->when) {
			/* that's the one we are showing */
			if (idx > 0)
				*prev_dive = dive_table.dives[idx - 1];
		}
		process_dive(dp, &stats);

		/* yearly statistics */
		utc_mkdate(dp->when, &tm);
		if (current_year == 0)
			current_year = tm.tm_year + 1900;

		if (current_year != tm.tm_year + 1900) {
			current_year = tm.tm_year + 1900;
			process_dive(dp, &(stats_yearly[++year_iter]));
			stats_yearly[year_iter].is_year = true;
		} else {
			process_dive(dp, &(stats_yearly[year_iter]));
		}
		stats_yearly[year_iter].selection_size++;
		stats_yearly[year_iter].period = current_year;

		if (dp->divetrip != NULL) {
			if (trip_ptr != dp->divetrip) {
				trip_ptr = dp->divetrip;
				trip_iter++;
			}

			/* stats_by_trip[0] is all the dives combined */
			stats_by_trip[0].selection_size++;
			process_dive(dp, &(stats_by_trip[0]));
			stats_by_trip[0].is_trip = true;
			stats_by_trip[0].location = strdup("All (by trip stats)");

			process_dive(dp, &(stats_by_trip[trip_iter]));
			stats_by_trip[trip_iter].selection_size++;
			stats_by_trip[trip_iter].is_trip = true;
			stats_by_trip[trip_iter].location = dp->divetrip->location;
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
		process_dive(dp, &(stats_monthly[month_iter]));
		stats_monthly[month_iter].selection_size++;
		stats_monthly[month_iter].period = current_month;
		prev_month = current_month;
		prev_year = current_year;
	}
}

/* make sure we skip the selected summary entries */
void process_selected_dives(void)
{
	struct dive *dive;
	unsigned int i, nr;

	memset(&stats_selection, 0, sizeof(stats_selection));

	nr = 0;
	for_each_dive(i, dive) {
		if (dive->selected) {
			process_dive(dive, &stats_selection);
			nr++;
		}
	}
	stats_selection.selection_size = nr;
}

char *get_time_string(int seconds, int maxdays)
{
	static char buf[80];
	if (maxdays && seconds > 3600 * 24 * maxdays) {
		snprintf(buf, sizeof(buf), translate("gettextFromC", "more than %d days"), maxdays);
	} else {
		int days = seconds / 3600 / 24;
		int hours = (seconds - days * 3600 * 24) / 3600;
		int minutes = (seconds - days * 3600 * 24 - hours * 3600) / 60;
		if (days > 0)
			snprintf(buf, sizeof(buf), translate("gettextFromC", "%dd %dh %dmin"), days, hours, minutes);
		else
			snprintf(buf, sizeof(buf), translate("gettextFromC", "%dh %dmin"), hours, minutes);
	}
	return buf;
}

/* this gets called when at least two but not all dives are selected */
static void get_ranges(char *buffer, int size)
{
	int i, len;
	int first = -1, last = -1;
	struct dive *dive;

	snprintf(buffer, size, "%s", translate("gettextFromC", "for dives #"));
	for_each_dive (i, dive) {
		if (!dive->selected)
			continue;
		if (dive->number < 1) {
			/* uhh - weird numbers - bail */
			snprintf(buffer, size, "%s", translate("gettextFromC", "for selected dives"));
			return;
		}
		len = strlen(buffer);
		if (last == -1) {
			snprintf(buffer + len, size - len, "%d", dive->number);
			first = last = dive->number;
		} else {
			if (dive->number == last + 1) {
				last++;
				continue;
			} else {
				if (first == last)
					snprintf(buffer + len, size - len, ", %d", dive->number);
				else if (first + 1 == last)
					snprintf(buffer + len, size - len, ", %d, %d", last, dive->number);
				else
					snprintf(buffer + len, size - len, "-%d, %d", last, dive->number);
				first = last = dive->number;
			}
		}
	}
	len = strlen(buffer);
	if (first != last) {
		if (first + 1 == last)
			snprintf(buffer + len, size - len, ", %d", last);
		else
			snprintf(buffer + len, size - len, "-%d", last);
	}
}

void get_selected_dives_text(char *buffer, int size)
{
	if (amount_selected == 1) {
		if (current_dive)
			snprintf(buffer, size, translate("gettextFromC", "for dive #%d"), current_dive->number);
		else
			snprintf(buffer, size, "%s", translate("gettextFromC", "for selected dive"));
	} else if (amount_selected == dive_table.nr) {
		snprintf(buffer, size, "%s", translate("gettextFromC", "for all dives"));
	} else if (amount_selected == 0) {
		snprintf(buffer, size, "%s", translate("gettextFromC", "(no dives)"));
	} else {
		get_ranges(buffer, size);
		if (strlen(buffer) == size - 1) {
			/* add our own ellipse... the way Pango does this is ugly
			 * as it will leave partial numbers there which I don't like */
			int offset = 4;
			while (offset < size && isdigit(buffer[size - offset]))
				offset++;
			strcpy(buffer + size - offset, "...");
		}
	}
}

bool is_cylinder_used(struct dive *dive, int idx)
{
	struct divecomputer *dc;
	bool firstGasExplicit = false;
	if (cylinder_none(&dive->cylinder[idx]))
		return false;

	for_each_dc(dive, dc) {
		struct event *event = get_next_event(dc->events, "gaschange");
		while (event) {
			if (dc->sample && event->time.seconds == dc->sample[0].time.seconds)
				firstGasExplicit = true;
			if (get_cylinder_index(dive, event) == idx)
				return true;
			event = get_next_event(event->next, "gaschange");
		}
		if (dc->dctype == CCR && (idx == dive->diluent_cylinder_index || idx == dive->oxygen_cylinder_index))
			return true;
	}
	if (idx == 0 && !firstGasExplicit)
		return true;
	return false;
}

void get_gas_used(struct dive *dive, volume_t gases[MAX_CYLINDERS])
{
	int idx;
	for (idx = 0; idx < MAX_CYLINDERS; idx++) {
		cylinder_t *cyl = &dive->cylinder[idx];
		pressure_t start, end;

		if (!is_cylinder_used(dive, idx))
			continue;

		start = cyl->start.mbar ? cyl->start : cyl->sample_start;
		end = cyl->end.mbar ? cyl->end : cyl->sample_end;
		if (end.mbar && start.mbar > end.mbar)
			gases[idx].mliter = gas_volume(cyl, start) - gas_volume(cyl, end);
	}
}

/* Quite crude reverse-blender-function, but it produces a approx result */
static void get_gas_parts(struct gasmix mix, volume_t vol, int o2_in_topup, volume_t *o2, volume_t *he)
{
	volume_t air = {};

	if (gasmix_is_air(&mix)) {
		o2->mliter = 0;
		he->mliter = 0;
		return;
	}

	air.mliter = rint(((double)vol.mliter * (1000 - get_he(&mix) - get_o2(&mix))) / (1000 - o2_in_topup));
	he->mliter = rint(((double)vol.mliter * get_he(&mix)) / 1000.0);
	o2->mliter += vol.mliter - he->mliter - air.mliter;
}

void selected_dives_gas_parts(volume_t *o2_tot, volume_t *he_tot)
{
	int i, j;
	struct dive *d;
	for_each_dive (i, d) {
		if (!d->selected)
			continue;
		volume_t diveGases[MAX_CYLINDERS] = {};
		get_gas_used(d, diveGases);
		for (j = 0; j < MAX_CYLINDERS; j++) {
			if (diveGases[j].mliter) {
				volume_t o2 = {}, he = {};
				get_gas_parts(d->cylinder[j].gasmix, diveGases[j], O2_IN_AIR, &o2, &he);
				o2_tot->mliter += o2.mliter;
				he_tot->mliter += he.mliter;
			}
		}
	}
}
