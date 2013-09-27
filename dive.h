#ifndef DIVE_H
#define DIVE_H

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <libxml/tree.h>
#include <libxslt/transform.h>

#include "sha1.h"

#ifdef __cplusplus
extern "C" {
#else
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef int bool;
#endif
#endif

#define O2_IN_AIR		209     // permille
#define N2_IN_AIR		781
#define O2_DENSITY		1429    // mg/Liter
#define N2_DENSITY		1251
#define HE_DENSITY		179
#define SURFACE_PRESSURE	1013    // mbar
#define SURFACE_PRESSURE_STRING "1013"
#define ZERO_C_IN_MKELVIN	273150  // mKelvin

/* Salinity is expressed in weight in grams per 10l */
#define SEAWATER_SALINITY	10300
#define FRESHWATER_SALINITY	10000

/* Dive tag definitions */
#define DTAG_INVALID		(1 << 0)
#define DTAG_BOAT		(1 << 1)
#define DTAG_SHORE		(1 << 2)
#define DTAG_DRIFT		(1 << 3)
#define DTAG_DEEP		(1 << 4)
#define DTAG_CAVERN		(1 << 5)
#define DTAG_ICE		(1 << 6)
#define DTAG_WRECK		(1 << 7)
#define DTAG_CAVE		(1 << 8)
#define DTAG_ALTITUDE		(1 << 9)
#define DTAG_POOL		(1 << 10)
#define DTAG_LAKE		(1 << 11)
#define DTAG_RIVER		(1 << 12)
#define DTAG_NIGHT		(1 << 13)
#define DTAG_FRESH		(1 << 14)
#define DTAG_FRESH_NR		14
#define DTAG_STUDENT		(1 << 15)
#define DTAG_INSTRUCTOR		(1 << 16)
#define DTAG_PHOTO		(1 << 17)
#define DTAG_VIDEO		(1 << 18)
#define DTAG_DECO		(1 << 19)
#define DTAG_NR			20
/* defined in statistics.c */
extern char *dtag_names[DTAG_NR];
extern int dtag_shown[DTAG_NR];
extern int dive_mask;

/*
 * Some silly typedefs to make our units very explicit.
 *
 * Also, the units are chosen so that values can be expressible as
 * integers, so that we never have FP rounding issues. And they
 * are small enough that converting to/from imperial units doesn't
 * really matter.
 *
 * We also strive to make '0' a meaningless number saying "not
 * initialized", since many values are things that may not have
 * been reported (eg cylinder pressure or temperature from dive
 * computers that don't support them). But sometimes -1 is an even
 * more explicit way of saying "not there".
 *
 * Thus "millibar" for pressure, for example, or "millikelvin" for
 * temperatures. Doing temperatures in celsius or fahrenheit would
 * make for loss of precision when converting from one to the other,
 * and using millikelvin is SI-like but also means that a temperature
 * of '0' is clearly just a missing temperature or cylinder pressure.
 *
 * Also strive to use units that can not possibly be mistaken for a
 * valid value in a "normal" system without conversion. If the max
 * depth of a dive is '20000', you probably didn't convert from mm on
 * output, or if the max depth gets reported as "0.2ft" it was either
 * a really boring dive, or there was some missing input conversion,
 * and a 60-ft dive got recorded as 60mm.
 *
 * Doing these as "structs containing value" means that we always
 * have to explicitly write out those units in order to get at the
 * actual value. So there is hopefully little fear of using a value
 * in millikelvin as Fahrenheit by mistake.
 *
 * We don't actually use these all yet, so maybe they'll change, but
 * I made a number of types as guidelines.
 */
typedef gint64 timestamp_t;

typedef struct {
	int seconds;
} duration_t;

typedef struct {
	int mm;
} depth_t;

typedef struct {
	int mbar;
} pressure_t;

typedef struct {
	int mkelvin;
} temperature_t;

typedef struct {
	int mliter;
} volume_t;

typedef struct {
	int permille;
} fraction_t;

typedef struct {
	int grams;
} weight_t;

typedef struct {
	int udeg;
} degrees_t;

struct gasmix {
	fraction_t o2;
	fraction_t he;
};

typedef struct {
	volume_t size;
	pressure_t workingpressure;
	const char *description;	/* "LP85", "AL72", "AL80", "HP100+" or whatever */
} cylinder_type_t;

typedef struct {
	cylinder_type_t type;
	struct gasmix gasmix;
	pressure_t start, end, sample_start, sample_end;
} cylinder_t;

typedef struct {
	weight_t weight;
	const char *description;	/* "integrated", "belt", "ankle" */
} weightsystem_t;

extern int get_pressure_units(unsigned int mb, const char **units);
extern double get_depth_units(unsigned int mm, int *frac, const char **units);
extern double get_volume_units(unsigned int ml, int *frac, const char **units);
extern double get_temp_units(unsigned int mk, const char **units);
extern double get_weight_units(unsigned int grams, int *frac, const char **units);

static inline double grams_to_lbs(int grams)
{
	return grams / 453.6;
}

static inline int lbs_to_grams(double lbs)
{
	return lbs * 453.6 + 0.5;
}

static inline double ml_to_cuft(int ml)
{
	return ml / 28316.8466;
}

static inline double cuft_to_l(double cuft)
{
	return cuft * 28.3168466;
}

static inline double mm_to_feet(int mm)
{
	return mm * 0.00328084;
}

static inline unsigned long feet_to_mm(double feet)
{
	return feet * 304.8 + 0.5;
}

static inline int to_feet(depth_t depth)
{
	return mm_to_feet(depth.mm) + 0.5;
}

static inline double mkelvin_to_C(int mkelvin)
{
	return (mkelvin - ZERO_C_IN_MKELVIN) / 1000.0;
}

static inline double mkelvin_to_F(int mkelvin)
{
	return mkelvin * 9 / 5000.0 - 459.670;
}

static inline unsigned long F_to_mkelvin(double f)
{
	return (f-32) * 1000 / 1.8 + ZERO_C_IN_MKELVIN + 0.5;
}

static inline unsigned long C_to_mkelvin(double c)
{
	return c * 1000 + ZERO_C_IN_MKELVIN + 0.5;
}

static inline double psi_to_bar(double psi)
{
	return psi / 14.5037738;
}

static inline long psi_to_mbar(double psi)
{
	return psi_to_bar(psi)*1000 + 0.5;
}

static inline int to_PSI(pressure_t pressure)
{
	return pressure.mbar * 0.0145037738 + 0.5;
}

static inline double bar_to_atm(double bar)
{
	return bar / SURFACE_PRESSURE * 1000;
}

/* Volume in mliter of a cylinder at pressure 'p' */
extern int gas_volume(cylinder_t *cyl, pressure_t p);
extern int wet_volume(double cuft, pressure_t p);

static inline int mbar_to_PSI(int mbar)
{
	pressure_t p = {mbar};
	return to_PSI(p);
}

static inline int get_o2(const struct gasmix *mix)
{
	return mix->o2.permille ? : O2_IN_AIR;
}

static inline int get_he(const struct gasmix *mix)
{
	return mix->he.permille;
}

static inline gboolean is_air(int o2, int he)
{
	return (he == 0) && (o2 == 0 || ((o2 >= O2_IN_AIR - 1) && (o2 <= O2_IN_AIR + 1)));
}

/* Linear interpolation between 'a' and 'b', when we are 'part'way into the 'whole' distance from a to b */
static inline int interpolate(int a, int b, int part, int whole)
{
	/* It is doubtful that we actually need floating point for this, but whatever */
	double x = (double) a * (whole - part) + (double) b * part;
	return rint(x / whole);
}

struct sample {
	duration_t time;
	depth_t depth;
	temperature_t temperature;
	pressure_t cylinderpressure;
	int sensor;		/* Cylinder pressure sensor index */
	duration_t ndl;
	duration_t stoptime;
	depth_t stopdepth;
	gboolean in_deco;
	int cns;
	int po2;
};

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
	gboolean deleted;
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
#define MAX_WEIGHTSYSTEMS (4)
#define W_IDX_PRIMARY 0
#define W_IDX_SECONDARY 1

typedef enum { TF_NONE, NO_TRIP, IN_TRIP, ASSIGNED_TRIP, NUM_TRIPFLAGS } tripflag_t;

typedef struct dive_trip {
	timestamp_t when;
	char *location;
	char *notes;
	struct dive *dives;
	int nrdives;
	int index;
	unsigned expanded:1, selected:1, autogen:1, fixup:1;
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
	gboolean downloaded;
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
	int dive_tags;

	struct divecomputer dc;
};

extern int get_index_for_dive(struct dive *dive);

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

static inline int get_surface_pressure_in_mbar(const struct dive *dive, gboolean non_null)
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
	mbar += depth / 10.0 * specific_weight + 0.5;
	return mbar;
}

