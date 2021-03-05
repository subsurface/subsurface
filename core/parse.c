#include "ssrf.h"
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "membuffer.h"
#include <stdlib.h>
#include <unistd.h>
#include <libdivecomputer/parser.h>

#include "parse.h"
#include "dive.h"
#include "divesite.h"
#include "errorhelper.h"
#include "sample.h"
#include "subsurface-string.h"
#include "picture.h"
#include "trip.h"
#include "device.h"
#include "gettext.h"

struct dive_table dive_table;

void init_parser_state(struct parser_state *state)
{
	memset(state, 0, sizeof(*state));
	state->metric = true;
	state->cur_event.deleted = 1;
	state->sample_rate = 0;
}

void free_parser_state(struct parser_state *state)
{
	free_dive(state->cur_dive);
	free_trip(state->cur_trip);
	free_dive_site(state->cur_dive_site);
	free_filter_preset(state->cur_filter);
	free((void *)state->cur_extra_data.key);
	free((void *)state->cur_extra_data.value);
	free((void *)state->cur_settings.dc.model);
	free((void *)state->cur_settings.dc.nickname);
	free((void *)state->cur_settings.dc.serial_nr);
	free((void *)state->cur_settings.dc.firmware);
	free(state->country);
	free(state->city);
	free(state->fulltext);
	free(state->fulltext_string_mode);
	free(state->filter_constraint_type);
	free(state->filter_constraint_string_mode);
	free(state->filter_constraint_range_mode);
	free(state->filter_constraint);
}

/*
 * If we don't have an explicit dive computer,
 * we use the implicit one that every dive has..
 */
struct divecomputer *get_dc(struct parser_state *state)
{
	return state->cur_dc ?: &state->cur_dive->dc;
}

/* Trim a character string by removing leading and trailing white space characters.
 * Parameter: a pointer to a null-terminated character string (buffer);
 * Return value: length of the trimmed string, excluding the terminal 0x0 byte
 * The original pointer (buffer) remains valid after this function has been called
 * and points to the trimmed string */
int trimspace(char *buffer)
{
	int i, size, start, end;
	size = strlen(buffer);

	if (!size)
		return 0;
	for(start = 0; isspace(buffer[start]); start++)
		if (start >= size) return 0;	// Find 1st character following leading whitespace
	for(end = size - 1; isspace(buffer[end]); end--) // Find last character before trailing whitespace
		if (end <= 0) return 0;
	for(i = start; i <= end; i++)		// Move the nonspace characters to the start of the string
		buffer[i-start] = buffer[i];
	size = end - start + 1;
	buffer[size] = 0x0;			// then terminate the string
	return size;				// return string length
}

/*
 * Add a dive into the dive_table array
 */
void record_dive_to_table(struct dive *dive, struct dive_table *table)
{
	add_to_dive_table(table, table->nr, fixup_dive(dive));
}

void start_match(const char *type, const char *name, char *buffer)
{
	if (verbose > 2)
		printf("Matching %s '%s' (%s)\n",
		       type, name, buffer);
}

void nonmatch(const char *type, const char *name, char *buffer)
{
	if (verbose > 1)
		printf("Unable to match %s '%s' (%s)\n",
		       type, name, buffer);
}

void event_start(struct parser_state *state)
{
	memset(&state->cur_event, 0, sizeof(state->cur_event));
	state->cur_event.deleted = 0;	/* Active */
}

