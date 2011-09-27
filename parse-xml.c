#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "dive.h"

int verbose;

struct dive_table dive_table;

/*
 * Add a dive into the dive_table array
 */
void record_dive(struct dive *dive)
{
	int nr = dive_table.nr, allocated = dive_table.allocated;
	struct dive **dives = dive_table.dives;

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		dives = realloc(dives, allocated * sizeof(struct dive *));
		if (!dives)
			exit(1);
		dive_table.dives = dives;
		dive_table.allocated = allocated;
	}
	dives[nr] = fixup_dive(dive);
	dive_table.nr = nr+1;
}

static void start_match(const char *type, const char *name, char *buffer)
{
	if (verbose > 2)
		printf("Matching %s '%s' (%s)\n",
			type, name, buffer);
}

static void nonmatch(const char *type, const char *name, char *buffer)
{
	if (verbose > 1)
		printf("Unable to match %s '%s' (%s)\n",
			type, name, buffer);
	free(buffer);
}

typedef void (*matchfn_t)(char *buffer, void *);

static int match(const char *pattern, int plen,
		 const char *name, int nlen,
		 matchfn_t fn, char *buf, void *data)
{
	if (plen > nlen)
		return 0;
	if (memcmp(pattern, name + nlen - plen, plen))
		return 0;
	fn(buf, data);
	return 1;
}


struct units input_units;

/*
 * We're going to default to SI units for input. Yes,
 * technically the SI unit for pressure is Pascal, but
 * we default to bar (10^5 pascal), which people
 * actually use. Similarly, C instead of Kelvin.
 */
const struct units SI_units = {
	.length = METERS,
	.volume = LITER,
	.pressure = BAR,
	.temperature = CELSIUS,
	.weight = KG
};

const struct units IMPERIAL_units = {
	.length = FEET,
	.volume = CUFT,
	.pressure = PSI,
	.temperature = FAHRENHEIT,
	.weight = LBS
};

/*
 * Dive info as it is being built up..
 */
static struct dive *dive;
static struct sample *sample;
static struct {
	int active;
	duration_t time;
	int type, flags, value;
	const char *name;
} event;
static struct tm tm;
static int cylinder_index;

static enum import_source {
	UNKNOWN,
	LIBDIVECOMPUTER,
	SUUNTO,
	UEMIS,
	DIVINGLOG,
	UDDF,
} import_source;

time_t utc_mktime(struct tm *tm)
{
	static const int mdays[] = {
	    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	int year = tm->tm_year;
	int month = tm->tm_mon;
	int day = tm->tm_mday;

	/* First normalize relative to 1900 */
	if (year < 70)
		year += 100;
	else if (year > 1900)
		year -= 1900;

	/* Normalized to Jan 1, 1970: unix time */
	year -= 70;

	if (year < 0 || year > 129) /* algo only works for 1970-2099 */
		return -1;
	if (month < 0 || month > 11) /* array bounds */
		return -1;
	if (month < 2 || (year + 2) % 4)
		day--;
	if (tm->tm_hour < 0 || tm->tm_min < 0 || tm->tm_sec < 0)
		return -1;
	return (year * 365 + (year + 1) / 4 + mdays[month] + day) * 24*60*60UL +
		tm->tm_hour * 60*60 + tm->tm_min * 60 + tm->tm_sec;
}

static void divedate(char *buffer, void *_when)
{
	int d,m,y;
	time_t *when = _when;
	int success = 0;

	success = tm.tm_sec | tm.tm_min | tm.tm_hour;
	if (sscanf(buffer, "%d.%d.%d", &d, &m, &y) == 3) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
	} else if (sscanf(buffer, "%d-%d-%d", &y, &m, &d) == 3) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
	} else {
		fprintf(stderr, "Unable to parse date '%s'\n", buffer);
		success = 0;
	}

	if (success)
		*when = utc_mktime(&tm);

	free(buffer);
}

static void divetime(char *buffer, void *_when)
{
	int h,m,s = 0;
	time_t *when = _when;

	if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) >= 2) {
		tm.tm_hour = h;
		tm.tm_min = m;
		tm.tm_sec = s;
		if (tm.tm_year)
			*when = utc_mktime(&tm);
	}
	free(buffer);
}

/* Libdivecomputer: "2011-03-20 10:22:38" */
static void divedatetime(char *buffer, void *_when)
{
	int y,m,d;
	int hr,min,sec;
	time_t *when = _when;

	if (sscanf(buffer, "%d-%d-%d %d:%d:%d",
		&y, &m, &d, &hr, &min, &sec) == 6) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
		tm.tm_hour = hr;
		tm.tm_min = min;
		tm.tm_sec = sec;
		*when = utc_mktime(&tm);
	}
	free(buffer);
}

union int_or_float {
	double fp;
};

enum number_type {
	NEITHER,
	FLOAT
};

