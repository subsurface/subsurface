#ifndef DIVE_H
#define DIVE_H

#include <stdlib.h>
#include <time.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <libxml/tree.h>

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

extern gboolean cylinder_nodata(cylinder_t *cyl);
extern gboolean cylinder_nosamples(cylinder_t *cyl);
extern gboolean cylinder_none(void *_data);
extern gboolean no_cylinders(cylinder_t *cyl);
extern gboolean no_weightsystems(weightsystem_t *ws);
extern gboolean weightsystems_equal(weightsystem_t *ws1, weightsystem_t *ws2);

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
	return (mkelvin - 273150) / 1000.0;
}

static inline double mkelvin_to_F(int mkelvin)
{
	return mkelvin * 9 / 5000.0 - 459.670;
}

static inline unsigned long F_to_mkelvin(double f)
{
	return (f-32) * 1000 / 1.8 + 273150.5;
}

static inline unsigned long C_to_mkelvin(double c)
{
	return c * 1000 + 273150.5;
}

static inline int to_C(temperature_t temp)
{
	if (!temp.mkelvin)
		return 0;
	return mkelvin_to_C(temp.mkelvin) + 0.5;
}

static inline int to_F(temperature_t temp)
{
	if (!temp.mkelvin)
		return 0;
	return mkelvin_to_F(temp.mkelvin) + 0.5;
}

static inline int to_K(temperature_t temp)
{
	if (!temp.mkelvin)
		return 0;
	return (temp.mkelvin + 499)/1000;
}

static inline double psi_to_bar(double psi)
{
	return psi / 14.5037738;
}

static inline unsigned long psi_to_mbar(double psi)
{
	return psi_to_bar(psi)*1000 + 0.5;
}

static inline int to_PSI(pressure_t pressure)
{
	return pressure.mbar * 0.0145037738 + 0.5;
}

static inline double bar_to_atm(double bar)
{
	return bar / 1.01325;
}

static inline double to_ATM(pressure_t pressure)
{
	return pressure.mbar / 1013.25;
}

static inline int mbar_to_PSI(int mbar)
{
	pressure_t p = {mbar};
	return to_PSI(p);
}

struct sample {
	duration_t time;
	depth_t depth;
	temperature_t temperature;
	pressure_t cylinderpressure;
	int cylinderindex;
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
	char name[];
};

#define MAX_CYLINDERS (8)
#define MAX_WEIGHTSYSTEMS (4)
#define W_IDX_PRIMARY 0
#define W_IDX_SECONDARY 1

typedef gint64 timestamp_t;
typedef enum { TF_NONE, NO_TRIP, IN_TRIP, ASSIGNED_TRIP, AUTOGEN_TRIP, NUM_TRIPFLAGS } tripflag_t;
extern const char *tripflag_names[NUM_TRIPFLAGS];

typedef struct dive_trip {
	tripflag_t tripflag;
	timestamp_t when;
	timestamp_t when_from_file;
	char *location;
	char *notes;
	int expanded:1, selected:1;
} dive_trip_t;

struct dive {
	int number;
	tripflag_t tripflag;
	dive_trip_t *divetrip;
	int selected;
	timestamp_t when;
	char *location;
	char *notes;
	char *divemaster, *buddy;
	int rating;
	double latitude, longitude;
	depth_t maxdepth, meandepth;
	duration_t duration, surfacetime;
	depth_t visibility;
	temperature_t airtemp, watertemp;
	cylinder_t cylinder[MAX_CYLINDERS];
	weightsystem_t weightsystem[MAX_WEIGHTSYSTEMS];
	char *suit;
	int sac, otu;
	struct event *events;
	int samples, alloc_samples;
	struct sample sample[];
};

/* this is a global spot for a temporary dive structure that we use to
 * be able to edit a dive without unintended side effects */
extern struct dive edit_dive;

extern GList *dive_trip_list;
extern gboolean autogroup;
/* random threashold: three days without diving -> new trip
 * this works very well for people who usually dive as part of a trip and don't
 * regularly dive at a local facility; this is why trips are an optional feature */
#define TRIP_THRESHOLD 3600*24*3

