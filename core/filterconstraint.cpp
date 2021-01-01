// SPDX-License-Identifier: GPL-2.0
#include "filterconstraint.h"
#include "dive.h"
#include "divesite.h"
#include "errorhelper.h"
#include "gettextfromc.h"
#include "qthelper.h"
#include "tag.h"
#include "trip.h"
#include "string-format.h"
#include "subsurface-string.h"
#include "subsurface-time.h"
#include <QDateTime>

// We use the units enum only internally.
// Therefore define it here, not in the header file.
enum filter_constraint_units {
	FILTER_CONSTRAINT_NO_UNIT = 0,
	FILTER_CONSTRAINT_LENGTH_UNIT,
	FILTER_CONSTRAINT_DURATION_UNIT,
	FILTER_CONSTRAINT_TEMPERATURE_UNIT,
	FILTER_CONSTRAINT_WEIGHT_UNIT,
	FILTER_CONSTRAINT_VOLUME_UNIT,
	FILTER_CONSTRAINT_VOLUMETRIC_FLOW_UNIT,
	FILTER_CONSTRAINT_DENSITY_UNIT,
	FILTER_CONSTRAINT_PERCENTAGE_UNIT
};

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

static struct type_description {
	filter_constraint_type type;
	const char *token;		// untranslated token, which will be written to the log and should not contain spaces
	const char *text_ui;		// text for the UI, which will be translated
	bool has_string_mode;		// constraint has textual input
	bool has_range_mode;		// constraint has equal, less than, greater than and from-to modes
	bool is_star_widget;		// is a star widget
	filter_constraint_units units;	// numerical constraints may have units
	int decimal_places;		// number of displayed decimal places for numerical constraints
	bool has_date;			// has a date widget
	bool has_time;			// has a time widget
} type_descriptions[] = {
	{ FILTER_CONSTRAINT_DATE, "date", QT_TRANSLATE_NOOP("gettextFromC", "date"), false, true, false, FILTER_CONSTRAINT_NO_UNIT, 0, true, false },
	{ FILTER_CONSTRAINT_DATE_TIME, "date_time", QT_TRANSLATE_NOOP("gettextFromC", "date time"), false, true, false, FILTER_CONSTRAINT_NO_UNIT, 0, true, true },
	{ FILTER_CONSTRAINT_TIME_OF_DAY, "time_of_day", QT_TRANSLATE_NOOP("gettextFromC", "time of day"), false, true, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, true },
	{ FILTER_CONSTRAINT_YEAR, "year", QT_TRANSLATE_NOOP("gettextFromC", "year"), false, true, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_DAY_OF_WEEK, "day_of_week", QT_TRANSLATE_NOOP("gettextFromC", "week day"), false, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },

	{ FILTER_CONSTRAINT_RATING, "rating", QT_TRANSLATE_NOOP("gettextFromC", "rating"), false, true, true, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_WAVESIZE, "wavesize", QT_TRANSLATE_NOOP("gettextFromC", "wave size"), false, true, true, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_CURRENT, "current", QT_TRANSLATE_NOOP("gettextFromC", "current"), false, true, true, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_VISIBILITY, "visibility", QT_TRANSLATE_NOOP("gettextFromC", "visibility"), false, true, true, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_SURGE, "surge", QT_TRANSLATE_NOOP("gettextFromC", "surge"), false, true, true, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_CHILL, "chill", QT_TRANSLATE_NOOP("gettextFromC", "chill"), false, true, true, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },

	{ FILTER_CONSTRAINT_DEPTH, "depth", QT_TRANSLATE_NOOP("gettextFromC", "max. depth"), false, true, false, FILTER_CONSTRAINT_LENGTH_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_DURATION, "duration", QT_TRANSLATE_NOOP("gettextFromC", "duration"), false, true, false, FILTER_CONSTRAINT_DURATION_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_WEIGHT, "weight", QT_TRANSLATE_NOOP("gettextFromC", "weight"), false, true, false, FILTER_CONSTRAINT_WEIGHT_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_WATER_TEMP, "water_temp", QT_TRANSLATE_NOOP("gettextFromC", "water temp."), false, true, false, FILTER_CONSTRAINT_TEMPERATURE_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_AIR_TEMP, "air_temp", QT_TRANSLATE_NOOP("gettextFromC", "air temp."), false, true, false, FILTER_CONSTRAINT_TEMPERATURE_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_WATER_DENSITY, "water_density", QT_TRANSLATE_NOOP("gettextFromC", "water density"), false, true, false, FILTER_CONSTRAINT_DENSITY_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_SAC, "sac", QT_TRANSLATE_NOOP("gettextFromC", "SAC"), false, true, false, FILTER_CONSTRAINT_VOLUMETRIC_FLOW_UNIT, 1, false, false },

	{ FILTER_CONSTRAINT_LOGGED, "logged", QT_TRANSLATE_NOOP("gettextFromC", "logged"), false, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_PLANNED, "planned", QT_TRANSLATE_NOOP("gettextFromC", "planned"), false, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },

	{ FILTER_CONSTRAINT_DIVE_MODE, "dive_mode", QT_TRANSLATE_NOOP("gettextFromC", "dive mode"), false, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },

	{ FILTER_CONSTRAINT_TAGS, "tags", QT_TRANSLATE_NOOP("gettextFromC", "tags"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_PEOPLE, "people", QT_TRANSLATE_NOOP("gettextFromC", "people"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_LOCATION, "location", QT_TRANSLATE_NOOP("gettextFromC", "location"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_WEIGHT_TYPE, "weight_type", QT_TRANSLATE_NOOP("gettextFromC", "weight type"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_CYLINDER_TYPE, "cylinder_type", QT_TRANSLATE_NOOP("gettextFromC", "cylinder type"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_CYLINDER_SIZE, "cylinder_size", QT_TRANSLATE_NOOP("gettextFromC", "cylinder size"), false, true, false, FILTER_CONSTRAINT_VOLUME_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_CYLINDER_N2, "cylinder_n2", QT_TRANSLATE_NOOP("gettextFromC", "gas N₂ content"), false, true, false, FILTER_CONSTRAINT_PERCENTAGE_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_CYLINDER_O2, "cylinder_o2", QT_TRANSLATE_NOOP("gettextFromC", "gas O₂ content"), false, true, false, FILTER_CONSTRAINT_PERCENTAGE_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_CYLINDER_HE, "cylinder_he", QT_TRANSLATE_NOOP("gettextFromC", "gas He content"), false, true, false, FILTER_CONSTRAINT_PERCENTAGE_UNIT, 1, false, false },
	{ FILTER_CONSTRAINT_SUIT, "suit", QT_TRANSLATE_NOOP("gettextFromC", "suit"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
	{ FILTER_CONSTRAINT_NOTES, "notes", QT_TRANSLATE_NOOP("gettextFromC", "notes"), true, false, false, FILTER_CONSTRAINT_NO_UNIT, 0, false, false },
};

static struct string_mode_description {
	filter_constraint_string_mode mode;
	const char *token;		// untranslated token, which will be written to the log and should not contain spaces
	const char *text_ui;
} string_mode_descriptions[] = {
	{ FILTER_CONSTRAINT_STARTS_WITH, "starts_with", QT_TRANSLATE_NOOP("gettextFromC", "starting with") },
	{ FILTER_CONSTRAINT_SUBSTRING, "substring", QT_TRANSLATE_NOOP("gettextFromC", "with substring") },
	{ FILTER_CONSTRAINT_EXACT, "exact", QT_TRANSLATE_NOOP("gettextFromC", "exactly") },
};

static struct range_mode_description {
	filter_constraint_range_mode mode;
	const char *token;		// untranslated token, which will be written to the log and should not contain spaces
	const char *text_ui;
	const char *text_ui_date;
} range_mode_descriptions[] = {
	{ FILTER_CONSTRAINT_EQUAL, "equal", QT_TRANSLATE_NOOP("gettextFromC", "equal to"), QT_TRANSLATE_NOOP("gettextFromC", "at") },
	{ FILTER_CONSTRAINT_LESS, "less", QT_TRANSLATE_NOOP("gettextFromC", "at most"), QT_TRANSLATE_NOOP("gettextFromC", "before") },
	{ FILTER_CONSTRAINT_GREATER, "greater", QT_TRANSLATE_NOOP("gettextFromC", "at least"), QT_TRANSLATE_NOOP("gettextFromC", "after") },
	{ FILTER_CONSTRAINT_RANGE, "range", QT_TRANSLATE_NOOP("gettextFromC", "in range"), QT_TRANSLATE_NOOP("gettextFromC", "in range") },
};

static const char *negate_description[2] {
	QT_TRANSLATE_NOOP("gettextFromC", "is"),
	QT_TRANSLATE_NOOP("gettextFromC", "is not"),
};

// std::size() is only available starting in C++17, so let's roll our own.
template <typename T, size_t N>
static constexpr size_t array_size(const T (&)[N])
{
	return N;
}

static constexpr size_t type_descriptions_count()
{
	return array_size(type_descriptions);
}

static constexpr size_t string_mode_descriptions_count()
{
	return array_size(string_mode_descriptions);
}

static constexpr size_t range_mode_descriptions_count()
{
	return array_size(range_mode_descriptions);
}

static const type_description *get_type_description(enum filter_constraint_type type)
{
	for (size_t i = 0; i < type_descriptions_count(); ++i) {
		if (type_descriptions[i].type == type)
			return &type_descriptions[i];
	}
	report_error("unknown filter constraint type: %d", type);
	return nullptr;
}

static const string_mode_description *get_string_mode_description(enum filter_constraint_string_mode mode)
{
	for (size_t i = 0; i < string_mode_descriptions_count(); ++i) {
		if (string_mode_descriptions[i].mode == mode)
			return &string_mode_descriptions[i];
	}
	report_error("unknown filter constraint string mode: %d", mode);
	return nullptr;
}

static const range_mode_description *get_range_mode_description(enum filter_constraint_range_mode mode)
{
	for (size_t i = 0; i < range_mode_descriptions_count(); ++i) {
		if (range_mode_descriptions[i].mode == mode)
			return &range_mode_descriptions[i];
	}
	report_error("unknown filter constraint range mode: %d", mode);
	return nullptr;
}

static enum filter_constraint_type filter_constraint_type_from_string(const char *s)
{
	for (size_t i = 0; i < type_descriptions_count(); ++i) {
		if (same_string(type_descriptions[i].token, s))
			return type_descriptions[i].type;
	}
	report_error("unknown filter constraint type: %s", s);
	return FILTER_CONSTRAINT_DATE;
}

static enum filter_constraint_string_mode filter_constraint_string_mode_from_string(const char *s)
{
	for (size_t i = 0; i < string_mode_descriptions_count(); ++i) {
		if (same_string(string_mode_descriptions[i].token, s))
			return string_mode_descriptions[i].mode;
	}
	report_error("unknown filter constraint string mode: %s", s);
	return FILTER_CONSTRAINT_EXACT;
}

static enum filter_constraint_range_mode filter_constraint_range_mode_from_string(const char *s)
{
	for (size_t i = 0; i < range_mode_descriptions_count(); ++i) {
		if (same_string(range_mode_descriptions[i].token, s))
			return range_mode_descriptions[i].mode;
	}
	report_error("unknown filter constraint range mode: %s", s);
	return FILTER_CONSTRAINT_EQUAL;
}

extern "C" const char *filter_constraint_type_to_string(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc ? desc->token : "unknown";
}

extern "C" const char *filter_constraint_string_mode_to_string(enum filter_constraint_string_mode mode)
{
	const string_mode_description *desc = get_string_mode_description(mode);
	return desc ? desc->token : "unknown";
}

extern "C" const char *filter_constraint_range_mode_to_string(enum filter_constraint_range_mode mode)
{
	const range_mode_description *desc = get_range_mode_description(mode);
	return desc ? desc->token : "unknown";
}

extern "C" int filter_constraint_type_to_index(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc ? desc - type_descriptions : -1;
}

extern "C" int filter_constraint_string_mode_to_index(enum filter_constraint_string_mode mode)
{
	const string_mode_description *desc = get_string_mode_description(mode);
	return desc ? desc - string_mode_descriptions : -1;
}

extern "C" int filter_constraint_range_mode_to_index(enum filter_constraint_range_mode mode)
{
	const range_mode_description *desc = get_range_mode_description(mode);
	return desc ? desc - range_mode_descriptions : -1;
}

extern "C" enum filter_constraint_type filter_constraint_type_from_index(int index)
{
	if (index >= 0 && index < (int)type_descriptions_count())
		return type_descriptions[index].type;
	return (enum filter_constraint_type)-1;
}

extern "C" enum filter_constraint_string_mode filter_constraint_string_mode_from_index(int index)
{
	if (index >= 0 && index < (int)string_mode_descriptions_count())
		return string_mode_descriptions[index].mode;
	return (enum filter_constraint_string_mode)-1;
}

extern "C" enum filter_constraint_range_mode filter_constraint_range_mode_from_index(int index)
{
	if (index >= 0 && index < (int)range_mode_descriptions_count())
		return range_mode_descriptions[index].mode;
	return (enum filter_constraint_range_mode)-1;
}

QString filter_constraint_type_to_string_translated(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc ? gettextFromC::tr(desc->text_ui) : "unknown";
}

QString filter_constraint_string_mode_to_string_translated(enum filter_constraint_string_mode type)
{
	const string_mode_description *desc = get_string_mode_description(type);
	return desc ? gettextFromC::tr(desc->text_ui) : "unknown";
}

QString filter_constraint_range_mode_to_string_translated(enum filter_constraint_range_mode type)
{
	const range_mode_description *desc = get_range_mode_description(type);
	return desc ? gettextFromC::tr(desc->text_ui) : "unknown";
}

QString filter_constraint_negate_to_string_translated(bool negate)
{
	return gettextFromC::tr(negate_description[negate]);
}

static enum filter_constraint_units type_to_unit(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc ? desc->units : FILTER_CONSTRAINT_NO_UNIT;
}

QString filter_constraint_get_unit(enum filter_constraint_type type)
{
	switch (type_to_unit(type)) {
	case FILTER_CONSTRAINT_NO_UNIT:
	default:
		return QString();
	case FILTER_CONSTRAINT_LENGTH_UNIT:
		return get_depth_unit();
	case FILTER_CONSTRAINT_DURATION_UNIT:
		return "min";
	case FILTER_CONSTRAINT_TEMPERATURE_UNIT:
		return get_temp_unit();
	case FILTER_CONSTRAINT_WEIGHT_UNIT:
		return get_weight_unit();
	case FILTER_CONSTRAINT_VOLUME_UNIT:
		return get_volume_unit();
	case FILTER_CONSTRAINT_VOLUMETRIC_FLOW_UNIT:
		return get_volume_unit() + "/min";
	case FILTER_CONSTRAINT_DENSITY_UNIT:
		return "g/ℓ";
	case FILTER_CONSTRAINT_PERCENTAGE_UNIT:
		return "%";
	}
}

static int display_to_base_unit(double f, enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	if (!desc)
		return -1;
	switch (desc->units) {
	case FILTER_CONSTRAINT_NO_UNIT:
	default:
		return (int)lrint(f);
	case FILTER_CONSTRAINT_LENGTH_UNIT:
		return prefs.units.length == units::METERS ? lrint(f * 1000.0) : feet_to_mm(f);
	case FILTER_CONSTRAINT_DURATION_UNIT:
		return lrint(f * 60.0);
	case FILTER_CONSTRAINT_TEMPERATURE_UNIT:
		return prefs.units.temperature == units::CELSIUS ? C_to_mkelvin(f) : F_to_mkelvin(f);
	case FILTER_CONSTRAINT_WEIGHT_UNIT:
		return prefs.units.weight == units::KG ? lrint(f * 1000.0) : lbs_to_grams(f);
	case FILTER_CONSTRAINT_VOLUME_UNIT:
	case FILTER_CONSTRAINT_VOLUMETRIC_FLOW_UNIT:
		return prefs.units.volume == units::LITER ? lrint(f * 1000.0) : lrint(cuft_to_l(f) * 1000.0);
	case FILTER_CONSTRAINT_DENSITY_UNIT:
		return lrint(f * 10.0); // Yippie, only "sane" units for density (g/l)
	case FILTER_CONSTRAINT_PERCENTAGE_UNIT:
		return lrint(f * 10.0); // Display percent, store permille
	}
}

static double base_to_display_unit(int i, enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	if (!desc)
		return 0.0;
	switch (desc->units) {
	case FILTER_CONSTRAINT_NO_UNIT:
	default:
		return (double)i;
	case FILTER_CONSTRAINT_LENGTH_UNIT:
		return prefs.units.length == units::METERS ? (double)i / 1000.0 : mm_to_feet(i);
	case FILTER_CONSTRAINT_DURATION_UNIT:
		return (double)i / 60.0;
	case FILTER_CONSTRAINT_TEMPERATURE_UNIT:
		return prefs.units.temperature == units::CELSIUS ? mkelvin_to_C(i) : mkelvin_to_F(i);
	case FILTER_CONSTRAINT_WEIGHT_UNIT:
		return prefs.units.weight == units::KG ? (double)i / 1000.0 : grams_to_lbs(i);
	case FILTER_CONSTRAINT_VOLUME_UNIT:
	case FILTER_CONSTRAINT_VOLUMETRIC_FLOW_UNIT:
		return prefs.units.volume == units::LITER ? (double)i / 1000.0 : ml_to_cuft(i);
	case FILTER_CONSTRAINT_DENSITY_UNIT:
		return (double)i / 10.0; // Yippie, only "sane" units for density (g/l)
	case FILTER_CONSTRAINT_PERCENTAGE_UNIT:
		return (double)i / 10.0; // Display percent, store permille
	}
}

QStringList filter_constraint_type_list_translated()
{
	QStringList res;
	for (size_t i = 0; i < type_descriptions_count(); ++i)
		res.push_back(gettextFromC::tr(type_descriptions[i].text_ui));
	return res;
}

QStringList filter_constraint_string_mode_list_translated()
{
	QStringList res;
	for (size_t i = 0; i < string_mode_descriptions_count(); ++i)
		res.push_back(gettextFromC::tr(string_mode_descriptions[i].text_ui));
	return res;
}

QStringList filter_constraint_range_mode_list_translated()
{
	QStringList res;
	for (size_t i = 0; i < range_mode_descriptions_count(); ++i)
		res.push_back(gettextFromC::tr(range_mode_descriptions[i].text_ui));
	return res;
}

QStringList filter_constraint_range_mode_list_translated_date()
{
	QStringList res;
	for (size_t i = 0; i < range_mode_descriptions_count(); ++i)
		res.push_back(gettextFromC::tr(range_mode_descriptions[i].text_ui_date));
	return res;
}

QStringList filter_constraint_negate_list_translated()
{
	return { gettextFromC::tr(negate_description[false]),
		 gettextFromC::tr(negate_description[true]) };
}

// Currently, we support only two multiple choice items. Hardcode them.
static bool filter_constraint_is_multiple_choice(enum filter_constraint_type type)
{
	return type == FILTER_CONSTRAINT_DIVE_MODE || type == FILTER_CONSTRAINT_DAY_OF_WEEK;
}

QStringList filter_contraint_multiple_choice_translated(enum filter_constraint_type type)
{
	// Currently, we support only two multiple choice lists. Hardcode them.
	if (type == FILTER_CONSTRAINT_DIVE_MODE) {
		QStringList types;
		for (int i = 0; i < NUM_DIVEMODE; i++)
			types.append(gettextFromC::tr(divemode_text_ui[i]));
		return types;
	} else if (type == FILTER_CONSTRAINT_DAY_OF_WEEK) {
		QStringList days;
		for (int i = 0; i < 7; ++i)
			days.push_back(formatDayOfWeek(i));
		return days;
	}
	return QStringList();
}

extern "C" bool filter_constraint_is_string(filter_constraint_type type)
{
	// Currently a constraint is filter based if and only if it has a string
	// mode (i.e. starts with, substring, exact). In the future we might also
	// support string based constraints that do not feature such a mode.
	return filter_constraint_has_string_mode(type);
}

extern "C" bool filter_constraint_is_timestamp(filter_constraint_type type)
{
	return type == FILTER_CONSTRAINT_DATE || type == FILTER_CONSTRAINT_DATE_TIME;
}

static bool is_numerical_constraint(filter_constraint_type type)
{
	return !filter_constraint_is_string(type) && !filter_constraint_is_timestamp(type);
}

extern "C" bool filter_constraint_has_string_mode(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc && desc->has_string_mode;
}

extern "C" bool filter_constraint_has_range_mode(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc && desc->has_range_mode;
}

extern "C" bool filter_constraint_is_star(filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc && desc->is_star_widget;
}

extern "C" bool filter_constraint_has_date_widget(filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc && desc->has_date;
}

extern "C" bool filter_constraint_has_time_widget(filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc && desc->has_time;
}

extern "C" int filter_constraint_num_decimals(enum filter_constraint_type type)
{
	const type_description *desc = get_type_description(type);
	return desc ? desc->decimal_places : 1;
}

// String constraints are valid if there is at least one term.
// Other constraints are always valid.
extern "C" bool filter_constraint_is_valid(const struct filter_constraint *constraint)
{
	if (!filter_constraint_is_string(constraint->type))
		return true;
	return !constraint->data.string_list->isEmpty();
}

filter_constraint::filter_constraint(filter_constraint_type typeIn) :
	type(typeIn),
	string_mode(FILTER_CONSTRAINT_STARTS_WITH),
	range_mode(FILTER_CONSTRAINT_GREATER),
	negate(false)
{
	if (filter_constraint_is_timestamp(type)) {
		// For time constraint, default to something completely arbitrary:
		// Dives in the last year up to next week (newly added dives typically are in the future).
		QDateTime now = QDateTime::currentDateTimeUtc();
		data.timestamp_range.from = dateTimeToTimestamp(now.addYears(-1));
		data.timestamp_range.to = dateTimeToTimestamp(now.addDays(7));
	} else if (filter_constraint_is_string(type)) {
		data.string_list = new QStringList;
	} else if (filter_constraint_is_star(type)) {
		data.numerical_range.from = 0;
		data.numerical_range.to = 5;
	} else if (filter_constraint_is_multiple_choice(type)) {
		// By default select all the possible items
		data.multiple_choice = ~0LLU;
	} else if (filter_constraint_has_time_widget(type) && !filter_constraint_has_date_widget(type)) {
		// This is a time field. For now let's set it arbitrarily to the whole day.
		data.numerical_range.from = 0;
		data.numerical_range.to = 24 * 3600 - 60;
	} else if (type == FILTER_CONSTRAINT_YEAR) {
		// Find dives in the last five years.
		int year = QDateTime::currentDateTimeUtc().date().year();
		data.numerical_range.from = year - 5;
		data.numerical_range.to = year;
	} else {
		// For numerical data let's try to find sensible defaults based on the unit.
		// Obviously, these are arbitrary and we might want to save the user supplied values.
		switch (type_to_unit(type)) {
		case FILTER_CONSTRAINT_NO_UNIT:
		default:
			data.numerical_range.from = 0;
			data.numerical_range.to = 0;
			break;
		case FILTER_CONSTRAINT_LENGTH_UNIT:
			// Length: 0-20 m
			data.numerical_range.from = 0 * 1000;
			data.numerical_range.to = 20 * 1000;
			break;
		case FILTER_CONSTRAINT_DURATION_UNIT:
			// Duration: 0-60 min
			data.numerical_range.from = 0 * 60;
			data.numerical_range.to = 60 * 60;
			break;
		case FILTER_CONSTRAINT_TEMPERATURE_UNIT:
			// Temperature: 20-30°C
			data.numerical_range.from = C_to_mkelvin(20);
			data.numerical_range.to = C_to_mkelvin(30);
			break;
		case FILTER_CONSTRAINT_WEIGHT_UNIT:
			// Weight: 0-10 kg
			data.numerical_range.from = 0 * 1000;
			data.numerical_range.to = 10 * 1000;
			break;
		case FILTER_CONSTRAINT_VOLUMETRIC_FLOW_UNIT:
			// SAC: 0-10 l/min
			data.numerical_range.from = 0 * 1000;
			data.numerical_range.to = 10 * 1000;
			break;
		case FILTER_CONSTRAINT_VOLUME_UNIT:
			// Cylinder size: 1-100 l
			data.numerical_range.from = 0 * 1000;
			data.numerical_range.to = 100 * 1000;
			break;
		case FILTER_CONSTRAINT_DENSITY_UNIT:
			// Water density: 1000-1027 g/l
			data.numerical_range.from = 1000 * 10;
			data.numerical_range.to = 1027 * 10;
			break;
		case FILTER_CONSTRAINT_PERCENTAGE_UNIT:
			// Percentage: 0-100%
			data.numerical_range.from = 0 * 10;
			data.numerical_range.to = 100 * 10;
			break;
		}
	}
}

filter_constraint::filter_constraint(const filter_constraint &c) :
	type(c.type),
	string_mode(c.string_mode),
	range_mode(c.range_mode),
	negate(c.negate)
{
	if (filter_constraint_is_timestamp(type))
		data.timestamp_range = c.data.timestamp_range;
	else if (filter_constraint_is_string(type))
		data.string_list = new QStringList(*c.data.string_list);
	else if (filter_constraint_is_multiple_choice(type))
		data.multiple_choice = c.data.multiple_choice;
	else
		data.numerical_range = c.data.numerical_range;
}

filter_constraint::filter_constraint(const char *type_in, const char *string_mode_in,
				     const char *range_mode_in, bool negate_in, const char *s_in) :
	type(filter_constraint_type_from_string(type_in)),
	string_mode(FILTER_CONSTRAINT_STARTS_WITH),
	range_mode(FILTER_CONSTRAINT_GREATER),
	negate(negate_in)
{
	QString s(s_in);
	if (filter_constraint_has_string_mode(type))
		string_mode = filter_constraint_string_mode_from_string(string_mode_in);
	if (filter_constraint_has_range_mode(type))
		range_mode = filter_constraint_range_mode_from_string(range_mode_in);
	if (filter_constraint_is_timestamp(type)) {
		QStringList l = s.split(',');
		data.timestamp_range.from = parse_datetime(qPrintable(l[0]));
		data.timestamp_range.to = l.size() >= 2 ? parse_datetime(qPrintable(l[1])) : 0;
	} else if (filter_constraint_is_string(type)) {
		// TODO: this obviously breaks if the strings contain ",".
		// That is currently not supported by the UI, but one day we might
		// have to escape the strings.
		data.string_list = new QStringList(s.split(','));
	} else if (filter_constraint_is_multiple_choice(type)) {
		data.multiple_choice = s.toLongLong();
	} else {
		QStringList l = s.split(',');
		data.numerical_range.from = l[0].toInt();
		data.numerical_range.to = l.size() >= 2 ? l[1].toInt() : 0;
	}
}

filter_constraint &filter_constraint::operator=(const filter_constraint &c)
{
	if (filter_constraint_is_string(type))
		delete data.string_list;
	type = c.type;
	string_mode = c.string_mode;
	range_mode = c.range_mode;
	negate = c.negate;
	if (filter_constraint_is_timestamp(type))
		data.timestamp_range = c.data.timestamp_range;
	else if (filter_constraint_is_string(type))
		data.string_list = new QStringList(*c.data.string_list);
	else if (filter_constraint_is_multiple_choice(type))
		data.multiple_choice = c.data.multiple_choice;
	else
		data.numerical_range = c.data.numerical_range;
	return *this;
}

bool filter_constraint::operator==(const filter_constraint &f2) const
{
	if (type != f2.type || string_mode != f2.string_mode || range_mode != f2.range_mode || negate != f2.negate)
		return false;
	if (filter_constraint_is_timestamp(type))
		return data.timestamp_range.from == f2.data.timestamp_range.from &&
		       data.timestamp_range.to == f2.data.timestamp_range.to;
	else if (filter_constraint_is_string(type))
		return *data.string_list == *(f2.data.string_list);
	else if (filter_constraint_is_multiple_choice(type))
		return data.multiple_choice == f2.data.multiple_choice;
	else
		return data.numerical_range.from == f2.data.numerical_range.from &&
		       data.numerical_range.to == f2.data.numerical_range.to;
}

filter_constraint::~filter_constraint()
{
	if (filter_constraint_is_string(type))
		delete data.string_list;
}

extern "C" char *filter_constraint_data_to_string(const filter_constraint *c)
{
	QString s;
	if (filter_constraint_is_timestamp(c->type)) {
		char *from_s = format_datetime(c->data.timestamp_range.from);
		char *to_s = format_datetime(c->data.timestamp_range.to);
		s = QString(from_s) + ',' + QString(to_s);
		free(from_s);
		free(to_s);
	} else if (filter_constraint_is_string(c->type)) {
		// TODO: this obviously breaks if the strings contain ",".
		// That is currently not supported by the UI, but one day we might
		// have to escape the strings.
		s = c->data.string_list->join(",");
	} else if (filter_constraint_is_multiple_choice(c->type)) {
		s = QString::number(c->data.multiple_choice);
	} else {
		s = QString::number(c->data.numerical_range.from) + ',' +
		    QString::number(c->data.numerical_range.to);
	}
	return copy_qstring(s);
}

void filter_constraint_set_stringlist(filter_constraint &c, const QString &s)
{
	if (!filter_constraint_is_string(c.type)) {
		fprintf(stderr, "Setting strings in non-string constraint!\n");
		return;
	}
	c.data.string_list->clear();
	for (const QString &part: s.split(",", SKIP_EMPTY))
		c.data.string_list->push_back(part.trimmed());
}

void filter_constraint_set_timestamp_from(filter_constraint &c, timestamp_t from)
{
	if (!filter_constraint_is_timestamp(c.type)) {
		fprintf(stderr, "Setting timestamp from in non-timestamp constraint!\n");
		return;
	}
	c.data.timestamp_range.from = from;
}

void filter_constraint_set_timestamp_to(filter_constraint &c, timestamp_t to)
{
	if (!filter_constraint_is_timestamp(c.type)) {
		fprintf(stderr, "Setting timestamp to in non-timestamp constraint!\n");
		return;
	}
	c.data.timestamp_range.to = to;
}

void filter_constraint_set_integer_from(filter_constraint &c, int from)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Setting integer from of non-numerical constraint!\n");
		return;
	}
	c.data.numerical_range.from = from;
}

void filter_constraint_set_integer_to(filter_constraint &c, int to)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Setting integer to of non-numerical constraint!\n");
		return;
	}
	c.data.numerical_range.to = to;
}

void filter_constraint_set_float_from(filter_constraint &c, double from)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Setting float from of non-numerical constraint!\n");
		return;
	}
	c.data.numerical_range.from = display_to_base_unit(from, c.type);
}

void filter_constraint_set_float_to(filter_constraint &c, double to)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Setting float to of non-numerical constraint!\n");
		return;
	}
	c.data.numerical_range.to = display_to_base_unit(to, c.type);
}

