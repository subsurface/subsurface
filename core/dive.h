// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_H
#define DIVE_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <zip.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>
#include "divesite.h"

/* Windows has no MIN/MAX macros - so let's just roll our own */
#define MIN(x, y) ({                \
	__typeof__(x) _min1 = (x);          \
	__typeof__(y) _min2 = (y);          \
	(void) (&_min1 == &_min2);      \
	_min1 < _min2 ? _min1 : _min2; })

#define MAX(x, y) ({                \
	__typeof__(x) _max1 = (x);          \
	__typeof__(y) _max2 = (y);          \
	(void) (&_max1 == &_max2);      \
	_max1 > _max2 ? _max1 : _max2; })

#define IS_FP_SAME(_a, _b) (fabs((_a) - (_b)) <= 0.000001 * MAX(fabs(_a), fabs(_b)))

static inline int same_string(const char *a, const char *b)
{
	return !strcmp(a ?: "", b ?: "");
}

static inline int same_string_caseinsensitive(const char *a, const char *b)
{
	return !strcasecmp(a ?: "", b ?: "");
}

static inline int includes_string_caseinsensitive(const char *haystack, const char *needle)
{
	if (!needle)
		return 1; /* every string includes the NULL string */
	if (!haystack)
		return 0; /* nothing is included in the NULL string */
	int len = strlen(needle);
	while (*haystack) {
		if (strncasecmp(haystack, needle, len))
			return 1;
		haystack++;
	}
	return 0;
}

static inline char *copy_string(const char *s)
{
	return (s && *s) ? strdup(s) : NULL;
}

#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "sha1.h"
#include "units.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

extern int last_xml_version;

enum dive_comp_type {OC, CCR, PSCR, FREEDIVE, NUM_DC_TYPE};	// Flags (Open-circuit and Closed-circuit-rebreather) for setting dive computer type
enum cylinderuse {OC_GAS, DILUENT, OXYGEN, NOT_USED, NUM_GAS_USE}; // The different uses for cylinders

extern const char *cylinderuse_text[];
extern const char *divemode_text[];
struct git_state;

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
		/*
		 * Currently only for gas switch events.
		 *
		 * NOTE! The index may be -1, which means "unknown". In that
		 * case, the get_cylinder_index() function will give the best
		 * match with the cylinders in the dive based on gasmix.
		 */
		struct {
			int index;
			struct gasmix mix;
		} gas;
	};
	bool deleted;
	char name[];
};

extern int event_is_gaschange(struct event *ev);

extern int get_pressure_units(int mb, const char **units);
extern double get_depth_units(int mm, int *frac, const char **units);
extern double get_volume_units(unsigned int ml, int *frac, const char **units);
extern double get_temp_units(unsigned int mk, const char **units);
extern double get_weight_units(unsigned int grams, int *frac, const char **units);
extern double get_vertical_speed_units(unsigned int mms, int *frac, const char **units);

extern depth_t units_to_depth(double depth);
extern int units_to_sac(double volume);

/* Volume in mliter of a cylinder at pressure 'p' */
extern int gas_volume(cylinder_t *cyl, pressure_t p);
extern double gas_compressibility_factor(struct gasmix *gas, double bar);
extern double isothermal_pressure(struct gasmix *gas, double p1, int volume1, int volume2);
extern double gas_density(struct gasmix *gas, int pressure);
extern int same_gasmix(struct gasmix *a, struct gasmix *b);

static inline int get_o2(const struct gasmix *mix)
{
	return mix->o2.permille ?: O2_IN_AIR;
}

static inline int get_he(const struct gasmix *mix)
{
	return mix->he.permille;
}

struct gas_pressures {
	double o2, n2, he;
};

extern void fill_pressures(struct gas_pressures *pressures, const double amb_pressure, const struct gasmix *mix, double po2, enum dive_comp_type dctype);

