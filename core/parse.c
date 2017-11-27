#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "membuffer.h"
#include <stdlib.h>
#include <unistd.h>
#include <libdivecomputer/parser.h>

#include "dive.h"
#include "parse.h"
#include "divelist.h"
#include "device.h"
#include "gettext.h"

int metric = 1;
int diveid = -1;

/*
static union {
	struct event event;
	char allocation[sizeof(struct event)+MAX_EVENT_NAME];
} event_allocation = { .event.deleted = 1 };

static event_allocation cur_event = { .event.deleted = 1 };
static inline union {
	struct event event;
	char allocation[sizeof(struct event)+MAX_EVENT_NAME];
} event_allocation = { .event.deleted = 1 };
#define cur_event event_allocation.event
*/
event_allocation_t event_allocation = { .event.deleted = 1 };


struct divecomputer *cur_dc = NULL;
struct dive *cur_dive = NULL;
struct dive_site *cur_dive_site = NULL;
degrees_t cur_latitude, cur_longitude;
dive_trip_t *cur_trip = NULL;
struct sample *cur_sample = NULL;
struct picture *cur_picture = NULL;

bool in_settings = false;
bool in_userid = false;
struct tm cur_tm;
int cur_cylinder_index, cur_ws_index;
int lastcylinderindex, next_o2_sensor;
int o2pressure_sensor;
struct extra_data cur_extra_data;

struct dive_table dive_table;
struct dive_table *target_table = NULL;

/*
 * If we don't have an explicit dive computer,
 * we use the implicit one that every dive has..
 */
