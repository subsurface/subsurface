#ifndef DIVE_H
#define DIVE_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <zip.h>
#include <sqlite3.h>
#include <string.h>

/* Windows has no MIN/MAX macros - so let's just roll our own */
#define MIN(x, y) ({                \
	typeof(x) _min1 = (x);          \
	typeof(y) _min2 = (y);          \
	(void) (&_min1 == &_min2);      \
	_min1 < _min2 ? _min1 : _min2; })

#define MAX(x, y) ({                \
	typeof(x) _max1 = (x);          \
	typeof(y) _max2 = (y);          \
	(void) (&_max1 == &_max2);      \
	_max1 > _max2 ? _max1 : _max2; })

#define IS_FP_SAME(_a, _b) (fabs((_a) - (_b)) < 0.000001 * MAX(fabs(_a), fabs(_b)))

static inline int same_string(const char *a, const char *b)
{
	return !strcmp(a ? : "", b ? : "");
}

#include <libxml/tree.h>
#include <libxslt/transform.h>

#include "sha1.h"
#include "units.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

struct gasmix {
	fraction_t o2;
	fraction_t he;
};

typedef struct
{
	volume_t size;
	pressure_t workingpressure;
	const char *description; /* "LP85", "AL72", "AL80", "HP100+" or whatever */
} cylinder_type_t;

typedef struct
{
	cylinder_type_t type;
	struct gasmix gasmix;
	pressure_t start, end, sample_start, sample_end;
	depth_t depth;
	bool used;
} cylinder_t;

typedef struct
{
	weight_t weight;
	const char *description; /* "integrated", "belt", "ankle" */
} weightsystem_t;

extern int get_pressure_units(unsigned int mb, const char **units);
extern double get_depth_units(int mm, int *frac, const char **units);
extern double get_volume_units(unsigned int ml, int *frac, const char **units);
extern double get_temp_units(unsigned int mk, const char **units);
extern double get_weight_units(unsigned int grams, int *frac, const char **units);
extern double get_vertical_speed_units(unsigned int mms, int *frac, const char **units);

extern unsigned int units_to_depth(double depth);

/* Volume in mliter of a cylinder at pressure 'p' */
extern int gas_volume(cylinder_t *cyl, pressure_t p);
extern int wet_volume(double cuft, pressure_t p);

static inline int get_o2(const struct gasmix *mix)
{
	return mix->o2.permille ?: O2_IN_AIR;
}

static inline int get_he(const struct gasmix *mix)
{
	return mix->he.permille;
}

extern void sanitize_gasmix(struct gasmix *mix);
extern int gasmix_distance(const struct gasmix *a, const struct gasmix *b);

static inline bool is_air(int o2, int he)
{
	return (he == 0) && (o2 == 0 || ((o2 >= O2_IN_AIR - 1) && (o2 <= O2_IN_AIR + 1)));
}

/* Linear interpolation between 'a' and 'b', when we are 'part'way into the 'whole' distance from a to b */
static inline int interpolate(int a, int b, int part, int whole)
{
	/* It is doubtful that we actually need floating point for this, but whatever */
	double x = (double)a * (whole - part) + (double)b * part;
	return rint(x / whole);
}

struct sample {
	duration_t time;
	depth_t depth;
	temperature_t temperature;
	pressure_t cylinderpressure;
	int sensor; /* Cylinder pressure sensor index */
	duration_t ndl;
	duration_t stoptime;
	depth_t stopdepth;
	bool in_deco;
	int cns;
	int po2;
	int heartbeat;
	int bearing;
};

struct divetag {
	/*
	 * The name of the divetag. If a translation is available, name contains
	 * the translated tag
	 */
	char *name;
	/*
	 * If a translation is available, we write the original tag to source.
	 * This enables us to write a non-localized tag to the xml file.
	 */
	char *source;
};

struct tag_entry {
	struct divetag *tag;
	struct tag_entry *next;
};

/*
 * divetags are only stored once, each dive only contains
 * a list of tag_entries which then point to the divetags
 * in the global g_tag_list
 */

extern struct tag_entry *g_tag_list;

struct divetag *taglist_add_tag(struct tag_entry **tag_list, const char *tag);

