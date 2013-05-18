#ifndef DIVELIST_H
#define DIVELIST_H

#ifdef __cplusplus
extern "C" {
#endif

struct dive;

extern void report_dives(bool imported, bool prefer_imported);
extern void dive_list_update_dives(void);
extern void update_dive_list_col_visibility(void);
extern void update_dive_list_units(void);
extern void flush_divelist(struct dive *);
extern void update_cylinder_related_info(struct dive *);
extern void mark_divelist_changed(int);
extern int unsaved_changes(void);
extern void remove_autogen_trips(void);
extern void select_next_dive(void);
extern void select_prev_dive(void);
extern void show_and_select_dive(struct dive *dive);
extern double init_decompression(struct dive * dive);
extern void export_all_dives_uddf_cb();
extern void upload_all_dives_divelogs_cb();

/* divelist core logic functions */
extern void process_dives(bool imported, bool prefer_imported);
extern char *get_dive_date_string(timestamp_t when);
extern char *get_trip_date_string(timestamp_t when, int nr);
extern char *get_nitrox_string(struct dive *dive);
extern void clear_trip_indexes(void);
extern dive_trip_t *find_trip_by_idx(int idx);
extern int dive_nr_sort(int idx_a, int idx_b, timestamp_t when_a, timestamp_t when_b);
extern int trip_has_selected_dives(dive_trip_t *trip);
extern void get_depth_values(int depth, int *depth_int, int *depth_decimal, int *show_decimal);
extern void get_dive_gas(struct dive *dive, int *o2_p, int *he_p, int *o2low_p);
extern int get_divenr(struct dive *dive);
extern void get_location(struct dive *dive, char **str);
extern void get_cylinder(struct dive *dive, char **str);
extern void get_suit(struct dive *dive, char **str);
extern dive_trip_t *find_matching_trip(timestamp_t when);
extern void remove_dive_from_trip(struct dive *dive);
extern dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive);
extern void autogroup_dives(void);
extern void merge_dive_index(int i, struct dive *a);
extern void select_dive(int idx);
extern void deselect_dive(int idx);

#ifdef DEBUG_TRIP
extern void dump_selection(void);
extern void dump_trip_list(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
