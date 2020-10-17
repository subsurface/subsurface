// SPDX-License-Identifier: GPL-2.0
#include "filterpreset.h"
#include "qthelper.h"
#include "subsurface-string.h"

struct filter_preset_table filter_preset_table;

extern "C" void clear_filter_presets(void)
{
	filter_preset_table.clear();
}

extern "C" int filter_presets_count(void)
{
	return (int)filter_preset_table.size();
}

extern "C" char *filter_preset_name(int preset)
{
	return copy_qstring(filter_preset_name_qstring(preset));
}

extern "C" char *filter_preset_fulltext_query(int preset)
{
	return copy_qstring(filter_preset_table[preset].data.fullText.originalQuery);
}

extern "C" const char *filter_preset_fulltext_mode(int preset)
{
	switch (filter_preset_table[preset].data.fulltextStringMode) {
	default:
	case StringFilterMode::SUBSTRING:
		return "substring";
	case StringFilterMode::STARTSWITH:
		return "startswith";
	case StringFilterMode::EXACT:
		return "exact";
	}
}

extern "C" void filter_preset_set_fulltext(struct filter_preset *preset, const char *fulltext, const char *fulltext_string_mode)
{
	if (same_string(fulltext_string_mode, "substring"))
		preset->data.fulltextStringMode = StringFilterMode::SUBSTRING;
	else if (same_string(fulltext_string_mode, "startswith"))
		preset->data.fulltextStringMode = StringFilterMode::STARTSWITH;
	else // if (same_string(fulltext_string_mode, "exact"))
		preset->data.fulltextStringMode = StringFilterMode::EXACT;
	preset->data.fullText = fulltext;
}

extern "C" int filter_preset_constraint_count(int preset)
{
	return (int)filter_preset_table[preset].data.constraints.size();
}

extern "C" const filter_constraint *filter_preset_constraint(int preset, int constraint)
{
	return &filter_preset_table[preset].data.constraints[constraint];
}

extern "C" struct filter_preset *alloc_filter_preset()
{
	return new filter_preset;
}

extern "C" void free_filter_preset(const struct filter_preset *preset)
{
	delete preset;
}

extern "C" void filter_preset_set_name(struct filter_preset *preset, const char *name)
{
	preset->name = name;
}

static int filter_preset_add_to_table(const QString &name, const FilterData &d, struct filter_preset_table &table)
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
static QString get_unique_preset_name(const QString &orig, const struct filter_preset_table &table)
{
	QString res = orig;
	int count = 2;
	while (std::find_if(table.begin(), table.end(),
			    [&res](const filter_preset &preset)
			    { return preset.name == res; }) != table.end()) {
		res = orig + "#" + QString::number(count);
		++count;
	}
	return res;
}

extern "C" void add_filter_preset_to_table(const struct filter_preset *preset, struct filter_preset_table *table)
{
	QString name = get_unique_preset_name(preset->name, *table);
	filter_preset_add_to_table(name, preset->data, *table);
}

extern "C" void filter_preset_add_constraint(struct filter_preset *preset, const char *type, const char *string_mode,
					     const char *range_mode, bool negate, const char *data)
{
	preset->data.constraints.emplace_back(type, string_mode, range_mode, negate, data);
}

int filter_preset_id(const QString &name)
{
	auto it = std::find_if(filter_preset_table.begin(), filter_preset_table.end(),
			       [&name] (filter_preset &p) { return p.name == name; });
	return it != filter_preset_table.end() ? it - filter_preset_table.begin() : -1;
}

QString filter_preset_name_qstring(int preset)
{
	return filter_preset_table[preset].name;
}

void filter_preset_set(int preset, const FilterData &data)
{
	filter_preset_table[preset].data = data;
}

FilterData filter_preset_get(int preset)
{
	return filter_preset_table[preset].data;
}

int filter_preset_add(const QString &nameIn, const FilterData &d)
{
	QString name = get_unique_preset_name(nameIn, filter_preset_table);
	return filter_preset_add_to_table(name, d, filter_preset_table);
}

void filter_preset_delete(int preset)
{
	filter_preset_table.erase(filter_preset_table.begin() + preset);
}
