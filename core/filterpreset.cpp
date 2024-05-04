// SPDX-License-Identifier: GPL-2.0
#include "filterpreset.h"
#include "divelog.h"
#include "qthelper.h"
#include "subsurface-string.h"

static filter_preset_table &global_table()
{
	return *divelog.filter_presets;
}

int filter_presets_count(void)
{
	return (int)global_table().size();
}

extern std::string filter_preset_fulltext_query(int preset)
{
	return global_table()[preset].data.fullText.originalQuery.toStdString();
}

const char *filter_preset_fulltext_mode(int preset)
{
	switch (global_table()[preset].data.fulltextStringMode) {
	default:
	case StringFilterMode::SUBSTRING:
		return "substring";
	case StringFilterMode::STARTSWITH:
		return "startswith";
	case StringFilterMode::EXACT:
		return "exact";
	}
}

void filter_preset_set_fulltext(struct filter_preset *preset, const char *fulltext, const char *fulltext_string_mode)
{
	if (same_string(fulltext_string_mode, "substring"))
		preset->data.fulltextStringMode = StringFilterMode::SUBSTRING;
	else if (same_string(fulltext_string_mode, "startswith"))
		preset->data.fulltextStringMode = StringFilterMode::STARTSWITH;
	else // if (same_string(fulltext_string_mode, "exact"))
		preset->data.fulltextStringMode = StringFilterMode::EXACT;
	preset->data.fullText = fulltext;
}

int filter_preset_constraint_count(int preset)
{
	return (int)global_table()[preset].data.constraints.size();
}

const filter_constraint *filter_preset_constraint(int preset, int constraint)
{
	return &global_table()[preset].data.constraints[constraint];
}

void filter_preset_set_name(struct filter_preset *preset, const char *name)
{
	preset->name = name;
}

static int filter_preset_add_to_table(const std::string name, const FilterData &d, struct filter_preset_table &table)
{
	// std::lower_bound does a binary search - the vector must be sorted.
	filter_preset newEntry { name, d };
	auto it = std::lower_bound(table.begin(), table.end(), newEntry,
				   [](const filter_preset &p1, const filter_preset &p2)
				   { return p1.name < p2.name; });
	it = table.insert(it, newEntry);
	return it - table.begin();
}

// Take care that the name doesn't already exist by adding numbers
static std::string get_unique_preset_name(const std::string &orig, const struct filter_preset_table &table)
{
	std::string res = orig;
	int count = 2;
	while (std::find_if(table.begin(), table.end(),
			    [&res](const filter_preset &preset)
			    { return preset.name == res; }) != table.end()) {
		res = orig + "#" + std::to_string(count);
		++count;
	}
	return res;
}

void add_filter_preset_to_table(const struct filter_preset *preset, struct filter_preset_table *table)
{
	std::string name = get_unique_preset_name(preset->name, *table);
	filter_preset_add_to_table(name, preset->data, *table);
}

void filter_preset_add_constraint(struct filter_preset *preset, const char *type, const char *string_mode,
					     const char *range_mode, bool negate, const char *data)
{
	preset->data.constraints.emplace_back(type, string_mode, range_mode, negate, data);
}

int filter_preset_id(const std::string &name)
{
	auto it = std::find_if(global_table().begin(), global_table().end(),
			       [&name] (filter_preset &p) { return p.name == name; });
	return it != global_table().end() ? it - global_table().begin() : -1;
}

std::string filter_preset_name(int preset)
{
	return global_table()[preset].name;
}

void filter_preset_set(int preset, const FilterData &data)
{
	global_table()[preset].data = data;
}

FilterData filter_preset_get(int preset)
{
	return global_table()[preset].data;
}

int filter_preset_add(const std::string &nameIn, const FilterData &d)
{
	std::string name = get_unique_preset_name(nameIn, global_table());
	return filter_preset_add_to_table(name, d, global_table());
}

void filter_preset_delete(int preset)
{
	global_table().erase(global_table().begin() + preset);
}
