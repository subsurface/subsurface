// collect all event names and whether we display events of that type
// SPDX-License-Identifier: GPL-2.0
#ifndef EVENTNAME_H
#define EVENTNAME_H

#ifdef __cplusplus
extern "C" {
#endif

extern void clear_event_names(void);
extern void remember_event_name(const char *eventname, const int flags);
extern bool is_event_hidden(const char *eventname, const int flags);
extern void hide_similar_events(const char *eventname, const int flags);
extern void show_all_events();
extern bool any_events_hidden();

#ifdef __cplusplus
}
#endif

#endif
