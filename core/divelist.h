// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELIST_H
#define DIVELIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* this is used for both git and xml format */
#define DATAFORMAT_VERSION 3

struct dive;

extern void update_cylinder_related_info(struct dive *);
extern void mark_divelist_changed(bool);
extern int unsaved_changes(void);
extern void remove_autogen_trips(void);
extern int init_decompression(struct deco_state *ds, struct dive *dive);

/* divelist core logic functions */
extern void process_loaded_dives();
extern void process_imported_dives(struct dive_table *import_table, bool prefer_imported, bool downloaded);
extern char *get_dive_gas_string(struct dive *dive);

struct dive **grow_dive_table(struct dive_table *table);
extern void get_dive_gas(struct dive *dive, int *o2_p, int *he_p, int *o2low_p);
extern int get_divenr(const struct dive *dive);
extern int get_divesite_idx(const struct dive_site *ds);
extern struct dive_trip *unregister_dive_from_trip(struct dive *dive, short was_autogen);
extern void remove_dive_from_trip(struct dive *dive, short was_autogen);
extern dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive);
extern void autogroup_dives(void);
extern struct dive *merge_two_dives(struct dive *a, struct dive *b);
extern bool consecutive_selected();
extern void select_dive(int idx);
extern void deselect_dive(int idx);
extern void select_dives_in_trip(struct dive_trip *trip);
extern void deselect_dives_in_trip(struct dive_trip *trip);
extern void filter_dive(struct dive *d, bool shown);
extern void combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b);
extern void find_new_trip_start_time(dive_trip_t *trip);
extern struct dive *first_selected_dive();
extern struct dive *last_selected_dive();
extern bool is_trip_before_after(const struct dive *dive, bool before);
extern int get_dive_nr_at_idx(int idx);
extern void set_dive_nr_for_current_dive();
extern timestamp_t get_surface_interval(timestamp_t when);
extern void delete_dive_from_table(struct dive_table *table, int idx);

int get_min_datafile_version();
void reset_min_datafile_version();
void report_datafile_version(int version);
int get_dive_id_closest_to(timestamp_t when);
void clear_dive_file_data();

#ifdef DEBUG_TRIP
extern void dump_selection(void);
extern void dump_trip_list(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // DIVELIST_H