void filter_constraint_set_multiple_choice(filter_constraint &c, uint64_t multiple_choice)
{
	if (!filter_constraint_is_multiple_choice(c.type)) {
		fprintf(stderr, "Setting multiple-choice to of non-multiple-choice constraint!\n");
		return;
	}
	c.data.multiple_choice = multiple_choice;
}

QString filter_constraint_get_string(const filter_constraint &c)
{
	if (!filter_constraint_is_string(c.type)) {
		fprintf(stderr, "Getting string of non-string constraint!\n");
		return QString();
	}
	return c.data.string_list->join(",");
}

int filter_constraint_get_integer_from(const filter_constraint &c)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Getting integer from of non-numerical constraint!\n");
		return -1;
	}
	return c.data.numerical_range.from;
}

int filter_constraint_get_integer_to(const filter_constraint &c)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Getting integer to of non-numerical constraint!\n");
		return -1;
	}
	return c.data.numerical_range.to;
}

double filter_constraint_get_float_from(const filter_constraint &c)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Getting float from of non-numerical constraint!\n");
		return 0.0;
	}
	return base_to_display_unit(c.data.numerical_range.from, c.type);
}

double filter_constraint_get_float_to(const filter_constraint &c)
{
	if (!is_numerical_constraint(c.type)) {
		fprintf(stderr, "Getting float to of non-numerical constraint!\n");
		return 0.0;
	}
	return base_to_display_unit(c.data.numerical_range.to, c.type);
}

