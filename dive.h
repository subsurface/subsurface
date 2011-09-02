#ifndef DIVE_H
#define DIVE_H

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
 * been reported (eg tank pressure or temperature from dive
 * computers that don't support them). But sometimes -1 is an even
 * more explicit way of saying "not there".
 *
 * Thus "millibar" for pressure, for example, or "millikelvin" for
 * temperatures. Doing temperatures in celsius or fahrenheit would
 * make for loss of precision when converting from one to the other,
 * and using millikelvin is SI-like but also means that a temperature
 * of '0' is clearly just a missing temperature or tank pressure.
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

typedef struct {
	fraction_t o2;
	fraction_t he;
} gasmix_t;

typedef struct {
	volume_t size;
	pressure_t pressure;
} tank_type_t;

static inline int to_feet(depth_t depth)
{
	return depth.mm * 0.00328084 + 0.5;
}

static inline int to_C(temperature_t temp)
{
	if (!temp.mkelvin)
		return 0;
	return (temp.mkelvin - 273150) / 1000;
}

static inline int to_PSI(pressure_t pressure)
{
	return pressure.mbar * 0.0145037738 + 0.5;
}

struct sample {
	duration_t time;
	depth_t depth;
	temperature_t temperature;
	pressure_t tankpressure;
	int tankindex;
};

#define MAX_MIXES (4)

struct dive {
	const char *name;
	time_t when;
	char *location;
	char *notes;
	depth_t maxdepth, meandepth;
	duration_t duration, surfacetime;
	depth_t visibility;
	temperature_t airtemp, watertemp;
	pressure_t beginning_pressure, end_pressure;
	gasmix_t gasmix[MAX_MIXES];
	int samples;
	struct sample sample[];
};

extern int verbose;

struct dive_table {
	int nr, allocated;
	struct dive **dives;
};

extern struct dive_table dive_table;

static inline struct dive *get_dive(unsigned int nr)
{
	if (nr >= dive_table.nr)
		return NULL;
	return dive_table.dives[nr];
}

extern void parse_xml_init(void);
extern void parse_xml_file(const char *filename);

extern void flush_dive_info_changes(void);
extern void save_dives(const char *filename);

#endif /* DIVE_H */