static enum number_type integer_or_float(char *buffer, union int_or_float *res)
{
	char *end;
	long val;
	double fp;

	/* Integer or floating point? */
	val = strtol(buffer, &end, 10);
	if (val < 0 || end == buffer)
		return NEITHER;

	/* Looks like it might be floating point? */
	if (*end == '.') {
		errno = 0;
		fp = strtod(buffer, &end);
		if (!errno) {
			res->fp = fp;
			return FLOAT;
		}
	}

	res->fp = val;
	return FLOAT;
}

static void pressure(char *buffer, void *_press)
{
	double mbar;
	pressure_t *pressure = _press;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Just ignore zero values */
		if (!val.fp)
			break;
		switch (input_units.pressure) {
		case PASCAL:
			mbar = val.fp / 100;
			break;
		case BAR:
			/* Assume mbar, but if it's really small, it's bar */
			mbar = val.fp;
			if (mbar < 5000)
				mbar = mbar * 1000;
			break;
		case PSI:
			mbar = val.fp * 68.95;
			break;
		}
		if (mbar > 5 && mbar < 500000) {
			pressure->mbar = mbar + 0.5;
			break;
		}
	/* fallthrough */
	default:
		printf("Strange pressure reading %s\n", buffer);
	}
	free(buffer);
}

static void depth(char *buffer, void *_depth)
{
	depth_t *depth = _depth;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		switch (input_units.length) {
		case METERS:
			depth->mm = val.fp * 1000 + 0.5;
			break;
		case FEET:
			depth->mm = val.fp * 304.8 + 0.5;
			break;
		}
		break;
	default:
		printf("Strange depth reading %s\n", buffer);
	}
	free(buffer);
}

static void temperature(char *buffer, void *_temperature)
{
	temperature_t *temperature = _temperature;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Ignore zero. It means "none" */
		if (!val.fp)
			break;
		/* Celsius */
		switch (input_units.temperature) {
		case KELVIN:
			temperature->mkelvin = val.fp * 1000;
			break;
		case CELSIUS:
			temperature->mkelvin = (val.fp + 273.15) * 1000 + 0.5;
			break;
		case FAHRENHEIT:
			temperature->mkelvin = (val.fp + 459.67) * 5000/9;
			break;
		}
		break;
	default:
		printf("Strange temperature reading %s\n", buffer);
	}
	free(buffer);
}

static void sampletime(char *buffer, void *_time)
{
	int i;
	int min, sec;
	duration_t *time = _time;

	i = sscanf(buffer, "%d:%d", &min, &sec);
	switch (i) {
	case 1:
		sec = min;
		min = 0;
	/* fallthrough */
	case 2:
		time->seconds = sec + min*60;
		break;
	default:
		printf("Strange sample time reading %s\n", buffer);
	}
	free(buffer);
}

static void duration(char *buffer, void *_time)
{
	sampletime(buffer, _time);
}

static void percent(char *buffer, void *_fraction)
{
	fraction_t *fraction = _fraction;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		if (val.fp <= 100.0)
			fraction->permille = val.fp * 10 + 0.5;
		break;

	default:
		printf("Strange percentage reading %s\n", buffer);
		break;
	}
	free(buffer);
}

static void gasmix(char *buffer, void *_fraction)
{
	/* libdivecomputer does negative percentages. */
	if (*buffer == '-')
		return;
	if (cylinder_index < MAX_CYLINDERS)
		percent(buffer, _fraction);
}

static void gasmix_nitrogen(char *buffer, void *_gasmix)
{
	/* Ignore n2 percentages. There's no value in them. */
}

static void cylindersize(char *buffer, void *_volume)
{
	volume_t *volume = _volume;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		volume->mliter = val.fp * 1000 + 0.5;
		break;

	default:
		printf("Strange volume reading %s\n", buffer);
		break;
	}
	free(buffer);
}

static void utf8_string(char *buffer, void *_res)
{
	*(char **)_res = buffer;
}

/*
 * Uemis water_pressure. In centibar. And when converting to
 * depth, I'm just going to always use saltwater, because I
 * think "true depth" is just stupid. From a diving standpoint,
 * "true depth" is pretty much completely pointless, unless
 * you're doing some kind of underwater surveying work.
 *
 * So I give water depths in "pressure depth", always assuming
 * salt water. So one atmosphere per 10m.
 */
static void water_pressure(char *buffer, void *_depth)
{
	depth_t *depth = _depth;
	union int_or_float val;
	double atm, cm;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		if (!val.fp)
			break;
		/* cbar to atm */
		atm = (val.fp / 100) / 1.01325;
		/*
		 * atm to cm. Why not mm? The precision just isn't
		 * there.
		 */
		cm = 100 * atm + 0.5;
		if (cm > 0) {
			depth->mm = 10 * (long)cm;
			break;
		}
	default:
		fprintf(stderr, "Strange water pressure '%s'\n", buffer);
	}
	free(buffer);
}

