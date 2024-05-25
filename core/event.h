// SPDX-License-Identifier: GPL-2.0
#ifndef EVENT_H
#define EVENT_H

#include "divemode.h"
#include "gas.h"
#include "units.h"

#include <string>
#include <libdivecomputer/parser.h>

struct divecomputer;

enum event_severity {
	EVENT_SEVERITY_NONE = 0,
	EVENT_SEVERITY_INFO,
	EVENT_SEVERITY_WARN,
	EVENT_SEVERITY_ALARM
};

/*
 * Events are currently based straight on what libdivecomputer gives us.
 *  We need to wrap these into our own events at some point to remove some of the limitations.
 */

struct event {
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
	bool hidden;
	std::string name;
	event();
	event(unsigned int time, int type, int flags, int value, const std::string &name);
	~event();

	bool operator==(const event &b2) const;
};

class event_loop
{
	std::string name;
	size_t idx;
public:
	event_loop(const char *name);
	struct event *next(struct divecomputer &dc); // nullptr -> end
	const struct event *next(const struct divecomputer &dc); // nullptr -> end
};

/* Get gasmixes at increasing timestamps. */
class gasmix_loop {
	const struct dive &dive;
	const struct divecomputer &dc;
	struct gasmix last;
	event_loop loop;
	const struct event *ev;
public:
	gasmix_loop(const struct dive &dive, const struct divecomputer &dc);
	gasmix next(int time);
};

/* Get divemodes at increasing timestamps. */
class divemode_loop {
	const struct divecomputer &dc;
	divemode_t last;
	event_loop loop;
	const struct event *ev;
public:
	divemode_loop(const struct divecomputer &dc);
	divemode_t next(int time);
};

extern bool event_is_gaschange(const struct event &ev);
extern bool event_is_divemodechange(const struct event &ev);
extern enum event_severity get_event_severity(const struct event &ev);

extern const struct event *get_first_event(const struct divecomputer &dc, const std::string &name);
extern struct event *get_first_event(struct divecomputer &dc, const std::string &name);

#endif
