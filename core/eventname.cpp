// SPDX-License-Identifier: GPL-2.0
#include "eventname.h"
#include "subsurface-string.h"

#include <string>
#include <vector>
#include <algorithm>

struct event_name {
	std::string name;
	int flags;
	bool plot;
};

static std::vector<event_name> event_names;

// Small helper so that we can compare events to C-strings
static bool operator==(const event_name &en1, const event_name &en2)
{
	return en1.name == en2.name && en1.flags == en2.flags;
}

extern "C" void clear_event_names()
{
	event_names.clear();
}

extern "C" void remember_event_name(const char *eventname, const int flags)
{
	if (empty_string(eventname))
		return;
	if (std::find(event_names.begin(), event_names.end(), event_name{ eventname, flags }) != event_names.end())
		return;
	event_names.push_back({ eventname, flags, true });
}

extern "C" bool is_event_hidden(const char *eventname, const int flags)
{
	auto it = std::find(event_names.begin(), event_names.end(), event_name{ eventname, flags });
	return it != event_names.end() && !it->plot;
}

extern "C" void hide_similar_events(const char *eventname, const int flags)
{
	auto it = std::find(event_names.begin(), event_names.end(), event_name{ eventname, flags });
	if (it != event_names.end())
		it->plot = false;
}

extern "C" void show_all_events()
{
	for (event_name &en: event_names)
		en.plot = true;
}

extern "C" bool any_events_hidden()
{
	return std::any_of(event_names.begin(), event_names.end(),
			   [] (const event_name &en) { return !en.plot; });
}