#define MATCH(pattern, fn, dest) \
	match(pattern, strlen(pattern), name, len, fn, buf, dest)

static void get_index(char *buffer, void *_i)
{
	int *i = _i;
	*i = atoi(buffer);
	free(buffer);
}

static void centibar(char *buffer, void *_pressure)
{
	pressure_t *pressure = _pressure;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		pressure->mbar = val.fp * 10 + 0.5;
		break;
	default:
		fprintf(stderr, "Strange centibar pressure '%s'\n", buffer);
	}
	free(buffer);
}

static void decicelsius(char *buffer, void *_temp)
{
	temperature_t *temp = _temp;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		temp->mkelvin = (val.fp/10 + 273.15) * 1000 + 0.5;
		break;
	default:
		fprintf(stderr, "Strange julian date: %s", buffer);
	}
	free(buffer);
}

static int uemis_fill_sample(struct sample *sample, const char *name, int len, char *buf)
{
	return	MATCH(".reading.dive_time", sampletime, &sample->time) ||
		MATCH(".reading.water_pressure", water_pressure, &sample->depth) ||
		MATCH(".reading.active_tank", get_index, &sample->cylinderindex) ||
		MATCH(".reading.tank_pressure", centibar, &sample->cylinderpressure) ||
		MATCH(".reading.dive_temperature", decicelsius, &sample->temperature) ||
		0;
}

/*
 * Divinglog is crazy. The temperatures are in celsius. EXCEPT
 * for the sample temperatures, that are in Fahrenheit.
 * WTF?
 *
 * Oh, and I think Diving Log *internally* probably kept them
 * in celsius, because I'm seeing entries like
 *
 *	<Temp>32.0</Temp>
 *
 * in there. Which is freezing, aka 0 degC. I bet the "0" is
 * what Diving Log uses for "no temperature".
 *
 * So throw away crap like that.
 */
static void fahrenheit(char *buffer, void *_temperature)
{
	temperature_t *temperature = _temperature;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Floating point equality is evil, but works for small integers */
		if (val.fp == 32.0)
			break;
		temperature->mkelvin = (val.fp + 459.67) * 5000/9;
		break;
	default:
		fprintf(stderr, "Crazy Diving Log temperature reading %s\n", buffer);
	}
	free(buffer);
}

/*
 * Did I mention how bat-shit crazy divinglog is? The sample
 * pressures are in PSI. But the tank working pressure is in
 * bar. WTF^2?
 *
 * Crazy stuff like this is why subsurface has everything in
 * these inconvenient typed structures, and you have to say
 * "pressure->mbar" to get the actual value. Exactly so that
 * you can never have unit confusion.
 */
static void psi(char *buffer, void *_pressure)
{
	pressure_t *pressure = _pressure;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		pressure->mbar = val.fp * 68.95 + 0.5;
		break;
	default:
		fprintf(stderr, "Crazy Diving Log PSI reading %s\n", buffer);
	}
	free(buffer);
}

static int divinglog_fill_sample(struct sample *sample, const char *name, int len, char *buf)
{
	return	MATCH(".p.time", sampletime, &sample->time) ||
		MATCH(".p.depth", depth, &sample->depth) ||
		MATCH(".p.temp", fahrenheit, &sample->temperature) ||
		MATCH(".p.press1", psi, &sample->cylinderpressure) ||
		0;
}

static int uddf_fill_sample(struct sample *sample, const char *name, int len, char *buf)
{
	return	MATCH(".divetime", sampletime, &sample->time) ||
		MATCH(".depth", depth, &sample->depth) ||
		MATCH(".temperature", temperature, &sample->temperature) ||
		0;
}

static void eventtime(char *buffer, void *_duration)
{
	duration_t *duration = _duration;
	sampletime(buffer, duration);
	if (sample)
		duration->seconds += sample->time.seconds;
}

static void try_to_fill_event(const char *name, char *buf)
{
	int len = strlen(name);

	start_match("event", name, buf);
	if (MATCH(".event", utf8_string, &event.name))
		return;
	if (MATCH(".name", utf8_string, &event.name))
		return;
	if (MATCH(".time", eventtime, &event.time))
		return;
	if (MATCH(".type", get_index, &event.type))
		return;
	if (MATCH(".flags", get_index, &event.flags))
		return;
	nonmatch("event", name, buf);
}

