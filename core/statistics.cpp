// SPDX-License-Identifier: GPL-2.0
/* statistics.cpp
 *
 * core logic for the Info & Stats page
 */

#include "statistics.h"
#include "dive.h"
#include "divelog.h"
#include "event.h"
#include "gettext.h"
#include "sample.h"
#include "subsurface-time.h"
#include "trip.h"
#include "units.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void process_temperatures(struct dive *dp, stats_t &stats)
{
	temperature_t min_temp, mean_temp, max_temp = {.mkelvin = 0};

	max_temp.mkelvin = dp->maxtemp.mkelvin;
	if (max_temp.mkelvin && (!stats.max_temp.mkelvin || max_temp.mkelvin > stats.max_temp.mkelvin))
		stats.max_temp.mkelvin = max_temp.mkelvin;

	min_temp.mkelvin = dp->mintemp.mkelvin;
	if (min_temp.mkelvin && (!stats.min_temp.mkelvin || min_temp.mkelvin < stats.min_temp.mkelvin))
		stats.min_temp.mkelvin = min_temp.mkelvin;

	if (min_temp.mkelvin || max_temp.mkelvin) {
		mean_temp.mkelvin = min_temp.mkelvin;
		if (mean_temp.mkelvin)
			mean_temp.mkelvin = (mean_temp.mkelvin + max_temp.mkelvin) / 2;
		else
			mean_temp.mkelvin = max_temp.mkelvin;
		stats.combined_temp.mkelvin += mean_temp.mkelvin;
		stats.combined_count++;
	}
}

static void process_dive(struct dive *dive, stats_t &stats)
{
	int old_tadt, sac_time = 0;
	int32_t duration = dive->duration.seconds;

	old_tadt = stats.total_average_depth_time.seconds;
	stats.total_time.seconds += duration;
	if (duration > stats.longest_time.seconds)
		stats.longest_time.seconds = duration;
	if (stats.shortest_time.seconds == 0 || duration < stats.shortest_time.seconds)
		stats.shortest_time.seconds = duration;
	if (dive->maxdepth.mm > stats.max_depth.mm)
		stats.max_depth.mm = dive->maxdepth.mm;
	if (stats.min_depth.mm == 0 || dive->maxdepth.mm < stats.min_depth.mm)
		stats.min_depth.mm = dive->maxdepth.mm;
	stats.combined_max_depth.mm += dive->maxdepth.mm;

	process_temperatures(dive, stats);

	/* Maybe we should drop zero-duration dives */
	if (!duration)
		return;
	if (dive->meandepth.mm) {
		stats.total_average_depth_time.seconds += duration;
		stats.avg_depth.mm = lrint((1.0 * old_tadt * stats.avg_depth.mm +
					duration * dive->meandepth.mm) /
					stats.total_average_depth_time.seconds);
	}
	if (dive->sac > 100) { /* less than .1 l/min is bogus, even with a pSCR */
		sac_time = stats.total_sac_time.seconds + duration;
		stats.avg_sac.mliter = lrint((1.0 * stats.total_sac_time.seconds * stats.avg_sac.mliter +
					 duration * dive->sac) /
					 sac_time);
		if (dive->sac > stats.max_sac.mliter)
			stats.max_sac.mliter = dive->sac;
		if (stats.min_sac.mliter == 0 || dive->sac < stats.min_sac.mliter)
			stats.min_sac.mliter = dive->sac;
		stats.total_sac_time.seconds = sac_time;
	}
}

/*
 * Calculate a summary of the statistics and put in the stats_summary
 * structure provided in the first parameter.
 * Before first use, it should be initialized with init_stats_summary().
 * After use, memory must be released with free_stats_summary().
 */
