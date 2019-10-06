// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_H
#define DIVE_H

// dive and dive computer related structures and helpers

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "divemode.h"
#include "equipment.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int last_xml_version;

extern const char *cylinderuse_text[NUM_GAS_USE];
extern const char *divemode_text_ui[];
extern const char *divemode_text[];

/*
 * Events are currently based straight on what libdivecomputer gives us.
 *  We need to wrap these into our own events at some point to remove some of the limitations.
 */
struct event {
	struct event *next;
	duration_t time;
	int type;
	/* This is the annoying libdivecomputer format. */
	int flags, value;
	/* .. and this is our "extended" data for some event types */
	union {
		enum divemode_t divemode; // for divemode change events
		/*
		 * NOTE! The index may be -1, which means "unknown". In that
		 * case, the get_cylinder_index() function will give the best
		 * match with the cylinders in the dive based on gasmix.
		 */
		struct { // for gas switch events
			int index;
			struct gasmix mix;
		} gas;
	};
	bool deleted;
	char name[];
};

extern int event_is_gaschange(const struct event *ev);

extern void fill_pressures(struct gas_pressures *pressures, const double amb_pressure, struct gasmix mix, double po2, enum divemode_t dctype);

/* Linear interpolation between 'a' and 'b', when we are 'part'way into the 'whole' distance from a to b */
static inline int interpolate(int a, int b, int part, int whole)
{
	/* It is doubtful that we actually need floating point for this, but whatever */
	if (whole) {
		double x = (double)a * (whole - part) + (double)b * part;
		return (int)lrint(x / whole);
	}
	return (a+b)/2;
}

#define MAX_SENSORS 2
struct sample                         // BASE TYPE BYTES  UNITS    RANGE               DESCRIPTION
{                                     // --------- -----  -----    -----               -----------
	duration_t time;                  // int32_t    4  seconds  (0-34 yrs)             elapsed dive time up to this sample
	duration_t stoptime;              // int32_t    4  seconds  (0-34 yrs)             time duration of next deco stop
	duration_t ndl;                   // int32_t    4  seconds  (-1 no val, 0-34 yrs)  time duration before no-deco limit
	duration_t tts;                   // int32_t    4  seconds  (0-34 yrs)             time duration to reach the surface
	duration_t rbt;                   // int32_t    4  seconds  (0-34 yrs)             remaining bottom time
	depth_t depth;                    // int32_t    4    mm     (0-2000 km)            dive depth of this sample
	depth_t stopdepth;                // int32_t    4    mm     (0-2000 km)            depth of next deco stop
	temperature_t temperature;        // uint32_t   4    mK     (0-4 MK)               ambient temperature
	pressure_t pressure[MAX_SENSORS]; // int32_t    4    mbar   (0-2 Mbar)             cylinder pressures (main and CCR o2)
	o2pressure_t setpoint;            // uint16_t   2    mbar   (0-65 bar)             O2 partial pressure (will be setpoint)
	o2pressure_t o2sensor[3];         // uint16_t   6    mbar   (0-65 bar)             Up to 3 PO2 sensor values (rebreather)
	bearing_t bearing;                // int16_t    2  degrees  (-1 no val, 0-360 deg) compass bearing
	uint8_t sensor[MAX_SENSORS];      // uint8_t    1  sensorID (0-255)                ID of cylinder pressure sensor
	uint16_t cns;                     // uint16_t   1     %     (0-64k %)              cns% accumulated
	uint8_t heartbeat;                // uint8_t    1  beats/m  (0-255)                heart rate measurement
	volume_t sac;                     //            4  ml/min                          predefined SAC
	bool in_deco;                     // bool       1    y/n      y/n                  this sample is part of deco
	bool manually_entered;            // bool       1    y/n      y/n                  this sample was entered by the user,
					  //                                               not calculated when planning a dive
};	                                  // Total size of structure: 57 bytes, excluding padding at end

struct extra_data {
	const char *key;
	const char *value;
	struct extra_data *next;
};

/*
 * NOTE! The deviceid and diveid are model-specific *hashes* of
 * whatever device identification that model may have. Different
 * dive computers will have different identifying data, it could
 * be a firmware number or a serial ID (in either string or in
 * numeric format), and we do not care.
 *
 * The only thing we care about is that subsurface will hash
 * that information the same way. So then you can check the ID
 * of a dive computer by comparing the hashes for equality.
 *
 * A deviceid or diveid of zero is assumed to be "no ID".
 */