/* We're in samples - try to convert the random xml value to something useful */
static void try_to_fill_sample(struct sample *sample, const char *name, char *buf)
{
	int len = strlen(name);

	start_match("sample", name, buf);
	if (MATCH(".sample.pressure", pressure, &sample->cylinderpressure))
		return;
	if (MATCH(".sample.cylpress", pressure, &sample->cylinderpressure))
		return;
	if (MATCH(".sample.depth", depth, &sample->depth))
		return;
	if (MATCH(".sample.temp", temperature, &sample->temperature))
		return;
	if (MATCH(".sample.temperature", temperature, &sample->temperature))
		return;
	if (MATCH(".sample.sampletime", sampletime, &sample->time))
		return;
	if (MATCH(".sample.time", sampletime, &sample->time))
		return;

	switch (import_source) {
	case UEMIS:
		if (uemis_fill_sample(sample, name, len, buf))
			return;
		break;

	case DIVINGLOG:
		if (divinglog_fill_sample(sample, name, len, buf))
			return;
		break;

	case UDDF:
		if (uddf_fill_sample(sample, name, len, buf))
			return;
		break;

	default:
		break;
	}

	nonmatch("sample", name, buf);
}

/*
 * Crazy suunto xml. Look at how those o2/he things match up.
 */
static int suunto_dive_match(struct dive *dive, const char *name, int len, char *buf)
{
	return	MATCH(".o2pct", percent, &dive->cylinder[0].gasmix.o2) ||
		MATCH(".hepct_0", percent, &dive->cylinder[0].gasmix.he) ||
		MATCH(".o2pct_2", percent, &dive->cylinder[1].gasmix.o2) ||
		MATCH(".hepct_1", percent, &dive->cylinder[1].gasmix.he) ||
		MATCH(".o2pct_3", percent, &dive->cylinder[2].gasmix.o2) ||
		MATCH(".hepct_2", percent, &dive->cylinder[2].gasmix.he) ||
		MATCH(".o2pct_4", percent, &dive->cylinder[3].gasmix.o2) ||
		MATCH(".hepct_3", percent, &dive->cylinder[3].gasmix.he) ||
		MATCH(".cylindersize", cylindersize, &dive->cylinder[0].type.size) ||
		MATCH(".cylinderworkpressure", pressure, &dive->cylinder[0].type.workingpressure) ||
		0;
}

static const char *country, *city;

static void divinglog_place(char *place, void *_location)
{
	char **location = _location;
	char buffer[256], *p;
	int len;

	len = snprintf(buffer, sizeof(buffer),
		"%s%s%s%s%s",
		place,
		city ? ", " : "",
		city ? city : "",
		country ? ", " : "",
		country ? country : "");

	p = malloc(len+1);
	memcpy(p, buffer, len+1);
	*location = p;

	city = NULL;
	country = NULL;
}

static int divinglog_dive_match(struct dive *dive, const char *name, int len, char *buf)
{
	return	MATCH(".divedate", divedate, &dive->when) ||
		MATCH(".entrytime", divetime, &dive->when) ||
		MATCH(".depth", depth, &dive->maxdepth) ||
		MATCH(".tanksize", cylindersize, &dive->cylinder[0].type.size) ||
		MATCH(".presw", pressure, &dive->cylinder[0].type.workingpressure) ||
		MATCH(".comments", utf8_string, &dive->notes) ||
		MATCH(".buddy.names", utf8_string, &dive->buddy) ||
		MATCH(".country.name", utf8_string, &country) ||
		MATCH(".city.name", utf8_string, &city) ||
		MATCH(".place.name", divinglog_place, &dive->location) ||
		0;
}

static int buffer_value(char *buffer)
{
	int val = atoi(buffer);
	free(buffer);
	return val;
}

static void uemis_length_unit(char *buffer, void *_unused)
{
	input_units.length = buffer_value(buffer) ? FEET : METERS;
}

static void uemis_volume_unit(char *buffer, void *_unused)
{
	input_units.volume = buffer_value(buffer) ? CUFT : LITER;
}

static void uemis_pressure_unit(char *buffer, void *_unused)
{
#if 0
	input_units.pressure = buffer_value(buffer) ? PSI : BAR;
#endif
}

static void uemis_temperature_unit(char *buffer, void *_unused)
{
	input_units.temperature = buffer_value(buffer) ? FAHRENHEIT : CELSIUS;
}

static void uemis_weight_unit(char *buffer, void *_unused)
{
	input_units.weight = buffer_value(buffer) ? LBS : KG;
}

static void uemis_time_unit(char *buffer, void *_unused)
{
}

static void uemis_date_unit(char *buffer, void *_unused)
{
}

/* Modified julian day, yay! */
static void uemis_date_time(char *buffer, void *_when)
{
	time_t *when = _when;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		*when = (val.fp - 40587) * 86400;
		break;
	default:
		fprintf(stderr, "Strange julian date: %s", buffer);
	}
	free(buffer);
}

/*
 * Uemis doesn't know time zones. You need to do them as
 * minutes, not hours.
 *
 * But that's ok, we don't track timezones yet either. We
 * just turn everything into "localtime expressed as UTC".
 */
static void uemis_time_zone(char *buffer, void *_when)
{
#if 0 /* seems like this is only used to display it correctly
       * the stored time appears to be UTC */

	time_t *when = _when;
	signed char tz = atoi(buffer);

	*when += tz * 3600;
#endif
}