timestamp_t filter_constraint_get_timestamp_from(const filter_constraint &c)
{
	if (!filter_constraint_is_timestamp(c.type)) {
		fprintf(stderr, "Getting timestamp from of non-timestamp constraint!\n");
		return 0;
	}
	return c.data.timestamp_range.from;
}

timestamp_t filter_constraint_get_timestamp_to(const filter_constraint &c)
{
	if (!filter_constraint_is_timestamp(c.type)) {
		fprintf(stderr, "Getting timestamp to of non-timestamp constraint!\n");
		return 0;
	}
	return c.data.timestamp_range.to;
}

uint64_t filter_constraint_get_multiple_choice(const filter_constraint &c)
{
	if (!filter_constraint_is_multiple_choice(c.type)) {
		fprintf(stderr, "Getting multiple-choice of non-multiple choice constraint!\n");
		return 0;
	}
	return c.data.multiple_choice;
}

// Pointer to function that takes two strings and returns whether
// the first matches the second according to a criterion (substring, starts-with, exact).
using StrCheck = bool (*) (const QString &s1, const QString &s2);

// Check if a string-list contains at least one string containing the second argument.
// Comparison is non case sensitive and removes white space.
static bool listContainsSuperstring(const QStringList &list, const QString &s, StrCheck strchk)
{
	return std::any_of(list.begin(), list.end(), [&s,strchk](const QString &s2)
			   { return strchk(s2, s); } );
}

