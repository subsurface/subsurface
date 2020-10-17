// SPDX-License-Identifier: GPL-2.0
// A list of filter settings with names. Every name is unique, which
// means that saving to an old name will overwrite the old preset.
//
// Even though the filter data itself is a C++/Qt class to simplify
// string manipulation and memory management, the data is accessible
// via pure C functions so that it can be written to the log (git or xml).
#ifndef FILTER_PRESETS_H
#define FILTER_PRESETS_H

struct dive;
struct filter_constraint;

// So that we can pass filter preset table between C and C++ we define
// it as an opaque type in C. Thus we can easily create the table in C++
// without having to do our own memory management and pass pointers to
// void through C.
#ifdef __cplusplus
#include "divefilter.h"
#include <vector>
#include <QStringList>
struct filter_preset {
	QString name;
	FilterData data;
};

// Subclassing standard library containers is generally
// not recommended. However, this makes interaction with
// C-code easier and since we don't add any member functions,
// this is not a problem.
struct filter_preset_table : public std::vector<filter_preset>
{
};

extern struct filter_preset_table filter_preset_table;
#else
struct filter_preset;
struct filter_preset_table;
#endif


#ifdef __cplusplus
extern "C" {
#endif

// The C IO code accesses the filter presets via integer indices.
extern void clear_filter_presets(void);
extern int filter_presets_count(void);
extern char *filter_preset_name(int preset); // name of filter preset - caller must free the result.
extern char *filter_preset_fulltext_query(int preset); // fulltext query of filter preset - caller must free the result.
extern const char *filter_preset_fulltext_mode(int preset); // string mode of fulltext query. ownership is *not* passed to caller.
extern int filter_preset_constraint_count(int preset); // number of constraints in the filter preset.
extern const struct filter_constraint *filter_preset_constraint(int preset, int constraint); // get constraint. ownership is *not* passed to caller.
extern struct filter_preset *alloc_filter_preset();
extern void free_filter_preset(const struct filter_preset *preset);
extern void filter_preset_set_name(struct filter_preset *preset, const char *name);
extern void filter_preset_set_fulltext(struct filter_preset *preset, const char *fulltext, const char *fulltext_string_mode);
extern void add_filter_preset_to_table(const struct filter_preset *preset, struct filter_preset_table *table);
extern void filter_preset_add_constraint(struct filter_preset *preset, const char *type, const char *string_mode,
					 const char *range_mode, bool negate, const char *data); // called by the parser, therefore data passed as strings.

#ifdef __cplusplus
}
#endif

// C++ only functions
#ifdef __cplusplus

struct FilterData;

int filter_preset_id(const QString &s); // for now, we assume that names are unique. returns -1 if no preset with that name.
QString filter_preset_name_qstring(int preset);
void filter_preset_set(int preset, const FilterData &d); // this will override a preset if the name already exists.
FilterData filter_preset_get(int preset);
int filter_preset_add(const QString &name, const FilterData &d); // returns point of insertion
void filter_preset_delete(int preset);

#endif

#endif
