// SPDX-License-Identifier: GPL-2.0
/*
 * statistics.h
 *
 * core logic functions called from statistics UI
 * common types and variables
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#include "core/units.h"

#define STATS_MAX_DEPTH 300	/* Max depth for stats bucket is 300m */
#define STATS_DEPTH_BUCKET 10	/* Size of buckets for depth range */
#define STATS_MAX_TEMP 40	/* Max temp for stats bucket is 40C */
#define STATS_TEMP_BUCKET 5	/* Size of buckets for temp range */

struct dive;

#include <string>
#include <vector>

struct stats_t
{
	int period = 0;
	duration_t total_time ;
	/* total time of dives with non-zero average depth */
	duration_t total_average_depth_time;
	/* avg_time is simply total_time / nr -- let's not keep this */
	duration_t shortest_time;
	duration_t longest_time;
	depth_t max_depth;
	depth_t min_depth;
	depth_t avg_depth;
	depth_t combined_max_depth;
	volume_t max_sac;
	volume_t min_sac;
	volume_t avg_sac;
	temperature_t max_temp;
	temperature_t min_temp;
	temperature_sum_t combined_temp;
	unsigned int combined_count = 0;
	unsigned int selection_size = 0;
	duration_t total_sac_time;
	bool is_year = false;
	bool is_trip = false;
	std::string location;
};

struct stats_summary {
	stats_summary();
	~stats_summary();
	std::vector<stats_t> stats_yearly;
	std::vector<stats_t> stats_monthly;
	std::vector<stats_t> stats_by_trip;
	std::vector<stats_t> stats_by_type;
	std::vector<stats_t> stats_by_depth;
	std::vector<stats_t> stats_by_temp;
};

extern stats_summary calculate_stats_summary(bool selected_only);
extern stats_t calculate_stats_selected();
extern std::vector<volume_t> get_gas_used(struct dive *dive);
extern std::pair<volume_t, volume_t> selected_dives_gas_parts(); // returns (O2, He) tuple

#endif // STATISTICS_H
