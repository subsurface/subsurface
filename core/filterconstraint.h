// SPDX-License-Identifier: GPL-2.0
// Structure describing a filter constraint. Accessible from C,
// since it is written to the log (git or xml).
#ifndef FILTER_CONSTRAINT_H
#define FILTER_CONSTRAINT_H

#include "units.h"

struct dive;

#ifdef __cplusplus
#include <QStringList>
extern "C" {
#else
typedef void QStringList;
#endif

enum filter_constraint_type {
	FILTER_CONSTRAINT_DATE,
	FILTER_CONSTRAINT_DATE_TIME,
	FILTER_CONSTRAINT_TIME_OF_DAY,
	FILTER_CONSTRAINT_YEAR,
	FILTER_CONSTRAINT_DAY_OF_WEEK,
	FILTER_CONSTRAINT_RATING,
	FILTER_CONSTRAINT_WAVESIZE,
	FILTER_CONSTRAINT_CURRENT,
	FILTER_CONSTRAINT_VISIBILITY,
	FILTER_CONSTRAINT_SURGE,
	FILTER_CONSTRAINT_CHILL,
	FILTER_CONSTRAINT_DEPTH,
	FILTER_CONSTRAINT_DURATION,
	FILTER_CONSTRAINT_WEIGHT,
	FILTER_CONSTRAINT_WATER_TEMP,
	FILTER_CONSTRAINT_AIR_TEMP,
	FILTER_CONSTRAINT_WATER_DENSITY,
	FILTER_CONSTRAINT_SAC,
	FILTER_CONSTRAINT_LOGGED,
	FILTER_CONSTRAINT_PLANNED,
	FILTER_CONSTRAINT_DIVE_MODE,
	FILTER_CONSTRAINT_TAGS,
	FILTER_CONSTRAINT_PEOPLE,
	FILTER_CONSTRAINT_LOCATION,
	FILTER_CONSTRAINT_WEIGHT_TYPE,
	FILTER_CONSTRAINT_CYLINDER_TYPE,
	FILTER_CONSTRAINT_CYLINDER_SIZE,
	FILTER_CONSTRAINT_CYLINDER_N2,
	FILTER_CONSTRAINT_CYLINDER_O2,
	FILTER_CONSTRAINT_CYLINDER_HE,
	FILTER_CONSTRAINT_SUIT,
	FILTER_CONSTRAINT_NOTES
};

// For string filters
enum filter_constraint_string_mode {
	FILTER_CONSTRAINT_STARTS_WITH,
	FILTER_CONSTRAINT_SUBSTRING,
	FILTER_CONSTRAINT_EXACT
};

// For range filters
enum filter_constraint_range_mode {
	FILTER_CONSTRAINT_EQUAL,
	FILTER_CONSTRAINT_LESS,
	FILTER_CONSTRAINT_GREATER,
	FILTER_CONSTRAINT_RANGE
};

struct filter_constraint {
	enum filter_constraint_type type;
	enum filter_constraint_string_mode string_mode;
	enum filter_constraint_range_mode range_mode;
	bool negate;
	union {
		struct {
			int from; // used for equality comparison
			int to;
		} numerical_range;
		struct {
			timestamp_t from; // used for equality comparison
			timestamp_t to;
		} timestamp_range;
		QStringList *string_list;
		uint64_t multiple_choice; // bit-field for multiple choice lists. currently, we support 64 items, extend if needed.
	} data;
#ifdef __cplusplus
	// For C++, define constructors, assignment operators and destructor to make our lives easier.
	filter_constraint(filter_constraint_type type);
	filter_constraint(const char *type, const char *string_mode,
			  const char *range_mode, bool negate, const char *data); // from parser data
	filter_constraint(const filter_constraint &);
	filter_constraint &operator=(const filter_constraint &);
	~filter_constraint();
	bool operator==(const filter_constraint &f2) const;
#endif
};

extern const char *filter_constraint_type_to_string(enum filter_constraint_type);
extern const char *filter_constraint_string_mode_to_string(enum filter_constraint_string_mode);
extern const char *filter_constraint_range_mode_to_string(enum filter_constraint_range_mode);
extern bool filter_constraint_is_string(enum filter_constraint_type type);
extern bool filter_constraint_is_timestamp(enum filter_constraint_type type);
extern bool filter_constraint_is_star(enum filter_constraint_type type);

// These functions convert enums to indices and vice-versa. We could just define the enums to be identical to the index.
// However, by using these functions we are more robust, because the lists can be ordered differently than the enums.
extern int filter_constraint_type_to_index(enum filter_constraint_type);
extern int filter_constraint_string_mode_to_index(enum filter_constraint_string_mode);
extern int filter_constraint_range_mode_to_index(enum filter_constraint_range_mode);
extern enum filter_constraint_type filter_constraint_type_from_index(int index);
extern enum filter_constraint_string_mode filter_constraint_string_mode_from_index(int index);
extern enum filter_constraint_range_mode filter_constraint_range_mode_from_index(int index);

extern bool filter_constraint_has_string_mode(enum filter_constraint_type);
extern bool filter_constraint_has_range_mode(enum filter_constraint_type);
extern bool filter_constraint_has_date_widget(enum filter_constraint_type);
extern bool filter_constraint_has_time_widget(enum filter_constraint_type);
extern int filter_constraint_num_decimals(enum filter_constraint_type);
extern bool filter_constraint_is_valid(const struct filter_constraint *constraint);
extern char *filter_constraint_data_to_string(const struct filter_constraint *constraint); // caller takes ownership of returned string

#ifdef __cplusplus
}
#endif

