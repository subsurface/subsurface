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

	bool is_gaschange() const;
	bool is_divemodechange() const;
	event_severity get_severity() const;
};

class event_loop
{
	std::string name;
	size_t idx;
	const struct divecomputer &dc;
public:
	event_loop(const char *name, const struct divecomputer &dc);
	const struct event *next(); // nullptr -> end
};

/* Get gasmixes at increasing timestamps. */
class gasmix_loop {
	const struct dive &dive;
	const struct divecomputer &dc;
	bool first_run;
	event_loop loop;
	const struct event *next_event;
	int last_cylinder_index;
	int last_time;
	std::pair<gasmix, int> get_last_gasmix();
public:
	gasmix_loop(const struct dive &dive, const struct divecomputer &dc);
	// Return the next cylinder index / gasmix from the list of gas switches
	// and the time in seconds when this gas switch happened
	// (including the potentially imaginary first gas switch to cylinder 0 / air)
	std::pair<int, int> next_cylinder_index(); // <-1, 0> => implicit air cylinder, <-1, INT_MAX> => end
	std::pair<gasmix, int> next(); // <gasmix_invalid, INT_MAX> => end

	// Return the cylinder index / gasmix at a given time during the dive
	// and the time in seconds when this switch to this gas happened
	// (including the potentially imaginary first gas switch to cylinder 0 / air)
	std::pair<int, int> cylinder_index_at(int time); // <-1, 0> => implicit air cylinder
	std::pair<gasmix, int> at(int time);

	bool has_next() const;
};

/* Get divemodes at increasing timestamps. */
class divemode_loop {
	divemode_t last;
	int last_time;
	event_loop loop;
	const struct event *ev;
public:
	divemode_loop(const struct divecomputer &dc);
	// Return the divemode at a given time during the dive
	// and the time in seconds when the switch to this divemode has happened
	std::pair<divemode_t, int> at(int time);
};

extern const struct event *get_first_event(const struct divecomputer &dc, const std::string &name);
extern struct event *get_first_event(struct divecomputer &dc, const std::string &name);

#endif
