// SPDX-License-Identifier: GPL-2.0
#ifndef UNITS_H
#define UNITS_H

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

#define FRACTION(n, x) ((unsigned)(n) / (x)), ((unsigned)(n) % (x))

#define O2_IN_AIR 209 // permille
#define N2_IN_AIR 781
#define O2_DENSITY 1331 // mg/Liter
#define N2_DENSITY 1165
#define HE_DENSITY 166
#define SURFACE_PRESSURE 1013 // mbar
#define ZERO_C_IN_MKELVIN 273150 // mKelvin

#ifdef __cplusplus
#define M_OR_FT(_m, _f) ((prefs.units.length == units::METERS) ? ((_m) * 1000) : (feet_to_mm(_f)))
#else
#define M_OR_FT(_m, _f) ((prefs.units.length == METERS) ? ((_m) * 1000) : (feet_to_mm(_f)))
#endif

/* Salinity is expressed in weight in grams per 10l */
#define SEAWATER_SALINITY 10300
#define EN13319_SALINITY 10200
#define BRACKISH_SALINITY 10100
#define FRESHWATER_SALINITY 10000

#include <stdint.h>
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
 * computers that don't support them). But for some of the values
 * 0 doesn't works as a flag for not initialized. Examples are
 * compass bearing (bearing_t) or NDL (duration_t).
 * Therefore some types have a default value which is -1 and has to
 * be set at certain points in the code.
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
 * output, or if the max. depth gets reported as "0.2ft" it was either
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
typedef int64_t timestamp_t;

typedef struct
{
	int32_t seconds; // durations up to 34 yrs
} duration_t;

static const duration_t zero_duration = { 0 };

typedef struct
{
	int32_t seconds; // offsets up to +/- 34 yrs
} offset_t;

typedef struct
{
	int32_t mm;
} depth_t; // depth to 2000 km

typedef struct
{
	int32_t mbar; // pressure up to 2000 bar
} pressure_t;

typedef struct
{
	uint16_t mbar;
} o2pressure_t; // pressure up to 65 bar

typedef struct
{
	int16_t degrees;
} bearing_t; // compass bearing

typedef struct
{
	uint32_t mkelvin; // up to 4 MK (temperatures in K are always positive)
} temperature_t;

typedef struct
{
	uint64_t mkelvin; // up to 18446744073 MK (temperatures in K are always positive)
} temperature_sum_t;

typedef struct
{
	int mliter;
} volume_t;

typedef struct
{
	int permille;
} fraction_t;

typedef struct
{
	int grams;
} weight_t;

typedef struct
{
	int udeg;
} degrees_t;

typedef struct pos {
	degrees_t lat, lon;
} location_t;

static const location_t zero_location = { { 0 }, { 0 }};

extern void parse_location(const char *, location_t *);

static inline bool has_location(const location_t *loc)
{
	return loc->lat.udeg || loc->lon.udeg;
}

static inline bool same_location(const location_t *a, const location_t *b)
{
	return (a->lat.udeg == b->lat.udeg) && (a->lon.udeg == b->lon.udeg);
}

static inline location_t create_location(double lat, double lon)
{
	location_t location = {
		{ (int) lrint(lat * 1000000) },
		{ (int) lrint(lon * 1000000) }
	};
	return location;
}

static inline double udeg_to_radians(int udeg)
{
	return (udeg * M_PI) / (1000000.0 * 180.0);
}

static inline double grams_to_lbs(int grams)
{
	return grams / 453.6;
}

