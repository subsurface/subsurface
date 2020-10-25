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
#define STATS_MAX_TEMP 40		/* Max temp for stats bucket is 40C */
#define STATS_TEMP_BUCKET 5 /* Size of buckets for temp range */

struct dive;

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
	depth_t combined_max_depth;
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

struct stats_summary {
	stats_t *stats_yearly;
	stats_t *stats_monthly;
	stats_t *stats_by_trip;
	stats_t *stats_by_type;
	stats_t *stats_by_depth;
	stats_t *stats_by_temp;
};

#ifdef __cplusplus
extern "C" {
#endif

extern char *get_minutes(int seconds);
extern void init_stats_summary(struct stats_summary *stats);
extern void free_stats_summary(struct stats_summary *stats);
extern void calculate_stats_summary(struct stats_summary *stats, bool selected_only);
extern void calculate_stats_selected(stats_t *stats_selection);
extern volume_t *get_gas_used(struct dive *dive);
extern void selected_dives_gas_parts(volume_t *o2_tot, volume_t *he_tot);

#ifdef __cplusplus
}
#endif

/*
 * For C++ code, provide a convenience version of stats_summary
 * that initializes the structure on construction and frees
 * resources when it goes out of scope. Apart from that, it
 * can be used as a stats_summary replacement.
 */
#ifdef __cplusplus
struct stats_summary_auto_free : public stats_summary {
	stats_summary_auto_free();
	~stats_summary_auto_free();
};
inline stats_summary_auto_free::stats_summary_auto_free()
{
	init_stats_summary(this);
}
inline stats_summary_auto_free::~stats_summary_auto_free()
{
	free_stats_summary(this);
}
#endif

#endif // STATISTICS_H
