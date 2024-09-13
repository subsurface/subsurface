#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "membuffer.h"
#include <stdlib.h>
#include <unistd.h>
#include <libdivecomputer/parser.h>

#include "parse.h"
#include "dive.h"
#include "divelog.h"
#include "divesite.h"
#include "errorhelper.h"
#include "format.h"
#include "sample.h"
#include "subsurface-string.h"
#include "picture.h"
#include "trip.h"
#include "device.h"
#include "gettext.h"

parser_state::parser_state() = default;
parser_state::~parser_state() = default;

/*
 * If we don't have an explicit dive computer,
 * we use the implicit one that every dive has..
 */
struct divecomputer *get_dc(struct parser_state *state)
{
	return state->cur_dc ?: &state->cur_dive->dcs[0];
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
	state->cur_event = event();
	state->event_active = true;	/* Active */
}

void event_end(struct parser_state *state)
{
	struct divecomputer *dc = get_dc(state);
	if (state->cur_event.type == 123) {
		struct picture pic;
		pic.filename = state->cur_event.name;
		/* theoretically this could fail - but we didn't support multi year offsets */
		pic.offset.seconds = state->cur_event.time.seconds;
		add_picture(state->cur_dive->pictures, std::move(pic));
	} else {
		/* At some point gas change events did not have any type. Thus we need to add
		 * one on import, if we encounter the type one missing.
		 */
		if (state->cur_event.type == 0 && state->cur_event.name == "gaschange")
			state->cur_event.type = state->cur_event.value >> 16 > 0 ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE;

		struct event ev(state->cur_event.time.seconds, state->cur_event.type, state->cur_event.flags,
				state->cur_event.value, state->cur_event.name);

		/*
		 * Older logs might mark the dive to be CCR by having an "SP change" event at time 0:00. Better
		 * to mark them being CCR on import so no need for special treatments elsewhere on the code.
		 */
		if (ev.time.seconds == 0 && ev.type == SAMPLE_EVENT_PO2 && ev.value && dc->divemode==OC)
			dc->divemode = CCR;

		if (ev.is_gaschange()) {
			/* See try_to_fill_event() on why the filled-in index is one too big */
			ev.gas.index = state->cur_event.gas.index-1;
			if (state->cur_event.gas.mix.o2.permille || state->cur_event.gas.mix.he.permille)
				ev.gas.mix = state->cur_event.gas.mix;
		}

		add_event_to_dc(dc, std::move(ev));
	}
	state->event_active = false;	/* No longer active */
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
		(state->cur_dive->dive_site || state->cur_dive->when || !state->cur_dive->dcs[0].samples.empty());
}

void reset_dc_info(struct divecomputer *, struct parser_state *state)
{
	/* WARN: reset dc info does't touch the dc? */
	state->lastcylinderindex = 0;
}

void reset_dc_settings(struct parser_state *state)
{
	state->cur_settings.dc.model.clear();
	state->cur_settings.dc.nickname.clear();
	state->cur_settings.dc.serial_nr.clear();
	state->cur_settings.dc.firmware.clear();
	state->cur_settings.dc.deviceid = 0;
}

void reset_fingerprint(struct parser_state *state)
{
	state->cur_settings.fingerprint.data.clear();
	state->cur_settings.fingerprint.model = 0;
	state->cur_settings.fingerprint.serial = 0;
	state->cur_settings.fingerprint.fdeviceid = 0;
	state->cur_settings.fingerprint.fdiveid = 0;
}

void settings_start(struct parser_state *state)
{
	state->in_settings = true;
}

void settings_end(struct parser_state *state)
{
	state->in_settings = false;
}

void fingerprint_settings_start(struct parser_state *state)
{
	reset_fingerprint(state);
}

void fingerprint_settings_end(struct parser_state *state)
{
	create_fingerprint_node_from_hex(*state->fingerprints,
			state->cur_settings.fingerprint.model,
			state->cur_settings.fingerprint.serial,
			state->cur_settings.fingerprint.data,
			state->cur_settings.fingerprint.fdeviceid,
			state->cur_settings.fingerprint.fdiveid);
}

void dc_settings_start(struct parser_state *state)
{
	reset_dc_settings(state);
}

void dc_settings_end(struct parser_state *state)
{
	create_device_node(state->log->devices,
		state->cur_settings.dc.model,
		state->cur_settings.dc.serial_nr,
		state->cur_settings.dc.nickname);
	reset_dc_settings(state);
}

void dive_site_start(struct parser_state *state)
{
	if (state->cur_dive_site)
		return;
	state->taxonomy_category = -1;
	state->taxonomy_origin = -1;
	state->cur_dive_site = std::make_unique<dive_site>();
}

void dive_site_end(struct parser_state *state)
{
	if (!state->cur_dive_site)
		return;

	struct dive_site *ds = state->log->sites.alloc_or_get(state->cur_dive_site->uuid);
	ds->merge(*state->cur_dive_site);

	if (verbose > 3)
		printf("completed dive site uuid %x8 name {%s}\n", ds->uuid, ds->name.c_str());

	state->cur_dive_site.reset();
}

void filter_preset_start(struct parser_state *state)
{
	if (state->cur_filter)
		return;
	state->cur_filter = std::make_unique<filter_preset>();
}

