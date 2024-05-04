// SPDX-License-Identifier: GPL-2.0
#include "event.h"
#include "eventtype.h"
#include "subsurface-string.h"

#include <string.h>
#include <stdlib.h>

event::event() : next(nullptr), type(SAMPLE_EVENT_NONE), flags(0), value(0),
	divemode(OC), deleted(false), hidden(false)
{
	/* That overwrites divemode. Is this a smart thing to do? */
	gas.index = -1;
	gas.mix = gasmix_air;
}

event::~event()
{
}

int event_is_gaschange(const struct event *ev)
{
	return ev->type == SAMPLE_EVENT_GASCHANGE ||
		ev->type == SAMPLE_EVENT_GASCHANGE2;
}

bool event_is_divemodechange(const struct event *ev)
{
	return ev->name == "modechange";
}

struct event *clone_event(const struct event *src_ev)
{
	struct event *ev;
	if (!src_ev)
		return NULL;

	ev = new event;
	*ev = *src_ev;
	ev->next = NULL;

	return ev;
}

void free_events(struct event *ev)
{
	while (ev) {
		struct event *next = ev->next;
		delete ev;
		ev = next;
	}
}

struct event *create_event(unsigned int time, int type, int flags, int value, const std::string &name)
{
	int gas_index = -1;
	struct event *ev;

	ev = new event;
	ev->name = name;
	ev->time.seconds = time;
	ev->type = type;
	ev->flags = flags;
	ev->value = value;

	/*
	 * Expand the events into a sane format. Currently
	 * just gas switches
	 */
	switch (type) {
	case SAMPLE_EVENT_GASCHANGE2:
		/* High 16 bits are He percentage */
		ev->gas.mix.he.permille = (value >> 16) * 10;

		/* Extension to the GASCHANGE2 format: cylinder index in 'flags' */
		/* TODO: verify that gas_index < num_cylinders. */
		if (flags > 0)
			gas_index = flags-1;
	/* Fallthrough */
	case SAMPLE_EVENT_GASCHANGE:
		/* Low 16 bits are O2 percentage */
		ev->gas.mix.o2.permille = (value & 0xffff) * 10;
		ev->gas.index = gas_index;
		break;
	}

	remember_event_type(ev);

	return ev;
}

struct event *clone_event_rename(const struct event *ev, const std::string &name)
{
	return create_event(ev->time.seconds, ev->type, ev->flags, ev->value, name);
}

bool same_event(const struct event *a, const struct event *b)
{
	if (a->time.seconds != b->time.seconds)
		return 0;
	if (a->type != b->type)
		return 0;
	if (a->flags != b->flags)
		return 0;
	if (a->value != b->value)
		return 0;
	return a->name == b->name;
}

extern enum event_severity get_event_severity(const struct event *ev)
{
	switch (ev->flags & SAMPLE_FLAGS_SEVERITY_MASK) {
	case SAMPLE_FLAGS_SEVERITY_INFO:
		return EVENT_SEVERITY_INFO;
	case SAMPLE_FLAGS_SEVERITY_WARN:
		return EVENT_SEVERITY_WARN;
	case SAMPLE_FLAGS_SEVERITY_ALARM:
		return EVENT_SEVERITY_ALARM;
	default:
		return EVENT_SEVERITY_NONE;
	}
}