/*
 * Writes all divetags in tag_list to buffer, limited by the buffer's (len)gth.
 * Returns the characters written
 */
int taglist_get_tagstring(struct tag_entry *tag_list, char *buffer, int len);

void taglist_init_global();
void taglist_free(struct tag_entry *tag_list);

/*
 * Events are currently pretty meaningless. This is
 * just based on the random data that libdivecomputer
 * gives us. I'm not sure what a real "architected"
 * event model would actually look like, but right
 * now you can associate a list of events with a dive,
 * and we'll do something about it.
 */
struct event {
	struct event *next;
	duration_t time;
	int type, flags, value;
	bool deleted;
	char name[];
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
	duration_t duration, surfacetime;
	depth_t maxdepth, meandepth;
	temperature_t airtemp, watertemp;
	pressure_t surface_pressure;
	int salinity; // kg per 10000 l
	const char *model;
	uint32_t deviceid, diveid;
	int samples, alloc_samples;
	struct sample *sample;
	struct event *events;
	struct divecomputer *next;
};

#define MAX_CYLINDERS (8)
#define MAX_WEIGHTSYSTEMS (6)
#define W_IDX_PRIMARY 0
#define W_IDX_SECONDARY 1

typedef enum {
	TF_NONE,
	NO_TRIP,
	IN_TRIP,
	ASSIGNED_TRIP,
	NUM_TRIPFLAGS
} tripflag_t;

typedef struct dive_trip
{
	timestamp_t when;
	char *location;
	char *notes;
	struct dive *dives;
	int nrdives;
	int index;
	unsigned expanded : 1, selected : 1, autogen : 1, fixup : 1;
	struct dive_trip *next;
} dive_trip_t;

/* List of dive trips (sorted by date) */
extern dive_trip_t *dive_trip_list;

struct dive {
	int number;
	tripflag_t tripflag;
	dive_trip_t *divetrip;
	struct dive *next, **pprev;
	int selected;
	bool downloaded;
	timestamp_t when;
	char *location;
	char *notes;
	char *divemaster, *buddy;
	int rating;
	degrees_t latitude, longitude;
	int visibility; /* 0 - 5 star rating */
	cylinder_t cylinder[MAX_CYLINDERS];
	weightsystem_t weightsystem[MAX_WEIGHTSYSTEMS];
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
};

static inline int dive_has_gps_location(struct dive *dive)
{
	return dive->latitude.udeg || dive->longitude.udeg;
}

static inline void copy_gps_location(struct dive *from, struct dive *to)
{
	if (from && to) {
		to->latitude.udeg = from->latitude.udeg;
		to->longitude.udeg = from->longitude.udeg;
	}
}

static inline int get_surface_pressure_in_mbar(const struct dive *dive, bool non_null)
{
	int mbar = dive->surface_pressure.mbar;
	if (!mbar && non_null)
		mbar = SURFACE_PRESSURE;
	return mbar;
}

/* Pa = N/m^2 - so we determine the weight (in N) of the mass of 10m
 * of water (and use standard salt water at 1.03kg per liter if we don't know salinity)
 * and add that to the surface pressure (or to 1013 if that's unknown) */
static inline int calculate_depth_to_mbar(int depth, pressure_t surface_pressure, int salinity)
{
	double specific_weight;
	int mbar = surface_pressure.mbar;

	if (!mbar)
		mbar = SURFACE_PRESSURE;
	if (!salinity)
		salinity = SEAWATER_SALINITY;
	specific_weight = salinity / 10000.0 * 0.981;
	mbar += rint(depth / 10.0 * specific_weight);
	return mbar;
}

static inline int depth_to_mbar(int depth, struct dive *dive)
{
	return calculate_depth_to_mbar(depth, dive->surface_pressure, dive->salinity);
}

static inline double depth_to_atm(int depth, struct dive *dive)
{
	return mbar_to_atm(depth_to_mbar(depth, dive));
}

/* for the inverse calculation we use just the relative pressure
 * (that's the one that some dive computers like the Uemis Zurich
 * provide - for the other models that do this libdivecomputer has to
 * take care of this, but the Uemis we support natively */