struct divecomputer {
	timestamp_t when;
	duration_t duration, surfacetime, last_manual_time;
	depth_t maxdepth, meandepth;
	temperature_t airtemp, watertemp;
	pressure_t surface_pressure;
	enum divemode_t divemode;	// dive computer type: OC(default) or CCR
	uint8_t no_o2sensors;		// rebreathers: number of O2 sensors used
	int salinity; 			// kg per 10000 l
	const char *model, *serial, *fw_version;
	uint32_t deviceid, diveid;
	int samples, alloc_samples;
	struct sample *sample;
	struct event *events;
	struct extra_data *extra_data;
	struct divecomputer *next;
};

typedef struct dive_table {
	int nr, allocated;
	struct dive **dives;
} dive_table_t;

struct picture;
struct dive_site;
struct dive_site_table;
struct dive_trip;
struct trip_table;
struct dive {
	int number;
	bool notrip; /* Don't autogroup this dive to a trip */
	struct dive_trip *divetrip;
	bool selected;
	bool hidden_by_filter;
	timestamp_t when;
	struct dive_site *dive_site;
	char *notes;
	char *divemaster, *buddy;
	int rating;
	int visibility; /* 0 - 5 star rating */
	cylinder_t cylinder[MAX_CYLINDERS];
	struct weightsystem_table weightsystems;
	char *suit;
	int sac, otu, cns, maxcns;

	/* Calculated based on dive computer data */
	temperature_t mintemp, maxtemp, watertemp, airtemp;
	depth_t maxdepth, meandepth;
	pressure_t surface_pressure;
	duration_t duration;
	int salinity; // kg per 10000 l

	struct tag_entry *tag_list;
	struct divecomputer dc;
	int id; // unique ID for this dive
	struct picture *picture_list;
	unsigned char git_id[20];
};

/* For the top-level list: an entry is either a dive or a trip */
struct dive_or_trip {
	struct dive *dive;
	struct dive_trip *trip;
};

extern void invalidate_dive_cache(struct dive *dive);
extern bool dive_cache_is_valid(const struct dive *dive);

extern int get_cylinder_idx_by_use(const struct dive *dive, enum cylinderuse cylinder_use_type);
extern void cylinder_renumber(struct dive *dive, int mapping[]);
extern int same_gasmix_cylinder(cylinder_t *cyl, int cylid, struct dive *dive, bool check_unused);

/* when selectively copying dive information, which parts should be copied? */
struct dive_components {
	unsigned int divesite : 1;
	unsigned int notes : 1;
	unsigned int divemaster : 1;
	unsigned int buddy : 1;
	unsigned int suit : 1;
	unsigned int rating : 1;
	unsigned int visibility : 1;
	unsigned int tags : 1;
	unsigned int cylinders : 1;
	unsigned int weights : 1;
};

extern enum divemode_t get_current_divemode(const struct divecomputer *dc, int time, const struct event **evp, enum divemode_t *divemode);
extern struct event *get_next_divemodechange(const struct event **evd, bool update_pointer);
extern enum divemode_t get_divemode_at_time(const struct divecomputer *dc, int dtime, const struct event **ev_dmc);

/* picture list and methods related to dive picture handling */
struct picture {
	char *filename;
	offset_t offset;
	location_t location;
	struct picture *next;
};

#define FOR_EACH_PICTURE(_dive) \
	if (_dive)              \
		for (struct picture *picture = (_dive)->picture_list; picture; picture = picture->next)

extern struct picture *alloc_picture();
extern void free_picture(struct picture *picture);
extern void create_picture(const char *filename, int shift_time, bool match_all);
extern void dive_add_picture(struct dive *d, struct picture *newpic);
extern bool dive_remove_picture(struct dive *d, const char *filename);
extern unsigned int dive_get_picture_count(struct dive *d);
extern bool picture_check_valid_time(timestamp_t timestamp, int shift_time);
extern void picture_free(struct picture *picture);

extern bool has_gaschange_event(const struct dive *dive, const struct divecomputer *dc, int idx);
extern int explicit_first_cylinder(const struct dive *dive, const struct divecomputer *dc);
extern int get_depth_at_time(const struct divecomputer *dc, unsigned int time);

extern fraction_t best_o2(depth_t depth, const struct dive *dive);
extern fraction_t best_he(depth_t depth, const struct dive *dive);

extern int get_surface_pressure_in_mbar(const struct dive *dive, bool non_null);
extern int calculate_depth_to_mbar(int depth, pressure_t surface_pressure, int salinity);
extern int depth_to_mbar(int depth, const struct dive *dive);
extern double depth_to_bar(int depth, const struct dive *dive);
extern double depth_to_atm(int depth, const struct dive *dive);
extern int rel_mbar_to_depth(int mbar, const struct dive *dive);
extern int mbar_to_depth(int mbar, const struct dive *dive);
extern depth_t gas_mod(struct gasmix mix, pressure_t po2_limit, const struct dive *dive, int roundto);
extern depth_t gas_mnd(struct gasmix mix, depth_t end, const struct dive *dive, int roundto);