/* 0 - air ; 1 - nitrox1 ; 2 - nitrox2 ; 3 = nitrox3 */
static int uemis_gas_template;

/*
 * Christ. Uemis tank data is a total mess.
 *
 * We're passed a "virtual cylinder" (0 - 6) for the different
 * Uemis tank cases ("air", "nitrox_1", "nitrox_2.{bottom,deco}"
 * and "nitrox_3.{bottom,deco,travel}". We need to turn that
 * into the actual cylinder data depending on the gas template,
 * and ignore the ones that are irrelevant for that template.
 *
 * So for "template 2" (nitrox_2), we ignore virtual tanks 0-1
 * (which are "air" and "nitrox_1" respectively), and tanks 4-6
 * (which are the three "nitrox_3" tanks), and we turn virtual
 * tanks 2/3 into actual tanks 0/1.
 *
 * Confused yet?
 */
static int uemis_cylinder_index(void *_cylinder)
{
	cylinder_t *cylinder = _cylinder;
	unsigned int index = cylinder - dive->cylinder;

	if (index > 6) {
		fprintf(stderr, "Uemis cylinder pointer calculations broken\n");
		return -1;
	}
	switch(uemis_gas_template) {
	case 1:	/* Dive uses tank 1 */
		index -= 1;
	/* Fallthrough */
	case 0:	/* Dive uses tank 0 */
		if (index)
			index = -1;
		break;
	case 2: /* Dive uses tanks 2-3 */
		index -= 2;
		if (index > 1)
			index = -1;
		break;
	case 3: /* Dive uses tanks 4-6 */
		index -= 4;
		if (index > 2)
			index = -1;
		break;
	}
	return index;
}

static void uemis_cylindersize(char *buffer, void *_cylinder)
{
	int index = uemis_cylinder_index(_cylinder);
	if (index >= 0)
		cylindersize(buffer, &dive->cylinder[index].type.size);
}

static void uemis_percent(char *buffer, void *_cylinder)
{
	int index = uemis_cylinder_index(_cylinder);
	if (index >= 0)
		percent(buffer, &dive->cylinder[index].gasmix.o2);
}

static int uemis_dive_match(struct dive *dive, const char *name, int len, char *buf)
{
	return	MATCH(".units.length", uemis_length_unit, &input_units) ||
		MATCH(".units.volume", uemis_volume_unit, &input_units) ||
		MATCH(".units.pressure", uemis_pressure_unit, &input_units) ||
		MATCH(".units.temperature", uemis_temperature_unit, &input_units) ||
		MATCH(".units.weight", uemis_weight_unit, &input_units) ||
		MATCH(".units.time", uemis_time_unit, &input_units) ||
		MATCH(".units.date", uemis_date_unit, &input_units) ||
		MATCH(".date_time", uemis_date_time, &dive->when) ||
		MATCH(".time_zone", uemis_time_zone, &dive->when) ||
		MATCH(".ambient.temperature", decicelsius, &dive->airtemp) ||
		MATCH(".gas.template", get_index, &uemis_gas_template) ||
		MATCH(".air.bottom_tank.size", uemis_cylindersize, dive->cylinder + 0) ||
		MATCH(".air.bottom_tank.oxygen", uemis_percent, dive->cylinder + 0) ||
		MATCH(".nitrox_1.bottom_tank.size", uemis_cylindersize, dive->cylinder + 1) ||
		MATCH(".nitrox_1.bottom_tank.oxygen", uemis_percent, dive->cylinder + 1) ||
		MATCH(".nitrox_2.bottom_tank.size", uemis_cylindersize, dive->cylinder + 2) ||
		MATCH(".nitrox_2.bottom_tank.oxygen", uemis_percent, dive->cylinder + 2) ||
		MATCH(".nitrox_2.deco_tank.size", uemis_cylindersize, dive->cylinder + 3) ||
		MATCH(".nitrox_2.deco_tank.oxygen", uemis_percent, dive->cylinder + 3) ||
		MATCH(".nitrox_3.bottom_tank.size", uemis_cylindersize, dive->cylinder + 4) ||
		MATCH(".nitrox_3.bottom_tank.oxygen", uemis_percent, dive->cylinder + 4) ||
		MATCH(".nitrox_3.deco_tank.size", uemis_cylindersize, dive->cylinder + 5) ||
		MATCH(".nitrox_3.deco_tank.oxygen", uemis_percent, dive->cylinder + 5) ||
		MATCH(".nitrox_3.travel_tank.size", uemis_cylindersize, dive->cylinder + 6) ||
		MATCH(".nitrox_3.travel_tank.oxygen", uemis_percent, dive->cylinder + 6) ||
		0;
}

/*
 * Uddf specifies ISO 8601 time format.
 *
 * There are many variations on that. This handles the useful cases.
 */