// Check whether any of the items of the first list is in the second list as a super string.
// The mode is controlled by the second argument
static bool check(const filter_constraint &c, const QStringList &list)
{
	StrCheck strchk =
		c.string_mode == FILTER_CONSTRAINT_SUBSTRING ?
			[](const QString &s1, const QString &s2) { return s1.contains(s2, Qt::CaseInsensitive); } :
		c.string_mode == FILTER_CONSTRAINT_STARTS_WITH ?
			[](const QString &s1, const QString &s2) { return s1.startsWith(s2, Qt::CaseInsensitive); } :
		/* FILTER_CONSTRAINT_EXACT */
			[](const QString &s1, const QString &s2) { return s1.compare(s2, Qt::CaseInsensitive) == 0; };
	return std::any_of(c.data.string_list->begin(), c.data.string_list->end(),
			   [&list, strchk](const QString &item)
			   { return listContainsSuperstring(list, item, strchk); }) != c.negate;
}

static bool has_tags(const filter_constraint &c, const struct dive *d)
{
	QStringList dive_tags;
	for (const tag_entry *tag = d->tag_list; tag; tag = tag->next)
		dive_tags.push_back(QString(tag->tag->name).trimmed());
	dive_tags.append(gettextFromC::tr(divemode_text_ui[d->dc.divemode]).trimmed());
	return check(c, dive_tags);
}

