/*
 * statistics.h
 *
 * core logic functions called from statistics UI
 * common types and variables
 */

#ifndef STATISTICS_H
#define STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int period;
	duration_t total_time;
	/* avg_time is simply total_time / nr -- let's not keep this */
	duration_t shortest_time;
	duration_t longest_time;
	depth_t max_depth;
	depth_t min_depth;
	depth_t avg_depth;
	volume_t max_sac;
	volume_t min_sac;
	volume_t avg_sac;
	int max_temp;
	int min_temp;
	double combined_temp;
	unsigned int combined_count;
	unsigned int selection_size;
	unsigned int total_sac_time;
	bool is_year;
	bool is_trip;
	char *location;
} stats_t;
extern stats_t stats_selection;
extern stats_t *stats_yearly;
extern stats_t *stats_monthly;
extern stats_t *stats_by_trip;

extern char *get_time_string(int seconds, int maxdays);
extern char *get_minutes(int seconds);
extern void process_all_dives(struct dive *dive, struct dive **prev_dive);
extern void get_selected_dives_text(char *buffer, int size);
extern void get_gas_used(struct dive *dive, volume_t gases[MAX_CYLINDERS]);
extern void process_selected_dives(void);
void selected_dives_gas_parts(volume_t *o2_tot, volume_t *he_tot);

#ifdef __cplusplus
}
#endif

#endif // STATISTICS_H