extern void sanitize_gasmix(struct gasmix *mix);
extern int gasmix_distance(const struct gasmix *a, const struct gasmix *b);
extern int find_best_gasmix_match(struct gasmix *mix, cylinder_t array[], unsigned int used);

static inline bool gasmix_is_air(const struct gasmix *gasmix)
{
	int o2 = gasmix->o2.permille;
	int he = gasmix->he.permille;
	return (he == 0) && (o2 == 0 || ((o2 >= O2_IN_AIR - 1) && (o2 <= O2_IN_AIR + 1)));
}

/* Linear interpolation between 'a' and 'b', when we are 'part'way into the 'whole' distance from a to b */
static inline int interpolate(int a, int b, int part, int whole)
{
	/* It is doubtful that we actually need floating point for this, but whatever */
	if (whole) {
		double x = (double)a * (whole - part) + (double)b * part;
		return lrint(x / whole);
	}
	return (a+b)/2;
}

void get_gas_string(const struct gasmix *gasmix, char *text, int len);
const char *gasname(const struct gasmix *gasmix);

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
	temperature_t temperature;        // int32_t    4  mdegrK   (0-2 MdegK)            ambient temperature
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
 * Writes all divetags in tag_list to buffer, limited by the buffer's (len)gth.
 * Returns the characters written
 */
int taglist_get_tagstring(struct tag_entry *tag_list, char *buffer, int len);

/* cleans up a list: removes empty tags and duplicates */
void taglist_cleanup(struct tag_entry **tag_list);

void taglist_init_global();
void taglist_free(struct tag_entry *tag_list);

bool taglist_contains(struct tag_entry *tag_list, const char *tag);
bool taglist_equal(struct tag_entry *tl1, struct tag_entry *tl2);
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
	duration_t duration, surfacetime;
	depth_t maxdepth, meandepth;
	temperature_t airtemp, watertemp;
	pressure_t surface_pressure;
	enum dive_comp_type divemode;	// dive computer type: OC(default) or CCR
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

static inline void invalidate_dive_cache(struct dive *dive)
{
	memset(dive->git_id, 0, 20);
}

static inline bool dive_cache_is_valid(const struct dive *dive)
{
	static const unsigned char null_id[20] = { 0, };
	return !!memcmp(dive->git_id, null_id, 20);
}

extern int get_cylinder_idx_by_use(struct dive *dive, enum cylinderuse cylinder_use_type);
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

/* picture list and methods related to dive picture handling */
struct picture {
	char *filename;
	char *hash;
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
extern struct picture *clone_picture(struct picture *src);
extern bool dive_check_picture_time(struct dive *d, int shift_time, timestamp_t timestamp);
extern void dive_create_picture(struct dive *d, const char *filename, int shift_time, bool match_all);
extern void dive_add_picture(struct dive *d, struct picture *newpic);
extern void dive_remove_picture(char *filename);
extern unsigned int dive_get_picture_count(struct dive *d);
extern bool picture_check_valid(const char *filename, int shift_time);
extern void picture_load_exif_data(struct picture *p);
extern timestamp_t picture_get_timestamp(const char *filename);
extern void dive_set_geodata_from_picture(struct dive *d, struct picture *pic);
extern void picture_free(struct picture *picture);

extern bool has_gaschange_event(struct dive *dive, struct divecomputer *dc, int idx);
extern int explicit_first_cylinder(struct dive *dive, struct divecomputer *dc);
extern int get_depth_at_time(struct divecomputer *dc, unsigned int time);

extern fraction_t best_o2(depth_t depth, struct dive *dive);
extern fraction_t best_he(depth_t depth, struct dive *dive);

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
	if (salinity < 500)
		salinity += FRESHWATER_SALINITY;
	specific_weight = salinity / 10000.0 * 0.981;
	mbar += lrint(depth / 10.0 * specific_weight);
	return mbar;
}

static inline int depth_to_mbar(int depth, struct dive *dive)
{
	return calculate_depth_to_mbar(depth, dive->surface_pressure, dive->salinity);
}

