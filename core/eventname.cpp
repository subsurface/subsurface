// SPDX-License-Identifier: GPL-2.0
#include "eventname.h"
#include "subsurface-string.h"

#include <string>
#include <vector>
#include <algorithm>

struct event_name {
	std::string name;
	bool plot;
};

static std::vector<event_name> event_names;

// Small helper so that we can compare events to C-strings
static bool operator==(const event_name &en, const char *s)
{
	return en.name == s;
}

extern "C" void clear_event_names()
{
	event_names.clear();
}

extern "C" void remember_event_name(const char *eventname)
{
	if (empty_string(eventname))
		return;
	if (std::find(event_names.begin(), event_names.end(), eventname) != event_names.end())
		return;
	event_names.push_back({ eventname, true });
}

extern "C" bool is_event_hidden(const char *eventname)
{
	auto it = std::find(event_names.begin(), event_names.end(), eventname);
	return it != event_names.end() && !it->plot;
}

extern "C" void hide_event(const char *eventname)
{
	auto it = std::find(event_names.begin(), event_names.end(), eventname);
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