#define SURFACE_THRESHOLD 750 /* somewhat arbitrary: only below 75cm is it really diving */

extern bool autogroup;

struct dive *unregister_dive(int idx);
extern void delete_single_dive(int idx);

extern int run_survey, quit, force_root;

extern struct dive_table dive_table;
extern struct dive displayed_dive;
extern unsigned int dc_number;
extern struct dive *current_dive;
#define current_dc (get_dive_dc(current_dive, dc_number))
#define displayed_dc (get_dive_dc(&displayed_dive, dc_number))

extern struct dive *get_dive(int nr);
extern struct dive *get_dive_from_table(int nr, const struct dive_table *dt);
extern struct dive_site *get_dive_site_for_dive(const struct dive *dive);
extern const char *get_dive_country(const struct dive *dive);
extern const char *get_dive_location(const struct dive *dive);
extern unsigned int number_of_computers(const struct dive *dive);
extern struct divecomputer *get_dive_dc(struct dive *dive, int nr);
extern timestamp_t dive_endtime(const struct dive *dive);

extern struct dive *make_first_dc(const struct dive *d, int dc_number);
extern int count_divecomputers(const struct dive *d);
extern struct dive *clone_delete_divecomputer(const struct dive *d, int dc_number);
void split_divecomputer(const struct dive *src, int num, struct dive **out1, struct dive **out2);

/*
 * Iterate over each dive, with the first parameter being the index
 * iterator variable, and the second one being the dive one.
 *
 * I don't think anybody really wants the index, and we could make
 * it local to the for-loop, but that would make us requires C99.
 */
#define for_each_dive(_i, _x) \
	for ((_i) = 0; ((_x) = get_dive(_i)) != NULL; (_i)++)

#define for_each_dc(_dive, _dc) \
	for (_dc = &_dive->dc; _dc; _dc = _dc->next)

extern struct dive *get_dive_by_uniq_id(int id);
extern int get_idx_by_uniq_id(int id);
extern bool dive_site_has_gps_location(const struct dive_site *ds);
extern int dive_has_gps_location(const struct dive *dive);
extern location_t dive_get_gps_location(const struct dive *d);

extern bool dive_within_time_range(struct dive *dive, timestamp_t when, timestamp_t offset);
extern bool time_during_dive_with_offset(struct dive *dive, timestamp_t when, timestamp_t offset);
struct dive *find_dive_n_near(timestamp_t when, int n, timestamp_t offset);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(const struct divecomputer *a, const struct divecomputer *b);

extern void parse_xml_init(void);
extern int parse_xml_buffer(const char *url, const char *buf, int size, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites, const char **params);
extern void parse_xml_exit(void);
extern void set_filename(const char *filename);

extern int save_dives(const char *filename);
extern int save_dives_logic(const char *filename, bool select_only, bool anonymize);
extern int save_dive(FILE *f, struct dive *dive, bool anonymize);
extern int export_dives_xslt(const char *filename, const bool selected, const int units, const char *export_xslt, bool anonymize);

extern int save_dive_sites_logic(const char *filename, const struct dive_site *sites[], int nr_sites, bool anonymize);

struct membuffer;
extern void save_one_dive_to_mb(struct membuffer *b, struct dive *dive, bool anonymize);

struct user_info {
	char *name;
	char *email;
};

extern void subsurface_user_info(struct user_info *);
extern void subsurface_console_init(void);
extern void subsurface_console_exit(void);
extern bool subsurface_user_is_root(void);

extern timestamp_t get_times();

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

extern struct dive *alloc_dive(void);
extern void free_dive(struct dive *);
extern void free_dive_dcs(struct divecomputer *dc);
extern void record_dive_to_table(struct dive *dive, struct dive_table *table);
extern void record_dive(struct dive *dive);
extern void clear_dive(struct dive *dive);
extern void copy_dive(const struct dive *s, struct dive *d);
extern void selective_copy_dive(const struct dive *s, struct dive *d, struct dive_components what, bool clear);
extern struct dive *move_dive(struct dive *s);

extern void alloc_samples(struct divecomputer *dc, int num);
extern void free_samples(struct divecomputer *dc);
extern struct sample *prepare_sample(struct divecomputer *dc);
extern void finish_sample(struct divecomputer *dc);
extern struct sample *add_sample(const struct sample *sample, int time, struct divecomputer *dc);
extern void add_sample_pressure(struct sample *sample, int sensor, int mbar);
extern int legacy_format_o2pressures(const struct dive *dive, const struct divecomputer *dc);