static inline double depth_to_bar(int depth, struct dive *dive)
{
	return depth_to_mbar(depth, dive) / 1000.0;
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
	cm = lrint(mbar / specific_weight);
	return cm * 10;
}

static inline int mbar_to_depth(int mbar, struct dive *dive)
{
	pressure_t surface_pressure;
	if (dive->surface_pressure.mbar)
		surface_pressure = dive->surface_pressure;
	else
		surface_pressure.mbar = SURFACE_PRESSURE;
	return rel_mbar_to_depth(mbar - surface_pressure.mbar, dive);
}

/* MOD rounded to multiples of roundto mm */
static inline depth_t gas_mod(struct gasmix *mix, pressure_t po2_limit, struct dive *dive, int roundto) {
	depth_t rounded_depth;

	double depth = (double) mbar_to_depth(po2_limit.mbar * 1000 / get_o2(mix), dive);
	rounded_depth.mm = lrint(depth / roundto) * roundto;
	return rounded_depth;
}

/* Maximum narcotic depth rounded to multiples of roundto mm */
static inline depth_t gas_mnd(struct gasmix *mix, depth_t end, struct dive *dive, int roundto) {
	depth_t rounded_depth;
	pressure_t ppo2n2;
	ppo2n2.mbar = depth_to_mbar(end.mm, dive);

	int maxambient = lrint(ppo2n2.mbar / (1 - get_he(mix) / 1000.0));
	rounded_depth.mm = lrint(((double)mbar_to_depth(maxambient, dive)) / roundto) * roundto;
	return rounded_depth;
}

#define SURFACE_THRESHOLD 750 /* somewhat arbitrary: only below 75cm is it really diving */

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

static inline struct dive *get_dive(int nr)
{
	if (nr >= dive_table.nr || nr < 0)
		return NULL;
	return dive_table.dives[nr];
}

static inline struct dive *get_dive_from_table(int nr, struct dive_table *dt)
{
	if (nr >= dt->nr || nr < 0)
		return NULL;
	return dt->dives[nr];
}

static inline struct dive_site *get_dive_site_for_dive(struct dive *dive)
{
	if (dive)
		return get_dive_site_by_uuid(dive->dive_site_uuid);
	return NULL;
}

static inline const char *get_dive_country(struct dive *dive)
{
	struct dive_site *ds = get_dive_site_by_uuid(dive->dive_site_uuid);
	if (ds) {
		int idx = taxonomy_index_for_category(&ds->taxonomy, TC_COUNTRY);
		if (idx >= 0)
			return ds->taxonomy.category[idx].value;
	}
	return NULL;
}

static inline char *get_dive_location(struct dive *dive)
{
	struct dive_site *ds = get_dive_site_by_uuid(dive->dive_site_uuid);
	if (ds && ds->name)
		return ds->name;
	return NULL;
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
	struct divecomputer *dc;
	if (!dive)
		return NULL;
	dc = &dive->dc;

	while (nr-- > 0) {
		dc = dc->next;
		if (!dc)
			return &dive->dc;
	}
	return dc;
}

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

static inline struct dive *get_dive_by_uniq_id(int id)
{
	int i;
	struct dive *dive = NULL;

	for_each_dive (i, dive) {
		if (dive->id == id)
			break;
	}
#ifdef DEBUG
	if (dive == NULL) {
		fprintf(stderr, "Invalid id %x passed to get_dive_by_diveid, try to fix the code\n", id);
		exit(1);
	}
#endif
	return dive;
}

static inline int get_idx_by_uniq_id(int id)
{
	int i;
	struct dive *dive = NULL;

	for_each_dive (i, dive) {
		if (dive->id == id)
			break;
	}
#ifdef DEBUG
	if (dive == NULL) {
		fprintf(stderr, "Invalid id %x passed to get_dive_by_diveid, try to fix the code\n", id);
		exit(1);
	}
#endif
	return i;
}

static inline bool dive_site_has_gps_location(struct dive_site *ds)
{
	return ds && (ds->latitude.udeg || ds->longitude.udeg);
}