static inline int rel_mbar_to_depth(int mbar, struct dive *dive)
{
	int cm;
	double specific_weight = 1.03 * 0.981;
	if (dive->dc.salinity)
		specific_weight = dive->dc.salinity / 10000.0 * 0.981;
	/* whole mbar gives us cm precision */
	cm = rint(mbar / specific_weight);
	return cm * 10;
}

#define SURFACE_THRESHOLD 750 /* somewhat arbitrary: only below 75cm is it really diving */

/* this is a global spot for a temporary dive structure that we use to
 * be able to edit a dive without unintended side effects */
extern struct dive edit_dive;

extern short autogroup;
/* random threashold: three days without diving -> new trip
 * this works very well for people who usually dive as part of a trip and don't
 * regularly dive at a local facility; this is why trips are an optional feature */
#define TRIP_THRESHOLD 3600 * 24 * 3

#define UNGROUPED_DIVE(_dive) ((_dive)->tripflag == NO_TRIP)
#define DIVE_IN_TRIP(_dive) ((_dive)->tripflag == IN_TRIP || (_dive)->tripflag == ASSIGNED_TRIP)
#define DIVE_NEEDS_TRIP(_dive) ((_dive)->tripflag == TF_NONE)

extern void add_dive_to_trip(struct dive *, dive_trip_t *);

extern void delete_single_dive(int idx);
extern void add_single_dive(int idx, struct dive *dive);

extern void insert_trip(dive_trip_t **trip);


extern const struct units SI_units, IMPERIAL_units;
extern struct units xml_parsing_units;

extern struct units *get_units(void);
extern int verbose, quit;

struct dive_table {
	int nr, allocated, preexisting;
	struct dive **dives;
};

extern struct dive_table dive_table;

extern int selected_dive;
#define current_dive (get_dive(selected_dive))
#define current_dc (get_dive_dc(current_dive, dc_number))

static inline struct dive *get_gps_location(int nr, struct dive_table *table)
{
	if (nr >= table->nr || nr < 0)
		return NULL;
	return table->dives[nr];
}

static inline struct dive *get_dive(int nr)
{
	if (nr >= dive_table.nr || nr < 0)
		return NULL;
	return dive_table.dives[nr];
}

static inline unsigned int number_of_computers(struct dive *dive)
{
	unsigned int total_number = 0;
	struct divecomputer *dc = &dive->dc;

	if (!dive)
		return 1;

	do {
		total_number++;
		dc = dc->next;
	} while (dc);
	return total_number;
}

static inline struct divecomputer *get_dive_dc(struct dive *dive, int nr)
{
	struct divecomputer *dc = &dive->dc;

	while (nr-- > 0) {
		dc = dc->next;
		if (!dc)
			return &dive->dc;
	}
	return dc;
}

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

#define for_each_gps_location(_i, _x) \
	for ((_i) = 0; ((_x) = get_gps_location(_i, &gps_location_table)) != NULL; (_i)++)

static inline struct dive *get_dive_by_uemis_diveid(uint32_t diveid, uint32_t deviceid)
{
	int i;
	struct dive *dive;

	for_each_dive(i, dive) {
		struct divecomputer *dc = &dive->dc;
		do {
			if (dc->diveid == diveid && dc->deviceid == deviceid)
				return dive;
		} while ((dc = dc->next) != NULL);
	}
	return NULL;
}

static inline struct dive *get_dive_by_diveid(int id)
{
	int i;
	struct dive *dive = NULL;

	for_each_dive(i, dive) {
		if (dive->id == id)
			break;
	}
#ifdef DEBUG
	if(dive == NULL){
		fprintf(stderr, "Invalid id %x passed to get_dive_by_diveid, try to fix the code\n", id);
		exit(1);
	}
#endif
	return dive;
}

#ifdef __cplusplus
extern "C" {
#endif

extern int report_error(const char *fmt, ...);
extern const char *get_error_string(void);

extern struct dive *find_dive_including(timestamp_t when);
extern bool dive_within_time_range(struct dive *dive, timestamp_t when, timestamp_t offset);
struct dive *find_dive_n_near(timestamp_t when, int n, timestamp_t offset);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(struct divecomputer *a, struct divecomputer *b);

extern void parse_xml_init(void);
extern void parse_xml_buffer(const char *url, const char *buf, int size, struct dive_table *table, const char **params);
extern void parse_xml_exit(void);
extern void set_filename(const char *filename, bool force);

extern int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);