void event_end(struct parser_state *state)
{
	struct divecomputer *dc = get_dc(state);
	if (state->cur_event.type == 123) {
		struct picture pic = empty_picture;
		pic.filename = strdup(state->cur_event.name);
		/* theoretically this could fail - but we didn't support multi year offsets */
		pic.offset.seconds = state->cur_event.time.seconds;
		add_picture(&state->cur_dive->pictures, pic); /* Takes ownership. */
	} else {
		struct event *ev;
		/* At some point gas change events did not have any type. Thus we need to add
		 * one on import, if we encounter the type one missing.
		 */
		if (state->cur_event.type == 0 && strcmp(state->cur_event.name, "gaschange") == 0)
			state->cur_event.type = state->cur_event.value >> 16 > 0 ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE;
		ev = add_event(dc, state->cur_event.time.seconds,
			       state->cur_event.type, state->cur_event.flags,
			       state->cur_event.value, state->cur_event.name);

		/*
		 * Older logs might mark the dive to be CCR by having an "SP change" event at time 0:00. Better
		 * to mark them being CCR on import so no need for special treatments elsewhere on the code.
		 */
		if (ev && state->cur_event.time.seconds == 0 && state->cur_event.type == SAMPLE_EVENT_PO2 && state->cur_event.value && dc->divemode==OC) {
			dc->divemode = CCR;
		}

		if (ev && event_is_gaschange(ev)) {
			/* See try_to_fill_event() on why the filled-in index is one too big */
			ev->gas.index = state->cur_event.gas.index-1;
			if (state->cur_event.gas.mix.o2.permille || state->cur_event.gas.mix.he.permille)
				ev->gas.mix = state->cur_event.gas.mix;
		}
	}
	state->cur_event.deleted = 1;	/* No longer active */
}

/*
 * While in some formats file boundaries are dive boundaries, in many
 * others (as for example in our native format) there are
 * multiple dives per file, so there can be other events too that
 * trigger a "new dive" marker and you may get some nesting due
 * to that. Just ignore nesting levels.
 * On the flipside it is possible that we start an XML file that ends
 * up having no dives in it at all - don't create a bogus empty dive
 * for those. It's not entirely clear what is the minimum set of data
 * to make a dive valid, but if it has no location, no date and no
 * samples I'm pretty sure it's useless.
 */
bool is_dive(struct parser_state *state)
{
	return state->cur_dive &&
		(state->cur_dive->dive_site || state->cur_dive->when || state->cur_dive->dc.samples);
}

void reset_dc_info(struct divecomputer *dc, struct parser_state *state)
{
	/* WARN: reset dc info does't touch the dc? */
	UNUSED(dc);
	state->lastcylinderindex = 0;
}

void reset_dc_settings(struct parser_state *state)
{
	free((void *)state->cur_settings.dc.model);
	free((void *)state->cur_settings.dc.nickname);
	free((void *)state->cur_settings.dc.serial_nr);
	free((void *)state->cur_settings.dc.firmware);
	state->cur_settings.dc.model = NULL;
	state->cur_settings.dc.nickname = NULL;
	state->cur_settings.dc.serial_nr = NULL;
	state->cur_settings.dc.firmware = NULL;
	state->cur_settings.dc.deviceid = 0;
}

void settings_start(struct parser_state *state)
{
	state->in_settings = true;
}

void settings_end(struct parser_state *state)
{
	state->in_settings = false;
}

void dc_settings_start(struct parser_state *state)
{
	reset_dc_settings(state);
}

void dc_settings_end(struct parser_state *state)
{
	create_device_node(state->devices, state->cur_settings.dc.model, state->cur_settings.dc.deviceid, state->cur_settings.dc.serial_nr,
			   state->cur_settings.dc.firmware, state->cur_settings.dc.nickname);
	reset_dc_settings(state);
}

void dive_site_start(struct parser_state *state)
{
	if (state->cur_dive_site)
		return;
	state->taxonomy_category = -1;
	state->taxonomy_origin = -1;
	state->cur_dive_site = calloc(1, sizeof(struct dive_site));
}

void dive_site_end(struct parser_state *state)
{
	if (!state->cur_dive_site)
		return;
	if (state->cur_dive_site->uuid) {
		struct dive_site *ds = alloc_or_get_dive_site(state->cur_dive_site->uuid, state->sites);
		merge_dive_site(ds, state->cur_dive_site);

		if (verbose > 3)
			printf("completed dive site uuid %x8 name {%s}\n", ds->uuid, ds->name);
	}
	free_dive_site(state->cur_dive_site);
	state->cur_dive_site = NULL;
}

void filter_preset_start(struct parser_state *state)
{
	if (state->cur_filter)
		return;
	state->cur_filter = alloc_filter_preset();
}

