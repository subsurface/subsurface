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
#include "core/dive.h"	// For MAX_CYLINDERS

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int period;
	duration_t total_time;
	/* total time of dives with non-zero average depth */
	duration_t total_average_depth_time;
	/* avg_time is simply total_time / nr -- let's not keep this */
	duration_t shortest_time;
	duration_t longest_time;
	depth_t max_depth;
	depth_t min_depth;
	depth_t avg_depth;
	volume_t max_sac;
	volume_t min_sac;
	volume_t avg_sac;
	temperature_t max_temp;
	temperature_t min_temp;
	temperature_sum_t combined_temp;
	unsigned int combined_count;
	unsigned int selection_size;
	duration_t total_sac_time;
	bool is_year;
	bool is_trip;
	char *location;
} stats_t;
extern stats_t stats_selection;
extern stats_t *stats_yearly;
extern stats_t *stats_monthly;
extern stats_t *stats_by_trip;
extern stats_t *stats_by_type;

extern char *get_minutes(int seconds);
extern void process_all_dives();
extern void get_gas_used(struct dive *dive, volume_t gases[MAX_CYLINDERS]);
extern void process_selected_dives(void);
void selected_dives_gas_parts(volume_t *o2_tot, volume_t *he_tot);

#ifdef __cplusplus
}
#endif

#endif // STATISTICS_H