extern int parse_file(const char *filename);
extern int parse_csv_file(const char *filename, int time, int depth, int temp, int po2f, int cnsf, int stopdepthf, int sepidx, const char *csvtemplate, int units);
extern int parse_manual_file(const char *filename, int separator_index, int units, int number, int date, int time, int duration, int location, int gps, int maxdepth, int meandepth, int buddy, int notes, int weight, int tags);

extern int save_dives(const char *filename);
extern int save_dives_logic(const char *filename, bool select_only);
extern int save_dive(FILE *f, struct dive *dive);
extern int export_dives_xslt(const char *filename, const bool selected, const char *export_xslt);

struct git_oid;
struct git_repository;
#define dummy_git_repository ((git_repository *) 3ul) /* Random bogus pointer, not NULL */
extern struct git_repository *is_git_repository(const char *filename, const char **branchp);
extern int git_save_dives(struct git_repository *, const char *, bool select_only);
extern int git_load_dives(struct git_repository *, const char *);
extern const char *saved_git_id;
extern void clear_git_id(void);
extern void set_git_id(const struct git_oid *);

struct user_info {
	const char *name;
	const char *email;
};

extern void subsurface_user_info(struct user_info *);
extern int subsurface_rename(const char *path, const char *newpath);
extern int subsurface_open(const char *path, int oflags, mode_t mode);
extern FILE *subsurface_fopen(const char *path, const char *mode);
extern void *subsurface_opendir(const char *path);
extern struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp);
extern int subsurface_zip_close(struct zip *zip);
extern void subsurface_console_init(bool dedicated);
extern void subsurface_console_exit(void);

extern void shift_times(const timestamp_t amount);
extern timestamp_t get_times();

extern xsltStylesheetPtr get_stylesheet(const char *name);

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

extern struct dive *alloc_dive(void);
extern void record_dive(struct dive *dive);

extern struct sample *prepare_sample(struct divecomputer *dc);
extern void finish_sample(struct divecomputer *dc);

extern bool has_hr_data(struct divecomputer *dc);

extern void sort_table(struct dive_table *table);
extern struct dive *fixup_dive(struct dive *dive);
extern int dive_getUniqID(struct dive *d);
extern unsigned int dc_airtemp(struct divecomputer *dc);
extern unsigned int dc_watertemp(struct divecomputer *dc);
extern struct dive *merge_dives(struct dive *a, struct dive *b, int offset, bool prefer_downloaded);
extern struct dive *try_to_merge(struct dive *a, struct dive *b, bool prefer_downloaded);
extern void renumber_dives(int nr);
extern void copy_events(struct dive *s, struct dive *d);
extern void copy_cylinders(struct dive *s, struct dive *d);
extern void copy_samples(struct dive *s, struct dive *d);

extern void fill_default_cylinder(cylinder_t *cyl);
extern void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int time, int idx);
extern void add_event(struct divecomputer *dc, int time, int type, int flags, int value, const char *name);
extern void per_cylinder_mean_depth(struct dive *dive, struct divecomputer *dc, int *mean, int *duration);
extern int get_cylinder_index(struct dive *dive, struct event *ev);
extern int nr_cylinders(struct dive *dive);
extern int nr_weightsystems(struct dive *dive);

/* UI related protopypes */

// extern void report_error(GError* error);

extern void add_cylinder_description(cylinder_type_t *);
extern void add_weightsystem_description(weightsystem_t *);
extern void remember_event(const char *eventname);

#if WE_DONT_USE_THIS /* this is a missing feature in Qt - selecting which events to display */
extern int evn_foreach(void (*callback)(const char *, bool *, void *), void *data);
#endif /* WE_DONT_USE_THIS */

extern void clear_events(void);

extern void set_dc_nickname(struct dive *dive);
extern void set_autogroup(bool value);
extern int total_weight(struct dive *);