struct divecomputer *get_dc(void)
{
	return cur_dc ?: &cur_dive->dc;
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
 * Clear a dive_table
 */
void clear_table(struct dive_table *table)
{
	for (int i = 0; i < table->nr; i++)
		free(table->dives[i]);
	table->nr = 0;
}

/*
 * Add a dive into the dive_table array
 */
void record_dive_to_table(struct dive *dive, struct dive_table *table)
{
	assert(table != NULL);
	struct dive **dives = grow_dive_table(table);
	int nr = table->nr;

	dives[nr] = fixup_dive(dive);
	table->nr = nr + 1;
}

void record_dive(struct dive *dive)
{
	record_dive_to_table(dive, &dive_table);
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

typedef void (*matchfn_t)(char *buffer, void *);

int match(const char *pattern, int plen,
		 const char *name,
		 matchfn_t fn, char *buf, void *data)
{
	switch (name[plen]) {
	case '\0':
	case '.':
		break;
	default:
		return 0;
	}
	if (memcmp(pattern, name, plen))
		return 0;
	fn(buf, data);
	return 1;
}

void event_start(void)
{
	memset(&cur_event, 0, sizeof(cur_event));
	cur_event.deleted = 0;	/* Active */
}

void event_end(void)
{
	struct divecomputer *dc = get_dc();
	if (cur_event.type == 123) {
		struct picture *pic = alloc_picture();
		pic->filename = strdup(cur_event.name);
		/* theoretically this could fail - but we didn't support multi year offsets */
		pic->offset.seconds = cur_event.time.seconds;
		dive_add_picture(cur_dive, pic);
	} else {
		struct event *ev;
		/* At some point gas change events did not have any type. Thus we need to add
		 * one on import, if we encounter the type one missing.
		 */
		if (cur_event.type == 0 && strcmp(cur_event.name, "gaschange") == 0)
			cur_event.type = cur_event.value >> 16 > 0 ? SAMPLE_EVENT_GASCHANGE2 : SAMPLE_EVENT_GASCHANGE;
		ev = add_event(dc, cur_event.time.seconds,
			       cur_event.type, cur_event.flags,
			       cur_event.value, cur_event.name);

		/*
		 * Older logs might mark the dive to be CCR by having an "SP change" event at time 0:00. Better
		 * to mark them being CCR on import so no need for special treatments elsewhere on the code.
		 */
		if (ev && cur_event.time.seconds == 0 && cur_event.type == SAMPLE_EVENT_PO2 && cur_event.value && dc->divemode==OC) {
			dc->divemode = CCR;
		}

		if (ev && event_is_gaschange(ev)) {
			/* See try_to_fill_event() on why the filled-in index is one too big */
			ev->gas.index = cur_event.gas.index-1;
			if (cur_event.gas.mix.o2.permille || cur_event.gas.mix.he.permille)
				ev->gas.mix = cur_event.gas.mix;
		}
	}
	cur_event.deleted = 1;	/* No longer active */
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
bool is_dive(void)
{
	return (cur_dive &&
		(cur_dive->dive_site_uuid || cur_dive->when || cur_dive->dc.samples));
}

void reset_dc_info(struct divecomputer *dc)
{
	/* WARN: reset dc info does't touch the dc? */
	(void) dc;
	lastcylinderindex = 0;
}

void reset_dc_settings(void)
{
	free((void *)cur_settings.dc.model);
	free((void *)cur_settings.dc.nickname);
	free((void *)cur_settings.dc.serial_nr);
	free((void *)cur_settings.dc.firmware);
	cur_settings.dc.model = NULL;
	cur_settings.dc.nickname = NULL;
	cur_settings.dc.serial_nr = NULL;
	cur_settings.dc.firmware = NULL;
	cur_settings.dc.deviceid = 0;
}

void settings_start(void)
{
	in_settings = true;
}

void settings_end(void)
{
	in_settings = false;
}

void dc_settings_start(void)
{
	reset_dc_settings();
}

void dc_settings_end(void)
{
	create_device_node(cur_settings.dc.model, cur_settings.dc.deviceid, cur_settings.dc.serial_nr,
			   cur_settings.dc.firmware, cur_settings.dc.nickname);
	reset_dc_settings();
}

void dive_site_start(void)
{
	if (cur_dive_site)
		return;
	cur_dive_site = calloc(1, sizeof(struct dive_site));
}

void dive_site_end(void)
{
	if (!cur_dive_site)
		return;
	if (cur_dive_site->taxonomy.nr == 0) {
		free(cur_dive_site->taxonomy.category);
		cur_dive_site->taxonomy.category = NULL;
	}
	if (cur_dive_site->uuid) {
		struct dive_site *ds = alloc_or_get_dive_site(cur_dive_site->uuid);
		merge_dive_site(ds, cur_dive_site);

		if (verbose > 3)
			printf("completed dive site uuid %x8 name {%s}\n", ds->uuid, ds->name);
	}
	free_taxonomy(&cur_dive_site->taxonomy);
	free(cur_dive_site);
	cur_dive_site = NULL;
}

// now we need to add the code to parse the parts of the divesite enry

void dive_start(void)
{
	if (cur_dive)
		return;
	cur_dive = alloc_dive();
	reset_dc_info(&cur_dive->dc);
	memset(&cur_tm, 0, sizeof(cur_tm));
	if (cur_trip) {
		add_dive_to_trip(cur_dive, cur_trip);
		cur_dive->tripflag = IN_TRIP;
	}
	o2pressure_sensor = 1;
}

void dive_end(void)
{
	if (!cur_dive)
		return;
	if (!is_dive())
		free(cur_dive);
	else
		record_dive_to_table(cur_dive, target_table);
	cur_dive = NULL;
	cur_dc = NULL;
	cur_latitude.udeg = 0;
	cur_longitude.udeg = 0;
	cur_cylinder_index = 0;
	cur_ws_index = 0;
}

void trip_start(void)
{
	if (cur_trip)
		return;
	dive_end();
	cur_trip = calloc(1, sizeof(dive_trip_t));
	memset(&cur_tm, 0, sizeof(cur_tm));
}

void trip_end(void)
{
	if (!cur_trip)
		return;
	insert_trip(&cur_trip);
	cur_trip = NULL;
}

void picture_start(void)
{
	cur_picture = alloc_picture();
}

void picture_end(void)
{
	dive_add_picture(cur_dive, cur_picture);
	cur_picture = NULL;
}

void cylinder_start(void)
{
}

void cylinder_end(void)
{
	cur_cylinder_index++;
}

void ws_start(void)
{
}

void ws_end(void)
{
	cur_ws_index++;
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
void sample_start(void)
{
	struct divecomputer *dc = get_dc();
	struct sample *sample = prepare_sample(dc);

	if (sample != dc->sample) {
		memcpy(sample, sample-1, sizeof(struct sample));
		sample->pressure[0].mbar = 0;
		sample->pressure[1].mbar = 0;
	} else {
		sample->sensor[0] = !o2pressure_sensor;
		sample->sensor[1] = o2pressure_sensor;
	}
	cur_sample = sample;
	next_o2_sensor = 0;
}

void sample_end(void)
{
	if (!cur_dive)
		return;

	finish_sample(get_dc());
	cur_sample = NULL;
}

void divecomputer_start(void)
{
	struct divecomputer *dc;

	/* Start from the previous dive computer */
	dc = &cur_dive->dc;
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
	cur_dc = dc;
	reset_dc_info(dc);
}

void divecomputer_end(void)
{
	if (!cur_dc->when)
		cur_dc->when = cur_dive->when;
	cur_dc = NULL;
}

void userid_start(void)
{
	in_userid = true;
	//if the xml contains userid, keep saving it.
	// don't call the prefs method here as we don't wanna
	// actually change the preferences, this is temporary and
	// will be reverted when the file finishes.

	prefs.save_userid_local = true;
}

void userid_stop(void)
{
	in_userid = false;
}

void utf8_string(char *buffer, void *_res)
{
	char **res = _res;
	int size;
	size = trimspace(buffer);
	if(size)
		*res = strdup(buffer);
}

void add_dive_site(char *ds_name, struct dive *dive)
{
	static int suffix = 1;
	char *buffer = ds_name;
	char *to_free = NULL;
	int size = trimspace(buffer);
	if(size) {
		uint32_t uuid = dive->dive_site_uuid;
		struct dive_site *ds = get_dive_site_by_uuid(uuid);
		if (uuid && !ds) {
			// that's strange - we have a uuid but it doesn't exist - let's just ignore it
			fprintf(stderr, "dive contains a non-existing dive site uuid %x\n", dive->dive_site_uuid);
			uuid = 0;
		}
		if (!uuid) {
			// if the dive doesn't have a uuid, check if there's already a dive site by this name
			uuid = get_dive_site_uuid_by_name(buffer, &ds);
			if (uuid && import_source == SSRF_WS) {
				// when downloading GPS fixes from the Subsurface webservice we will often
				// get a lot of dives with identical names (the autogenerated fixes).
				// So in this case modify the name to make it unique
				int name_size = strlen(buffer) + 10; // 8 digits - enough for 100 million sites
				to_free = buffer = malloc(name_size);
				do {
					suffix++;
					snprintf(buffer, name_size, "%s %8d", ds_name, suffix);
				} while (get_dive_site_uuid_by_name(buffer, NULL) != 0);
				ds = NULL;
			}
		}
		if (ds) {
			// we have a uuid, let's hope there isn't a different name
			if (same_string(ds->name, "")) {
				ds->name = copy_string(buffer);
			} else if (!same_string(ds->name, buffer)) {
				// if it's not the same name, it's not the same dive site
				// but wait, we could have gotten this one based on GPS coords and could
				// have had two different names for the same site... so let's search the other
				// way around
				uint32_t exact_match_uuid = get_dive_site_uuid_by_gps_and_name(buffer, ds->latitude, ds->longitude);
				if (exact_match_uuid) {
					dive->dive_site_uuid = exact_match_uuid;
				} else {
					dive->dive_site_uuid = create_dive_site(buffer, dive->when);
					struct dive_site *newds = get_dive_site_by_uuid(dive->dive_site_uuid);
					if (cur_latitude.udeg || cur_longitude.udeg) {
						// we started this uuid with GPS data, so lets use those
						newds->latitude = cur_latitude;
						newds->longitude = cur_longitude;
					} else {
						newds->latitude = ds->latitude;
						newds->longitude = ds->longitude;
					}
					newds->notes = add_to_string(newds->notes, translate("gettextFromC", "additional name for site: %s\n"), ds->name);
				}
			} else {
				// add the existing dive site to the current dive
				dive->dive_site_uuid = uuid;
			}
		} else {
			dive->dive_site_uuid = create_dive_site(buffer, dive->when);
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

