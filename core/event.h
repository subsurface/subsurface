// SPDX-License-Identifier: GPL-2.0
#ifndef EVENT_H
#define EVENT_H

#include "divemode.h"
#include "gas.h"
#include "units.h"

#include <libdivecomputer/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Events are currently based straight on what libdivecomputer gives us.
 *  We need to wrap these into our own events at some point to remove some of the limitations.
 */
struct event {
	struct event *next;
	duration_t time;
	int type;
	/* This is the annoying libdivecomputer format. */
	int flags, value;
	/* .. and this is our "extended" data for some event types */
	union {
		enum divemode_t divemode; // for divemode change events
		/*
		 * NOTE! The index may be -1, which means "unknown". In that
		 * case, the get_cylinder_index() function will give the best
		 * match with the cylinders in the dive based on gasmix.
		 */
		struct { // for gas switch events
			int index;
			struct gasmix mix;
		} gas;
	};
	bool deleted;
	char name[];
};

struct ev_select {
	char *ev_name;
	bool plot_ev;
};

/* collect all event names and whether we display them */
extern struct ev_select *ev_namelist;
extern int evn_used;

extern int event_is_gaschange(const struct event *ev);
extern bool event_is_divemodechange(const struct event *ev);
extern struct event *clone_event(const struct event *src_ev);
extern void free_events(struct event *ev);
extern struct event *create_event(unsigned int time, int type, int flags, int value, const char *name);
extern struct event *clone_event_rename(const struct event *ev, const char *name);
extern bool same_event(const struct event *a, const struct event *b);
extern void remember_event(const char *eventname);
extern void clear_events(void);

/* Since C doesn't have parameter-based overloading, two versions of get_next_event. */
extern const struct event *get_next_event(const struct event *event, const char *name);
extern struct event *get_next_event_mutable(struct event *event, const char *name);


#ifdef __cplusplus
}
#endif

#endif