#ifdef __cplusplus
}
#endif

#define DIVE_ERROR_PARSE 1
#define DIVE_ERROR_PLAN 2

const char *weekday(int wday);
const char *monthname(int mon);

#define UTF8_DEGREE "\xc2\xb0"
#define UTF8_DELTA "\xce\x94"
#define UTF8_UPWARDS_ARROW "\xE2\x86\x91"
#define UTF8_DOWNWARDS_ARROW "\xE2\x86\x93"
#define UTF8_AVERAGE "\xc3\xb8"
#define UTF8_SUBSCRIPT_2 "\xe2\x82\x82"
#define UTF8_WHITESTAR "\xe2\x98\x86"
#define UTF8_BLACKSTAR "\xe2\x98\x85"

extern const char *existing_filename;
extern void subsurface_command_line_init(int *, char ***);
extern void subsurface_command_line_exit(int *, char ***);

#define FRACTION(n, x) ((unsigned)(n) / (x)), ((unsigned)(n) % (x))

extern double add_segment(double pressure, const struct gasmix *gasmix, int period_in_seconds, int setpoint, const struct dive *dive);
extern void clear_deco(double surface_pressure);
extern void dump_tissues(void);
extern unsigned int deco_allowed_depth(double tissues_tolerance, double surface_pressure, struct dive *dive, bool smooth);
extern void set_gf(short gflow, short gfhigh, bool gf_low_at_maxdepth);
extern void cache_deco_state(double, char **datap);
extern double restore_deco_state(char *data);

struct divedatapoint {
	int time;
	unsigned int depth;
	int o2;
	int he;
	int po2;
	bool entered;
	struct divedatapoint *next;
};

struct diveplan {
	timestamp_t when;
	int lastdive_nr;
	int surface_pressure; /* mbar */
	int bottomsac;	/* ml/min */
	int decosac;	  /* ml/min */
	short gflow;
	short gfhigh;
	struct divedatapoint *dp;
};

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he, int po2, bool entered);
void get_gas_string(int o2, int he, char *buf, int len);
struct divedatapoint *create_dp(int time_incr, int depth, int o2, int he, int po2);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
void plan(struct diveplan *diveplan, char **cached_datap, struct dive **divep, bool add_deco);
void delete_single_dive(int idx);

struct event *get_next_event(struct event *event, char *name);


/* these structs holds the information that
 * describes the cylinders / weight systems.
 * they are global variables initialized in equipment.c
 * used to fill the combobox in the add/edit cylinder
 * dialog
 */

struct tank_info_t {
	const char *name;
	int cuft, ml, psi, bar;
};
extern struct tank_info_t tank_info[100];

struct ws_info_t {
	const char *name;
	int grams;
};
extern struct ws_info_t ws_info[100];

extern bool cylinder_nodata(cylinder_t *cyl);
extern bool cylinder_none(void *_data);
extern bool weightsystem_none(void *_data);
extern bool no_weightsystems(weightsystem_t *ws);
extern bool weightsystems_equal(weightsystem_t *ws1, weightsystem_t *ws2);
extern void remove_cylinder(struct dive *dive, int idx);
extern void remove_weightsystem(struct dive *dive, int idx);

/*
 * String handling.
 */
#define STRTOD_NO_SIGN 0x01
#define STRTOD_NO_DOT 0x02
#define STRTOD_NO_COMMA 0x04
#define STRTOD_NO_EXPONENT 0x08
extern double strtod_flags(const char *str, const char **ptr, unsigned int flags);

#define STRTOD_ASCII (STRTOD_NO_COMMA)

#define ascii_strtod(str, ptr) strtod_flags(str, ptr, STRTOD_ASCII)

extern void set_save_userid_local(short value);
extern void set_userid(char* user_id);

#ifdef __cplusplus
}
#endif

extern weight_t string_to_weight(const char *str);
extern depth_t string_to_depth(const char *str);
extern pressure_t string_to_pressure(const char *str);
extern volume_t string_to_volume(const char *str, pressure_t workp);
extern fraction_t string_to_fraction(const char *str);
extern int average_depth(struct diveplan *dive);

#include "pref.h"

#endif // DIVE_H