extern bool dive_less_than(const struct dive *a, const struct dive *b);
extern bool dive_or_trip_less_than(struct dive_or_trip a, struct dive_or_trip b);
extern void sort_dive_table(struct dive_table *table);
extern struct dive *fixup_dive(struct dive *dive);
extern pressure_t calculate_surface_pressure(const struct dive *dive);
extern pressure_t un_fixup_surface_pressure(const struct dive *d);
extern void fixup_dc_duration(struct divecomputer *dc);
extern int dive_getUniqID();
extern unsigned int dc_airtemp(const struct divecomputer *dc);
extern unsigned int dc_watertemp(const struct divecomputer *dc);
extern int split_dive(const struct dive *dive, struct dive **new1, struct dive **new2);
extern int split_dive_at_time(const struct dive *dive, duration_t time, struct dive **new1, struct dive **new2);
extern struct dive *merge_dives(const struct dive *a, const struct dive *b, int offset, bool prefer_downloaded, struct dive_trip **trip, struct dive_site **site);
extern struct dive *try_to_merge(struct dive *a, struct dive *b, bool prefer_downloaded);
extern struct event *clone_event(const struct event *src_ev);
extern void copy_events(const struct divecomputer *s, struct divecomputer *d);
extern void free_events(struct event *ev);
extern void copy_cylinders(const struct dive *s, struct dive *d, bool used_only);
extern void copy_samples(const struct divecomputer *s, struct divecomputer *d);
extern bool is_cylinder_used(const struct dive *dive, int idx);
extern bool is_cylinder_prot(const struct dive *dive, int idx);
extern void fill_default_cylinder(struct dive *dive, int idx);
extern void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int time, int idx);
extern struct event *add_event(struct divecomputer *dc, unsigned int time, int type, int flags, int value, const char *name);
extern void remove_event(struct event *event);
extern void update_event_name(struct dive *d, struct event *event, const char *name);
extern void add_extra_data(struct divecomputer *dc, const char *key, const char *value);
extern void per_cylinder_mean_depth(const struct dive *dive, struct divecomputer *dc, int *mean, int *duration);
extern int get_cylinder_index(const struct dive *dive, const struct event *ev);
extern struct gasmix get_gasmix_from_event(const struct dive *, const struct event *ev);
extern int nr_cylinders(const struct dive *dive);
extern int nr_weightsystems(const struct dive *dive);

/* UI related protopypes */

// extern void report_error(GError* error);

extern void remember_event(const char *eventname);
extern void invalidate_dive_cache(struct dive *dc);

#if WE_DONT_USE_THIS /* this is a missing feature in Qt - selecting which events to display */
extern int evn_foreach(void (*callback)(const char *, bool *, void *), void *data);
#endif /* WE_DONT_USE_THIS */

extern void clear_events(void);

extern void set_dc_nickname(struct dive *dive);
extern void set_autogroup(bool value);
extern int total_weight(const struct dive *);

#define DIVE_ERROR_PARSE 1
#define DIVE_ERROR_PLAN 2

const char *monthname(int mon);

extern const char *existing_filename;
extern void subsurface_command_line_init(int *, char ***);
extern void subsurface_command_line_exit(int *, char ***);

#define FRACTION(n, x) ((unsigned)(n) / (x)), ((unsigned)(n) % (x))

#define DECOTIMESTEP 60 /* seconds. Unit of deco stop times */

extern bool is_dc_planner(const struct divecomputer *dc);
extern bool has_planned(const struct dive *dive, bool planned);

/* Since C doesn't have parameter-based overloading, two versions of get_next_event. */
extern const struct event *get_next_event(const struct event *event, const char *name);
extern struct event *get_next_event_mutable(struct event *event, const char *name);

/* Get gasmixes at increasing timestamps.
 * In "evp", pass a pointer to a "struct event *" which is NULL-initialized on first invocation.
 * On subsequent calls, pass the same "evp" and the "gasmix" from previous calls.
 */
extern struct gasmix get_gasmix(const struct dive *dive, const struct divecomputer *dc, int time, const struct event **evp, struct gasmix gasmix);

/* Get gasmix at a given time */
extern struct gasmix get_gasmix_at_time(const struct dive *dive, const struct divecomputer *dc, duration_t time);

/* these structs holds the information that
 * describes the cylinders / weight systems.
 * they are global variables initialized in equipment.c
 * used to fill the combobox in the add/edit cylinder
 * dialog
 */

extern void set_informational_units(const char *units);
extern void set_git_prefs(const char *prefs);

extern char *get_dive_date_c_string(timestamp_t when);
extern void update_setpoint_events(const struct dive *dive, struct divecomputer *dc);

#ifdef __cplusplus
}

/* Make pointers to dive and dive_trip "Qt metatypes" so that they can be passed through
 * QVariants and through QML.
 */
#include <QObject>
Q_DECLARE_METATYPE(struct dive *);

#endif

#include "pref.h"

#endif // DIVE_H