static void uddf_datetime(char *buffer, void *_when)
{
	char c;
	int y,m,d,hh,mm,ss;
	time_t *when = _when;
	struct tm tm = { 0 };
	int i;

	i = sscanf(buffer, "%d-%d-%d%c%d:%d:%d", &y, &m, &d, &c, &hh, &mm, &ss);
	if (i == 7)
		goto success;
	ss = 0;
	if (i == 6)
		goto success;

	i = sscanf(buffer, "%04d%02d%02d%c%02d%02d%02d", &y, &m, &d, &c, &hh, &mm, &ss);
	if (i == 7)
		goto success;
	ss = 0;
	if (i == 6)
		goto success;
bad_date:
	printf("Bad date time %s\n", buffer);
	free(buffer);
	return;

success:
	if (c != 'T' && c != ' ')
		goto bad_date;
	tm.tm_year = y;
	tm.tm_mon = m - 1;
	tm.tm_mday = d;
	tm.tm_hour = hh;
	tm.tm_min = mm;
	tm.tm_sec = ss;
	*when = utc_mktime(&tm);
	free(buffer);
}

static int uddf_dive_match(struct dive *dive, const char *name, int len, char *buf)
{
	return	MATCH(".datetime", uddf_datetime, &dive->when) ||
		MATCH(".diveduration", duration, &dive->duration) ||
		MATCH(".greatestdepth", depth, &dive->maxdepth) ||
		0;
}

static void gps_location(char *buffer, void *_dive)
{
	int i;
	struct dive *dive = _dive;
	double latitude, longitude;

	i = sscanf(buffer, "%lf %lf", &latitude, &longitude);
	if (i == 2) {
		dive->latitude = latitude;
		dive->longitude = longitude;
	}
	free(buffer);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, char *buf)
{
	int len = strlen(name);

	start_match("dive", name, buf);

	switch (import_source) {
	case SUUNTO:
		if (suunto_dive_match(dive, name, len, buf))
			return;
		break;

	case UEMIS:
		if (uemis_dive_match(dive, name, len, buf))
			return;
		break;

	case DIVINGLOG:
		if (divinglog_dive_match(dive, name, len, buf))
			return;
		break;

	case UDDF:
		if (uddf_dive_match(dive, name, len, buf))
			return;
		break;

	default:
		break;
	}

	if (MATCH(".number", get_index, &dive->number))
		return;
	if (MATCH(".date", divedate, &dive->when))
		return;
	if (MATCH(".time", divetime, &dive->when))
		return;
	if (MATCH(".datetime", divedatetime, &dive->when))
		return;
	if (MATCH(".maxdepth", depth, &dive->maxdepth))
		return;
	if (MATCH(".meandepth", depth, &dive->meandepth))
		return;
	if (MATCH(".depth.max", depth, &dive->maxdepth))
		return;
	if (MATCH(".depth.mean", depth, &dive->meandepth))
		return;
	if (MATCH(".duration", duration, &dive->duration))
		return;
	if (MATCH(".divetime", duration, &dive->duration))
		return;
	if (MATCH(".divetimesec", duration, &dive->duration))
		return;
	if (MATCH(".surfacetime", duration, &dive->surfacetime))
		return;
	if (MATCH(".airtemp", temperature, &dive->airtemp))
		return;
	if (MATCH(".watertemp", temperature, &dive->watertemp))
		return;
	if (MATCH(".temperature.air", temperature, &dive->airtemp))
		return;
	if (MATCH(".temperature.water", temperature, &dive->watertemp))
		return;
	if (MATCH(".cylinderstartpressure", pressure, &dive->cylinder[0].start))
		return;
	if (MATCH(".cylinderendpressure", pressure, &dive->cylinder[0].end))
		return;
	if (MATCH(".gps", gps_location, dive))
		return;
	if (MATCH(".location", utf8_string, &dive->location))
		return;
	if (MATCH(".notes", utf8_string, &dive->notes))
		return;
	if (MATCH(".divemaster", utf8_string, &dive->divemaster))
		return;
	if (MATCH(".buddy", utf8_string, &dive->buddy))
		return;

	if (MATCH(".cylinder.size", cylindersize, &dive->cylinder[cylinder_index].type.size))
		return;
	if (MATCH(".cylinder.workpressure", pressure, &dive->cylinder[cylinder_index].type.workingpressure))
		return;
	if (MATCH(".cylinder.description", utf8_string, &dive->cylinder[cylinder_index].type.description))
		return;
	if (MATCH(".cylinder.start", pressure, &dive->cylinder[cylinder_index].start))
		return;
	if (MATCH(".cylinder.end", pressure, &dive->cylinder[cylinder_index].end))
		return;

	if (MATCH(".o2", gasmix, &dive->cylinder[cylinder_index].gasmix.o2))
		return;
	if (MATCH(".n2", gasmix_nitrogen, &dive->cylinder[cylinder_index].gasmix))
		return;
	if (MATCH(".he", gasmix, &dive->cylinder[cylinder_index].gasmix.he))
		return;

	nonmatch("dive", name, buf);
}

