#ifndef DIVELIST_H
#define DIVELIST_H

#ifdef __cplusplus
extern "C" {
#endif

struct dive;

extern void update_cylinder_related_info(struct dive *);
extern void mark_divelist_changed(int);
extern int unsaved_changes(void);
extern void remove_autogen_trips(void);
extern double init_decompression(struct dive *dive);

/* divelist core logic functions */
extern void process_dives(bool imported, bool prefer_imported);
extern char *get_dive_gas_string(struct dive *dive);

extern dive_trip_t *find_trip_by_idx(int idx);

extern int trip_has_selected_dives(dive_trip_t *trip);
extern void get_dive_gas(struct dive *dive, int *o2_p, int *he_p, int *o2low_p);
extern int get_divenr(struct dive *dive);
extern dive_trip_t *find_matching_trip(timestamp_t when);
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
extern bool is_trip_before_after(struct dive *dive, bool before);
extern void set_dive_nr_for_current_dive();

#ifdef DEBUG_TRIP
extern void dump_selection(void);
extern void dump_trip_list(void);
#endif

#ifdef __cplusplus
}
#endif

#endif // DIVELIST_H
