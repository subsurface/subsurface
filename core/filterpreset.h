// SPDX-License-Identifier: GPL-2.0
// A list of filter settings with names. Every name is unique, which
// means that saving to an old name will overwrite the old preset.
//
// Even though the filter data itself is a C++/Qt class to simplify
// string manipulation and memory management, the data is accessible
// via pure C functions so that it can be written to the log (git or xml).
#ifndef FILTER_PRESETS_H
#define FILTER_PRESETS_H

#include "divefilter.h"
#include <vector>
#include <string>

struct dive;
struct filter_constraint;
struct FilterData;

struct filter_preset {
	std::string name;
	FilterData data;
};

// Subclassing standard library containers is generally
// not recommended. However, this makes interaction with
// C-code easier and since we don't add any member functions,
// this is not a problem.
struct filter_preset_table : public std::vector<filter_preset>
{
};

// The C IO code accesses the filter presets via integer indices.
extern int filter_presets_count();
extern const char *filter_preset_fulltext_mode(int preset); // string mode of fulltext query. ownership is *not* passed to caller.
extern int filter_preset_constraint_count(int preset); // number of constraints in the filter preset.
extern const struct filter_constraint *filter_preset_constraint(int preset, int constraint); // get constraint. ownership is *not* passed to caller.
extern void filter_preset_set_name(struct filter_preset *preset, const char *name);
extern void filter_preset_set_fulltext(struct filter_preset *preset, const char *fulltext, const char *fulltext_string_mode);
extern void add_filter_preset_to_table(const struct filter_preset *preset, struct filter_preset_table *table);
extern void filter_preset_add_constraint(struct filter_preset *preset, const char *type, const char *string_mode,
					 const char *range_mode, bool negate, const char *data); // called by the parser, therefore data passed as strings.
int filter_preset_id(const std::string &s); // for now, we assume that names are unique. returns -1 if no preset with that name.
void filter_preset_set(int preset, const FilterData &d); // this will override a preset if the name already exists.
FilterData filter_preset_get(int preset);
int filter_preset_add(const std::string &name, const FilterData &d); // returns point of insertion
void filter_preset_delete(int preset);
std::string filter_preset_name(int preset); // name of filter preset - caller must free the result.
std::string filter_preset_fulltext_query(int preset); // fulltext query of filter preset - caller must free the result.

#endif
