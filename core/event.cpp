// SPDX-License-Identifier: GPL-2.0
#include "event.h"
#include "divecomputer.h"
#include "eventtype.h"
#include "subsurface-string.h"

#include <tuple>

event::event() : type(SAMPLE_EVENT_NONE), flags(0), value(0),
	divemode(OC), hidden(false)
{
	/* That overwrites divemode. Is this a smart thing to do? */
	gas.index = -1;
	gas.mix = gasmix_air;
}

event::event(unsigned int time, int type, int flags, int value, const std::string &name) :
	type(type), flags(flags), value(value), divemode(OC),
	hidden(false), name(name)
{
	int gas_index = -1;
	fraction_t he;
	this->time.seconds = time;

	/*
	 * Expand the events into a sane format. Currently
	 * just gas switches
	 */
	switch (type) {
	case SAMPLE_EVENT_GASCHANGE2:
		/* High 16 bits are He percentage */
		he.permille = (value >> 16) * 10;

		/* Extension to the GASCHANGE2 format: cylinder index in 'flags' */
		/* TODO: verify that gas_index < num_cylinders. */
		if (flags > 0)
			gas_index = flags-1;
	/* Fallthrough */
	case SAMPLE_EVENT_GASCHANGE:
		/* Low 16 bits are O2 percentage */
		gas.mix.he = he;
		gas.mix.o2.permille = (value & 0xffff) * 10;
		gas.index = gas_index;
		break;
	}

	remember_event_type(this);
}

event::~event()
{
}

bool event::is_gaschange() const
{
	return type == SAMPLE_EVENT_GASCHANGE || type == SAMPLE_EVENT_GASCHANGE2;
}

bool event::is_divemodechange() const
{
	return name == "modechange";
}

bool event::operator==(const event &b) const
{
	return std::tie(time.seconds, type, flags, value, name) ==
	       std::tie(b.time.seconds, b.type, b.flags, b.value, b.name);
}

enum event_severity event::get_severity() const
{
	switch (flags & SAMPLE_FLAGS_SEVERITY_MASK) {
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

event_loop::event_loop(const char *name, const struct divecomputer &dc) : name(name), idx(0), dc(dc)
{
}

const struct event *event_loop::next()
{
	if (name.empty())
		return nullptr;
	while (idx < dc.events.size()) {
		const struct event &ev = dc.events[idx++];
		if (ev.name == name)
			return &ev;
	}
	return nullptr;
}

struct event *get_first_event(struct divecomputer &dc, const std::string &name)
{
	auto it = std::find_if(dc.events.begin(), dc.events.end(), [name](auto &ev) { return ev.name == name; });
	return it != dc.events.end() ? &*it : nullptr;
}

const struct event *get_first_event(const struct divecomputer &dc, const std::string &name)
{
	return get_first_event(const_cast<divecomputer &>(dc), name);
}