void filter_preset_end(struct parser_state *state)
{
	state->log->filter_presets.add(*state->cur_filter);
	state->cur_filter.reset();
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
	state->cur_filter->set_fulltext(std::move(state->fulltext), state->fulltext_string_mode);
	state->fulltext.clear();
	state->fulltext_string_mode.clear();
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
	state->cur_filter->add_constraint(state->filter_constraint_type, state->filter_constraint_string_mode,
					  state->filter_constraint_range_mode, state->filter_constraint_negate, state->filter_constraint);

	state->filter_constraint_type.clear();
	state->filter_constraint_string_mode.clear();
	state->filter_constraint_range_mode.clear();
	state->filter_constraint_negate = false;
	state->filter_constraint.clear();
	state->in_filter_constraint = false;
}

void dive_start(struct parser_state *state)
{
	if (state->cur_dive)
		return;
	state->cur_dive = std::make_unique<dive>();
	reset_dc_info(&state->cur_dive->dcs[0], state);
	memset(&state->cur_tm, 0, sizeof(state->cur_tm));
	state->o2pressure_sensor = 1;
}

void dive_end(struct parser_state *state)
{
	if (!state->cur_dive)
		return;
	if (is_dive(state)) {
		if (state->cur_trip)
			state->cur_trip->add_dive(state->cur_dive.get());
		// This would add dives in a sorted way:
		state->log->dives.record_dive(std::move(state->cur_dive));
	}
	state->cur_dive.reset();
	state->cur_dc = NULL;
	state->cur_location = location_t();
}

void trip_start(struct parser_state *state)
{
	if (state->cur_trip)
		return;
	dive_end(state);
	state->cur_trip = std::make_unique<dive_trip>();
	memset(&state->cur_tm, 0, sizeof(state->cur_tm));
}

void trip_end(struct parser_state *state)
{
	if (!state->cur_trip)
		return;
	state->log->trips.put(std::move(state->cur_trip));
}

void picture_start(struct parser_state *state)
{
}

void picture_end(struct parser_state *state)
{
	add_picture(state->cur_dive->pictures, std::move(state->cur_picture));
	/* dive_add_picture took ownership, we can just clear out copy of the data */
	state->cur_picture = picture();
}

cylinder_t *cylinder_start(struct parser_state *state)
{
	state->cur_dive->cylinders.emplace_back();
	return &state->cur_dive->cylinders.back();
}

void cylinder_end(struct parser_state *state)
{
}

void ws_start(struct parser_state *state)
{
	state->cur_dive->weightsystems.emplace_back();
}

void ws_end(struct parser_state *state)
{
}

/*
 * If the given cylinder doesn't exist, return NO_SENSOR.
 */
static int sanitize_sensor_id(const struct dive *d, int nr)
{
	return d && nr >= 0 && static_cast<size_t>(nr) < d->cylinders.size() ? nr : NO_SENSOR;
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

	if (dc->samples.size() > 1) {
		*sample = dc->samples[dc->samples.size() - 2];
		sample->pressure[0] = 0_bar;
		sample->pressure[1] = 0_bar;
	} else {
		sample->sensor[0] = sanitize_sensor_id(state->cur_dive.get(), !state->o2pressure_sensor);
		sample->sensor[1] = sanitize_sensor_id(state->cur_dive.get(), state->o2pressure_sensor);
	}
	state->cur_sample = sample;
	state->next_o2_sensor = 0;
}

void sample_end(struct parser_state *state)
{
	if (!state->cur_dive)
		return;

	state->cur_sample = NULL;
}

void divecomputer_start(struct parser_state *state)
{
	struct divecomputer *dc = &state->cur_dive->dcs.back();

	/* Did we already fill that in? */
	if (!dc->samples.empty() || !dc->model.empty() || dc->when) {
		state->cur_dive->dcs.emplace_back();
		dc = &state->cur_dive->dcs.back();
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
 * Copy whitespace-trimmed string.
 */
void utf8_string_std(const char *buffer, std::string *res)
{
	while (isspace(*buffer))
		++buffer;
	if (!*buffer) {
		res->clear();
		return;
	}
	const char *end = buffer + strlen(buffer);
	while (isspace(end[-1]))
		--end;
	*res = std::string(buffer, end - buffer);
}

void add_dive_site(const char *ds_name, struct dive *dive, struct parser_state *state)
{
	const char *buffer = ds_name;
	std::string trimmed = trimspace(buffer);
	if (!trimmed.empty()) {
		struct dive_site *ds = dive->dive_site;
		if (!ds) {
			// if the dive doesn't have a dive site, check if there's already a dive site by this name
			ds = state->log->sites.get_by_name(trimmed);
		}
		if (ds) {
			// we have a dive site, let's hope there isn't a different name
			if (ds->name.empty()) {
				ds->name = trimmed;
			} else if (trimmed != ds->name) {
				// if it's not the same name, it's not the same dive site
				// but wait, we could have gotten this one based on GPS coords and could
				// have had two different names for the same site... so let's search the other
				// way around
				struct dive_site *exact_match = state->log->sites.get_by_gps_and_name(trimmed, ds->location);
				if (exact_match) {
					unregister_dive_from_dive_site(dive);
					exact_match->add_dive(dive);
				} else {
					struct dive_site *newds = state->log->sites.create(trimmed.c_str());
					unregister_dive_from_dive_site(dive);
					newds->add_dive(dive);
					if (has_location(&state->cur_location)) {
						// we started this uuid with GPS data, so lets use those
						newds->location = state->cur_location;
					} else {
						newds->location = ds->location;
					}
					newds->notes += '\n';
					newds->notes += format_string_std(translate("gettextFromC", "additional name for site: %s\n"), ds->name.c_str());
				}
			} else if (dive->dive_site != ds) {
				// add the existing dive site to the current dive
				unregister_dive_from_dive_site(dive);
				ds->add_dive(dive);
			}
		} else {
			state->log->sites.create(trimmed)->add_dive(dive);
		}
	}
}
