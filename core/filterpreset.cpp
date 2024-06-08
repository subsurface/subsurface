// SPDX-License-Identifier: GPL-2.0
#include "filterpreset.h"
#include "divelog.h"
#include "qthelper.h"
#include "subsurface-string.h"

std::string filter_preset::fulltext_query() const
{
	return data.fullText.originalQuery.toStdString();
}

const char *filter_preset::fulltext_mode() const
{
	switch (data.fulltextStringMode) {
	default:
	case StringFilterMode::SUBSTRING:
		return "substring";
	case StringFilterMode::STARTSWITH:
		return "startswith";
	case StringFilterMode::EXACT:
		return "exact";
	}
}

void filter_preset::set_fulltext(const std::string fulltext, const std::string &fulltext_string_mode)
{
	if (fulltext_string_mode == "substring")
		data.fulltextStringMode = StringFilterMode::SUBSTRING;
	else if (fulltext_string_mode == "startswith")
		data.fulltextStringMode = StringFilterMode::STARTSWITH;
	else // if (fulltext_string_mode == "exact"))
		data.fulltextStringMode = StringFilterMode::EXACT;
	data.fullText = QString::fromStdString(std::move(fulltext));
}

void filter_preset::add_constraint(const std::string &type, const std::string &string_mode,
				   const std::string &range_mode, bool negate, const std::string &constraint_data)
{
	data.constraints.emplace_back(type, string_mode, range_mode, negate, constraint_data);
}