static inline int dive_has_gps_location(struct dive *dive)
{
	if (!dive)
		return false;
	return dive_site_has_gps_location(get_dive_site_by_uuid(dive->dive_site_uuid));
}

#ifdef __cplusplus
extern "C" {
#endif

extern int report_error(const char *fmt, ...);
extern void report_message(const char *msg);
extern const char *get_error_string(void);
extern void set_error_cb(void(*cb)(void));

extern struct dive *find_dive_including(timestamp_t when);
extern bool dive_within_time_range(struct dive *dive, timestamp_t when, timestamp_t offset);
extern bool time_during_dive_with_offset(struct dive *dive, timestamp_t when, timestamp_t offset);
struct dive *find_dive_n_near(timestamp_t when, int n, timestamp_t offset);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(struct divecomputer *a, struct divecomputer *b);

extern void parse_xml_init(void);
extern int parse_xml_buffer(const char *url, const char *buf, int size, struct dive_table *table, const char **params);
extern void parse_xml_exit(void);

extern int parse_dm4_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_dm5_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_shearwater_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_cobalt_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_divinglog_buffer(sqlite3 *handle, const char *url, const char *buf, int size, struct dive_table *table);
extern int parse_dlf_buffer(unsigned char *buffer, size_t size);

extern int parse_file(const char *filename);
extern int parse_file_git(struct git_state *);
extern int parse_csv_file(const char *filename, char **params, int pnr, const char *csvtemplate);
extern int parse_seabear_log(const char *filename);
extern int parse_seabear_csv_file(const char *filename, char **params, int pnr, const char *csvtemplate);
extern int parse_txt_file(const char *filename, const char *csv);
extern int parse_manual_file(const char *filename, char **params, int pnr);
extern int save_dives_file(const char *filename);
extern int save_dives_git(struct git_state *);
extern int save_dives_logic(const char *filename, bool select_only);
extern int save_dive(FILE *f, struct dive *dive);
extern int export_dives_xslt(const char *filename, const bool selected, const int units, const char *export_xslt);

struct membuffer;
extern void save_one_dive_to_mb(struct membuffer *b, struct dive *dive);

int cylinderuse_from_text(const char *text);


struct user_info {
	const char *name;
	const char *email;
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

extern void shift_times(const timestamp_t amount);
extern timestamp_t get_times();

extern xsltStylesheetPtr get_stylesheet(const char *name);

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

extern struct dive *alloc_dive(void);
extern void record_dive_to_table(struct dive *dive, struct dive_table *table);
extern void record_dive(struct dive *dive);
extern void clear_dive(struct dive *dive);
extern void copy_dive(struct dive *s, struct dive *d);
extern void selective_copy_dive(struct dive *s, struct dive *d, struct dive_components what, bool clear);
extern struct dive *clone_dive(struct dive *s);

extern void clear_table(struct dive_table *table);

extern struct sample *prepare_sample(struct divecomputer *dc);
extern void finish_sample(struct divecomputer *dc);
extern void add_sample_pressure(struct sample *sample, int sensor, int mbar);
extern int legacy_format_o2pressures(struct dive *dive, struct divecomputer *dc);

extern bool has_hr_data(struct divecomputer *dc);

extern void sort_table(struct dive_table *table);
extern struct dive *fixup_dive(struct dive *dive);
extern void fixup_dc_duration(struct divecomputer *dc);
extern int dive_getUniqID(struct dive *d);
extern unsigned int dc_airtemp(struct divecomputer *dc);
extern unsigned int dc_watertemp(struct divecomputer *dc);
extern int split_dive(struct dive *);
extern struct dive *merge_dives(struct dive *a, struct dive *b, int offset, bool prefer_downloaded);
extern struct dive *try_to_merge(struct dive *a, struct dive *b, bool prefer_downloaded);
extern struct event *clone_event(const struct event *src_ev);
extern void copy_events(struct divecomputer *s, struct divecomputer *d);
extern void free_events(struct event *ev);
extern void copy_cylinders(struct dive *s, struct dive *d, bool used_only);
extern void copy_samples(struct divecomputer *s, struct divecomputer *d);
extern bool is_cylinder_used(struct dive *dive, int idx);
extern void fill_default_cylinder(cylinder_t *cyl);
extern void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int time, int idx);
extern struct event *add_event(struct divecomputer *dc, unsigned int time, int type, int flags, int value, const char *name);
extern void remove_event(struct event *event);
extern void update_event_name(struct dive *d, struct event* event, char *name);
extern void add_extra_data(struct divecomputer *dc, const char *key, const char *value);
extern void per_cylinder_mean_depth(struct dive *dive, struct divecomputer *dc, int *mean, int *duration);
extern int get_cylinder_index(struct dive *dive, struct event *ev);
extern struct gasmix *get_gasmix_from_event(struct dive *, struct event *ev);
extern int nr_cylinders(struct dive *dive);
extern int nr_weightsystems(struct dive *dive);

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
};

