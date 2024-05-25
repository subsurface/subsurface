// collect all event names and whether we display events of that type
// SPDX-License-Identifier: GPL-2.0
#ifndef EVENTNAME_H
#define EVENTNAME_H

#include <vector>
#include <QString>

extern void clear_event_types();
extern void remember_event_type(const struct event *ev);
extern bool is_event_type_hidden(const struct event *ev);
extern void hide_event_type(const struct event *ev);
extern void show_all_event_types();
extern void show_event_type(int idx);
extern bool any_event_types_hidden();
extern std::vector<int> hidden_event_types();
extern QString event_type_name(const event &ev);
extern QString event_type_name(int idx);

#endif