// C++ only functions
#ifdef __cplusplus
QString filter_constraint_type_to_string_translated(enum filter_constraint_type);
QString filter_constraint_negate_to_string_translated(bool negate);
QString filter_constraint_string_mode_to_string_translated(enum filter_constraint_string_mode);
QString filter_constraint_range_mode_to_string_translated(enum filter_constraint_range_mode);
QString filter_constraint_get_unit(enum filter_constraint_type); // depends on preferences, empty string if no unit
QStringList filter_constraint_type_list_translated();
QStringList filter_constraint_string_mode_list_translated();
QStringList filter_constraint_range_mode_list_translated();
QStringList filter_constraint_range_mode_list_translated_date();
QStringList filter_constraint_negate_list_translated();
QStringList filter_contraint_multiple_choice_translated(enum filter_constraint_type); // Empty if no multiple-choice
QString filter_constraint_get_string(const filter_constraint &c);
int filter_constraint_get_integer_from(const filter_constraint &c);
int filter_constraint_get_integer_to(const filter_constraint &c);
double filter_constraint_get_float_from(const filter_constraint &c); // convert according to current units (metric or imperial)
double filter_constraint_get_float_to(const filter_constraint &c); // convert according to current units (metric or imperial)
timestamp_t filter_constraint_get_timestamp_from(const filter_constraint &c); // convert according to current units (metric or imperial)
timestamp_t filter_constraint_get_timestamp_to(const filter_constraint &c); // convert according to current units (metric or imperial)
uint64_t filter_constraint_get_multiple_choice(const filter_constraint &c);
void filter_constraint_set_stringlist(filter_constraint &c, const QString &s);
void filter_constraint_set_integer_from(filter_constraint &c, int from);
void filter_constraint_set_integer_to(filter_constraint &c, int to);
void filter_constraint_set_float_from(filter_constraint &c, double from); // convert according to current units (metric or imperial)
void filter_constraint_set_float_to(filter_constraint &c, double to); // convert according to current units (metric or imperial)
void filter_constraint_set_timestamp_from(filter_constraint &c, timestamp_t from); // convert according to current units (metric or imperial)
void filter_constraint_set_timestamp_to(filter_constraint &c, timestamp_t to); // convert according to current units (metric or imperial)
void filter_constraint_set_multiple_choice(filter_constraint &c, uint64_t);
bool filter_constraint_match_dive(const filter_constraint &c, const struct dive *d);
#endif

#endif
