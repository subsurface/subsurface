// SPDX-License-Identifier: GPL-2.0
#ifndef FILTER_PRESETS_H
#define FILTER_PRESETS_H

#include "divefilter.h"
#include <string>

struct dive;
struct filter_constraint;
struct FilterData;

struct filter_preset {
	std::string name;
	FilterData data;

	std::string fulltext_query() const; // Fulltext query of filter preset.
	const char *fulltext_mode() const; // String mode of fulltext query. Ownership is *not* passed to caller.
	void set_fulltext(const std::string fulltext, const std::string &fulltext_string_mode); // First argument is consumed.
	void add_constraint(const std::string &type, const std::string &string_mode,
			    const std::string &range_mode, bool negate, const std::string &data); // called by the parser, therefore data passed as strings.
};

#endif