#define UNGROUPED_DIVE(_dive) ((_dive)->tripflag == NO_TRIP)
#define DIVE_IN_TRIP(_dive) ((_dive)->tripflag == IN_TRIP || (_dive)->tripflag == ASSIGNED_TRIP)
#define DIVE_NEEDS_TRIP(_dive) ((_dive)->tripflag == TF_NONE)
#define NEXT_TRIP(_entry) ((_entry) ? g_list_next(_entry) : (dive_trip_list))
#define PREV_TRIP(_entry) ((_entry) ? g_list_previous(_entry) : g_list_last(dive_trip_list))
#define DIVE_TRIP(_trip) ((dive_trip_t *)(_trip)->data)
#define DIVE_FITS_TRIP(_dive, _dive_trip) ((_dive_trip)->when - TRIP_THRESHOLD <= (_dive)->when)

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

extern const struct units SI_units, IMPERIAL_units;
extern struct units input_units, output_units;

extern int verbose;

struct dive_table {
	int nr, allocated, preexisting;
	struct dive **dives;
};

extern struct dive_table dive_table;

extern int selected_dive;
#define current_dive (get_dive(selected_dive))

static inline struct dive *get_dive(int nr)
{
	if (nr >= dive_table.nr || nr < 0)
		return NULL;
	return dive_table.dives[nr];
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

extern void parse_xml_init(void);
extern void parse_xml_buffer(const char *url, const char *buf, int size, GError **error,
			gboolean possible_default_filename);
extern void parse_xml_exit(void);
extern void set_filename(const char *filename, gboolean force);

extern void parse_file(const char *filename, GError **error, gboolean possible_default_filename);

#ifdef XSLT
extern xmlDoc *test_xslt_transforms(xmlDoc *doc);
#endif

extern void show_dive_info(struct dive *);

extern void show_dive_equipment(struct dive *, int w_idx);
extern void clear_equipment_widgets(void);

extern void show_dive_stats(struct dive *);
extern void clear_stats_widgets(void);

extern void show_yearly_stats(void);

extern void update_dive(struct dive *new_dive);
extern void save_dives(const char *filename);

static inline unsigned int dive_size(int samples)
{
	return sizeof(struct dive) + samples*sizeof(struct sample);
}

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

extern struct dive *alloc_dive(void);
extern void record_dive(struct dive *dive);
extern void delete_dive(struct dive *dive);

extern struct sample *prepare_sample(struct dive **divep);
extern void finish_sample(struct dive *dive);

extern void report_dives(gboolean imported);
extern struct dive *fixup_dive(struct dive *dive);
extern struct dive *try_to_merge(struct dive *a, struct dive *b);

extern void renumber_dives(int nr);

extern void add_event(struct dive *dive, int time, int type, int flags, int value, const char *name);

/* UI related protopypes */

extern void init_ui(int *argcp, char ***argvp);

extern void run_ui(void);
extern void exit_ui(void);

extern void report_error(GError* error);

extern void add_cylinder_description(cylinder_type_t *);
extern void add_weightsystem_description(weightsystem_t *);
extern void add_people(const char *string);
extern void add_location(const char *string);
extern void add_suit(const char *string);
extern void remember_event(const char *eventname);
extern void evn_foreach(void (*callback)(const char *, int *, void *), void *data);

extern int add_new_dive(struct dive *dive);
extern gboolean edit_trip(dive_trip_t *trip);
extern int edit_dive_info(struct dive *dive);
extern int edit_multi_dive_info(struct dive *single_dive);
extern void dive_list_update_dives(void);
extern void flush_divelist(struct dive *dive);

#define DIVE_ERROR_PARSE 1

const char *weekday(int wday);
const char *monthname(int mon);

#define UTF8_DEGREE "\xc2\xb0"
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

extern const char *default_filename;
extern const char *existing_filename;
extern const char *subsurface_default_filename(void);
extern const char *subsurface_gettext_domainpath(void);
extern void subsurface_command_line_init(gint *, gchar ***);
extern void subsurface_command_line_exit(gint *, gchar ***);
#define AIR_PERMILLE 209

#define FRACTION(n,x) ((unsigned)(n)/(x)),((unsigned)(n)%(x))

#ifdef DEBUGFILE
extern char *debugfilename;
extern FILE *debugfile;
#endif
#endif /* DIVE_H */