/*
 * File boundaries are dive boundaries. But sometimes there are
 * multiple dives per file, so there can be other events too that
 * trigger a "new dive" marker and you may get some nesting due
 * to that. Just ignore nesting levels.
 */
static void dive_start(void)
{
	if (dive)
		return;
	dive = alloc_dive();
	memset(&tm, 0, sizeof(tm));
}

static void sanitize_gasmix(struct gasmix *mix)
{
	unsigned int o2, he;

	o2 = mix->o2.permille;
	he = mix->he.permille;

	/* Regular air: leave empty */
	if (!he) {
		if (!o2)
			return;
		/* 20.9% or 21% O2 is just air */
		if (o2 >= 209 && o2 <= 210) {
			mix->o2.permille = 0;
			return;
		}
	}

	/* Sane mix? */
	if (o2 <= 1000 && he <= 1000 && o2+he <= 1000)
		return;
	fprintf(stderr, "Odd gasmix: %d O2 %d He\n", o2, he);
	memset(mix, 0, sizeof(*mix));
}

/*
 * See if the size/workingpressure looks like some standard cylinder
 * size, eg "AL80".
 */
static void match_standard_cylinder(cylinder_type_t *type)
{
	double cuft;
	int psi, len;
	const char *fmt;
	char buffer[20], *p;

	/* Do we already have a cylinder description? */
	if (type->description)
		return;

	cuft = type->size.mliter / 28317.0;
	cuft *= to_ATM(type->workingpressure);
	psi = type->workingpressure.mbar / 68.95;

	switch (psi) {
	case 2300 ... 2500:	/* 2400 psi: LP tank */
		fmt = "LP%d";
		break;
	case 2600 ... 2700:	/* 2640 psi: LP+10% */
		fmt = "LP%d";
		break;
	case 2900 ... 3100:	/* 3000 psi: ALx tank */
		fmt = "AL%d";
		break;
	case 3400 ... 3500:	/* 3442 psi: HP tank */
		fmt = "HP%d";
		break;
	case 3700 ... 3850:	/* HP+10% */
		fmt = "HP%d+";
		break;
	default:
		return;
	}
	len = snprintf(buffer, sizeof(buffer), fmt, (int) (cuft+0.5));
	p = malloc(len+1);
	if (!p)
		return;
	memcpy(p, buffer, len+1);
	type->description = p;
}


/*
 * There are two ways to give cylinder size information:
 *  - total amount of gas in cuft (depends on working pressure and physical size)
 *  - physical size
 *
 * where "physical size" is the one that actually matters and is sane.
 *
 * We internally use physical size only. But we save the workingpressure
 * so that we can do the conversion if required.
 */
static void sanitize_cylinder_type(cylinder_type_t *type)
{
	double volume_of_air, atm, volume;

	/* If we have no working pressure, it had *better* be just a physical size! */
	if (!type->workingpressure.mbar)
		return;

	/* No size either? Nothing to go on */
	if (!type->size.mliter)
		return;

	if (input_units.volume == CUFT || import_source == SUUNTO) {
		volume_of_air = type->size.mliter * 28.317;	/* milli-cu ft to milliliter */
		atm = to_ATM(type->workingpressure);		/* working pressure in atm */
		volume = volume_of_air / atm;			/* milliliters at 1 atm: "true size" */
		type->size.mliter = volume + 0.5;
	}

	/* Ok, we have both size and pressure: try to match a description */
	match_standard_cylinder(type);
}

static void sanitize_cylinder_info(struct dive *dive)
{
	int i;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		sanitize_gasmix(&dive->cylinder[i].gasmix);
		sanitize_cylinder_type(&dive->cylinder[i].type);
	}
}

static void dive_end(void)
{
	if (!dive)
		return;
	sanitize_cylinder_info(dive);
	record_dive(dive);
	dive = NULL;
	cylinder_index = 0;
}

static void event_start(void)
{
	memset(&event, 0, sizeof(event));
	event.active = 1;
}

static void event_end(void)
{
	if (event.name && strcmp(event.name, "surface") != 0)
		add_event(dive, event.time.seconds, event.type, event.flags, event.value, event.name);
	event.active = 0;
}

static void cylinder_start(void)
{
}

static void cylinder_end(void)
{
	cylinder_index++;
}

static void sample_start(void)
{
	sample = prepare_sample(&dive);
}

static void sample_end(void)
{
	if (!dive)
		return;

	finish_sample(dive, sample);
	sample = NULL;
}

static void entry(const char *name, int size, const char *raw)
{
	char *buf = malloc(size+1);

	if (!buf)
		return;
	memcpy(buf, raw, size);
	buf[size] = 0;
	if (event.active) {
		try_to_fill_event(name, buf);
		return;
	}
	if (sample) {
		try_to_fill_sample(sample, name, buf);
		return;
	}
	if (dive) {
		try_to_fill_dive(dive, name, buf);
		return;
	}
}

