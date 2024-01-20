// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include "ssrf.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <git2.h>

#include "dive.h"
#include "divelog.h"
#include "divesite.h"
#include "filterconstraint.h"
#include "filterpreset.h"
#include "sample.h"
#include "subsurface-string.h"
#include "trip.h"
#include "device.h"
#include "errorhelper.h"
#include "event.h"
#include "extradata.h"
#include "membuffer.h"
#include "git-access.h"
#include "version.h"
#include "picture.h"
#include "qthelper.h"
#include "gettext.h"
#include "tag.h"
#include "subsurface-time.h"

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

static void cond_put_format(int cond, struct membuffer *b, const char *fmt, ...)
{
	if (cond) {
		VA_BUF(b, fmt);
	}
}

#define SAVE(str, x) cond_put_format(dive->x, b, str " %d\n", dive->x)

static void quote(struct membuffer *b, const char *text)
{
	const char *p = text;

	for (;;) {
		const char *escape;

		switch (*p++) {
		default:
			continue;
		case 0:
			escape = NULL;
			break;
		case 1 ... 8:
		case 11:
		case 12:
		case 14 ... 31:
			escape = "?";
			break;
		case '\\':
			escape = "\\\\";
			break;
		case '"':
			escape = "\\\"";
			break;
		case '\n':
			escape = "\n\t";
			if (*p == '\n')
				escape = "\n";
			break;
		}
		put_bytes(b, text, (p - text - 1));
		if (!escape)
			break;
		put_string(b, escape);
		text = p;
	}
}

static void show_utf8(struct membuffer *b, const char *prefix, const char *value, const char *postfix)
{
	if (value) {
		put_format(b, "%s\"", prefix);
		quote(b, value);
		put_format(b, "\"%s", postfix);
	}
}

static void save_overview(struct membuffer *b, struct dive *dive)
{
	show_utf8(b, "divemaster ", dive->diveguide, "\n");
	show_utf8(b, "buddy ", dive->buddy, "\n");
	show_utf8(b, "suit ", dive->suit, "\n");
	show_utf8(b, "notes ", dive->notes, "\n");
}

static void save_tags(struct membuffer *b, struct tag_entry *tags)
{
	const char *sep = " ";

	if (!tags)
		return;
	put_string(b, "tags");
	while (tags) {
		show_utf8(b, sep, tags->tag->source ? : tags->tag->name, "");
		sep = ", ";
		tags = tags->next;
	}
	put_string(b, "\n");
}

static void save_extra_data(struct membuffer *b, struct extra_data *ed)
{
	while (ed) {
		if (ed->key && ed->value)
			put_format(b, "keyvalue \"%s\" \"%s\"\n", ed->key ? : "", ed->value ? : "");
		ed = ed->next;
	}
}

static void put_gasmix(struct membuffer *b, struct gasmix mix)
{
	int o2 = mix.o2.permille;
	int he = mix.he.permille;

	if (o2) {
		put_format(b, " o2=%u.%u%%", FRACTION(o2, 10));
		if (he)
			put_format(b, " he=%u.%u%%", FRACTION(he, 10));
	}
}

static void save_cylinder_info(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_cylinders(dive);
	for (i = 0; i < nr; i++) {
		cylinder_t *cylinder = get_cylinder(dive, i);
		int volume = cylinder->type.size.mliter;
		const char *description = cylinder->type.description;
		int use = cylinder->cylinder_use;

		put_string(b, "cylinder");
		if (volume)
			put_milli(b, " vol=", volume, "l");
		put_pressure(b, cylinder->type.workingpressure, " workpressure=", "bar");
		show_utf8(b, " description=", description, "");
		strip_mb(b);
		put_gasmix(b, cylinder->gasmix);
		put_pressure(b, cylinder->start, " start=", "bar");
		put_pressure(b, cylinder->end, " end=", "bar");
		if (use > OC_GAS && use < NUM_GAS_USE)
			show_utf8(b, " use=", cylinderuse_text[use], "");
		if (cylinder->depth.mm != 0)
			put_milli(b, " depth=", cylinder->depth.mm, "m");
		put_string(b, "\n");
	}
}

static void save_weightsystem_info(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_weightsystems(dive);
	for (i = 0; i < nr; i++) {
		weightsystem_t ws = dive->weightsystems.weightsystems[i];
		int grams = ws.weight.grams;
		const char *description = ws.description;

		put_string(b, "weightsystem");
		put_milli(b, " weight=", grams, "kg");
		show_utf8(b, " description=", description, "");
		put_string(b, "\n");
	}
}

static void save_dive_temperature(struct membuffer *b, struct dive *dive)
{
	if (dive->airtemp.mkelvin != dc_airtemp(&dive->dc))
		put_temperature(b, dive->airtemp, "airtemp ", "°C\n");
	if (dive->watertemp.mkelvin != dc_watertemp(&dive->dc))
		put_temperature(b, dive->watertemp, "watertemp ", "°C\n");
}

static void save_depths(struct membuffer *b, struct divecomputer *dc)
{
	put_depth(b, dc->maxdepth, "maxdepth ", "m\n");
	put_depth(b, dc->meandepth, "meandepth ", "m\n");
}

static void save_temperatures(struct membuffer *b, struct divecomputer *dc)
{
	put_temperature(b, dc->airtemp, "airtemp ", "°C\n");
	put_temperature(b, dc->watertemp, "watertemp ", "°C\n");
}

static void save_airpressure(struct membuffer *b, struct divecomputer *dc)
{
	put_pressure(b, dc->surface_pressure, "surfacepressure ", "bar\n");
}

static void save_salinity(struct membuffer *b, struct divecomputer *dc)
{
	if (!dc->salinity)
		return;
	put_salinity(b, dc->salinity, "salinity ", "g/l\n");
}

