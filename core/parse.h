// SPDX-License-Identifier: GPL-2.0
#ifndef PARSE_H
#define PARSE_H

#define MAX_EVENT_NAME 128

typedef union {
	struct event event;
	char allocation[sizeof(struct event) + MAX_EVENT_NAME];
} event_allocation_t;

extern event_allocation_t event_allocation;
#define cur_event event_allocation.event

/*
 * Dive info as it is being built up..
 */
extern struct divecomputer *cur_dc;
extern struct dive *cur_dive;
extern struct dive_site *cur_dive_site;
extern degrees_t cur_latitude, cur_longitude;
extern dive_trip_t *cur_trip;
extern struct sample *cur_sample;
extern struct picture *cur_picture;


struct {
	struct {
		const char *model;
		uint32_t deviceid;
		const char *nickname, *serial_nr, *firmware;
	} dc;
} cur_settings;

extern bool in_settings;
extern bool in_userid;
extern struct tm cur_tm;
extern int cur_cylinder_index, cur_ws_index;
extern int lastcylinderindex, next_o2_sensor;
extern int o2pressure_sensor;
extern struct extra_data cur_extra_data;

enum import_source {
	UNKNOWN,
	LIBDIVECOMPUTER,
	DIVINGLOG,
	UDDF,
	SSRF_WS,
} import_source;

/* the dive table holds the overall dive list; target table points at
 * the table we are currently filling */
extern struct dive_table dive_table;
extern struct dive_table *target_table;

extern int metric;

int trimspace(char *buffer);
void clear_table(struct dive_table *table);
void record_dive_to_table(struct dive *dive, struct dive_table *table);
void record_dive(struct dive *dive);
void start_match(const char *type, const char *name, char *buffer);
void nonmatch(const char *type, const char *name, char *buffer);
typedef void (*matchfn_t)(char *buffer, void *);
int match(const char *pattern, int plen, const char *name, matchfn_t fn, char *buf, void *data);
void event_start(void);
void event_end(void);
struct divecomputer *get_dc(void);

bool is_dive(void);
void reset_dc_info(struct divecomputer *dc);
void reset_dc_settings(void);
void settings_start(void);
void settings_end(void);
void dc_settings_start(void);
void dc_settings_end(void);
void dive_site_start(void);
void dive_site_end(void);
void dive_start(void);
void dive_end(void);
void trip_start(void);
void trip_end(void);
void picture_start(void);
void picture_end(void);
void cylinder_start(void);
void cylinder_end(void);
void ws_start(void);
void ws_end(void);

void sample_start(void);
void sample_end(void);
void divecomputer_start(void);
void divecomputer_end(void);
void userid_start(void);
void userid_stop(void);
void utf8_string(char *buffer, void *_res);

void add_dive_site(char *ds_name, struct dive *dive);

#endif