stats_summary calculate_stats_summary(bool selected_only)
{
	int idx;
	struct dive *dp;
	struct tm tm;
	int current_year = -1;
	int current_month = 0;
	int prev_month = 0, prev_year = 0;
	dive_trip_t *trip_ptr = nullptr;

	stats_summary out;

	/* stats_by_trip[0] is all the dives combined */
	out.stats_by_trip.emplace_back();

	/* Setting the is_trip to true to show the location as first
	 * field in the statistics window */
	out.stats_by_type.resize(NUM_DIVEMODE + 1);
	out.stats_by_type[0].location = translate("gettextFromC", "All (by type stats)");
	out.stats_by_type[0].is_trip = true;
	out.stats_by_type[1].location = translate("gettextFromC", divemode_text_ui[OC]);
	out.stats_by_type[1].is_trip = true;
	out.stats_by_type[2].location = translate("gettextFromC", divemode_text_ui[CCR]);
	out.stats_by_type[2].is_trip = true;
	out.stats_by_type[3].location = translate("gettextFromC", divemode_text_ui[PSCR]);
	out.stats_by_type[3].is_trip = true;
	out.stats_by_type[4].location = translate("gettextFromC", divemode_text_ui[FREEDIVE]);
	out.stats_by_type[4].is_trip = true;

	out.stats_by_depth.resize((STATS_MAX_DEPTH / STATS_DEPTH_BUCKET) + 1);
	out.stats_by_depth[0].location = translate("gettextFromC", "All (by max depth stats)");
	out.stats_by_depth[0].is_trip = true;

	out.stats_by_temp.resize((STATS_MAX_TEMP / STATS_TEMP_BUCKET) + 1);
	out.stats_by_temp[0].location = translate("gettextFromC", "All (by min. temp stats)");
	out.stats_by_temp[0].is_trip = true;

	/* this relies on the fact that the dives in the dive_table
	 * are in chronological order */
	for_each_dive (idx, dp) {
		if (selected_only && !dp->selected)
			continue;
		if (dp->invalid)
			continue;
		//process_dive(dp, &stats);

		/* yearly statistics */
		utc_mkdate(dp->when, &tm);

		if (current_year != tm.tm_year || out.stats_yearly.empty()) {
			current_year = tm.tm_year;
			out.stats_yearly.emplace_back();
			out.stats_yearly.back().is_year = true;
		}
		process_dive(dp, out.stats_yearly.back());

		out.stats_yearly.back().selection_size++;
		out.stats_yearly.back().period = current_year;

		/* stats_by_type[0] is all the dives combined */
		out.stats_by_type[0].selection_size++;
		process_dive(dp, out.stats_by_type[0]);

		process_dive(dp, out.stats_by_type[dp->dcs[0].divemode + 1]);
		out.stats_by_type[dp->dcs[0].divemode + 1].selection_size++;

		/* stats_by_depth[0] is all the dives combined */
		out.stats_by_depth[0].selection_size++;
		process_dive(dp, out.stats_by_depth[0]);

		int d_idx = dp->maxdepth.mm / (STATS_DEPTH_BUCKET * 1000);
		d_idx = std::clamp(d_idx, 0, STATS_MAX_DEPTH / STATS_DEPTH_BUCKET);
		process_dive(dp, out.stats_by_depth[d_idx + 1]);
		out.stats_by_depth[d_idx + 1].selection_size++;

		/* stats_by_temp[0] is all the dives combined */
		out.stats_by_temp[0].selection_size++;
		process_dive(dp, out.stats_by_temp[0]);

		int t_idx = ((int)mkelvin_to_C(dp->mintemp.mkelvin)) / STATS_TEMP_BUCKET;
		t_idx = std::clamp(t_idx, 0, STATS_MAX_TEMP / STATS_TEMP_BUCKET);
		process_dive(dp, out.stats_by_temp[t_idx + 1]);
		out.stats_by_temp[t_idx + 1].selection_size++;

		if (dp->divetrip != NULL) {
			if (trip_ptr != dp->divetrip) {
				trip_ptr = dp->divetrip;
				out.stats_by_trip.emplace_back();
			}

			/* stats_by_trip[0] is all the dives combined */
			/* TODO: yet, this doesn't seem to consider dives outside of trips !? */
			out.stats_by_trip[0].selection_size++;
			process_dive(dp, out.stats_by_trip[0]);
			out.stats_by_trip[0].is_trip = true;
			out.stats_by_trip[0].location = translate("gettextFromC", "All (by trip stats)");

			process_dive(dp, out.stats_by_trip.back());
			out.stats_by_trip.back().selection_size++;
			out.stats_by_trip.back().is_trip = true;
			out.stats_by_trip.back().location = dp->divetrip->location;
		}

		/* monthly statistics */
		if (current_month == 0 || out.stats_monthly.empty()) {
			current_month = tm.tm_mon + 1;
			out.stats_monthly.emplace_back();
		} else {
			if (current_month != tm.tm_mon + 1)
				current_month = tm.tm_mon + 1;
			if (prev_month != current_month || prev_year != current_year)
				out.stats_monthly.emplace_back();
		}
		process_dive(dp, out.stats_monthly.back());
		out.stats_monthly.back().selection_size++;
		out.stats_monthly.back().period = current_month;
		prev_month = current_month;
		prev_year = current_year;
	}

	/* add labels for depth ranges up to maximum depth seen */
	if (out.stats_by_depth[0].selection_size) {
		int d_idx = out.stats_by_depth[0].max_depth.mm;
		if (d_idx > STATS_MAX_DEPTH * 1000)
			d_idx = STATS_MAX_DEPTH * 1000;
		for (int r = 0; r * (STATS_DEPTH_BUCKET * 1000) < d_idx; ++r)
			out.stats_by_depth[r+1].is_trip = true;
	}

	/* add labels for depth ranges up to maximum temperature seen */
	if (out.stats_by_temp[0].selection_size) {
		int t_idx = (int)mkelvin_to_C(out.stats_by_temp[0].max_temp.mkelvin);
		if (t_idx > STATS_MAX_TEMP)
			t_idx = STATS_MAX_TEMP;
		for (int r = 0; r * STATS_TEMP_BUCKET < t_idx; ++r)
			out.stats_by_temp[r+1].is_trip = true;
	}

	return out;
}