static bool has_people(const filter_constraint &c, const struct dive *d)
{
	QStringList dive_people;
	for (const QString &s: QString(d->buddy).split(",", SKIP_EMPTY))
		dive_people.push_back(s.trimmed());
	for (const QString &s: QString(d->divemaster).split(",", SKIP_EMPTY))
		dive_people.push_back(s.trimmed());
	return check(c, dive_people);
}

static bool has_locations(const filter_constraint &c, const struct dive *d)
{
	QStringList diveLocations;
	if (d->divetrip)
		diveLocations.push_back(QString(d->divetrip->location).trimmed());

	if (d->dive_site)
		diveLocations.push_back(QString(d->dive_site->name).trimmed());

	return check(c, diveLocations);
}

static bool has_weight_type(const filter_constraint &c, const struct dive *d)
{
	QStringList weightsystemTypes;
	for (int i = 0; i < d->weightsystems.nr; ++i)
		weightsystemTypes.push_back(d->weightsystems.weightsystems[i].description);

	return check(c, weightsystemTypes);
}

static bool has_cylinder_type(const filter_constraint &c, const struct dive *d)
{
	QStringList cylinderTypes;
	for (int i = 0; i < d->cylinders.nr; ++i)
		cylinderTypes.push_back(d->cylinders.cylinders[i].type.description);

	return check(c, cylinderTypes);
}

