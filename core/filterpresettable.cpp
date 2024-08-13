// SPDX-License-Identifier: GPL-2.0
#include "filterpresettable.h"
#include "filterpreset.h"

#include <algorithm>

int filter_preset_table::preset_id(const std::string &name) const
{
	auto it = std::find_if(begin(), end(), [&name] (const filter_preset &p) { return p.name == name; });
	return it != end() ? it - begin() : -1;
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

void filter_preset_table::add(const filter_preset &preset)
{
	std::string name = get_unique_preset_name(preset.name, *this);
	filter_preset_add_to_table(std::move(name), preset.data, *this);
}

int filter_preset_table::add(const std::string &nameIn, const FilterData &d)
{
	std::string name = get_unique_preset_name(nameIn, *this);
	return filter_preset_add_to_table(std::move(name), d, *this);
}

void filter_preset_table::remove(int preset)
{
	erase(begin() + preset);
}
