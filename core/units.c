// SPDX-License-Identifier: GPL-2.0
#include "units.h"
#include "core/errorhelper.h"
#include "gettext.h"
#include "pref.h"
#include <stddef.h>
#include <stdlib.h>

#define IMPERIAL_UNITS                                                                                     \
        {                                                                                                  \
	        .length = FEET, .volume = CUFT, .pressure = PSI, .temperature = FAHRENHEIT, .weight = LBS, \
		.vertical_speed_time = MINUTES, .duration_units = MIXED, .show_units_table = false         \
        }

const struct units SI_units = SI_UNITS;
const struct units IMPERIAL_units = IMPERIAL_UNITS;

int get_pressure_units(int mb, const char **units)
{
	int pressure;
	const char *unit;
	const struct units *units_p = get_units();

	switch (units_p->pressure) {
	case BAR:
	default:
		pressure = (mb + 500) / 1000;
		unit = translate("gettextFromC", "bar");
		break;
	case PSI:
		pressure = (int)lrint(mbar_to_PSI(mb));
		unit = translate("gettextFromC", "psi");
		break;
	}
	if (units)
		*units = unit;
	return pressure;
}

double get_temp_units(unsigned int mk, const char **units)
{
	double deg;
	const char *unit;
	const struct units *units_p = get_units();

	if (units_p->temperature == FAHRENHEIT) {
		deg = mkelvin_to_F(mk);
		unit = "°F";
	} else {
		deg = mkelvin_to_C(mk);
		unit = "°C";
	}
	if (units)
		*units = unit;
	return deg;
}

double get_volume_units(unsigned int ml, int *frac, const char **units)
{
	int decimals;
	double vol;
	const char *unit;
	const struct units *units_p = get_units();

	switch (units_p->volume) {
	case LITER:
	default:
		vol = ml / 1000.0;
		unit = translate("gettextFromC", "ℓ");
		decimals = 1;
		break;
	case CUFT:
		vol = ml_to_cuft(ml);
		unit = translate("gettextFromC", "cuft");
		decimals = 2;
		break;
	}
	if (frac)
		*frac = decimals;
	if (units)
		*units = unit;
	return vol;
}

int units_to_sac(double volume)
{
	if (get_units()->volume == CUFT)
		return lrint(cuft_to_l(volume) * 1000.0);
	else
		return lrint(volume * 1000);
}

depth_t units_to_depth(double depth)
{
	depth_t internaldepth;
	if (get_units()->length == METERS) {
		internaldepth.mm = lrint(depth * 1000);
	} else {
		internaldepth.mm = feet_to_mm(depth);
	}
	return internaldepth;
}

double get_depth_units(int mm, int *frac, const char **units)
{
	int decimals;
	double d;
	const char *unit;
	const struct units *units_p = get_units();

	switch (units_p->length) {
	case METERS:
	default:
		d = mm / 1000.0;
		unit = translate("gettextFromC", "m");
		decimals = d < 20;
		break;
	case FEET:
		d = mm_to_feet(mm);
		unit = translate("gettextFromC", "ft");
		decimals = 0;
		break;
	}
	if (frac)
		*frac = decimals;
	if (units)
		*units = unit;
	return d;
}

double get_vertical_speed_units(unsigned int mms, int *frac, const char **units)
{
	double d;
	const char *unit;
	const struct units *units_p = get_units();
	const double time_factor = units_p->vertical_speed_time == MINUTES ? 60.0 : 1.0;

	switch (units_p->length) {
	case METERS:
	default:
		d = mms / 1000.0 * time_factor;
		if (units_p->vertical_speed_time == MINUTES)
			unit = translate("gettextFromC", "m/min");
		else
			unit = translate("gettextFromC", "m/s");
		break;
	case FEET:
		d = mm_to_feet(mms) * time_factor;
		if (units_p->vertical_speed_time == MINUTES)
			unit = translate("gettextFromC", "ft/min");
		else
			unit = translate("gettextFromC", "ft/s");
		break;
	}
	if (frac)
		*frac = d < 10;
	if (units)
		*units = unit;
	return d;
}

double get_weight_units(unsigned int grams, int *frac, const char **units)
{
	int decimals;
	double value;
	const char *unit;
	const struct units *units_p = get_units();

	if (units_p->weight == LBS) {
		value = grams_to_lbs(grams);
		unit = translate("gettextFromC", "lbs");
		decimals = 0;
	} else {
		value = grams / 1000.0;
		unit = translate("gettextFromC", "kg");
		decimals = 1;
	}
	if (frac)
		*frac = decimals;
	if (units)
		*units = unit;
	return value;
}

const struct units *get_units()
{
	return &prefs.units;
}

int parse_duration(const char *buffer, duration_t *time, bool dot_as_seperator)
{
	char *line = (char*)buffer;
	size_t n = 0;
	long values[3];
	char separator;
	while (n < sizeof(values)/sizeof(values[0])) {
		values[n++] = strtol(line, &line, 10);
		separator = *line;
		line++;
		/* special case of `mm.ss`
		 * DivingLog 5.08 (and maybe other versions) appear to sometimes
		 * store the dive time as 44.00 instead of 44:00 */
		if (dot_as_seperator && n == 1 && separator == '.')
			continue;
		if (separator != ':')
			break;
	}
	switch (n) {
	case 1:
		/* mm */
		time->seconds = values[0] * 60;
		time->ms = 0;
		goto exit_out;
	case 2:
		/* mm:ss */
		time->seconds = values[0] * 60 + values[1];
		break;
	case 3:
		/* hh:mm:ss */
		time->seconds = values[0] * 360 + values[1] * 60 + values[2];
		break;
	default:
		time->seconds = 0;
		time->ms = 0;
		report_info("Strange sample time reading %s", buffer);
		return 0;
	}
	long ms = 0;
	if (separator == '.')
		ms = strtol(line, &line, 10);
	time->ms = ms;
exit_out:
	return line - buffer;
}