static inline int lbs_to_grams(double lbs)
{
	return (int)lrint(lbs * 453.6);
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

static inline double m_to_mile(int m)
{
	return m / 1609.344;
}

static inline unsigned long feet_to_mm(double feet)
{
	return lrint(feet * 304.8);
}

static inline int to_feet(depth_t depth)
{
	return (int)lrint(mm_to_feet(depth.mm));
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
	return lrint((f - 32) * 1000 / 1.8 + ZERO_C_IN_MKELVIN);
}

static inline unsigned long C_to_mkelvin(double c)
{
	return lrint(c * 1000 + ZERO_C_IN_MKELVIN);
}

static inline unsigned long cC_to_mkelvin(double c)
{
	return lrint(c * 10 + ZERO_C_IN_MKELVIN);
}

static inline double psi_to_bar(double psi)
{
	return psi / 14.5037738;
}

static inline long psi_to_mbar(double psi)
{
	return lrint(psi_to_bar(psi) * 1000);
}

static inline int to_PSI(pressure_t pressure)
{
	return (int)lrint(pressure.mbar * 0.0145037738);
}

static inline double bar_to_atm(double bar)
{
	return bar / SURFACE_PRESSURE * 1000;
}

static inline double mbar_to_atm(int mbar)
{
	return (double)mbar / SURFACE_PRESSURE;
}

static inline int mbar_to_PSI(int mbar)
{
	pressure_t p = { mbar };
	return to_PSI(p);
}

static inline int32_t altitude_to_pressure(int32_t altitude) 	// altitude in mm above sea level
{						// returns atmospheric pressure in mbar
	return (int32_t) (1013.0 * exp(- altitude / 7800000.0));
}


static inline int32_t pressure_to_altitude(int32_t pressure)	// pressure in mbar
{						// returns altitude in mm above sea level
	return (int32_t) (log(1013.0 / pressure) * 7800000);
}

/*
 * We keep our internal data in well-specified units, but
 * the input and output may come in some random format. This
 * keeps track of those units.
 */
/* turns out in Win32 PASCAL is defined as a calling convention */
/* NOTE: these enums are duplicated in mobile-widgets/qmlinterface.h */
struct units {
	enum LENGTH {
		METERS,
		FEET
	} length;
	enum VOLUME {
		LITER,
		CUFT
	} volume;
	enum PRESSURE {
		BAR,
		PSI,
		PASCALS
	} pressure;
	enum TEMPERATURE {
		CELSIUS,
		FAHRENHEIT,
		KELVIN
	} temperature;
	enum WEIGHT {
		KG,
		LBS
	} weight;
	enum TIME {
		SECONDS,
		MINUTES
	} vertical_speed_time;
	enum DURATION {
		MIXED,
		MINUTES_ONLY,
		ALWAYS_HOURS
	} duration_units;
	bool show_units_table;
};

/*
 * We're going to default to SI units for input. Yes,
 * technically the SI unit for pressure is Pascal, but
 * we default to bar (10^5 pascal), which people
 * actually use. Similarly, C instead of Kelvin.
 * And kg instead of g.
 */
#define SI_UNITS                                                                                           \
        {                                                                                                  \
	        .length = METERS, .volume = LITER, .pressure = BAR, .temperature = CELSIUS, .weight = KG,  \
		.vertical_speed_time = MINUTES, .duration_units = MIXED, .show_units_table = false         \
        }

#define IMPERIAL_UNITS                                                                                     \
        {                                                                                                  \
	        .length = FEET, .volume = CUFT, .pressure = PSI, .temperature = FAHRENHEIT, .weight = LBS, \
		.vertical_speed_time = MINUTES, .duration_units = MIXED, .show_units_table = false         \
        }

extern const struct units SI_units, IMPERIAL_units;

extern const struct units *get_units(void);

extern int get_pressure_units(int mb, const char **units);
extern double get_depth_units(int mm, int *frac, const char **units);
extern double get_volume_units(unsigned int ml, int *frac, const char **units);
extern double get_temp_units(unsigned int mk, const char **units);
extern double get_weight_units(unsigned int grams, int *frac, const char **units);
extern double get_vertical_speed_units(unsigned int mms, int *frac, const char **units);

extern depth_t units_to_depth(double depth);
extern int units_to_sac(double volume);
#ifdef __cplusplus
}
#endif

#endif
