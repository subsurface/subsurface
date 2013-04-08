#ifndef PLANNER_H
#define PLANNER_H

extern void plan(struct diveplan *diveplan, char **cache_datap, struct dive **divep, char **error_string_p);
extern int validate_gas(const char *text, int *o2_p, int *he_p);
extern int validate_time(const char *text, int *sec_p, int *rel_p);
extern int validate_depth(const char *text, int *mm_p);
extern int validate_po2(const char *text, int *mbar_po2);
extern int validate_volume(const char *text, int *sac);
extern timestamp_t current_time_notz(void);
extern void show_planned_dive(char **error_string_p);
extern int add_duration_to_nth_dp(struct diveplan *diveplan, int idx, int duration, gboolean is_rel);
extern void add_po2_to_nth_dp(struct diveplan *diveplan, int idx, int po2);

extern struct diveplan diveplan;
extern struct dive *planned_dive;
extern char *cache_data;
extern char *disclaimer;
extern double plangflow, plangfhigh;

#endif /* PLANNER_H */
