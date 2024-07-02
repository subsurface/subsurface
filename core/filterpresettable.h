// SPDX-License-Identifier: GPL-2.0
// A list of filter settings with names. Every name is unique, which
// means that saving to an old name will overwrite the old preset.
#ifndef FILTER_PRESETSTABLE_H
#define FILTER_PRESETSTABLE_H

#include <string>
#include <vector>

struct filter_preset;
struct FilterData;

// Subclassing standard library containers is generally
// not recommended. However, this makes interaction with
// C-code easier and since we don't add any member functions,
// this is not a problem.
struct filter_preset_table : public std::vector<filter_preset>
{
	void add(const filter_preset &preset);
	int add(const std::string &name, const FilterData &d); // returns point of insertion
	void remove(int iox);
	int preset_id(const std::string &s) const; // for now, we assume that names are unique. returns -1 if no preset with that name
};

#endif
