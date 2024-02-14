// SPDX-License-Identifier: GPL-2.0
#include "eventtype.h"
#include "subsurface-string.h"

#include <string>
#include <vector>
#include <algorithm>

struct event_type {
	std::string name;
	int flags;
	bool plot;
};

static std::vector<event_type> event_types;

static bool operator==(const event_type &en1, const event_type &en2)
{
	return en1.name == en2.name && en1.flags == en2.flags;
}

extern "C" void clear_event_types()
{
	event_types.clear();
}

extern "C" void remember_event_type(const char *eventname, const int flags)
{
	if (empty_string(eventname))
		return;
	if (std::find(event_types.begin(), event_types.end(), event_type{ eventname, flags }) != event_types.end())
		return;
	event_types.push_back({ eventname, flags, true });
}

extern "C" bool is_event_type_hidden(const char *eventname, const int flags)
{
	auto it = std::find(event_types.begin(), event_types.end(), event_type{ eventname, flags });
	return it != event_types.end() && !it->plot;
}

extern "C" void hide_event_type(const char *eventname, const int flags)
{
	auto it = std::find(event_types.begin(), event_types.end(), event_type{ eventname, flags });
	if (it != event_types.end())
		it->plot = false;
}

extern "C" void show_all_event_types()
{
	for (event_type &e: event_types)
		e.plot = true;
}

extern "C" bool any_event_types_hidden()
{
	return std::any_of(event_types.begin(), event_types.end(),
			   [] (const event_type &e) { return !e.plot; });
}