static bool has_suits(const filter_constraint &c, const struct dive *d)
{
	QStringList diveSuits;
	if (d->suit)
		diveSuits.push_back(QString(d->suit));
	return check(c, diveSuits);
}

static bool has_notes(const filter_constraint &c, const struct dive *d)
{
	QStringList diveNotes;
	if (d->notes)
		diveNotes.push_back(QString(d->notes));
	return check(c, diveNotes);
}

static bool check_numerical_range(const filter_constraint &c, int v)
{
	switch (c.range_mode) {
	case FILTER_CONSTRAINT_EQUAL:
		return (v == c.data.numerical_range.from) != c.negate;
	case FILTER_CONSTRAINT_LESS:
		return (v <= c.data.numerical_range.to) != c.negate;
	case FILTER_CONSTRAINT_GREATER:
		return (v >= c.data.numerical_range.from) != c.negate;
	case FILTER_CONSTRAINT_RANGE:
		return (v >= c.data.numerical_range.from && v <= c.data.numerical_range.to) != c.negate;
	}
	return false;
}

// Check a numerical range, but consider a value of 0 as "not set".
static bool check_numerical_range_non_zero(const filter_constraint &c, int v)
{
	if (v == 0)
		return c.negate;
	return check_numerical_range(c, v);
}