static inline int depth_to_mbar(int depth, struct dive *dive)
{
	return calculate_depth_to_mbar(depth, dive->surface_pressure, dive->salinity);
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
	cm = mbar / specific_weight + 0.5;
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
#define TRIP_THRESHOLD 3600*24*3

#define UNGROUPED_DIVE(_dive) ((_dive)->tripflag == NO_TRIP)
#define DIVE_IN_TRIP(_dive) ((_dive)->tripflag == IN_TRIP || (_dive)->tripflag == ASSIGNED_TRIP)
#define DIVE_NEEDS_TRIP(_dive) ((_dive)->tripflag == TF_NONE)

extern void add_dive_to_trip(struct dive *, dive_trip_t *);

extern void delete_single_dive(int idx);
extern void add_single_dive(int idx, struct dive *dive);

extern void insert_trip(dive_trip_t **trip);

/*
 * We keep our internal data in well-specified units, but
 * the input and output may come in some random format. This
 * keeps track of those units.
 */
/* turns out in Win32 PASCAL is defined as a calling convention */
#ifdef WIN32
#undef PASCAL
#endif
struct units {
	enum { METERS, FEET } length;
	enum { LITER, CUFT } volume;
	enum { BAR, PSI, PASCAL } pressure;
	enum { CELSIUS, FAHRENHEIT, KELVIN } temperature;
	enum { KG, LBS } weight;
};

/*
 * We're going to default to SI units for input. Yes,
 * technically the SI unit for pressure is Pascal, but
 * we default to bar (10^5 pascal), which people
 * actually use. Similarly, C instead of Kelvin.
 * And kg instead of g.
 */
#define SI_UNITS {			\
	.length = METERS,		\
	.volume = LITER,		\
	.pressure = BAR,		\
	.temperature = CELSIUS,		\
	.weight = KG			\
}

