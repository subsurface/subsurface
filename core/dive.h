// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_H
#define DIVE_H

// dive and dive computer related structures and helpers

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <zip.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>
#include "divesite.h"
#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "sha1.h"
#include "units.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int last_xml_version;

enum divemode_t {OC, CCR, PSCR, FREEDIVE, NUM_DIVEMODE, UNDEF_COMP_TYPE};	// Flags (Open-circuit and Closed-circuit-rebreather) for setting dive computer type
enum cylinderuse {OC_GAS, DILUENT, OXYGEN, NOT_USED, NUM_GAS_USE}; // The different uses for cylinders

extern const char *cylinderuse_text[];
extern const char *divemode_text_ui[];
extern const char *divemode_text[];

// o2 == 0 && he == 0 -> air
// o2 < 0 -> invalid
struct gasmix {
	fraction_t o2;
	fraction_t he;
};
static const struct gasmix gasmix_invalid = { { -1 }, { -1 } };
static const struct gasmix gasmix_air = { { 0 }, { 0 } };

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
	bool manually_added;
	volume_t gas_used;
	volume_t deco_gas_used;
	enum cylinderuse cylinder_use;
	bool bestmix_o2;
	bool bestmix_he;
} cylinder_t;

typedef struct
{
	weight_t weight;
	const char *description; /* "integrated", "belt", "ankle" */
} weightsystem_t;

struct icd_data { // This structure provides communication between function isobaric_counterdiffusion() and the calling software.
	int dN2;      // The change in fraction (permille) of nitrogen during the change
	int dHe;      // The change in fraction (permille) of helium during the change
};

extern bool isobaric_counterdiffusion(struct gasmix oldgasmix, struct gasmix newgasmix, struct icd_data *results);

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

extern int get_pressure_units(int mb, const char **units);
extern double get_depth_units(int mm, int *frac, const char **units);
extern double get_volume_units(unsigned int ml, int *frac, const char **units);
extern double get_temp_units(unsigned int mk, const char **units);
extern double get_weight_units(unsigned int grams, int *frac, const char **units);
extern double get_vertical_speed_units(unsigned int mms, int *frac, const char **units);

extern depth_t units_to_depth(double depth);
extern int units_to_sac(double volume);

/* Volume in mliter of a cylinder at pressure 'p' */
extern int gas_volume(const cylinder_t *cyl, pressure_t p);
extern double gas_compressibility_factor(struct gasmix gas, double bar);
extern double isothermal_pressure(struct gasmix gas, double p1, int volume1, int volume2);
extern double gas_density(struct gasmix gas, int pressure);
extern int same_gasmix(struct gasmix a, struct gasmix b);

static inline int get_o2(struct gasmix mix)
{
	return mix.o2.permille ?: O2_IN_AIR;
}

static inline int get_he(struct gasmix mix)
{
	return mix.he.permille;
}

struct gas_pressures {
	double o2, n2, he;
};

extern void fill_pressures(struct gas_pressures *pressures, const double amb_pressure, struct gasmix mix, double po2, enum divemode_t dctype);

extern void sanitize_gasmix(struct gasmix *mix);
extern int gasmix_distance(struct gasmix a, struct gasmix b);
extern int find_best_gasmix_match(struct gasmix mix, const cylinder_t array[], unsigned int used);

extern bool gasmix_is_air(struct gasmix gasmix);

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

void get_gas_string(struct gasmix gasmix, char *text, int len);
const char *gasname(struct gasmix gasmix);

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
	temperature_t temperature;        // uint32_t   4  mdegrK   (0-4 MdegK)            ambient temperature
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
struct tag_entry *taglist_added(struct tag_entry *original_list, struct tag_entry *new_list);
void dump_taglist(const char *intro, struct tag_entry *tl);

/*
 * Writes all divetags form tag_list into internally allocated buffer
 * Function returns pointer to allocated buffer
 * Buffer contains comma separated list of tags names or null terminated string
 *
 * NOTE! The returned buffer must be freed once used.
 */
char *taglist_get_tagstring(struct tag_entry *tag_list);

/* cleans up a list: removes empty tags and duplicates */
void taglist_cleanup(struct tag_entry **tag_list);

void taglist_init_global();
void taglist_free(struct tag_entry *tag_list);

bool taglist_contains(struct tag_entry *tag_list, const char *tag);
int count_dives_with_tag(const char *tag);
int count_dives_with_person(const char *person);
int count_dives_with_location(const char *location);
int count_dives_with_suit(const char *suit);

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