static void show_date(struct membuffer *b, timestamp_t when)
{
	struct tm tm;

	utc_mkdate(when, &tm);

	put_format(b, "date %04u-%02u-%02u\n",
		   tm.tm_year, tm.tm_mon + 1, tm.tm_mday);
	put_format(b, "time %02u:%02u:%02u\n",
		   tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void show_integer(struct membuffer *b, int value, const char *pre, const char *post)
{
	put_format(b, " %s%d%s", pre, value, post);
}

static void show_index(struct membuffer *b, int value, const char *pre, const char *post)
{
	if (value)
		show_integer(b, value, pre, post);
}

/*
 * Samples are saved as densely as possible while still being readable,
 * since they are the bulk of the data.
 *
 * For parsing, look at the units to figure out what the numbers are.
 */
static void save_sample(struct membuffer *b, struct sample *sample, struct sample *old, int o2sensor)
{
	int idx;

	put_format(b, "%3u:%02u", FRACTION(sample->time.seconds, 60));
	put_milli(b, " ", sample->depth.mm, "m");
	put_temperature(b, sample->temperature, " ", "°C");

	for (idx = 0; idx < MAX_SENSORS; idx++) {
		pressure_t p = sample->pressure[idx];
		int sensor = sample->sensor[idx];

		if (sensor == NO_SENSOR)
			continue;

		if (!p.mbar)
			continue;

		/* Old-style "o2sensor" syntax for CCR dives? */
		if (o2sensor >= 0) {
			if (sensor == o2sensor) {
				put_pressure(b, sample->pressure[1]," o2pressure=","bar");
				continue;
			}

			put_pressure(b, p, " ", "bar");

			/*
			 * Note: regardless of which index we used for the non-O2
			 * sensor, we know there is only one non-O2 sensor in legacy
			 * mode, and "old->sensor[0]" contains that index.
			 */
			if (sensor != old->sensor[0]) {
				put_format(b, " sensor=%d", sensor);
				old->sensor[0] = sensor;
			}
			continue;
		}

		/* The new-style format is much simpler: the sensor is always encoded */
		put_pressure(b, p, " ", "bar");
		put_format(b, ":%d", sensor);
	}

	/* the deco/ndl values are stored whenever they change */
	if (sample->ndl.seconds != old->ndl.seconds) {
		put_format(b, " ndl=%u:%02u", FRACTION(sample->ndl.seconds, 60));
		old->ndl = sample->ndl;
	}
	if (sample->tts.seconds != old->tts.seconds) {
		put_format(b, " tts=%u:%02u", FRACTION(sample->tts.seconds, 60));
		old->tts = sample->tts;
	}
	if (sample->in_deco != old->in_deco) {
		put_format(b, " in_deco=%d", sample->in_deco ? 1 : 0);
		old->in_deco = sample->in_deco;
	}
	if (sample->stoptime.seconds != old->stoptime.seconds) {
		put_format(b, " stoptime=%u:%02u", FRACTION(sample->stoptime.seconds, 60));
		old->stoptime = sample->stoptime;
	}

	if (sample->stopdepth.mm != old->stopdepth.mm) {
		put_milli(b, " stopdepth=", sample->stopdepth.mm, "m");
		old->stopdepth = sample->stopdepth;
	}

	if (sample->cns != old->cns) {
		put_format(b, " cns=%u%%", sample->cns);
		old->cns = sample->cns;
	}

	if (sample->rbt.seconds != old->rbt.seconds) {
		put_format(b, " rbt=%u:%02u", FRACTION(sample->rbt.seconds, 60));
		old->rbt.seconds = sample->rbt.seconds;
	}

	if (sample->o2sensor[0].mbar != old->o2sensor[0].mbar) {
		put_milli(b, " sensor1=", sample->o2sensor[0].mbar, "bar");
		old->o2sensor[0] = sample->o2sensor[0];
	}

	if ((sample->o2sensor[1].mbar) && (sample->o2sensor[1].mbar != old->o2sensor[1].mbar)) {
		put_milli(b, " sensor2=", sample->o2sensor[1].mbar, "bar");
		old->o2sensor[1] = sample->o2sensor[1];
	}

	if ((sample->o2sensor[2].mbar) && (sample->o2sensor[2].mbar != old->o2sensor[2].mbar)) {
		put_milli(b, " sensor3=", sample->o2sensor[2].mbar, "bar");
		old->o2sensor[2] = sample->o2sensor[2];
	}

	if ((sample->o2sensor[3].mbar) && (sample->o2sensor[3].mbar != old->o2sensor[3].mbar)) {
		put_milli(b, " sensor4=", sample->o2sensor[3].mbar, "bar");
		old->o2sensor[3] = sample->o2sensor[3];
	}

	if ((sample->o2sensor[4].mbar) && (sample->o2sensor[4].mbar != old->o2sensor[4].mbar)) {
		put_milli(b, " sensor5=", sample->o2sensor[4].mbar, "bar");
		old->o2sensor[4] = sample->o2sensor[4];
	}

	if ((sample->o2sensor[5].mbar) && (sample->o2sensor[5].mbar != old->o2sensor[5].mbar)) {
		put_milli(b, " sensor6=", sample->o2sensor[5].mbar, "bar");
		old->o2sensor[5] = sample->o2sensor[5];
	}

	if (sample->setpoint.mbar != old->setpoint.mbar) {
		put_milli(b, " po2=", sample->setpoint.mbar, "bar");
		old->setpoint = sample->setpoint;
	}
	if (sample->heartbeat != old->heartbeat) {
		show_index(b, sample->heartbeat, "heartbeat=", "");
		old->heartbeat = sample->heartbeat;
	}
	if (sample->bearing.degrees != old->bearing.degrees) {
		show_index(b, sample->bearing.degrees, "bearing=", "°");
		old->bearing.degrees = sample->bearing.degrees;
	}
	put_format(b, "\n");
}

static void save_samples(struct membuffer *b, struct dive *dive, struct divecomputer *dc)
{
	int nr;
	int o2sensor;
	struct sample *s;
	struct sample dummy = { .bearing.degrees = -1, .ndl.seconds = -1 };

	/* Is this a CCR dive with the old-style "o2pressure" sensor? */
	o2sensor = legacy_format_o2pressures(dive, dc);
	if (o2sensor >= 0) {
		dummy.sensor[0] = !o2sensor;
		dummy.sensor[1] = o2sensor;
	}

	s = dc->sample;
	nr = dc->samples;
	while (--nr >= 0) {
		save_sample(b, s, &dummy, o2sensor);
		s++;
	}
}

static void save_one_event(struct membuffer *b, struct dive *dive, struct event *ev)
{
	put_format(b, "event %d:%02d", FRACTION(ev->time.seconds, 60));
	show_index(b, ev->type, "type=", "");
	show_index(b, ev->flags, "flags=", "");

	if (!strcmp(ev->name,"modechange"))
		show_utf8(b, " divemode=", divemode_text[ev->value], "");
	else
		show_index(b, ev->value, "value=", "");
	show_utf8(b, " name=", ev->name, "");
	if (event_is_gaschange(ev)) {
		struct gasmix mix = get_gasmix_from_event(dive, ev);
		if (ev->gas.index >= 0)
			show_integer(b, ev->gas.index, "cylinder=", "");
		put_gasmix(b, mix);
	}
	put_string(b, "\n");
}

static void save_events(struct membuffer *b, struct dive *dive, struct event *ev)
{
	while (ev) {
		save_one_event(b, dive, ev);
		ev = ev->next;
	}
}

static void save_dc(struct membuffer *b, struct dive *dive, struct divecomputer *dc)
{
	show_utf8(b, "model ", dc->model, "\n");
	if (dc->last_manual_time.seconds)
		put_duration(b, dc->last_manual_time, "lastmanualtime ", "min\n");
	if (dc->deviceid)
		put_format(b, "deviceid %08x\n", dc->deviceid);
	if (dc->diveid)
		put_format(b, "diveid %08x\n", dc->diveid);
	if (dc->when && dc->when != dive->when)
		show_date(b, dc->when);
	if (dc->duration.seconds && dc->duration.seconds != dive->dc.duration.seconds)
		put_duration(b, dc->duration, "duration ", "min\n");
	if (dc->divemode != OC) {
		put_format(b, "dctype %s\n", divemode_text[dc->divemode]);
	put_format(b, "numberofoxygensensors %d\n",dc->no_o2sensors);
	}

	save_depths(b, dc);
	save_temperatures(b, dc);
	save_airpressure(b, dc);
	save_salinity(b, dc);
	put_duration(b, dc->surfacetime, "surfacetime ", "min\n");

	save_extra_data(b, dc->extra_data);
	save_events(b, dive, dc->events);
	save_samples(b, dive, dc);
}

/*
 * Note that we don't save the date and time or dive
 * number: they are encoded in the filename.
 */
static void create_dive_buffer(struct dive *dive, struct membuffer *b)
{
	pressure_t surface_pressure = un_fixup_surface_pressure(dive);
	if (dive->dc.duration.seconds > 0)
		put_format(b, "duration %u:%02u min\n", FRACTION(dive->dc.duration.seconds, 60));
	SAVE("rating", rating);
	SAVE("visibility", visibility);
	SAVE("wavesize", wavesize);
	SAVE("current", current);
	SAVE("surge", surge);
	SAVE("chill", chill);
	if (dive->user_salinity)
		put_format(b, "watersalinity %d g/l\n", (int)(dive->user_salinity/10));
	if (surface_pressure.mbar)
		SAVE("airpressure", surface_pressure.mbar);
	cond_put_format(dive->notrip, b, "notrip\n");
	cond_put_format(dive->invalid, b, "invalid\n");
	save_tags(b, dive->tag_list);
	if (dive->dive_site)
		put_format(b, "divesiteid %08x\n", dive->dive_site->uuid);
	if (verbose && dive->dive_site)
		SSRF_INFO("removed reference to non-existant dive site with uuid %08x\n", dive->dive_site->uuid);
	save_overview(b, dive);
	save_cylinder_info(b, dive);
	save_weightsystem_info(b, dive);
	save_dive_temperature(b, dive);
}

/*
 * libgit2 has a "git_treebuilder" concept, but it's broken, and can not
 * be used to do a flat tree (like the git "index") nor a recursive tree.
 * Stupid.
 *
 * So we have to do that "keep track of recursive treebuilder entries"
 * ourselves. We use 'git_treebuilder' for any regular files, and our own
 * data structures for recursive trees.
 *
 * When finally writing it out, we traverse the subdirectories depth-
 * first, writing them out, and then adding the written-out trees to
 * the git_treebuilder they existed in.
 */
struct dir {
	git_treebuilder *files;
	struct dir *subdirs, *sibling;
	char unique, name[1];
};

static int tree_insert(git_treebuilder *dir, const char *name, int mkunique, git_oid *id, unsigned mode)
{
	int ret;
	struct membuffer uniquename = { 0 };

	if (mkunique && git_treebuilder_get(dir, name)) {
		char hex[8];
		git_oid_nfmt(hex, 7, id);
		hex[7] = 0;
		put_format(&uniquename, "%s~%s", name, hex);
		name = mb_cstring(&uniquename);
	}
	ret = git_treebuilder_insert(NULL, dir, name, id, mode);
	free_buffer(&uniquename);
	if (ret) {
		const git_error *gerr = giterr_last();
		if (gerr) {
			SSRF_INFO("tree_insert failed with return %d error %s\n", ret, gerr->message);
		}
	}
	return ret;
}

/*
 * This does *not* make sure the new subdirectory doesn't
 * alias some existing name. That is actually useful: you
 * can create multiple directories with the same name, and
 * set the "unique" flag, which will then append the SHA1
 * of the directory to the name when it is written.
 */
static struct dir *new_directory(git_repository *repo, struct dir *parent, struct membuffer *namebuf)
{
	struct dir *subdir;
	const char *name = mb_cstring(namebuf);
	int len = namebuf->len;

	subdir = malloc(sizeof(*subdir)+len);

	/*
	 * It starts out empty: no subdirectories of its own,
	 * and an empty treebuilder list of files.
	 */
	subdir->subdirs = NULL;
	git_treebuilder_new(&subdir->files, repo, NULL);
	memcpy(subdir->name, name, len);
	subdir->unique = 0;
	subdir->name[len] = 0;

	/* Add it to the list of subdirs of the parent */
	subdir->sibling = parent->subdirs;
	parent->subdirs = subdir;

	return subdir;
}

static struct dir *mktree(git_repository *repo, struct dir *dir, const char *fmt, ...)
{
	struct membuffer buf = { 0 };
	struct dir *subdir;

	VA_BUF(&buf, fmt);
	for (subdir = dir->subdirs; subdir; subdir = subdir->sibling) {
		if (subdir->unique)
			continue;
		if (strncmp(subdir->name, buf.buffer, buf.len))
			continue;
		if (!subdir->name[buf.len])
			break;
	}
	if (!subdir)
		subdir = new_directory(repo, dir, &buf);
	free_buffer(&buf);
	return subdir;
}

/*
 * The name of a dive is the date and the dive number (and possibly
 * the uniqueness suffix).
 *
 * Note that the time of the dive may not be the same as the
 * time of the directory structure it is created in: the dive
 * might be part of a trip that straddles a month (or even a
 * year).
 *
 * We do *not* want to use localized weekdays and cause peoples save
 * formats to depend on their locale.
 */
static void create_dive_name(struct dive *dive, struct membuffer *name, struct tm *dirtm)
{
	struct tm tm;
	static const char weekday[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	utc_mkdate(dive->when, &tm);
	if (tm.tm_year != dirtm->tm_year)
		put_format(name, "%04u-", tm.tm_year);
	if (tm.tm_mon != dirtm->tm_mon)
		put_format(name, "%02u-", tm.tm_mon+1);

	/* a colon is an illegal char in a file name on Windows - use an '=' instead */
	put_format(name, "%02u-%s-%02u=%02u=%02u",
		tm.tm_mday, weekday[tm.tm_wday],
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

/*
 * Write a membuffer to the git repo, and free it
 */
static int blob_insert(git_repository *repo, struct dir *tree, struct membuffer *b, const char *fmt, ...)
{
	int ret;
	git_oid blob_id;
	struct membuffer name = { 0 };

	ret = git_blob_create_frombuffer(&blob_id, repo, b->buffer, b->len);
	free_buffer(b);
	if (ret)
		return ret;

	VA_BUF(&name, fmt);
	ret = tree_insert(tree->files, mb_cstring(&name), 1, &blob_id, GIT_FILEMODE_BLOB);
	free_buffer(&name);
	return ret;
}

static int save_one_divecomputer(git_repository *repo, struct dir *tree, struct dive *dive, struct divecomputer *dc, int idx)
{
	int ret;
	struct membuffer buf = { 0 };

	save_dc(&buf, dive, dc);
	ret = blob_insert(repo, tree, &buf, "Divecomputer%c%03u", idx ? '-' : 0, idx);
	if (ret)
		report_error("divecomputer tree insert failed");
	return ret;
}

static int save_one_picture(git_repository *repo, struct dir *dir, struct picture *pic)
{
	int offset = pic->offset.seconds;
	struct membuffer buf = { 0 };
	char sign = '+';
	unsigned h;

	show_utf8(&buf, "filename ", pic->filename, "\n");
	put_location(&buf, &pic->location, "gps ", "\n");

	/* Picture loading will load even negative offsets.. */
	if (offset < 0) {
		offset = -offset;
		sign = '-';
	}

	/* Use full hh:mm:ss format to make it all sort nicely */
	h = offset / 3600;
	offset -= h *3600;
	return blob_insert(repo, dir, &buf, "%c%02u=%02u=%02u",
		sign, h, FRACTION(offset, 60));
}

static int save_pictures(git_repository *repo, struct dir *dir, struct dive *dive)
{
	if (dive->pictures.nr > 0) {
		dir = mktree(repo, dir, "Pictures");
		FOR_EACH_PICTURE(dive) {
			save_one_picture(repo, dir, picture);
		}
	}
	return 0;
}

static int save_one_dive(git_repository *repo, struct dir *tree, struct dive *dive, struct tm *tm, bool cached_ok)
{
	struct divecomputer *dc;
	struct membuffer buf = { 0 }, name = { 0 };
	struct dir *subdir;
	int ret, nr;

	/* Create dive directory */
	create_dive_name(dive, &name, tm);

	/*
	 * If the dive git ID is valid, we just create the whole directory
	 * with that ID
	 */
	if (cached_ok && dive_cache_is_valid(dive)) {
		git_oid oid;
		git_oid_fromraw(&oid, dive->git_id);
		ret = tree_insert(tree->files, mb_cstring(&name), 1,
			&oid, GIT_FILEMODE_TREE);
		free_buffer(&name);
		if (ret)
			return report_error("cached dive tree insert failed");
		return 0;
	}

	subdir = new_directory(repo, tree, &name);
	subdir->unique = 1;
	free_buffer(&name);

	create_dive_buffer(dive, &buf);
	nr = dive->number;
	ret = blob_insert(repo, subdir, &buf,
		"Dive%c%d", nr ? '-' : 0, nr);
	if (ret)
		return report_error("dive save-file tree insert failed");

	/*
	 * Save the dive computer data. If there is only one dive
	 * computer, use index 0 for that (which disables the index
	 * generation when naming it).
	 */
	dc = &dive->dc;
	nr = dc->next ? 1 : 0;
	do {
		save_one_divecomputer(repo, subdir, dive, dc, nr++);
		dc = dc->next;
	} while (dc);

	/* Save the picture data, if any */
	save_pictures(repo, subdir, dive);
	return 0;
}

/*
 * We'll mark the trip directories unique, so this does not
 * need to be unique per se. It could be just "trip". But
 * to make things a bit more user-friendly, we try to take
 * the trip location into account.
 *
 * But no special characters, and no numbers (numbers in the
 * name could be construed as a date).
 *
 * So we might end up with "02-Maui", and then the unique
 * flag will make us write it out as "02-Maui~54b4" or
 * similar.
 */
#define MAXTRIPNAME 15
static void create_trip_name(dive_trip_t *trip, struct membuffer *name, struct tm *tm)
{
	put_format(name, "%02u-", tm->tm_mday);
	if (trip->location) {
		char ascii_loc[MAXTRIPNAME+1], *p = trip->location;
		int i;

		for (i = 0; i < MAXTRIPNAME; ) {
			char c = *p++;
			switch (c) {
			case 0:
			case ',':
			case '.':
				break;

			case 'a' ... 'z':
			case 'A' ... 'Z':
				ascii_loc[i++] = c;
				continue;
			default:
				continue;
			}
			break;
		}
		if (i > 1) {
			put_bytes(name, ascii_loc, i);
			return;
		}
	}

	/* No useful name? */
	put_string(name, "trip");
}

static int save_trip_description(git_repository *repo, struct dir *dir, dive_trip_t *trip, struct tm *tm)
{
	int ret;
	git_oid blob_id;
	struct membuffer desc = { 0 };

	put_format(&desc, "date %04u-%02u-%02u\n",
		   tm->tm_year, tm->tm_mon + 1, tm->tm_mday);
	put_format(&desc, "time %02u:%02u:%02u\n",
		   tm->tm_hour, tm->tm_min, tm->tm_sec);

	show_utf8(&desc, "location ", trip->location, "\n");
	show_utf8(&desc, "notes ", trip->notes, "\n");

	ret = git_blob_create_frombuffer(&blob_id, repo, desc.buffer, desc.len);
	free_buffer(&desc);
	if (ret)
		return report_error("trip blob creation failed");
	ret = tree_insert(dir->files, "00-Trip", 0, &blob_id, GIT_FILEMODE_BLOB);
	if (ret)
		return report_error("trip description tree insert failed");
	return 0;
}

static void verify_shared_date(timestamp_t when, struct tm *tm)
{
	struct tm tmp_tm;

	utc_mkdate(when, &tmp_tm);
	if (tmp_tm.tm_year != tm->tm_year) {
		tm->tm_year = -1;
		tm->tm_mon = -1;
	}
	if (tmp_tm.tm_mon != tm->tm_mon)
		tm->tm_mon = -1;
}

#define MIN_TIMESTAMP (0)
#define MAX_TIMESTAMP (0x7fffffffffffffff)

static int save_one_trip(git_repository *repo, struct dir *tree, dive_trip_t *trip, struct tm *tm, bool cached_ok)
{
	int i;
	struct dive *dive;
	struct dir *subdir;
	struct membuffer name = { 0 };
	timestamp_t first, last;

	/* Create trip directory */
	create_trip_name(trip, &name, tm);
	subdir = new_directory(repo, tree, &name);
	subdir->unique = 1;
	free_buffer(&name);

	/* Trip description file */
	save_trip_description(repo, subdir, trip, tm);

	/* Make sure we write out the dates to the dives consistently */
	first = MAX_TIMESTAMP;
	last = MIN_TIMESTAMP;
	for_each_dive(i, dive) {
		if (dive->divetrip != trip)
			continue;
		if (dive->when < first)
			first = dive->when;
		if (dive->when > last)
			last = dive->when;
	}
	verify_shared_date(first, tm);
	verify_shared_date(last, tm);

	/* Save each dive in the directory */
	for_each_dive(i, dive) {
		if (dive->divetrip == trip)
			save_one_dive(repo, subdir, dive, tm, cached_ok);
	}

	return 0;
}

static void save_units(void *_b)
{
	struct membuffer *b =_b;
	if (prefs.unit_system == METRIC)
		put_string(b, "units METRIC\n");
	else if (prefs.unit_system == IMPERIAL)
		put_string(b, "units IMPERIAL\n");
	else
		put_format(b, "units PERSONALIZE %s %s %s %s %s %s\n",
			   prefs.units.length == METERS ? "METERS" : "FEET",
			   prefs.units.volume == LITER ? "LITER" : "CUFT",
			   prefs.units.pressure == BAR ? "BAR" : "PSI",
			   prefs.units.temperature == CELSIUS ? "CELSIUS" : prefs.units.temperature == FAHRENHEIT ? "FAHRENHEIT" : "KELVIN",
			   prefs.units.weight == KG ? "KG" : "LBS",
			   prefs.units.vertical_speed_time == SECONDS ? "SECONDS" : "MINUTES");
}

static void save_one_device(struct membuffer *b, const struct device *d)
{
	const char *model = device_get_model(d);
	const char *nickname = device_get_nickname(d);
	const char *serial = device_get_serial(d);

	if (empty_string(serial)) serial = NULL;
	if (empty_string(nickname)) nickname = NULL;
	if (!nickname || !serial)
		return;

	show_utf8(b, "divecomputerid ", model, "");
	put_format(b, " deviceid=%08x", calculate_string_hash(serial));
	show_utf8(b, " serial=", serial, "");
	show_utf8(b, " nickname=", nickname, "");
	put_string(b, "\n");
}

static void save_one_fingerprint(struct membuffer *b, unsigned int i)
{
	char *fp_data = fp_get_data(&fingerprint_table, i);
	put_format(b, "fingerprint model=%08x serial=%08x deviceid=%08x diveid=%08x data=\"%s\"\n",
		   fp_get_model(&fingerprint_table, i),
		   fp_get_serial(&fingerprint_table, i),
		   fp_get_deviceid(&fingerprint_table, i),
		   fp_get_diveid(&fingerprint_table, i),
		   fp_data);
	free(fp_data);
}

static void save_settings(git_repository *repo, struct dir *tree)
{
	struct membuffer b = { 0 };

	put_format(&b, "version %d\n", DATAFORMAT_VERSION);
	for (int i = 0; i < nr_devices(divelog.devices); i++)
		save_one_device(&b, get_device(divelog.devices, i));
	/* save the fingerprint data */
	for (unsigned int i = 0; i < nr_fingerprints(&fingerprint_table); i++)
		save_one_fingerprint(&b, i);

	cond_put_format(divelog.autogroup, &b, "autogroup\n");
	save_units(&b);
	if (prefs.tankbar)
		put_string(&b, "prefs TANKBAR\n");
	if (prefs.dcceiling)
		put_string(&b, "prefs DCCEILING\n");
	if (prefs.show_ccr_setpoint)
		put_string(&b, "prefs SHOW_SETPOINT\n");
	if (prefs.show_ccr_sensors)
		put_string(&b, "prefs SHOW_SENSORS\n");
	if (prefs.pp_graphs.po2)
		put_string(&b, "prefs PO2_GRAPH\n");

	blob_insert(repo, tree, &b, "00-Subsurface");
}

static void save_divesites(git_repository *repo, struct dir *tree)
{
	struct dir *subdir;
	struct membuffer dirname = { 0 };
	put_format(&dirname, "01-Divesites");
	subdir = new_directory(repo, tree, &dirname);
	free_buffer(&dirname);

	purge_empty_dive_sites(divelog.sites);
	for (int i = 0; i < divelog.sites->nr; i++) {
		struct membuffer b = { 0 };
		struct dive_site *ds = get_dive_site(i, divelog.sites);
		struct membuffer site_file_name = { 0 };
		put_format(&site_file_name, "Site-%08x", ds->uuid);
		show_utf8(&b, "name ", ds->name, "\n");
		show_utf8(&b, "description ", ds->description, "\n");
		show_utf8(&b, "notes ", ds->notes, "\n");
		put_location(&b, &ds->location, "gps ", "\n");
		for (int j = 0; j < ds->taxonomy.nr; j++) {
			struct taxonomy *t = &ds->taxonomy.category[j];
			if (t->category != TC_NONE && t->value) {
				put_format(&b, "geo cat %d origin %d ", t->category, t->origin);
				show_utf8(&b, "", t->value, "\n" );
			}
		}
		blob_insert(repo, subdir, &b, mb_cstring(&site_file_name));
		free_buffer(&site_file_name);
	}
}

/*
 * Format a filter constraint line to the membuffer b.
 * The format is
 *	constraint type "type of the constraint" [stringmode "string mode"] [rangemode "rangemode"] [negate] data "data of the constraint".
 * where brackets indicate optional blocks.
 * For possible types and how data is interpreted see core/filterconstraint.[c|h]
 * Whether stringmode or rangemode exist depends on the type of the constraint.
 * Any constraint can be negated.
 */
static void format_one_filter_constraint(int preset_id, int constraint_id, struct membuffer *b)
{
	const struct filter_constraint *constraint = filter_preset_constraint(preset_id, constraint_id);
	const char *type = filter_constraint_type_to_string(constraint->type);
	char *data;

	show_utf8(b, "constraint type=", type, "");
	if (filter_constraint_has_string_mode(constraint->type)) {
		const char *mode = filter_constraint_string_mode_to_string(constraint->string_mode);
		show_utf8(b, " stringmode=", mode, "");
	}
	if (filter_constraint_has_range_mode(constraint->type)) {
		const char *mode = filter_constraint_range_mode_to_string(constraint->range_mode);
		show_utf8(b, " rangemode=", mode, "");
	}
	if (constraint->negate)
		put_format(b, " negate");
	data = filter_constraint_data_to_string(constraint);
	show_utf8(b, " data=", data, "\n");
	free(data);
}

/*
 * Write a filter constraint to the membuffer b.
 * Each line starts with a type, which is either "name", "fulltext" or "constraint".
 * There must be one "name" entry, zero or one "fulltext" entries and an arbitrary number of "contraint" entries.
 * The "name" entry gives the name of the filter constraint.
 * The "fulltext" entry has the format
 *	fulltext mode "fulltext mode" query "the query as entered by the user"
 * The format of the "constraint" entry is described in the format_one_filter_constraint() function.
 */
static void format_one_filter_preset(int preset_id, struct membuffer *b)
{
	char *name, *fulltext;

	name = filter_preset_name(preset_id);
	show_utf8(b, "name ", name, "\n");
	free(name);

	fulltext = filter_preset_fulltext_query(preset_id);
	if (!empty_string(fulltext)) {
		show_utf8(b, "fulltext mode=", filter_preset_fulltext_mode(preset_id), "");
		show_utf8(b, " query=", fulltext, "\n");
	}
	free(fulltext);

	for (int i = 0; i < filter_preset_constraint_count(preset_id); i++)
		format_one_filter_constraint(preset_id, i, b);
}

static void save_filter_presets(git_repository *repo, struct dir *tree)
{
	struct membuffer dirname = { 0 };
	struct dir *filter_dir;
	put_format(&dirname, "02-Filterpresets");
	filter_dir = new_directory(repo, tree, &dirname);
	free_buffer(&dirname);

	for (int i = 0; i < filter_presets_count(); i++)
	{
		struct membuffer preset_name = { 0 };
		struct membuffer preset_buffer = { 0 };

		put_format(&preset_name, "Preset-%03d", i);
		format_one_filter_preset(i, &preset_buffer);

		blob_insert(repo, filter_dir, &preset_buffer, mb_cstring(&preset_name));

		free_buffer(&preset_name);
		free_buffer(&preset_buffer);
	}
}

static int create_git_tree(git_repository *repo, struct dir *root, bool select_only, bool cached_ok)
{
	int i;
	struct dive *dive;
	dive_trip_t *trip;

	git_storage_update_progress(translate("gettextFromC", "Start saving data"));
	save_settings(repo, root);

	save_divesites(repo, root);
	save_filter_presets(repo, root);

	for (i = 0; i < divelog.trips->nr; ++i)
		divelog.trips->trips[i]->saved = 0;

	/* save the dives */
	git_storage_update_progress(translate("gettextFromC", "Start saving dives"));
	for_each_dive(i, dive) {
		struct tm tm;
		struct dir *tree;


		trip = dive->divetrip;

		if (select_only) {
			if (!dive->selected)
				continue;
			/* We don't save trips when doing selected dive saves */
			trip = NULL;
		}

		/* Create the date-based hierarchy */
		utc_mkdate(trip ? trip_date(trip) : dive->when, &tm);
		tree = mktree(repo, root, "%04d", tm.tm_year);
		tree = mktree(repo, tree, "%02d", tm.tm_mon + 1);

		if (trip) {
			/* Did we already save this trip? */
			if (trip->saved)
				continue;
			trip->saved = 1;

			/* Pass that new subdirectory in for save-trip */
			save_one_trip(repo, tree, trip, &tm, cached_ok);
			continue;
		}

		save_one_dive(repo, tree, dive, &tm, cached_ok);
	}
	git_storage_update_progress(translate("gettextFromC", "Done creating local cache"));
	return 0;
}

/*
 * See if we can find the parent ID that the git data came from
 */
static git_object *try_to_find_parent(const char *hex_id, git_repository *repo)
{
	git_oid object_id;
	git_commit *commit;

	if (!hex_id)
		return NULL;
	if (git_oid_fromstr(&object_id, hex_id))
		return NULL;
	if (git_commit_lookup(&commit, repo, &object_id))
		return NULL;
	return (git_object *)commit;
}

static int notify_cb(git_checkout_notify_t why,
	const char *path,
	const git_diff_file *baseline,
	const git_diff_file *target,
	const git_diff_file *workdir,
	void *payload)
{
	UNUSED(baseline);
	UNUSED(target);
	UNUSED(workdir);
	UNUSED(payload);
	UNUSED(why);
	report_error("File '%s' does not match in working tree", path);
	return 0; /* Continue with checkout */
}

static git_tree *get_git_tree(git_repository *repo, git_object *parent)
{
	git_tree *tree;
	if (!parent)
		return NULL;
	if (git_tree_lookup(&tree, repo, git_commit_tree_id((const git_commit *) parent)))
		return NULL;
	return tree;
}

int update_git_checkout(git_repository *repo, git_object *parent, git_tree *tree)
{
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;

	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	opts.notify_flags = GIT_CHECKOUT_NOTIFY_CONFLICT | GIT_CHECKOUT_NOTIFY_DIRTY;
	opts.notify_cb = notify_cb;
	opts.baseline = get_git_tree(repo, parent);
	return git_checkout_tree(repo, (git_object *) tree, &opts);
}

int get_authorship(git_repository *repo, git_signature **authorp)
{
	if (git_signature_default(authorp, repo) == 0)
		return 0;

#ifdef SUBSURFACE_MOBILE
#define APPNAME "Subsurface-mobile"
#else
#define APPNAME "Subsurface"
#endif
	return git_signature_now(authorp, APPNAME, "subsurface-app-account@subsurface-divelog.org");
#undef APPNAME
}

static void create_commit_message(struct membuffer *msg, bool create_empty)
{
	int nr = divelog.dives->nr;
	struct dive *dive = get_dive(nr-1);
	char* changes_made = get_changes_made();

	if (create_empty) {
		put_string(msg, "Initial commit to create empty repo.\n\n");
	} else if (!empty_string(changes_made)) {
		put_format(msg, "Changes made: \n\n%s\n", changes_made);
	} else if (dive) {
		dive_trip_t *trip = dive->divetrip;
		const char *location = get_dive_location(dive) ? : "no location";
		struct divecomputer *dc = &dive->dc;
		const char *sep = "\n";

		if (dive->number)
			nr = dive->number;

		put_format(msg, "dive %d: %s", nr, location);
		if (trip && !empty_string(trip->location) && strcmp(trip->location, location))
			put_format(msg, " (%s)", trip->location);
		put_format(msg, "\n");
		do {
			if (!empty_string(dc->model)) {
				put_format(msg, "%s%s", sep, dc->model);
				sep = ", ";
			}
		} while ((dc = dc->next) != NULL);
		put_format(msg, "\n");
	}
	const char *user_agent = subsurface_user_agent();
	put_format(msg, "Created by %s\n", user_agent);
	free((void *)user_agent);
	free(changes_made);
	if (verbose)
		SSRF_INFO("Commit message:\n\n%s\n", mb_cstring(msg));
}

static int create_new_commit(struct git_info *info, git_oid *tree_id, bool create_empty)
{
	int ret;
	git_reference *ref;
	git_object *parent;
	git_oid commit_id;
	git_signature *author;
	git_commit *commit;
	git_tree *tree;

	ret = git_branch_lookup(&ref, info->repo, info->branch, GIT_BRANCH_LOCAL);
	switch (ret) {
	default:
		return report_error("Bad branch '%s' (%s)", info->branch, strerror(errno));
	case GIT_EINVALIDSPEC:
		return report_error("Invalid branch name '%s'", info->branch);
	case GIT_ENOTFOUND: /* We'll happily create it */
		ref = NULL;
		parent = try_to_find_parent(saved_git_id, info->repo);
		break;
	case 0:
		if (git_reference_peel(&parent, ref, GIT_OBJ_COMMIT))
			return report_error("Unable to look up parent in branch '%s'", info->branch);

		if (saved_git_id) {
			if (existing_filename && verbose)
				SSRF_INFO("existing filename %s\n", existing_filename);
			const git_oid *id = git_commit_id((const git_commit *) parent);
			/* if we are saving to the same git tree we got this from, let's make
			 * sure there is no confusion */
			if (same_string(existing_filename, info->url) && git_oid_strcmp(id, saved_git_id))
				return report_error("The git branch does not match the git parent of the source");
		}

		/* all good */
		break;
	}

	if (git_tree_lookup(&tree, info->repo, tree_id))
		return report_error("Could not look up newly created tree");

	if (get_authorship(info->repo, &author))
		return report_error("No user name configuration in git repo");

	/* If the parent commit has the same tree ID, do not create a new commit */
	if (parent && git_oid_equal(tree_id, git_commit_tree_id((const git_commit *) parent))) {
		/* If the parent already came from the ref, the commit is already there */
		if (ref) {
			git_signature_free(author);
			return 0;
		}
		/* Else we do want to create the new branch, but with the old commit */
		commit = (git_commit *) parent;
	} else {
		struct membuffer commit_msg = { 0 };

		create_commit_message(&commit_msg, create_empty);
		if (git_commit_create_v(&commit_id, info->repo, NULL, author, author, NULL, mb_cstring(&commit_msg), tree, parent != NULL, parent)) {
			git_signature_free(author);
			return report_error("Git commit create failed (%s)", strerror(errno));
		}
		free_buffer(&commit_msg);

		if (git_commit_lookup(&commit, info->repo, &commit_id)) {
			git_signature_free(author);
			return report_error("Could not look up newly created commit");
		}
	}

	git_signature_free(author);

	if (!ref) {
		if (git_branch_create(&ref, info->repo, info->branch, commit, 0))
			return report_error("Failed to create branch '%s'", info->branch);
	}
	/*
	 * If it's a checked-out branch, try to also update the working
	 * tree and index. If that fails (dirty working tree or whatever),
	 * this is not technically a save error (we did save things in
	 * the object database), but it can cause extreme confusion, so
	 * warn about it.
	 */
	if (git_branch_is_head(ref) && !git_repository_is_bare(info->repo)) {
		if (update_git_checkout(info->repo, parent, tree)) {
			const git_error *err = giterr_last();
			const char *errstr = err ? err->message : strerror(errno);
			report_error("Git branch '%s' is checked out, but worktree is dirty (%s)",
				info->branch, errstr);
		}
	}

	if (git_reference_set_target(&ref, ref, &commit_id, "Subsurface save event"))
		return report_error("Failed to update branch '%s'", info->branch);

	/*
	 * if this was the empty commit to initialize a new repo, don't remember the
	 * commit_id, otherwise we'll think that the cache is valid and fail when building
	 * the tree when we actually try to store the dive data
	 */
	if (! create_empty)
		set_git_id(&commit_id);

	return 0;
}

static int write_git_tree(git_repository *repo, struct dir *tree, git_oid *result)
{
	int ret;
	struct dir *subdir;

	/* Write out our subdirectories, add them to the treebuilder, and free them */
	while ((subdir = tree->subdirs) != NULL) {
		git_oid id;

		if (!write_git_tree(repo, subdir, &id))
			tree_insert(tree->files, subdir->name, subdir->unique, &id, GIT_FILEMODE_TREE);
		tree->subdirs = subdir->sibling;
		free(subdir);
	};

	/* .. write out the resulting treebuilder */
	ret = git_treebuilder_write(result, tree->files);
	if (ret && verbose) {
		const git_error *gerr = giterr_last();
		if (gerr)
			SSRF_INFO("tree_insert failed with return %d error %s\n", ret, gerr->message);
	}

	/* .. and free the now useless treebuilder */
	git_treebuilder_free(tree->files);

	return ret;
}

int do_git_save(struct git_info *info, bool select_only, bool create_empty)
{
	struct dir tree;
	git_oid id;
	bool cached_ok;

	if (!info->repo)
		return report_error("Unable to open git repository '%s[%s]'", info->url, info->branch);

	if (verbose)
		SSRF_INFO("git storage: do git save\n");

	if (!create_empty) // so we are actually saving the dives
		git_storage_update_progress(translate("gettextFromC", "Preparing to save data"));

	/*
	 * Check if we can do the cached writes - we need to
	 * have the original git commit we loaded in the repo
	 */
	cached_ok = try_to_find_parent(saved_git_id, info->repo);

	/* Start with an empty tree: no subdirectories, no files */
	tree.name[0] = 0;
	tree.subdirs = NULL;
	if (git_treebuilder_new(&tree.files, info->repo, NULL))
		return report_error("git treebuilder failed");

	if (!create_empty)
		/* Populate our tree data structure */
		if (create_git_tree(info->repo, &tree, select_only, cached_ok))
			return -1;

	if (verbose)
		SSRF_INFO("git storage, write git tree\n");

	if (write_git_tree(info->repo, &tree, &id))
		return report_error("git tree write failed");

	/* And save the tree! */
	if (create_new_commit(info, &id, create_empty))
		return report_error("creating commit failed");

	/* now sync the tree with the remote server */
	if (info->url && !git_local_only)
		return sync_with_remote(info);
	return 0;
}

int git_save_dives(struct git_info *info, bool select_only)
{
	/*
	 * First, just try to open the local git repo without
	 * doing any remote updates at all. If networking is
	 * reliable, the remote has been updated at open time,
	 * and we want to update *after* saving.
	 *
	 * And if networking isn't reliable, we want to save
	 * first anyway.
	 *
	 * Note that 'do_git_save()' will try to sync with the
	 * remote after saving (see sync_with_remote()), so we
	 * will be doing the remote access at that point, but
	 * at least the local state will be saved early in
	 * case something goes wrong.
	 */
	if (!git_repository_open(&info->repo, info->localdir))
		return do_git_save(info, select_only, false);

	/*
	 * Ok, so there was something wrong with the local
	 * repo (possibly it's just missing entirely). That
	 * means we don't want to just save to it, we'll
	 * need to try to load the remote state first.
	 *
	 * This shouldn't be the common case.
	 */
	if (!open_git_repository(info))
		return report_error(translate("gettextFromC", "Failed to save dives to %s[%s] (%s)"), info->url, info->branch, strerror(errno));

	return do_git_save(info, select_only, false);
}
