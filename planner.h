#ifndef PLANNER_H
#define PLANNER_H


#ifdef __cplusplus
extern "C" {
#endif

extern int validate_gas(const char *text, int *o2_p, int *he_p);
extern int validate_po2(const char *text, int *mbar_po2);
extern timestamp_t current_time_notz(void);
extern void show_planned_dive(char **error_string_p);
extern void set_last_stop(bool last_stop_6m);
extern void get_gas_from_events(struct divecomputer *dc, int time, int *o2, int *he);
extern int get_gasidx(struct dive *dive, int o2, int he);

extern struct diveplan diveplan;
extern struct dive *planned_dive;
extern char *cache_data;
extern char *disclaimer;
extern double plangflow, plangfhigh;


#ifdef __cplusplus
}
#endif
#endif /* PLANNER_H */