#define MAX_CYLINDERS (20)
#define MAX_WEIGHTSYSTEMS (6)
#define MAX_TANK_INFO (100)
#define MAX_WS_INFO (100)
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
	/* Used by the io-routines to mark trips that have already been written. */
	bool saved;
	unsigned expanded : 1, selected : 1, autogen : 1, fixup : 1;
	struct dive_trip *next;
} dive_trip_t;

/* List of dive trips (sorted by date) */
extern dive_trip_t *dive_trip_list;
struct picture;
struct dive {
	int number;
	tripflag_t tripflag;
	dive_trip_t *divetrip;
	struct dive *next, **pprev;
	bool selected;
	bool hidden_by_filter;
	bool downloaded;
	timestamp_t when;
	uint32_t dive_site_uuid;
	char *notes;
	char *divemaster, *buddy;
	int rating;
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
	struct picture *picture_list;
	unsigned char git_id[20];
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
	degrees_t latitude;
	degrees_t longitude;
	struct picture *next;
};

#define FOR_EACH_PICTURE(_dive) \
	if (_dive)              \
		for (struct picture *picture = (_dive)->picture_list; picture; picture = picture->next)

#define FOR_EACH_PICTURE_NON_PTR(_divestruct) \
	for (struct picture *picture = (_divestruct).picture_list; picture; picture = picture->next)

extern struct picture *alloc_picture();
extern bool dive_check_picture_time(const struct dive *d, int shift_time, timestamp_t timestamp);
extern void dive_create_picture(struct dive *d, const char *filename, int shift_time, bool match_all);
extern void dive_add_picture(struct dive *d, struct picture *newpic);
extern bool dive_remove_picture(struct dive *d, const char *filename);
extern unsigned int dive_get_picture_count(struct dive *d);
extern bool picture_check_valid(const char *filename, int shift_time);
extern void dive_set_geodata_from_picture(struct dive *d, struct picture *pic);
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
extern struct dive_trip *clone_empty_trip(struct dive_trip *trip);


extern const struct units SI_units, IMPERIAL_units;
extern struct units xml_parsing_units;

extern const struct units *get_units(void);
extern int run_survey, verbose, quit, force_root;

struct dive_table {
	int nr, allocated, preexisting;
	struct dive **dives;
};

extern struct dive_table dive_table, downloadTable;
extern struct dive displayed_dive;
extern struct dive_site displayed_dive_site;
extern int selected_dive;
extern unsigned int dc_number;
#define current_dive (get_dive(selected_dive))
#define current_dc (get_dive_dc(current_dive, dc_number))
#define displayed_dc (get_dive_dc(&displayed_dive, dc_number))

extern struct dive *get_dive(int nr);
extern struct dive *get_dive_from_table(int nr, struct dive_table *dt);
extern struct dive_site *get_dive_site_for_dive(const struct dive *dive);
extern const char *get_dive_country(const struct dive *dive);
extern const char *get_dive_location(const struct dive *dive);
extern unsigned int number_of_computers(const struct dive *dive);
extern struct divecomputer *get_dive_dc(struct dive *dive, int nr);
extern timestamp_t dive_endtime(const struct dive *dive);

extern void make_first_dc(void);
extern unsigned int count_divecomputers(void);
extern void delete_current_divecomputer(void);

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