extern void add_segment(struct deco_state *ds, double pressure, const struct gasmix *gasmix, int period_in_seconds, int setpoint, const struct dive *dive, int sac);
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

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int cylinderid, int po2, bool entered);
struct divedatapoint *create_dp(int time_incr, int depth, int cylinderid, int po2);
#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan);
#endif
struct decostop {
	int depth;
	int time;
};
bool plan(struct deco_state *ds, struct diveplan *diveplan, struct dive *dive, int timestep, struct decostop *decostoptable, struct deco_state **cached_datap, bool is_planner, bool show_disclaimer);
void calc_crushing_pressure(struct deco_state *ds, double pressure);
void vpmb_start_gradient(struct deco_state *ds);
void clear_vpmb_state();
void printdecotable(struct decostop *table);

void delete_single_dive(int idx);

struct event *get_next_event(struct event *event, const char *name);

static inline struct gasmix *get_gasmix(struct dive *dive, struct divecomputer *dc, int time, struct event **evp, struct gasmix *gasmix)
{
	struct event *ev = *evp;

	if (!gasmix) {
		int cyl = explicit_first_cylinder(dive, dc);
		gasmix = &dive->cylinder[cyl].gasmix;
		ev = dc ? dc->events : NULL;
	}
	while (ev && ev->time.seconds < time) {
		gasmix = get_gasmix_from_event(dive, ev);
		ev = get_next_event(ev->next, "gaschange");
	}
	*evp = ev;
	return gasmix;
}

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
extern struct ws_info_t ws_info[100];

extern bool cylinder_nodata(const cylinder_t *cyl);
extern bool cylinder_none(void *_data);
extern bool weightsystem_none(void *_data);
extern bool no_weightsystems(weightsystem_t *ws);
extern bool weightsystems_equal(weightsystem_t *ws1, weightsystem_t *ws2);
extern void remove_cylinder(struct dive *dive, int idx);
extern void remove_weightsystem(struct dive *dive, int idx);
extern void reset_cylinders(struct dive *dive, bool track_gas);
#ifdef DEBUG_CYL
extern void dump_cylinders(struct dive *dive, bool verbose);
#endif

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

extern void set_userid(char *user_id);
extern void set_informational_units(char *units);
extern void set_git_prefs(char *prefs);

extern const char *get_dive_date_c_string(timestamp_t when);
extern void update_setpoint_events(struct dive *dive, struct divecomputer *dc);
#ifdef __cplusplus
}
#endif

extern weight_t string_to_weight(const char *str);
extern depth_t string_to_depth(const char *str);
extern pressure_t string_to_pressure(const char *str);
extern volume_t string_to_volume(const char *str, pressure_t workp);
extern fraction_t string_to_fraction(const char *str);
extern void average_max_depth(struct diveplan *dive, int *avg_depth, int *max_depth);

extern struct dive_table downloadTable;

#include "pref.h"

#endif // DIVE_H