#define IMPERIAL_UNITS {		\
	.length = FEET,			\
	.volume = CUFT,			\
	.pressure = PSI,		\
	.temperature = FAHRENHEIT,	\
	.weight = LBS			\
}
extern const struct units SI_units, IMPERIAL_units;
extern struct units xml_parsing_units;

extern struct units *get_units(void);
extern int verbose;

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

static inline struct divecomputer *get_dive_dc(struct dive *dive, int nr)
{
	struct divecomputer *dc = NULL;
	if (nr >= 0)
		dc = &dive->dc;
	while (nr-- > 0)
		dc = dc->next;
	return dc;
}

/*
 * Iterate over each dive, with the first parameter being the index
 * iterator variable, and the second one being the dive one.
 *
 * I don't think anybody really wants the index, and we could make
 * it local to the for-loop, but that would make us requires C99.
 */
#define for_each_dive(_i,_x) \
	for ((_i) = 0; ((_x) = get_dive(_i)) != NULL; (_i)++)

#define for_each_dc(_dive,_dc) \
	for (_dc = &_dive->dc; _dc; _dc = _dc->next)

#define for_each_gps_location(_i,_x) \
	for ((_i) = 0; ((_x) = get_gps_location(_i, &gps_location_table)) != NULL; (_i)++)

static inline struct dive *get_dive_by_diveid(uint32_t diveid, uint32_t deviceid)
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
extern struct dive *find_dive_including(timestamp_t when);
extern gboolean dive_within_time_range(struct dive *dive, timestamp_t when, timestamp_t offset);
struct dive *find_dive_n_near(timestamp_t when, int n, timestamp_t offset);

/* Check if two dive computer entries are the exact same dive (-1=no/0=maybe/1=yes) */
extern int match_one_dc(struct divecomputer *a, struct divecomputer *b);

extern void parse_xml_init(void);
extern void parse_xml_buffer(const char *url, const char *buf, int size, struct dive_table *table, char **error);
extern void parse_xml_exit(void);
extern void set_filename(const char *filename, gboolean force);

extern int parse_dm4_buffer(const char *url, const char *buf, int size, struct dive_table *table, char **error);

extern void parse_file(const char *filename, char **error);

extern void show_dive_info(struct dive *);

extern void show_dive_equipment(struct dive *, int w_idx);
extern void clear_equipment_widgets(void);

extern void show_dive_stats(struct dive *);
extern void clear_stats_widgets(void);

extern void show_gps_locations(void);
extern void show_gps_location(struct dive *, void (*callback)(float, float));

extern void show_yearly_stats(void);

extern void update_dive(struct dive *new_dive);
extern void save_dives(const char *filename);
extern void save_dives_logic(const char *filename, gboolean select_only);
extern void save_dive(FILE *f, struct dive *dive);

extern xsltStylesheetPtr get_stylesheet(const char *name);

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

extern struct dive *alloc_dive(void);
extern void record_dive(struct dive *dive);

extern struct sample *prepare_sample(struct divecomputer *dc);
extern void finish_sample(struct divecomputer *dc);

extern void sort_table(struct dive_table *table);
extern struct dive *fixup_dive(struct dive *dive);
extern unsigned int dc_airtemp(struct divecomputer *dc);
extern struct dive *merge_dives(struct dive *a, struct dive *b, int offset, gboolean prefer_downloaded);
extern struct dive *try_to_merge(struct dive *a, struct dive *b, gboolean prefer_downloaded);
extern void renumber_dives(int nr);
extern void copy_samples(struct dive *s, struct dive *d);