void filter_preset_end(struct parser_state *state)
{
	add_filter_preset_to_table(state->cur_filter, state->filter_presets);
	free_filter_preset(state->cur_filter);
	state->cur_filter = NULL;
}

void fulltext_start(struct parser_state *state)
{
	if (!state->cur_filter)
		return;
	state->in_fulltext = true;
}

void fulltext_end(struct parser_state *state)
{
	if (!state->in_fulltext)
		return;
	filter_preset_set_fulltext(state->cur_filter, state->fulltext, state->fulltext_string_mode);
	free(state->fulltext);
	free(state->fulltext_string_mode);
	state->fulltext = NULL;
	state->fulltext_string_mode = NULL;
	state->in_fulltext = false;
}

void filter_constraint_start(struct parser_state *state)
{
	if (!state->cur_filter)
		return;
	state->in_filter_constraint = true;
}

void filter_constraint_end(struct parser_state *state)
{
	if (!state->in_filter_constraint)
		return;
	filter_preset_add_constraint(state->cur_filter, state->filter_constraint_type, state->filter_constraint_string_mode,
				     state->filter_constraint_range_mode, state->filter_constraint_negate, state->filter_constraint);
	free(state->filter_constraint_type);
	free(state->filter_constraint_string_mode);
	free(state->filter_constraint_range_mode);
	free(state->filter_constraint);

	state->filter_constraint_type = NULL;
	state->filter_constraint_string_mode = NULL;
	state->filter_constraint_range_mode = NULL;
	state->filter_constraint_negate = false;
	state->filter_constraint = NULL;
	state->in_filter_constraint = false;
}

void dive_start(struct parser_state *state)
{
	if (state->cur_dive)
		return;
	state->cur_dive = alloc_dive();
	reset_dc_info(&state->cur_dive->dc, state);
	memset(&state->cur_tm, 0, sizeof(state->cur_tm));
	state->o2pressure_sensor = 1;
}

void dive_end(struct parser_state *state)
{
	if (!state->cur_dive)
		return;
	if (!is_dive(state)) {
		free_dive(state->cur_dive);
	} else {
		record_dive_to_table(state->cur_dive, state->target_table);
		if (state->cur_trip)
			add_dive_to_trip(state->cur_dive, state->cur_trip);
	}
	state->cur_dive = NULL;
	state->cur_dc = NULL;
	state->cur_location.lat.udeg = 0;
	state->cur_location.lon.udeg = 0;
}

void trip_start(struct parser_state *state)
{
	if (state->cur_trip)
		return;
	dive_end(state);
	state->cur_trip = alloc_trip();
	memset(&state->cur_tm, 0, sizeof(state->cur_tm));
}

void trip_end(struct parser_state *state)
{
	if (!state->cur_trip)
		return;
	insert_trip(state->cur_trip, state->trips);
	state->cur_trip = NULL;
}

void picture_start(struct parser_state *state)
{
}

void picture_end(struct parser_state *state)
{
	add_picture(&state->cur_dive->pictures, state->cur_picture);
	/* dive_add_picture took ownership, we can just clear out copy of the data */
	state->cur_picture = empty_picture;
}

cylinder_t *cylinder_start(struct parser_state *state)
{
	return add_empty_cylinder(&state->cur_dive->cylinders);
}

void cylinder_end(struct parser_state *state)
{
}

void ws_start(struct parser_state *state)
{
	add_cloned_weightsystem(&state->cur_dive->weightsystems, empty_weightsystem);
}

void ws_end(struct parser_state *state)
{
}

/*
 * By default the sample data does not change unless the
 * save-file gives an explicit new value. So we copy the
 * data from the previous sample if one exists, and then
 * the parsing will update it as necessary.
 *
 * There are a few exceptions, like the sample pressure:
 * missing sample pressure doesn't mean "same as last
 * time", but "interpolate". We clear those ones
 * explicitly.
 *
 * NOTE! We default sensor use to 0, 1 respetively for
 * the two sensors, but for CCR dives with explicit
 * OXYGEN bottles we set the secondary sensor to that.
 * Then the primary sensor will be either the first
 * or the second cylinder depending on what isn't an
 * oxygen cylinder.
 */
