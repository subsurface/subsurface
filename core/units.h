// SPDX-License-Identifier: GPL-2.0
#ifndef UNITS_H
#define UNITS_H

#include "interpolate.h"

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FRACTION_TUPLE(n, x) ((unsigned)(n) / (x)), ((unsigned)(n) % (x))
#define SIGNED_FRAC_TRIPLET(n, x) ((n) >= 0 ? '+': '-'), ((n) >= 0 ? (unsigned)(n) / (x) : (-(n) / (x))), ((unsigned)((n) >= 0 ? (n) : -(n)) % (x))

#define O2_IN_AIR 209 // permille
#define N2_IN_AIR 781
#define O2_DENSITY 1331 // mg/Liter
#define N2_DENSITY 1165
#define HE_DENSITY 166
#define ZERO_C_IN_MKELVIN 273150 // mKelvin

/* Salinity is expressed in weight in grams per 10l */
#define SEAWATER_SALINITY 10300
#define EN13319_SALINITY 10200
#define BRACKISH_SALINITY 10100
#define FRESHWATER_SALINITY 10000

#include <stdint.h>
/*
 * Some silly structs to make our units very explicit.
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
 * In general, to initialize a variable, use named initializers:
 *	depth_t depth = { .mm = 10'000; }; // 10 m
 * However, for convenience, we define a number of user-defined
 * literals, which make the above more readable:
 *	depht_t depth = 10_m;
 * Currently, we only support integer literals, but might also
 * do floating points if that seems practical.
 *
 * Currently we define:
 *	_sec		-> duration_t in seconds
 *	_min		-> duration_t in minutes
 *	_mm		-> depth_t in millimeters
 *	_m		-> depth_t in meters
 *	_ft		-> depth_t in feet
 *	_mbar		-> pressure_t in millibar
 *	_bar		-> pressure_t in bar
 *	_atm		-> pressure_t in atmospheres
 *	_baro2		-> o2pressure_t in bar
 *	_K		-> temperature_t in kelvin
 *	_ml		-> volume_t in milliliters
 *	_l		-> volume_t in liters
 *	_percent	-> volume_t in liters
 *	_permille	-> fraction_t in ‰
 *	_percent	-> fraction_t in %
 *
 * For now, addition and subtraction of values of the same type
 * are supported.
 *
 * Moreover, multiplication and division with an integral are
 * supported. Attention: the latter uses standard C++ integer
 * semantics: it always rounds towards 0!
 *
 * Note: multiplication with a scalar is currently only supported
 * from the right: "depth * 2" is OK, "2 * depth" is (not yet) OK,
 * because that needs a free standing "operator*()" operator.
 *
 * A free standing interpolate() function can be used to interpolate
 * between types of the same kind (see also the interpolate.h header).
 *
 * We don't actually use these all yet, so maybe they'll change, but
 * I made a number of types as guidelines.
 */
using timestamp_t = int64_t;

/*
 * There is a semi-common pattern where lrint() is used to round
 * doubles to long integers and then cast down to a less wide
 * int. Since this is unwieldy, encapsulate this in this function
 */
template <typename INT>
INT int_cast(double v)
{
	return static_cast<INT>(lrint(v));
}

// Base class for all unit types using the "Curiously recurring template pattern"
// (https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
// to implement addition, subtraction and negation.
// Multiplication and division (which result in a different type)
// are not implemented. If we want that, we should switch to a proper
// units libary.
// Also note that some units may be based on unsigned integers and
// therefore subtraction may be ill-defined.
template <typename T>
struct unit_base {
	auto &get_base() {
		auto &[v] = static_cast<T &>(*this);
		return v;
	}
	auto get_base() const {
		auto [v] = static_cast<const T &>(*this);
		return v;
	}
	template <typename base_type>
	static T from_base(base_type v) {
		return { {}, v };
	}
	T operator+(const T &v2) const {
		return from_base(get_base() + v2.get_base());
	}
	T &operator+=(const T &v2) {
		get_base() += v2.get_base();
		return static_cast<T &>(*this);
	}
	T operator-(const T &v2) const {
		return from_base(get_base() - v2.get_base());
	}
	T &operator-=(const T &v2) {
		get_base() -= v2.get_base();
		return static_cast<T &>(*this);
	}
	template <typename base_type>
	T &operator*=(base_type v) {
		get_base() *= v;
		return static_cast<T &>(*this);
	}
	template <typename base_type>
	T operator*(base_type v) const {
		return from_base(get_base() * v);
	}
	// Attn: C++ integer semantics: this always rounds towards 0!
	template <typename base_type>
	T &operator/=(base_type v) {
		get_base() /= v;
		return static_cast<T &>(*this);
	}
	// Attn: C++ integer semantics: this always rounds towards 0!
	template <typename base_type>
	T operator/(base_type v) const {
		return from_base(get_base() / v);
	}
};

template <typename T,
	std::enable_if_t<!std::is_integral<T>::value, bool> = true>
T interpolate(T a, T b, int part, int whole)
{
	return T::from_base(
		interpolate(a.get_base(), b.get_base(), part, whole)
	);
}

struct duration_t : public unit_base<duration_t>
{
	int32_t seconds = 0; // durations up to 34 yrs
};
static constexpr inline duration_t operator""_sec(unsigned long long sec)
{
	return duration_t { .seconds = static_cast<int32_t>(sec) };
}
static constexpr inline duration_t operator""_min(unsigned long long min)
{
	return duration_t { .seconds = static_cast<int32_t>(min * 60) };
}

struct offset_t : public unit_base<offset_t>
{
	int32_t seconds = 0; // offsets up to +/- 34 yrs
};