extern void add_gas_switch_event(struct dive *dive, struct divecomputer *dc, int time, int idx);
extern void add_event(struct divecomputer *dc, int time, int type, int flags, int value, const char *name);

/* UI related protopypes */

extern void report_error(GError* error);

extern void add_cylinder_description(cylinder_type_t *);
extern void add_weightsystem_description(weightsystem_t *);
extern void add_people(const char *string);
extern void add_location(const char *string);
extern void add_suit(const char *string);
extern void remember_event(const char *eventname);
extern int evn_foreach(void (*callback)(const char *, int *, void *), void *data);
extern void clear_events(void);

extern int add_new_dive(struct dive *dive);
extern gboolean edit_trip(dive_trip_t *trip);
extern int edit_dive_info(struct dive *dive, gboolean newdive);
extern int edit_multi_dive_info(struct dive *single_dive);
extern void dive_list_update_dives(void);
extern void flush_divelist(struct dive *dive);

extern void set_dc_nickname(struct dive *dive);
extern void set_autogroup(gboolean value);
extern int total_weight(struct dive *);

#define DIVE_ERROR_PARSE 1
#define DIVE_ERROR_PLAN 2

const char *weekday(int wday);
const char *monthname(int mon);

#define UTF8_DEGREE "\xc2\xb0"
#define UTF8_DELTA "\xce\x94"
#define UTF8_UPWARDS_ARROW "\xE2\x86\x91"
#define UTF8_DOWNWARDS_ARROW "\xE2\x86\x93"
#define UTF8_AVERAGE "\xc3\xb8"
#define UCS4_DEGREE 0xb0
#define UTF8_SUBSCRIPT_2 "\xe2\x82\x82"
#define UTF8_WHITESTAR "\xe2\x98\x86"
#define UTF8_BLACKSTAR "\xe2\x98\x85"
#define ZERO_STARS	UTF8_WHITESTAR UTF8_WHITESTAR UTF8_WHITESTAR UTF8_WHITESTAR UTF8_WHITESTAR
#define ONE_STARS	UTF8_BLACKSTAR UTF8_WHITESTAR UTF8_WHITESTAR UTF8_WHITESTAR UTF8_WHITESTAR
#define TWO_STARS	UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_WHITESTAR UTF8_WHITESTAR UTF8_WHITESTAR
#define THREE_STARS	UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_WHITESTAR UTF8_WHITESTAR
#define FOUR_STARS	UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_WHITESTAR
#define FIVE_STARS	UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_BLACKSTAR UTF8_BLACKSTAR
extern const char *star_strings[];

/* enum holding list of OS features */
typedef enum {
	UTF8_FONT_WITH_STARS
} os_feature_t;

extern const char *existing_filename;
extern const char *subsurface_gettext_domainpath(char *);
extern gboolean subsurface_os_feature_available(os_feature_t);
extern gboolean subsurface_launch_for_uri(const char *);
extern void subsurface_command_line_init(gint *, gchar ***);
extern void subsurface_command_line_exit(gint *, gchar ***);

#define FRACTION(n,x) ((unsigned)(n)/(x)),((unsigned)(n)%(x))

extern double add_segment(double pressure, const struct gasmix *gasmix, int period_in_seconds, int setpoint, const struct dive *dive);
extern void clear_deco(double surface_pressure);
extern void dump_tissues(void);
extern unsigned int deco_allowed_depth(double tissues_tolerance, double surface_pressure, struct dive *dive, gboolean smooth);
extern void set_gf(short gflow, short gfhigh);
extern void cache_deco_state(double, char **datap);
extern double restore_deco_state(char *data);

struct divedatapoint {
	int time;
	int depth;
	int o2;
	int he;
	int po2;
	gboolean entered;
	struct divedatapoint *next;
};

struct diveplan {
	timestamp_t when;
	int lastdive_nr;
	int surface_pressure;		/* mbar */
	int bottomsac;			/* ml/min */
	int decosac;			/* ml/min */
	short gflow;
	short gfhigh;
	struct divedatapoint *dp;
};

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he, int po2);
void add_depth_to_nth_dp(struct diveplan *diveplan, int idx, int depth);
void add_gas_to_nth_dp(struct diveplan *diveplan, int idx, int o2, int he);
void free_dps(struct divedatapoint *dp);
void get_gas_string(int o2, int he, char *buf, int len);
struct divedatapoint *create_dp(int time_incr, int depth, int o2, int he, int po2);
void dump_plan(struct diveplan *diveplan);
void plan(struct diveplan *diveplan, char **cached_datap, struct dive **divep, bool add_deco, char **error_string_p);
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

#ifdef __cplusplus
}
#endif

#include "pref.h"

#endif /* DIVE_H */