void sample_start(struct parser_state *state)
{
	struct divecomputer *dc = get_dc(state);
	struct sample *sample = prepare_sample(dc);

	if (sample != dc->sample) {
		memcpy(sample, sample-1, sizeof(struct sample));
		sample->pressure[0].mbar = 0;
		sample->pressure[1].mbar = 0;
	} else {
		sample->sensor[0] = !state->o2pressure_sensor;
		sample->sensor[1] = state->o2pressure_sensor;
	}
	state->cur_sample = sample;
	state->next_o2_sensor = 0;
}

void sample_end(struct parser_state *state)
{
	if (!state->cur_dive)
		return;

	finish_sample(get_dc(state));
	state->cur_sample = NULL;
}

void divecomputer_start(struct parser_state *state)
{
	struct divecomputer *dc;

	/* Start from the previous dive computer */
	dc = &state->cur_dive->dc;
	while (dc->next)
		dc = dc->next;

	/* Did we already fill that in? */
	if (dc->samples || dc->model || dc->when) {
		struct divecomputer *newdc = calloc(1, sizeof(*newdc));
		if (newdc) {
			dc->next = newdc;
			dc = newdc;
		}
	}

	/* .. this is the one we'll use */
	state->cur_dc = dc;
	reset_dc_info(dc, state);
}

void divecomputer_end(struct parser_state *state)
{
	if (!state->cur_dc->when)
		state->cur_dc->when = state->cur_dive->when;
	state->cur_dc = NULL;
}

void userid_start(struct parser_state *state)
{
	state->in_userid = true;
}

void userid_stop(struct parser_state *state)
{
	state->in_userid = false;
}

/*
 * Copy whitespace-trimmed string. Warning: the passed in string will be freed,
 * therefore make sure to only pass in to NULL-initialized pointers or pointers
 * to owned strings
 */
void utf8_string(char *buffer, void *_res)
{
	char **res = _res;
	int size;
	free(*res);
	size = trimspace(buffer);
	if(size)
		*res = strdup(buffer);
}

void add_dive_site(char *ds_name, struct dive *dive, struct parser_state *state)
{
	char *buffer = ds_name;
	char *to_free = NULL;
	int size = trimspace(buffer);
	if (size) {
		struct dive_site *ds = dive->dive_site;
		if (!ds) {
			// if the dive doesn't have a dive site, check if there's already a dive site by this name
			ds = get_dive_site_by_name(buffer, state->sites);
		}
		if (ds) {
			// we have a dive site, let's hope there isn't a different name
			if (empty_string(ds->name)) {
				ds->name = copy_string(buffer);
			} else if (!same_string(ds->name, buffer)) {
				// if it's not the same name, it's not the same dive site
				// but wait, we could have gotten this one based on GPS coords and could
				// have had two different names for the same site... so let's search the other
				// way around
				struct dive_site *exact_match = get_dive_site_by_gps_and_name(buffer, &ds->location, state->sites);
				if (exact_match) {
					unregister_dive_from_dive_site(dive);
					add_dive_to_dive_site(dive, exact_match);
				} else {
					struct dive_site *newds = create_dive_site(buffer, state->sites);
					unregister_dive_from_dive_site(dive);
					add_dive_to_dive_site(dive, newds);
					if (has_location(&state->cur_location)) {
						// we started this uuid with GPS data, so lets use those
						newds->location = state->cur_location;
					} else {
						newds->location = ds->location;
					}
					newds->notes = add_to_string(newds->notes, translate("gettextFromC", "additional name for site: %s\n"), ds->name);
				}
			} else if (dive->dive_site != ds) {
				// add the existing dive site to the current dive
				unregister_dive_from_dive_site(dive);
				add_dive_to_dive_site(dive, ds);
			}
		} else {
			add_dive_to_dive_site(dive, create_dive_site(buffer, state->sites));
		}
	}
	free(to_free);
}

int atoi_n(char *ptr, unsigned int len)
{
	if (len < 10) {
		char buf[10];

		memcpy(buf, ptr, len);
		buf[len] = 0;
		return atoi(buf);
	}
	return 0;
}