static bool check_cylinder_size(const filter_constraint &c, const struct dive *d)
{
	for (int i = 0; i < d->cylinders.nr; ++i) {
		const cylinder_t &cyl = d->cylinders.cylinders[i];
		if (cyl.type.size.mliter && check_numerical_range(c, cyl.type.size.mliter))
			return true;
	}
	return false;
}

static bool check_gas_range(const filter_constraint &c, const struct dive *d, gas_component component)
{
	for (int i = 0; i < d->cylinders.nr; ++i) {
		const cylinder_t &cyl = d->cylinders.cylinders[i];
		if (check_numerical_range(c, get_gas_component_fraction(cyl.gasmix, component).permille))
			return true;
	}
	return false;
}

static long days_since_epoch(timestamp_t timestamp)
{
	return timestamp / (3600 * 24);
}

static int seconds_since_midnight(timestamp_t timestamp)
{
	return timestamp % (3600 * 24);
}

static bool check_date_range(const filter_constraint &c, const struct dive *d)
{
	// We don't consider dives past midnight. Should we?
	long day = days_since_epoch(d->when);
	switch (c.range_mode) {
	case FILTER_CONSTRAINT_EQUAL:
		return (day == days_since_epoch(c.data.timestamp_range.from)) != c.negate;
	case FILTER_CONSTRAINT_LESS:
		return (day <= days_since_epoch(c.data.timestamp_range.to)) != c.negate;
	case FILTER_CONSTRAINT_GREATER:
		return (day >= days_since_epoch(c.data.timestamp_range.from)) != c.negate;
	case FILTER_CONSTRAINT_RANGE:
		return (day >= days_since_epoch(c.data.timestamp_range.from) &&
			day <= days_since_epoch(c.data.timestamp_range.to)) != c.negate;
	}
	return false;
}

static bool check_datetime_range(const filter_constraint &c, const struct dive *d)
{
	switch (c.range_mode) {
	case FILTER_CONSTRAINT_EQUAL:
		// Exact mode is a bit strange for timestamps. Therefore we return any dive
		// where the given timestamp is during that dive.
		return time_during_dive_with_offset(d, c.data.timestamp_range.from, 0) != c.negate;
	case FILTER_CONSTRAINT_LESS:
		return (dive_endtime(d) <= c.data.timestamp_range.to) != c.negate;
	case FILTER_CONSTRAINT_GREATER:
		return (d->when >= c.data.timestamp_range.from) != c.negate;
	case FILTER_CONSTRAINT_RANGE:
		return (d->when >= c.data.timestamp_range.from &&
			dive_endtime(d) <= c.data.timestamp_range.to) != c.negate;
	}
	return false;
}

// This helper function takes explicit from and to values, because the caller might want
// to reverse them with respect to the order of the constraint. See comment in check_time_of_day_range().
static bool check_time_of_day_internal(const dive *d, enum filter_constraint_range_mode range_mode, int from, int to, bool negate)
{
	switch (range_mode) {
	case FILTER_CONSTRAINT_EQUAL:
		// Exact mode is a bit strange for time_of_day. Therefore we return any dive
		// where the given timestamp is during that dive. Note: this will fail for dives
		// that run past midnight. We might want to special case that.
		return (seconds_since_midnight(d->when) <= from &&
			seconds_since_midnight(dive_endtime(d)) >= from) != negate;
	case FILTER_CONSTRAINT_LESS:
		return (seconds_since_midnight(dive_endtime(d)) <= to) != negate;
	case FILTER_CONSTRAINT_GREATER:
		return (seconds_since_midnight(d->when) >= from) != negate;
	case FILTER_CONSTRAINT_RANGE:
		return (seconds_since_midnight(d->when) >= from &&
			seconds_since_midnight(dive_endtime(d)) <= to) != negate;
	}
	return false;
}

