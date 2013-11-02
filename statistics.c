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
			duration * dp->meandepth.mm) / stats->total_time.seconds;
	if (dp->sac > 2800) { /* less than .1 cuft/min (2800ml/min) is bogus */
		sac_time = stats->total_sac_time + duration;
		stats->avg_sac.mliter = (1.0 * stats->total_sac_time * stats->avg_sac.mliter +
				duration * dp->sac) / sac_time ;
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

	if (stats_yearly != NULL) {
		free(stats_yearly);
		free(stats_monthly);
	}
	size = sizeof(stats_t) * (dive_table.nr + 1);
	stats_yearly = malloc(size);
	stats_monthly = malloc(size);
	if (!stats_yearly || !stats_monthly)
		return;
	memset(stats_yearly, 0, size);
	memset(stats_monthly, 0, size);
	stats_yearly[0].is_year = TRUE;

	/* this relies on the fact that the dives in the dive_table
	 * are in chronological order */
	for (idx = 0; idx < dive_table.nr; idx++) {
		dp = dive_table.dives[idx];
		if (dive && dp->when == dive->when) {
			/* that's the one we are showing */
			if (idx > 0)
				*prev_dive = dive_table.dives[idx-1];
		}
		process_dive(dp, &stats);

		/* yearly statistics */
		utc_mkdate(dp->when, &tm);
		if (current_year == 0)
			current_year = tm.tm_year + 1900;

		if (current_year != tm.tm_year + 1900) {
			current_year = tm.tm_year + 1900;
			process_dive(dp, &(stats_yearly[++year_iter]));
			stats_yearly[year_iter].is_year = TRUE;
		} else {
			process_dive(dp, &(stats_yearly[year_iter]));
		}
		stats_yearly[year_iter].selection_size++;
		stats_yearly[year_iter].period = current_year;

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
		snprintf(buf, sizeof(buf), translate("gettextFromC","more than %d days"), maxdays);
	} else {
		int days = seconds / 3600 / 24;
		int hours = (seconds - days * 3600 * 24) / 3600;
		int minutes = (seconds - days * 3600 * 24 - hours * 3600) / 60;
		if (days > 0)
			snprintf(buf, sizeof(buf), translate("gettextFromC","%dd %dh %dmin"), days, hours, minutes);
		else
			snprintf(buf, sizeof(buf), translate("gettextFromC","%dh %dmin"), hours, minutes);
	}
	return buf;
}

/* this gets called when at least two but not all dives are selected */
static void get_ranges(char *buffer, int size)
{
	int i, len;
	int first, last = -1;

	snprintf(buffer, size, "%s", translate("gettextFromC","for dives #"));
	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		if (! dive->selected)
			continue;
		if (dive->number < 1) {
			/* uhh - weird numbers - bail */
			snprintf(buffer, size, "%s", translate("gettextFromC","for selected dives"));
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
			snprintf(buffer, size, translate("gettextFromC","for dive #%d"), current_dive->number);
		else
			snprintf(buffer, size, "%s", translate("gettextFromC","for selected dive"));
	} else if (amount_selected == dive_table.nr) {
		snprintf(buffer, size, "%s", translate("gettextFromC","for all dives"));
	} else if (amount_selected == 0) {
		snprintf(buffer, size, "%s", translate("gettextFromC","(no dives)"));
	} else {
		get_ranges(buffer, size);
		if (strlen(buffer) == size -1) {
			/* add our own ellipse... the way Pango does this is ugly
			 * as it will leave partial numbers there which I don't like */
			int offset = 4;
			while (offset < size && isdigit(buffer[size - offset]))
				offset++;
			strcpy(buffer + size - offset, "...");
		}
	}
}

volume_t get_gas_used(struct dive *dive)
{
	int idx;
	volume_t gas_used = { 0 };
	for (idx = 0; idx < MAX_CYLINDERS; idx++) {
		cylinder_t *cyl = &dive->cylinder[idx];
		pressure_t start, end;

		start = cyl->start.mbar ? cyl->start : cyl->sample_start;
		end = cyl->end.mbar ? cyl->end : cyl->sample_end;
		if (start.mbar && end.mbar)
			gas_used.mliter += gas_volume(cyl, start) - gas_volume(cyl, end);
	}
	return gas_used;
}

bool is_gas_used(struct dive *dive, int idx)
{
	cylinder_t *cyl = &dive->cylinder[idx];
	int o2, he;
	struct divecomputer *dc;
	bool used = FALSE;
	bool firstGasExplicit = FALSE;
	if (cylinder_none(cyl))
		return FALSE;

	o2 = get_o2(&cyl->gasmix);
	he = get_he(&cyl->gasmix);
	dc = &dive->dc;
	while (dc) {
		struct event *event = dc->events;
		while (event) {
			if (event->value) {
				if (event->name && !strcmp(event->name, "gaschange")) {
					unsigned int event_he = event->value >> 16;
					unsigned int event_o2 = event->value & 0xffff;
					if (event->time.seconds < 30)
						firstGasExplicit = TRUE;
					if (is_air(o2, he)) {
						if (is_air(event_o2 * 10, event_he * 10))
							used = TRUE;
					} else if (event->type == 25 && he == event_he * 10 && o2 == event_o2 * 10) {
						/* SAMPLE_EVENT_GASCHANGE2(25) contains both o2 and he */
						used = TRUE;
					} else if (o2 == event_o2 * 10) {
						/* SAMPLE_EVENT_GASCHANGE(11) only contains o2 */
						used = TRUE;
					}
				}
			}
			if (used)
				break;
			event = event->next;
		}
		if (used)
			break;
		dc = dc->next;
	}
	if (idx == 0 && !firstGasExplicit)
		used = TRUE;
	return used;
}

#define MAXBUF 80
/* for the O2/He readings just create a list of them */
char *get_gaslist(struct dive *dive)
{
	int idx, offset = 0;
	static char buf[MAXBUF];
	int o2, he;

	buf[0] = '\0';
	for (idx = 0; idx < MAX_CYLINDERS; idx++) {
		cylinder_t *cyl;
		if (!is_gas_used(dive, idx))
			continue;
		cyl = &dive->cylinder[idx];
		o2 = get_o2(&cyl->gasmix);
		he = get_he(&cyl->gasmix);
		if (is_air(o2, he))
			snprintf(buf + offset, MAXBUF - offset, (offset > 0) ? ", %s" : "%s", translate("gettextFromC","air"));
		else
			if (he == 0)
				snprintf(buf + offset, MAXBUF - offset, (offset > 0) ? translate("gettextFromC",", EAN%d") : translate("gettextFromC","EAN%d"),
					 (o2 + 5) / 10);
			else
				snprintf(buf + offset, MAXBUF - offset, (offset > 0) ? ", %d/%d" : "%d/%d",
				 (o2 + 5) / 10, (he + 5) / 10);
		offset = strlen(buf);
	}
	if (*buf == '\0')
		strncpy(buf, translate("gettextFromC","air"), MAXBUF);
	return buf;
}