#ifdef __cplusplus
extern "C" {
#endif

extern struct dive *get_dive_by_uniq_id(int id);
extern int get_idx_by_uniq_id(int id);
extern bool dive_site_has_gps_location(const struct dive_site *ds);
extern int dive_has_gps_location(const struct dive *dive);

extern int report_error(const char *fmt, ...);
extern void set_error_cb(void(*cb)(char *));	// Callback takes ownership of passed string

extern struct dive *find_dive_including(timestamp_t when);
extern bool dive_within_time_range(struct dive *dive, timestamp_t when, timestamp_t offset);
extern bool time_during_dive_with_offset(struct dive *dive, timestamp_t when, timestamp_t offset);
struct dive *find_dive_n_near(timestamp_t when, int n, timestamp_t offset);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(const struct divecomputer *a, const struct divecomputer *b);

extern void parse_xml_init(void);
extern int parse_xml_buffer(const char *url, const char *buf, int size, struct dive_table *table, const char **params);
extern void parse_xml_exit(void);
extern void set_filename(const char *filename);

extern int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_dm5_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_cobalt_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_divinglog_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_dlf_buffer(unsigned char *buffer, size_t size, struct dive_table *table);

extern int parse_file(const char *filename, struct dive_table *table);
extern int parse_csv_file(const char *filename, char **params, int pnr, const char *csvtemplate);
extern int parse_seabear_log(const char *filename);
extern int parse_txt_file(const char *filename, const char *csv);
extern int parse_manual_file(const char *filename, char **params, int pnr);
extern int save_dives(const char *filename);
extern int save_dives_logic(const char *filename, bool select_only);
extern int save_dive(FILE *f, struct dive *dive);
extern int export_dives_xslt(const char *filename, const bool selected, const int units, const char *export_xslt);

struct membuffer;
extern void save_one_dive_to_mb(struct membuffer *b, struct dive *dive);

int cylinderuse_from_text(const char *text);


struct user_info {
	char *name;
	char *email;
};

extern void subsurface_user_info(struct user_info *);
extern int subsurface_rename(const char *path, const char *newpath);
extern int subsurface_dir_rename(const char *path, const char *newpath);
extern int subsurface_open(const char *path, int oflags, mode_t mode);
extern FILE *subsurface_fopen(const char *path, const char *mode);
extern void *subsurface_opendir(const char *path);
extern int subsurface_access(const char *path, int mode);
extern int subsurface_stat(const char* path, struct stat* buf);
extern struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp);
extern int subsurface_zip_close(struct zip *zip);
extern void subsurface_console_init(void);
extern void subsurface_console_exit(void);
extern bool subsurface_user_is_root(void);

extern timestamp_t get_times();

extern xsltStylesheetPtr get_stylesheet(const char *name);

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

extern struct dive *alloc_dive(void);
extern void record_dive_to_table(struct dive *dive, struct dive_table *table);
extern void record_dive(struct dive *dive);
extern void clear_dive(struct dive *dive);
extern void copy_dive(const struct dive *s, struct dive *d);
extern void selective_copy_dive(const struct dive *s, struct dive *d, struct dive_components what, bool clear);
extern struct dive *clone_dive(struct dive *s);

extern void clear_table(struct dive_table *table);

extern void alloc_samples(struct divecomputer *dc, int num);
extern void free_samples(struct divecomputer *dc);
extern struct sample *prepare_sample(struct divecomputer *dc);
extern void finish_sample(struct divecomputer *dc);
extern struct sample *add_sample(const struct sample *sample, int time, struct divecomputer *dc);
extern void add_sample_pressure(struct sample *sample, int sensor, int mbar);
extern int legacy_format_o2pressures(const struct dive *dive, const struct divecomputer *dc);

extern void sort_table(struct dive_table *table);
extern struct dive *fixup_dive(struct dive *dive);
extern void fixup_dc_duration(struct divecomputer *dc);
extern int dive_getUniqID();
extern unsigned int dc_airtemp(const struct divecomputer *dc);
extern unsigned int dc_watertemp(const struct divecomputer *dc);
extern int split_dive(struct dive *);
extern void split_dive_at_time(struct dive *dive, duration_t time);
extern struct dive *merge_dives(struct dive *a, struct dive *b, int offset, bool prefer_downloaded);
extern struct dive *try_to_merge(struct dive *a, struct dive *b, bool prefer_downloaded);
extern struct event *clone_event(const struct event *src_ev);
extern void copy_events(const struct divecomputer *s, struct divecomputer *d);
extern void free_events(struct event *ev);
extern void copy_cylinders(const struct dive *s, struct dive *d, bool used_only);
extern void copy_samples(const struct divecomputer *s, struct divecomputer *d);
extern bool is_cylinder_used(const struct dive *dive, int idx);
extern bool is_cylinder_prot(const struct dive *dive, int idx);
extern void fill_default_cylinder(cylinder_t *cyl);
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

extern void add_cylinder_description(cylinder_type_t *);
extern void add_weightsystem_description(weightsystem_t *);
extern void remember_event(const char *eventname);
extern void invalidate_dive_cache(struct dive *dc);

#if WE_DONT_USE_THIS /* this is a missing feature in Qt - selecting which events to display */
extern int evn_foreach(void (*callback)(const char *, bool *, void *), void *data);
#endif /* WE_DONT_USE_THIS */

extern void clear_events(void);

extern void set_dc_nickname(struct dive *dive);
extern void set_autogroup(bool value);
extern int total_weight(const struct dive *);

#ifdef __cplusplus
}
#endif

#define DIVE_ERROR_PARSE 1
#define DIVE_ERROR_PLAN 2

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

#define DECOTIMESTEP 60 /* seconds. Unit of deco stop times */