struct depth_t : public unit_base<depth_t> // depth to 2000 km
{
	int32_t mm = 0;
};
static constexpr inline depth_t operator""_mm(unsigned long long mm)
{
	return depth_t { .mm = static_cast<int32_t>(mm) };
}
static constexpr inline depth_t operator""_m(unsigned long long m)
{
	return depth_t { .mm = static_cast<int32_t>(m * 1000) };
}
// Not yet constexpr, because round() not constexpr on our Android compiler
static inline depth_t operator""_ft(unsigned long long ft)
{
	return depth_t { .mm = static_cast<int32_t>(round(ft * 304.8)) };
}

// Return either the first argument in meters or the second argument in
// feet, depending on use settings.
depth_t m_or_ft(int m, int ft);

struct pressure_t : public unit_base<pressure_t>
{
	int32_t mbar = 0; // pressure up to 2000 bar
};
static constexpr inline pressure_t operator""_mbar(unsigned long long mbar)
{
	return pressure_t { .mbar = static_cast<int32_t>(mbar) };
}
static constexpr inline pressure_t operator""_bar(unsigned long long bar)
{
	return pressure_t { .mbar = static_cast<int32_t>(bar * 1000) };
}
// Not yet constexpr, because round() not constexpr on our Android compiler
static inline pressure_t operator""_atm(unsigned long long atm)
{
	return pressure_t { .mbar = static_cast<int32_t>(round(atm * 1013.25)) };
}

struct o2pressure_t : public unit_base<o2pressure_t>
{
	uint16_t mbar = 0;
};
static constexpr inline o2pressure_t operator""_baro2(unsigned long long bar)
{
	return o2pressure_t { .mbar = static_cast<uint16_t>(bar * 1000) };
}

struct bearing_t : public unit_base<bearing_t>
{
	int16_t degrees = 0;
};

struct temperature_t : public unit_base<temperature_t>
{
	uint32_t mkelvin = 0; // up to 4 MK (temperatures in K are always positive)
};
static constexpr inline temperature_t operator""_K(unsigned long long K)
{
	return temperature_t { .mkelvin = static_cast<uint32_t>(K * 1000) };
}

struct temperature_sum_t : public unit_base<temperature_sum_t>
{
	uint64_t mkelvin = 0; // up to 18446744073 MK (temperatures in K are always positive)
};

struct volume_t : public unit_base<volume_t>
{
	int mliter = 0;
};
static constexpr inline volume_t operator""_ml(unsigned long long ml)
{
	return volume_t { .mliter = static_cast<int>(ml) };
}
static constexpr inline volume_t operator""_l(unsigned long long l)
{
	return volume_t { .mliter = static_cast<int>(l * 1000) };
}

struct fraction_t : public unit_base<fraction_t>
{
	int permille = 0;
};
static constexpr inline fraction_t operator""_permille(unsigned long long permille)
{
	return fraction_t { .permille = static_cast<int>(permille) };
}
static constexpr inline fraction_t operator""_percent(unsigned long long percent)
{
	return fraction_t { .permille = static_cast<int>(percent * 10) };
}

struct weight_t : public unit_base<weight_t>
{
	int grams = 0;
};

struct degrees_t : public unit_base<degrees_t>
{
	int udeg = 0;
};

struct location_t {
	degrees_t lat, lon;
};

extern void parse_location(const char *, location_t *);
extern unsigned int get_distance(location_t loc1, location_t loc2);

static inline bool has_location(const location_t *loc)
{
	return loc->lat.udeg || loc->lon.udeg;
}

static inline bool operator==(const location_t &a, const location_t &b)
{
	return (a.lat.udeg == b.lat.udeg) && (a.lon.udeg == b.lon.udeg);
}

static inline bool operator!=(const location_t &a, const location_t &b)
{
	return !(a == b);
}

static inline location_t create_location(double lat, double lon)
{
	location_t location = {
		{ .udeg = int_cast<int>(lat * 1000000) },
		{ .udeg = int_cast<int>(lon * 1000000) }
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
	return int_cast<int>(lbs * 453.6);
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

static inline long feet_to_mm(double feet)
{
	return lrint(feet * 304.8);
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

static inline double to_PSI(pressure_t pressure)
{
	return pressure.mbar * 0.0145037738;
}

static inline double bar_to_atm(double bar)
{
	return bar / (1_atm).mbar * 1000;
}

static inline double mbar_to_atm(int mbar)
{
	return (double)mbar / (1_atm).mbar;
}

static inline double mbar_to_PSI(int mbar)
{
	pressure_t p = { .mbar = mbar };
	return to_PSI(p);
}

static inline pressure_t altitude_to_pressure(int32_t altitude) { 	// altitude in mm above sea level
	return pressure_t { .mbar = int_cast<int32_t> (1013.0 * exp(- altitude / 7800000.0)) };
}

depth_t pressure_to_altitude(pressure_t pressure);

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
#define SI_UNITS 																\
        { \
	        .length = units::METERS, .volume = units::LITER, .pressure = units::BAR, .temperature = units::CELSIUS, .weight = units::KG, \
		.vertical_speed_time = units::MINUTES, .duration_units = units::MIXED, .show_units_table = false \
        }

extern const struct units SI_units, IMPERIAL_units;

extern const struct units *get_units();

extern int get_pressure_units(int mb, const char **units);
extern double get_depth_units(depth_t mm, int *frac, const char **units);
extern double get_volume_units(unsigned int ml, int *frac, const char **units);
extern double get_temp_units(unsigned int mk, const char **units);
extern double get_weight_units(unsigned int grams, int *frac, const char **units);
extern double get_vertical_speed_units(unsigned int mms, int *frac, const char **units);

extern depth_t units_to_depth(double depth);
extern int units_to_sac(double volume);

#endif