static const char *nodename(xmlNode *node, char *buf, int len)
{
	if (!node || !node->name)
		return "root";

	buf += len;
	*--buf = 0;
	len--;

	for(;;) {
		const char *name = node->name;
		int i = strlen(name);
		while (--i >= 0) {
			unsigned char c = name[i];
			*--buf = tolower(c);
			if (!--len)
				return buf;
		}
		node = node->parent;
		if (!node || !node->name)
			return buf;
		*--buf = '.';
		if (!--len)
			return buf;
	}
}

#define MAXNAME 64

static void visit_one_node(xmlNode *node)
{
	int len;
	const unsigned char *content;
	char buffer[MAXNAME];
	const char *name;

	content = node->content;
	if (!content)
		return;

	/* Trim whitespace at beginning */
	while (isspace(*content))
		content++;

	/* Trim whitespace at end */
	len = strlen(content);
	while (len && isspace(content[len-1]))
		len--;

	if (!len)
		return;

	/* Don't print out the node name if it is "text" */
	if (!strcmp(node->name, "text"))
		node = node->parent;

	name = nodename(node, buffer, sizeof(buffer));

	entry(name, len, content);
}

static void traverse(xmlNode *root);

static void traverse_properties(xmlNode *node)
{
	xmlAttr *p;

	for (p = node->properties; p; p = p->next)
		traverse(p->children);
}

static void visit(xmlNode *n)
{
	visit_one_node(n);
	traverse_properties(n);
	traverse(n->children);
}

static void suunto_importer(void)
{
	import_source = SUUNTO;
	input_units = SI_units;
}

static void uemis_importer(void)
{
	import_source = UEMIS;
	input_units = SI_units;
}

static void DivingLog_importer(void)
{
	import_source = DIVINGLOG;

	/*
	 * Diving Log units are really strange.
	 *
	 * Temperatures are in C, except in samples,
	 * when they are in Fahrenheit. Depths are in
	 * meters, an dpressure is in PSI in the samples,
	 * but in bar when it comes to working pressure.
	 *
	 * Crazy f*%^ morons.
	 */
	input_units = SI_units;
}

static void uddf_importer(void)
{
	import_source = UDDF;
	input_units = SI_units;
	input_units.pressure = PASCAL;
	input_units.temperature = KELVIN;
}

/*
 * I'm sure this could be done as some fancy DTD rules.
 * It's just not worth the headache.
 */
static struct nesting {
	const char *name;
	void (*start)(void), (*end)(void);
} nesting[] = {
	{ "dive", dive_start, dive_end },
	{ "Dive", dive_start, dive_end },
	{ "sample", sample_start, sample_end },
	{ "waypoint", sample_start, sample_end },
	{ "SAMPLE", sample_start, sample_end },
	{ "reading", sample_start, sample_end },
	{ "event", event_start, event_end },
	{ "gasmix", cylinder_start, cylinder_end },
	{ "cylinder", cylinder_start, cylinder_end },
	{ "P", sample_start, sample_end },

	/* Import type recognition */
	{ "SUUNTO", suunto_importer },
	{ "Divinglog", DivingLog_importer },
	{ "pre_dive", uemis_importer },
	{ "uddf", uddf_importer },

	{ NULL, }
};

static void traverse(xmlNode *root)
{
	xmlNode *n;

	for (n = root; n; n = n->next) {
		struct nesting *rule = nesting;

		do {
			if (!strcmp(rule->name, n->name))
				break;
			rule++;
		} while (rule->name);

		if (rule->start)
			rule->start();
		visit(n);
		if (rule->end)
			rule->end();
	}
}

/* Per-file reset */
static void reset_all(void)
{
	/*
	 * We reset the units for each file. You'd think it was
	 * a per-dive property, but I'm not going to trust people
	 * to do per-dive setup. If the xml does have per-dive
	 * data within one file, we might have to reset it per
	 * dive for that format.
	 */
	input_units = SI_units;
	import_source = UNKNOWN;
}

void parse_xml_file(const char *filename, GError **error)
{
	xmlDoc *doc;

	doc = xmlReadFile(filename, NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to parse '%s'.\n", filename);
		if (error != NULL)
		{
			*error = g_error_new(g_quark_from_string("subsurface"),
					     DIVE_ERROR_PARSE,
					     "Failed to parse '%s'",
					     filename);
		}
		return;
	}
	/* we assume that the last (or only) filename passed as argument is a 
	 * great filename to use as default when saving the dives */ 
	set_filename(filename);
	reset_all();
	dive_start();
	traverse(xmlDocGetRootElement(doc));
	dive_end();
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

void parse_xml_init(void)
{
	LIBXML_TEST_VERSION
}