struct deco_state {
	double tissue_n2_sat[16];
	double tissue_he_sat[16];
	double tolerated_by_tissue[16];
	double tissue_inertgas_saturation[16];
	double buehlmann_inertgas_a[16];
	double buehlmann_inertgas_b[16];

	double max_n2_crushing_pressure[16];
	double max_he_crushing_pressure[16];

	double crushing_onset_tension[16];            // total inert gas tension in the t* moment
	double n2_regen_radius[16];                   // rs
	double he_regen_radius[16];
	double max_ambient_pressure;                  // last moment we were descending

	double bottom_n2_gradient[16];
	double bottom_he_gradient[16];

	double initial_n2_gradient[16];
	double initial_he_gradient[16];
	pressure_t first_ceiling_pressure;
	pressure_t max_bottom_ceiling_pressure;
	int ci_pointing_to_guiding_tissue;
	double gf_low_pressure_this_dive;
	int deco_time;
	bool icd_warning;
};

extern void add_segment(struct deco_state *ds, double pressure, struct gasmix gasmix, int period_in_seconds, int setpoint, enum divemode_t divemode, int sac);
extern void clear_deco(struct deco_state *ds, double surface_pressure);
extern void dump_tissues(struct deco_state *ds);
extern void set_gf(short gflow, short gfhigh);
extern void set_vpmb_conservatism(short conservatism);
extern void cache_deco_state(struct deco_state *source, struct deco_state **datap);
extern void restore_deco_state(struct deco_state *data, struct deco_state *target, bool keep_vpmb_state);
extern void nuclear_regeneration(struct deco_state *ds, double time);
extern void vpmb_start_gradient(struct deco_state *ds);
extern void vpmb_next_gradient(struct deco_state *ds, double deco_time, double surface_pressure);
extern double tissue_tolerance_calc(struct deco_state *ds, const struct dive *dive, double pressure);

/* this should be converted to use our types */
struct divedatapoint {
	int time;
	depth_t depth;
	int cylinderid;
	pressure_t minimum_gas;
	int setpoint;
	bool entered;
	struct divedatapoint *next;
	enum divemode_t divemode;
};

struct diveplan {
	timestamp_t when;
	int surface_pressure; /* mbar */
	int bottomsac;	/* ml/min */
	int decosac;	  /* ml/min */
	int salinity;
	short gflow;
	short gfhigh;
	short vpmb_conservatism;
	struct divedatapoint *dp;
	int eff_gflow, eff_gfhigh;
	int surface_interval;
};

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int cylinderid, int po2, bool entered, enum divemode_t divemode);
struct divedatapoint *create_dp(int time_incr, int depth, int cylinderid, int po2);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
struct decostop {
	int depth;
	int time;
};
extern bool plan(struct deco_state *ds, struct diveplan *diveplan, struct dive *dive, int timestep, struct decostop *decostoptable, struct deco_state **cached_datap, bool is_planner, bool show_disclaimer);
extern void calc_crushing_pressure(struct deco_state *ds, double pressure);
extern void vpmb_start_gradient(struct deco_state *ds);
extern void clear_vpmb_state(struct deco_state *ds);
extern void printdecotable(struct decostop *table);

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

struct tank_info_t {
	const char *name;
	int cuft, ml, psi, bar;
};
extern struct tank_info_t tank_info[MAX_TANK_INFO];

struct ws_info_t {
	const char *name;
	int grams;
};
extern struct ws_info_t ws_info[MAX_WS_INFO];

extern bool cylinder_nodata(const cylinder_t *cyl);
extern bool cylinder_none(const cylinder_t *cyl);
extern bool weightsystem_none(const weightsystem_t *ws);
extern void remove_cylinder(struct dive *dive, int idx);
extern void remove_weightsystem(struct dive *dive, int idx);
extern void reset_cylinders(struct dive *dive, bool track_gas);
#ifdef DEBUG_CYL
extern void dump_cylinders(struct dive *dive, bool verbose);
#endif

extern void set_informational_units(const char *units);
extern void set_git_prefs(const char *prefs);

extern char *get_dive_date_c_string(timestamp_t when);
extern void update_setpoint_events(const struct dive *dive, struct divecomputer *dc);

#ifdef __cplusplus
}
#endif

extern weight_t string_to_weight(const char *str);
extern depth_t string_to_depth(const char *str);
extern pressure_t string_to_pressure(const char *str);
extern volume_t string_to_volume(const char *str, pressure_t workp);
extern fraction_t string_to_fraction(const char *str);
extern void average_max_depth(struct diveplan *dive, int *avg_depth, int *max_depth);

#include "pref.h"

#endif // DIVE_H