static bool check_time_of_day_range(const filter_constraint &c, const struct dive *d)
{
	// Time-of-day gets special treatment: we consider the range as living on a cyclic support.
	// This means that if the user searches for dives between 10 pm and 2 am, we flag dives
	// that happen in either of the [10 pm-midnight] and [midnight-2 am] segments.
	// This can trivially be realized by inverting the limits and inverting the result.
	// In the example above: search for all dives which are *not* in the [2 am-10 pm] segment.
	// This however makes only sense in the EQUAL or RANGE modes.
	if ((c.range_mode == FILTER_CONSTRAINT_EQUAL || c.range_mode == FILTER_CONSTRAINT_RANGE) &&
	     c.data.numerical_range.from > c.data.numerical_range.to) {
		return check_time_of_day_internal(d, c.range_mode, c.data.numerical_range.to, c.data.numerical_range.from, !c.negate);
	} else {
		return check_time_of_day_internal(d, c.range_mode, c.data.numerical_range.from, c.data.numerical_range.to, c.negate);
	}
}

static bool check_year_range(const filter_constraint &c, const struct dive *d)
{
	// Note: we don't consider new-year dives. Should we?
	int year = utc_year(d->when);
	switch (c.range_mode) {
	case FILTER_CONSTRAINT_EQUAL:
		return (year == c.data.numerical_range.from) != c.negate;
	case FILTER_CONSTRAINT_LESS:
		return (year <= c.data.numerical_range.to) != c.negate;
	case FILTER_CONSTRAINT_GREATER:
		return (year >= c.data.numerical_range.from) != c.negate;
	case FILTER_CONSTRAINT_RANGE:
		return (year >= c.data.numerical_range.from &&
			year <= c.data.numerical_range.to) != c.negate;
	}
	return false;
}

static bool check_multiple_choice(const filter_constraint &c, int v)
{
	bool has_bit = c.data.multiple_choice & (1ULL << v);
	return has_bit != c.negate;
}

bool filter_constraint_match_dive(const filter_constraint &c, const struct dive *d)
{
	if (filter_constraint_is_string(c.type) && c.data.string_list->isEmpty())
		return true;

	switch (c.type) {
	case FILTER_CONSTRAINT_DATE:
		return check_date_range(c, d);
	case FILTER_CONSTRAINT_DATE_TIME:
		return check_datetime_range(c, d);
	case FILTER_CONSTRAINT_TIME_OF_DAY:
		return check_time_of_day_range(c, d);
	case FILTER_CONSTRAINT_YEAR:
		return check_year_range(c, d);
	case FILTER_CONSTRAINT_DAY_OF_WEEK:
		return check_multiple_choice(c, utc_weekday(d->when)); // no support for midnight dives - should we?
	case FILTER_CONSTRAINT_RATING:
		return check_numerical_range(c, d->rating);
	case FILTER_CONSTRAINT_WAVESIZE:
		return check_numerical_range(c, d->wavesize);
	case FILTER_CONSTRAINT_CURRENT:
		return check_numerical_range(c, d->current);
	case FILTER_CONSTRAINT_VISIBILITY:
		return check_numerical_range(c, d->visibility);
	case FILTER_CONSTRAINT_SURGE:
		return check_numerical_range(c, d->surge);
	case FILTER_CONSTRAINT_CHILL:
		return check_numerical_range(c, d->chill);
	case FILTER_CONSTRAINT_DEPTH:
		return check_numerical_range(c, d->maxdepth.mm);
	case FILTER_CONSTRAINT_DURATION:
		return check_numerical_range(c, d->duration.seconds);
	case FILTER_CONSTRAINT_WEIGHT:
		return check_numerical_range(c, total_weight(d));
	case FILTER_CONSTRAINT_WATER_TEMP:
		return check_numerical_range(c, d->watertemp.mkelvin);
	case FILTER_CONSTRAINT_AIR_TEMP:
		return check_numerical_range(c, d->airtemp.mkelvin);
	case FILTER_CONSTRAINT_WATER_DENSITY:
		return check_numerical_range(c, d->user_salinity ? d->user_salinity : d->salinity);
	case FILTER_CONSTRAINT_SAC:
		return check_numerical_range_non_zero(c, d->sac);
	case FILTER_CONSTRAINT_LOGGED:
		return has_planned(d, false) != c.negate;
	case FILTER_CONSTRAINT_PLANNED:
		return has_planned(d, true) != c.negate;
	case FILTER_CONSTRAINT_DIVE_MODE:
		return check_multiple_choice(c, (int)d->dc.divemode); // should we be smarter and check all DCs?
	case FILTER_CONSTRAINT_TAGS:
		return has_tags(c, d);
	case FILTER_CONSTRAINT_PEOPLE:
		return has_people(c, d);
	case FILTER_CONSTRAINT_LOCATION:
		return has_locations(c, d);
	case FILTER_CONSTRAINT_WEIGHT_TYPE:
		return has_weight_type(c, d);
	case FILTER_CONSTRAINT_CYLINDER_TYPE:
		return has_cylinder_type(c, d);
	case FILTER_CONSTRAINT_CYLINDER_SIZE:
		return check_cylinder_size(c, d);
	case FILTER_CONSTRAINT_CYLINDER_N2:
		return check_gas_range(c, d, N2);
	case FILTER_CONSTRAINT_CYLINDER_O2:
		return check_gas_range(c, d, O2);
	case FILTER_CONSTRAINT_CYLINDER_HE:
		return check_gas_range(c, d, HE);
	case FILTER_CONSTRAINT_SUIT:
		return has_suits(c, d);
	case FILTER_CONSTRAINT_NOTES:
		return has_notes(c, d);
	}
	return false;
}