stats_summary::stats_summary()
{
}

stats_summary::~stats_summary()
{
}

/* make sure we skip the selected summary entries */
stats_t calculate_stats_selected()
{
	stats_t stats_selection;
	struct dive *dive;
	unsigned int i, nr;

	nr = 0;
	for_each_dive(i, dive) {
		if (dive->selected && !dive->invalid) {
			process_dive(dive, stats_selection);
			nr++;
		}
	}
	stats_selection.selection_size = nr;
	return stats_selection;
}

#define SOME_GAS 5000 // 5bar drop in cylinder pressure makes cylinder used

bool has_gaschange_event(const struct dive *dive, const struct divecomputer *dc, int idx)
{
	bool first_gas_explicit = false;
	event_loop loop("gaschange");
	while (auto event = loop.next(*dc)) {
		if (!dc->samples.empty() && (event->time.seconds == 0 ||
				   (dc->samples[0].time.seconds == event->time.seconds)))
			first_gas_explicit = true;
		if (get_cylinder_index(dive, *event) == idx)
			return true;
	}
	return !first_gas_explicit && idx == 0;
}

bool is_cylinder_used(const struct dive *dive, int idx)
{
	cylinder_t *cyl;
	if (idx < 0 || idx >= dive->cylinders.nr)
		return false;

	cyl = get_cylinder(dive, idx);
	if ((cyl->start.mbar - cyl->end.mbar) > SOME_GAS)
		return true;

	if ((cyl->sample_start.mbar - cyl->sample_end.mbar) > SOME_GAS)
		return true;

	for (auto &dc: dive->dcs) {
		if (has_gaschange_event(dive, &dc, idx))
			return true;
		else if (dc.divemode == CCR && idx == get_cylinder_idx_by_use(dive, OXYGEN))
			return true;
	}
	return false;
}

bool is_cylinder_prot(const struct dive *dive, int idx)
{
	if (idx < 0 || idx >= dive->cylinders.nr)
		return false;

	return std::any_of(dive->dcs.begin(), dive->dcs.end(),
			   [dive, idx](auto &dc)
			   { return has_gaschange_event(dive, &dc, idx); });
}

/* Returns a vector with dive->cylinders.nr entries */
std::vector<volume_t> get_gas_used(struct dive *dive)
{
	std::vector<volume_t> gases(dive->cylinders.nr);
	for (int idx = 0; idx < dive->cylinders.nr; idx++) {
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

/* Quite crude reverse-blender-function, but it produces an approx result.
 * Returns an (O2, He) pair. */
static std::pair<volume_t, volume_t> get_gas_parts(struct gasmix mix, volume_t vol, int o2_in_topup)
{
	if (gasmix_is_air(mix))
		return { {0}, {0} };

	volume_t air = { (int)lrint(((double)vol.mliter * get_n2(mix)) / (1000 - o2_in_topup)) };
	volume_t he = { (int)lrint(((double)vol.mliter * get_he(mix)) / 1000.0) };
	volume_t o2 = { vol.mliter - he.mliter - air.mliter };
	return std::make_pair(o2, he);
}

std::pair<volume_t, volume_t> selected_dives_gas_parts()
{
	int i;
	struct dive *d;
	volume_t o2_tot, he_tot;
	for_each_dive (i, d) {
		if (!d->selected || d->invalid)
			continue;
		int j = 0;
		for (auto &gas: get_gas_used(d)) {
			if (gas.mliter) {
				auto [o2, he] = get_gas_parts(get_cylinder(d, j)->gasmix, gas, O2_IN_AIR);
				o2_tot.mliter += o2.mliter;
				he_tot.mliter += he.mliter;
			}
			j++;
		}
	}
	return std::make_pair(o2_tot, he_tot);
}
