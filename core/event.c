// SPDX-License-Identifier: GPL-2.0
#include "event.h"
#include "subsurface-string.h"

#include <string.h>
#include <stdlib.h>

int event_is_gaschange(const struct event *ev)
{
	return ev->type == SAMPLE_EVENT_GASCHANGE ||
		ev->type == SAMPLE_EVENT_GASCHANGE2;
}

bool event_is_divemodechange(const struct event *ev)
{
	return same_string(ev->name, "modechange");
}

struct event *clone_event(const struct event *src_ev)
{
	struct event *ev;
	if (!src_ev)
		return NULL;

	size_t size = sizeof(*src_ev) + strlen(src_ev->name) + 1;
	ev = (struct event*) malloc(size);
	if (!ev)
		exit(1);
	memcpy(ev, src_ev, size);
	ev->next = NULL;

	return ev;
}

void free_events(struct event *ev)
{
	while (ev) {
		struct event *next = ev->next;
		free(ev);
		ev = next;
	}
}

struct event *create_event(unsigned int time, int type, int flags, int value, const char *name)
{
	int gas_index = -1;
	struct event *ev;
	unsigned int size, len = strlen(name);

	size = sizeof(*ev) + len + 1;
	ev = malloc(size);
	if (!ev)
		return NULL;
	memset(ev, 0, size);
	memcpy(ev->name, name, len);
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

	return ev;
}

struct event *clone_event_rename(const struct event *ev, const char *name)
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
	return !strcmp(a->name, b->name);
}

/* collect all event names and whether we display them */
struct ev_select *ev_namelist = NULL;
int evn_used = 0;
static int evn_allocated = 0;

void clear_events(void)
{
	for (int i = 0; i < evn_used; i++)
		free(ev_namelist[i].ev_name);
	evn_used = 0;
}

void remember_event(const char *eventname)
{
	int i = 0, len;

	if (!eventname || (len = strlen(eventname)) == 0)
		return;
	while (i < evn_used) {
		if (!strncmp(eventname, ev_namelist[i].ev_name, len))
			return;
		i++;
	}
	if (evn_used == evn_allocated) {
		evn_allocated += 10;
		ev_namelist = realloc(ev_namelist, evn_allocated * sizeof(struct ev_select));
		if (!ev_namelist)
			/* we are screwed, but let's just bail out */
			return;
	}
	ev_namelist[evn_used].ev_name = strdup(eventname);
	ev_namelist[evn_used].plot_ev = true;
	evn_used++;
}
