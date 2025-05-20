// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

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
#include <memory>

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
#include "range.h"
#include "gettext.h"
#include "tag.h"
#include "tanksensormapping.h"
#include "subsurface-time.h"

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

static void cond_put_format(int cond, struct membuffer *b, const char *fmt, ...)
{
	if (cond) {
		VA_BUF(b, fmt);
	}
}

#define SAVE(str, x) cond_put_format(dive.x, b, str " %d\n", dive.x)

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

static void save_overview(struct membuffer *b, const struct dive &dive)
{
	show_utf8(b, "divemaster ", dive.diveguide.c_str(), "\n");
	show_utf8(b, "buddy ", dive.buddy.c_str(), "\n");
	show_utf8(b, "suit ", dive.suit.c_str(), "\n");
	show_utf8(b, "notes ", dive.notes.c_str(), "\n");
}

static void save_tags(struct membuffer *b, const tag_list &tags)
{
	const char *sep = " ";

	if (tags.empty())
		return;
	put_string(b, "tags");
	for (const divetag *tag: tags) {
		show_utf8(b, sep, tag->source.empty() ? tag->name.c_str() : tag->source.c_str(), "");
		sep = ", ";
	}
	put_string(b, "\n");
}

static void save_extra_data(struct membuffer *b, const struct divecomputer &dc)
{
	for (const auto &ed: dc.extra_data) {
		if (!ed.key.empty() && !ed.value.empty())
			put_format(b, "keyvalue \"%s\" \"%s\"\n", ed.key.c_str(), ed.value.c_str());
	}
}

static void save_tank_sensor_mappings(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc)
{
	const auto mappings = get_tank_sensor_mappings_for_storage(dive, dc);
	for (const auto &mapping: mappings)
		put_format(b, "tanksensormapping \"%d\" \"%d\"\n", mapping.sensor_id, mapping.cylinder_index);
}

static void put_gasmix(struct membuffer *b, struct gasmix mix)
{
	int o2 = mix.o2.permille;
	int he = mix.he.permille;

	if (o2) {
		put_format(b, " o2=%u.%u%%", FRACTION_TUPLE(o2, 10));
		if (he)
			put_format(b, " he=%u.%u%%", FRACTION_TUPLE(he, 10));
	}
}

static void save_cylinder_info(struct membuffer *b, const struct dive &dive)
{
	for (auto &cyl: dive.cylinders) {
		int volume = cyl.type.size.mliter;
		int use = cyl.cylinder_use;

		put_string(b, "cylinder");
		if (volume)
			put_milli(b, " vol=", volume, "l");
		put_pressure(b, cyl.type.workingpressure, " workpressure=", "bar");
		show_utf8(b, " description=", cyl.type.description.c_str(), "");
		strip_mb(b);
		put_gasmix(b, cyl.gasmix);
		put_pressure(b, cyl.start, " start=", "bar");
		put_pressure(b, cyl.end, " end=", "bar");
		if (use > OC_GAS && use < NUM_GAS_USE)
			show_utf8(b, " use=", cylinderuse_text[use], "");
		if (cyl.depth.mm != 0)
			put_milli(b, " depth=", cyl.depth.mm, "m");
		put_string(b, "\n");
	}
}

static void save_weightsystem_info(struct membuffer *b, const struct dive &dive)
{
	for (auto &ws: dive.weightsystems) {
		int grams = ws.weight.grams;

		put_string(b, "weightsystem");
		put_milli(b, " weight=", grams, "kg");
		show_utf8(b, " description=", ws.description.c_str(), "");
		put_string(b, "\n");
	}
}

static void save_dive_temperature(struct membuffer *b, const struct dive &dive)
{
	if (dive.airtemp.mkelvin != dive.dc_airtemp().mkelvin)
		put_temperature(b, dive.airtemp, "airtemp ", "°C\n");
	if (dive.watertemp.mkelvin != dive.dc_watertemp().mkelvin)
		put_temperature(b, dive.watertemp, "watertemp ", "°C\n");
}

static void save_depths(struct membuffer *b, const struct divecomputer &dc)
{
	put_depth(b, dc.maxdepth, "maxdepth ", "m\n");
	put_depth(b, dc.meandepth, "meandepth ", "m\n");
}

static void save_temperatures(struct membuffer *b, const struct divecomputer &dc)
{
	put_temperature(b, dc.airtemp, "airtemp ", "°C\n");
	put_temperature(b, dc.watertemp, "watertemp ", "°C\n");
}

static void save_airpressure(struct membuffer *b, const struct divecomputer &dc)
{
	put_pressure(b, dc.surface_pressure, "surfacepressure ", "bar\n");
}

static void save_salinity(struct membuffer *b, const struct divecomputer &dc)
{
	if (!dc.salinity)
		return;
	put_salinity(b, dc.salinity, "salinity ", "g/l\n");
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
static void save_sample(struct membuffer *b, const struct sample &sample, struct sample &old)
{
	int idx;

	put_format(b, "%3u:%02u", FRACTION_TUPLE(sample.time.seconds, 60));
	put_milli(b, " ", sample.depth.mm, "m");
	put_temperature(b, sample.temperature, " ", "°C");

	for (idx = 0; idx < MAX_SENSORS; idx++) {
		pressure_t p = sample.pressure[idx];
		int sensor = sample.sensor[idx];

		if (sensor == NO_SENSOR || !p.mbar || (old.sensor[idx] == sensor && old.pressure[idx].mbar == p.mbar))
			continue;

		put_pressure(b, p, " ", "bar");
		put_format(b, ":%d", sensor);
		old.sensor[idx] = sensor;
		old.pressure[idx] = p;
	}

	/* the deco/ndl values are stored whenever they change */
	if (sample.ndl.seconds != old.ndl.seconds) {
		put_format(b, " ndl=%u:%02u", FRACTION_TUPLE(sample.ndl.seconds, 60));
		old.ndl = sample.ndl;
	}
	if (sample.tts.seconds != old.tts.seconds) {
		put_format(b, " tts=%u:%02u", FRACTION_TUPLE(sample.tts.seconds, 60));
		old.tts = sample.tts;
	}
	if (sample.in_deco != old.in_deco) {
		put_format(b, " in_deco=%d", sample.in_deco ? 1 : 0);
		old.in_deco = sample.in_deco;
	}
	if (sample.stoptime.seconds != old.stoptime.seconds) {
		put_format(b, " stoptime=%u:%02u", FRACTION_TUPLE(sample.stoptime.seconds, 60));
		old.stoptime = sample.stoptime;
	}

	if (sample.stopdepth.mm != old.stopdepth.mm) {
		put_milli(b, " stopdepth=", sample.stopdepth.mm, "m");
		old.stopdepth = sample.stopdepth;
	}

	if (sample.cns != old.cns) {
		put_format(b, " cns=%u%%", sample.cns);
		old.cns = sample.cns;
	}

	if (sample.rbt.seconds != old.rbt.seconds) {
		put_format(b, " rbt=%u:%02u", FRACTION_TUPLE(sample.rbt.seconds, 60));
		old.rbt.seconds = sample.rbt.seconds;
	}

	if (sample.o2sensor[0].mbar != old.o2sensor[0].mbar) {
		put_milli(b, " sensor1=", sample.o2sensor[0].mbar, "bar");
		old.o2sensor[0] = sample.o2sensor[0];
	}

	if ((sample.o2sensor[1].mbar) && (sample.o2sensor[1].mbar != old.o2sensor[1].mbar)) {
		put_milli(b, " sensor2=", sample.o2sensor[1].mbar, "bar");
		old.o2sensor[1] = sample.o2sensor[1];
	}

	if ((sample.o2sensor[2].mbar) && (sample.o2sensor[2].mbar != old.o2sensor[2].mbar)) {
		put_milli(b, " sensor3=", sample.o2sensor[2].mbar, "bar");
		old.o2sensor[2] = sample.o2sensor[2];
	}

	if ((sample.o2sensor[3].mbar) && (sample.o2sensor[3].mbar != old.o2sensor[3].mbar)) {
		put_milli(b, " sensor4=", sample.o2sensor[3].mbar, "bar");
		old.o2sensor[3] = sample.o2sensor[3];
	}

	if ((sample.o2sensor[4].mbar) && (sample.o2sensor[4].mbar != old.o2sensor[4].mbar)) {
		put_milli(b, " sensor5=", sample.o2sensor[4].mbar, "bar");
		old.o2sensor[4] = sample.o2sensor[4];
	}

	if ((sample.o2sensor[5].mbar) && (sample.o2sensor[5].mbar != old.o2sensor[5].mbar)) {
		put_milli(b, " sensor6=", sample.o2sensor[5].mbar, "bar");
		old.o2sensor[5] = sample.o2sensor[5];
	}

	if ((sample.o2sensor[6].mbar) && (sample.o2sensor[6].mbar != old.o2sensor[6].mbar)) {
		put_milli(b, " dc_supplied_ppo2=", sample.o2sensor[6].mbar, "bar");
		old.o2sensor[6] = sample.o2sensor[6];
	}

	if (sample.setpoint.mbar != old.setpoint.mbar) {
		put_milli(b, " po2=", sample.setpoint.mbar, "bar");
		old.setpoint = sample.setpoint;
	}
	if (sample.heartbeat != old.heartbeat) {
		show_index(b, sample.heartbeat, "heartbeat=", "");
		old.heartbeat = sample.heartbeat;
	}
	if (sample.bearing.degrees != old.bearing.degrees) {
		show_index(b, sample.bearing.degrees, "bearing=", "°");
		old.bearing.degrees = sample.bearing.degrees;
	}
	put_format(b, "\n");
}

static void save_samples(struct membuffer *b, const struct divecomputer &dc)
{
	struct sample dummy;
	for (const auto &s: dc.samples)
		save_sample(b, s, dummy);
}

static void save_one_event(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc, const struct event &ev)
{
	put_format(b, "event %d:%02d", FRACTION_TUPLE(ev.time.seconds, 60));
	show_index(b, ev.type, "type=", "");
	show_index(b, ev.flags, "flags=", "");

	if (ev.name == "modechange")
		show_utf8(b, " divemode=", divemode_text[ev.value], "");
	else
		show_index(b, ev.value, "value=", "");
	show_utf8(b, " name=", ev.name.c_str(), "");
	if (ev.is_gaschange()) {
		struct gasmix mix = dive.get_gasmix_from_event(ev, dc).first;
		if (ev.gas.index >= 0)
			show_integer(b, ev.gas.index, "cylinder=", "");
		put_gasmix(b, mix);
	}
	put_string(b, "\n");
}

static void save_events(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc)
{
	for (auto &ev: dc.events)
		save_one_event(b, dive, dc, ev);
}

static void save_dc(struct membuffer *b, const struct dive &dive, const struct divecomputer &dc)
{
	show_utf8(b, "model ", dc.model.c_str(), "\n");
	if (dc.last_manual_time.seconds)
		put_duration(b, dc.last_manual_time, "lastmanualtime ", "min\n");
	if (dc.deviceid)
		put_format(b, "deviceid %08x\n", dc.deviceid);
	if (dc.diveid)
		put_format(b, "diveid %08x\n", dc.diveid);
	if (dc.when && dc.when != dive.when)
		show_date(b, dc.when);
	if (dc.duration.seconds && dc.duration.seconds != dive.dcs[0].duration.seconds)
		put_duration(b, dc.duration, "duration ", "min\n");
	if (dc.divemode != OC) {
		put_format(b, "dctype %s\n", divemode_text[dc.divemode]);
		put_format(b, "numberofoxygensensors %d\n", dc.no_o2sensors);
	}

	save_depths(b, dc);
	save_temperatures(b, dc);
	save_airpressure(b, dc);
	save_salinity(b, dc);
	put_duration(b, dc.surfacetime, "surfacetime ", "min\n");

	save_extra_data(b, dc);
	save_tank_sensor_mappings(b, dive, dc);
	save_events(b, dive, dc);
	save_samples(b, dc);
}

/*
 * Note that we don't save the date and time or dive
 * number: they are encoded in the filename.
 */
static void create_dive_buffer(const struct dive &dive, struct membuffer *b)
{
	pressure_t surface_pressure = dive.un_fixup_surface_pressure();
	if (dive.dcs[0].duration.seconds > 0)
		put_format(b, "duration %u:%02u min\n", FRACTION_TUPLE(dive.dcs[0].duration.seconds, 60));
	SAVE("rating", rating);
	SAVE("visibility", visibility);
	SAVE("wavesize", wavesize);
	SAVE("current", current);
	SAVE("surge", surge);
	SAVE("chill", chill);
	if (dive.user_salinity)
		put_format(b, "watersalinity %d g/l\n", (int)(dive.user_salinity/10));
	if (surface_pressure.mbar)
		SAVE("airpressure", surface_pressure.mbar);
	cond_put_format(dive.notrip, b, "notrip\n");
	cond_put_format(dive.invalid, b, "invalid\n");
	save_tags(b, dive.tags);
	if (dive.dive_site)
		put_format(b, "divesiteid %08x\n", dive.dive_site->uuid);
	if (verbose && dive.dive_site)
		report_info("removed reference to non-existant dive site with uuid %08x\n", dive.dive_site->uuid);
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
	std::vector<std::unique_ptr<dir>> subdirs;
	bool unique;
	std::string name;

	dir() : files(nullptr), unique(false)
	{
	}

	~dir()
	{
		if (files)
			git_treebuilder_free(files);
	}
};

static int tree_insert(git_treebuilder *dir, const char *name, int mkunique, git_oid *id, git_filemode_t mode)
{
	int ret;
	membuffer uniquename;

	if (mkunique && git_treebuilder_get(dir, name)) {
		char hex[8];
		git_oid_nfmt(hex, 7, id);
		hex[7] = 0;
		put_format(&uniquename, "%s~%s", name, hex);
		name = mb_cstring(&uniquename);
	}
	ret = git_treebuilder_insert(NULL, dir, name, id, mode);
	if (ret) {
		const git_error *gerr = giterr_last();
		if (gerr) {
			report_info("tree_insert failed with return %d error %s\n", ret, gerr->message);
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
	auto subdir = std::make_unique<dir>();
	subdir->name = mb_cstring(namebuf);

	/*
	 * It starts out empty: no subdirectories of its own,
	 * and an empty treebuilder list of files.
	 */
	git_treebuilder_new(&subdir->files, repo, NULL);

	/* Add it to the list of subdirs of the parent
	 * Note: append at front. Do we want that?
	 */
	parent->subdirs.insert(parent->subdirs.begin(), std::move(subdir));

	return parent->subdirs.front().get();
}

static struct dir *mktree(git_repository *repo, struct dir *dir, const char *fmt, ...)
{
	membuffer buf;

	VA_BUF(&buf, fmt);
	for (auto &subdir: dir->subdirs) {
		if (subdir->unique)
			continue;
		if (strncmp(subdir->name.c_str(), buf.buffer, buf.len))
			continue;
		if (!subdir->name[buf.len])
			return subdir.get();
	}
	return new_directory(repo, dir, &buf);
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
static void create_dive_name(struct dive &dive, struct membuffer *name, struct tm *dirtm)
{
	struct tm tm;
	static const char weekday[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	utc_mkdate(dive.when, &tm);
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
	membuffer name;

	ret = git_blob_create_frombuffer(&blob_id, repo, b->buffer, b->len);
	if (ret)
		return ret;

	VA_BUF(&name, fmt);
	ret = tree_insert(tree->files, mb_cstring(&name), 1, &blob_id, GIT_FILEMODE_BLOB);
	return ret;
}

static int save_one_divecomputer(git_repository *repo, struct dir *tree, const struct dive &dive, const struct divecomputer &dc, int idx)
{
	int ret;
	membuffer buf;

	save_dc(&buf, dive, dc);
	ret = blob_insert(repo, tree, &buf, "Divecomputer%c%03u", idx ? '-' : 0, idx);
	if (ret)
		report_error("divecomputer tree insert failed");
	return ret;
}

static int save_one_picture(git_repository *repo, struct dir *dir, const struct picture &pic)
{
	int offset = pic.offset.seconds;
	membuffer buf;
	char sign = '+';
	unsigned h;

	show_utf8(&buf, "filename ", pic.filename.c_str(), "\n");
	put_location(&buf, &pic.location, "gps ", "\n");

	/* Picture loading will load even negative offsets.. */
	if (offset < 0) {
		offset = -offset;
		sign = '-';
	}

	/* Use full hh:mm:ss format to make it all sort nicely */
	h = offset / 3600;
	offset -= h *3600;
	return blob_insert(repo, dir, &buf, "%c%02u=%02u=%02u",
		sign, h, FRACTION_TUPLE(offset, 60));
}

static int save_pictures(git_repository *repo, struct dir *dir, const struct dive &dive)
{
	if (!dive.pictures.empty()) {
		dir = mktree(repo, dir, "Pictures");
		for (auto &picture: dive.pictures)
			save_one_picture(repo, dir, picture);
	}
	return 0;
}

static int save_one_dive(git_repository *repo, struct dir *tree, struct dive &dive, struct tm *tm, bool cached_ok)
{
	membuffer buf, name;
	struct dir *subdir;
	int ret, nr;

	/* Create dive directory */
	create_dive_name(dive, &name, tm);

	/*
	 * If the dive git ID is valid, we just create the whole directory
	 * with that ID
	 */
	if (cached_ok && dive.cache_is_valid()) {
		git_oid oid;
		git_oid_fromraw(&oid, dive.git_id.data());
		ret = tree_insert(tree->files, mb_cstring(&name), 1,
			&oid, GIT_FILEMODE_TREE);
		if (ret)
			return report_error("cached dive tree insert failed");
		return 0;
	}

	subdir = new_directory(repo, tree, &name);
	subdir->unique = true;

	create_dive_buffer(dive, &buf);
	nr = dive.number;
	ret = blob_insert(repo, subdir, &buf,
		"Dive%c%d", nr ? '-' : 0, nr);
	if (ret)
		return report_error("dive save-file tree insert failed");

	/*
	 * Save the dive computer data. If there is only one dive
	 * computer, use index 0 for that (which disables the index
	 * generation when naming it).
	 */
	nr = dive.dcs.size() > 1 ? 1 : 0;
	for (auto &dc: dive.dcs)
		save_one_divecomputer(repo, subdir, dive, dc, nr++);

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
static void create_trip_name(dive_trip *trip, struct membuffer *name, struct tm *tm)
{
	put_format(name, "%02u-", tm->tm_mday);
	if (!trip->location.empty()) {
		char ascii_loc[MAXTRIPNAME+1];
		const char *p = trip->location.c_str();
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

static int save_trip_description(git_repository *repo, struct dir *dir, dive_trip *trip, struct tm *tm)
{
	int ret;
	git_oid blob_id;
	membuffer desc;

	put_format(&desc, "date %04u-%02u-%02u\n",
		   tm->tm_year, tm->tm_mon + 1, tm->tm_mday);
	put_format(&desc, "time %02u:%02u:%02u\n",
		   tm->tm_hour, tm->tm_min, tm->tm_sec);

	show_utf8(&desc, "location ", trip->location.c_str(), "\n");
	show_utf8(&desc, "notes ", trip->notes.c_str(), "\n");

	ret = git_blob_create_frombuffer(&blob_id, repo, desc.buffer, desc.len);
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

static int save_one_trip(git_repository *repo, struct dir *tree, dive_trip *trip, struct tm *tm, bool cached_ok)
{
	struct dir *subdir;
	membuffer name;
	timestamp_t first, last;

	/* Create trip directory */
	create_trip_name(trip, &name, tm);
	subdir = new_directory(repo, tree, &name);
	subdir->unique = true;

	/* Trip description file */
	save_trip_description(repo, subdir, trip, tm);

	/* Make sure we write out the dates to the dives consistently */
	first = MAX_TIMESTAMP;
	last = MIN_TIMESTAMP;
	for (auto &dive: divelog.dives) {
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
	for (auto &dive: divelog.dives) {
		if (dive->divetrip == trip)
			save_one_dive(repo, subdir, *dive, tm, cached_ok);
	}

	return 0;
}

static void save_units(void *_b)
{
	struct membuffer *b = (membuffer *)_b;
	if (prefs.unit_system == METRIC)
		put_string(b, "units METRIC\n");
	else if (prefs.unit_system == IMPERIAL)
		put_string(b, "units IMPERIAL\n");
	else
		put_format(b, "units PERSONALIZE %s %s %s %s %s %s\n",
			   prefs.units.length == units::METERS ? "METERS" : "FEET",
			   prefs.units.volume == units::LITER ? "LITER" : "CUFT",
			   prefs.units.pressure == units::BAR ? "BAR" : "PSI",
			   prefs.units.temperature == units::CELSIUS ? "CELSIUS" : prefs.units.temperature == units::FAHRENHEIT ? "FAHRENHEIT" : "KELVIN",
			   prefs.units.weight == units::KG ? "KG" : "LBS",
			   prefs.units.vertical_speed_time == units::SECONDS ? "SECONDS" : "MINUTES");
}

static void save_one_device(struct membuffer *b, const struct device &d)
{
	if (d.nickName.empty() || d.serialNumber.empty())
		return;

	show_utf8(b, "divecomputerid ", d.model.c_str(), "");
	put_format(b, " deviceid=%08x", calculate_string_hash(d.serialNumber.c_str()));
	show_utf8(b, " serial=", d.serialNumber.c_str(), "");
	show_utf8(b, " nickname=", d.nickName.c_str(), "");
	put_string(b, "\n");
}

static void save_one_fingerprint(struct membuffer *b, const fingerprint_record &fp)
{
	put_format(b, "fingerprint model=%08x serial=%08x deviceid=%08x diveid=%08x data=\"%s\"\n",
		   fp.model, fp.serial, fp.fdeviceid, fp.fdiveid, fp.get_data().c_str());
}

static void save_settings(git_repository *repo, struct dir *tree)
{
	membuffer b;

	put_format(&b, "version %d\n", dataformat_version);
	for (auto &dev: divelog.devices)
		save_one_device(&b, dev);
	/* save the fingerprint data */
	for (auto &fp: fingerprints)
		save_one_fingerprint(&b, fp);

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
	membuffer dirname;
	put_format(&dirname, "01-Divesites");
	subdir = new_directory(repo, tree, &dirname);

	divelog.sites.purge_empty();
	for (const auto &ds: divelog.sites) {
		membuffer b;
		membuffer site_file_name;
		put_format(&site_file_name, "Site-%08x", ds->uuid);
		show_utf8(&b, "name ", ds->name.c_str(), "\n");
		show_utf8(&b, "description ", ds->description.c_str(), "\n");
		show_utf8(&b, "notes ", ds->notes.c_str(), "\n");
		put_location(&b, &ds->location, "gps ", "\n");
		for (const auto &t: ds->taxonomy) {
			if (t.category != TC_NONE && !t.value.empty()) {
				put_format(&b, "geo cat %d origin %d ", t.category, t.origin);
				show_utf8(&b, "", t.value.c_str(), "\n" );
			}
		}
		blob_insert(repo, subdir, &b, mb_cstring(&site_file_name));
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
static void format_one_filter_constraint(const filter_constraint &constraint, struct membuffer *b)
{
	const char *type = filter_constraint_type_to_string(constraint.type);

	show_utf8(b, "constraint type=", type, "");
	if (filter_constraint_has_string_mode(constraint.type)) {
		const char *mode = filter_constraint_string_mode_to_string(constraint.string_mode);
		show_utf8(b, " stringmode=", mode, "");
	}
	if (filter_constraint_has_range_mode(constraint.type)) {
		const char *mode = filter_constraint_range_mode_to_string(constraint.range_mode);
		show_utf8(b, " rangemode=", mode, "");
	}
	if (constraint.negate)
		put_format(b, " negate");
	std::string data = filter_constraint_data_to_string(constraint);
	show_utf8(b, " data=", data.c_str(), "\n");
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
static void format_one_filter_preset(const filter_preset &preset, struct membuffer *b)
{
	show_utf8(b, "name ", preset.name.c_str(), "\n");

	std::string fulltext = preset.fulltext_query();
	if (!fulltext.empty()) {
		show_utf8(b, "fulltext mode=", preset.fulltext_mode(), "");
		show_utf8(b, " query=", fulltext.c_str(), "\n");
	}

	for (auto &constraint: preset.data.constraints)
		format_one_filter_constraint(constraint, b);
}

static void save_filter_presets(git_repository *repo, struct dir *tree)
{
	membuffer dirname;
	struct dir *filter_dir;
	put_format(&dirname, "02-Filterpresets");
	filter_dir = new_directory(repo, tree, &dirname);

	for (auto [i, filter_preset]: enumerated_range(divelog.filter_presets))
	{
		membuffer preset_name;
		membuffer preset_buffer;

		put_format(&preset_name, "Preset-%03d", i);
		format_one_filter_preset(filter_preset, &preset_buffer);

		blob_insert(repo, filter_dir, &preset_buffer, mb_cstring(&preset_name));
	}
}

static int create_git_tree(git_repository *repo, struct dir *root, bool select_only, bool cached_ok)
{
	git_storage_update_progress(translate("gettextFromC", "Start saving data"));
	save_settings(repo, root);

	save_divesites(repo, root);
	save_filter_presets(repo, root);

	for (auto &trip: divelog.trips)
		trip->saved = false;

	/* save the dives */
	git_storage_update_progress(translate("gettextFromC", "Start saving dives"));
	for (auto &dive: divelog.dives) {
		struct tm tm;
		struct dir *tree;

		dive_trip *trip = dive->divetrip;

		if (select_only) {
			if (!dive->selected)
				continue;
			/* We don't save trips when doing selected dive saves */
			trip = NULL;
		}

		/* Create the date-based hierarchy */
		utc_mkdate(trip ? trip->date() : dive->when, &tm);
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

		save_one_dive(repo, tree, *dive, &tm, cached_ok);
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

static int notify_cb(git_checkout_notify_t,
	const char *path,
	const git_diff_file *,
	const git_diff_file *,
	const git_diff_file *,
	void *)
{
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
	int nr = static_cast<int>(divelog.dives.size());
	std::string changes_made = get_changes_made();

	if (create_empty) {
		put_string(msg, "Initial commit to create empty repo.\n\n");
	} else if (!changes_made.empty()) {
		put_format(msg, "Changes made: \n\n%s\n", changes_made.c_str());
	} else if (!divelog.dives.empty()) {
		const struct dive &dive = *divelog.dives.back();
		dive_trip *trip = dive.divetrip;
		std::string location = dive.get_location();
		if (location.empty())
			location = "no location";
		const char *sep = "\n";

		if (dive.number)
			nr = dive.number;

		put_format(msg, "dive %d: %s", nr, location.c_str());
		if (trip && !trip->location.empty() && location != trip->location)
			put_format(msg, " (%s)", trip->location.c_str());
		put_format(msg, "\n");
		for (auto &dc: dive.dcs)  {
			if (!dc.model.empty()) {
				put_format(msg, "%s%s", sep, dc.model.c_str());
				sep = ", ";
			}
		}
		put_format(msg, "\n");
	}
	put_format(msg, "Created by %s\n", subsurface_user_agent().c_str());
	if (verbose)
		report_info("Commit message:\n\n%s\n", mb_cstring(msg));
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

	ret = git_branch_lookup(&ref, info->repo, info->branch.c_str(), GIT_BRANCH_LOCAL);
	switch (ret) {
	default:
		return report_error("Bad branch '%s' (%s)", info->branch.c_str(), strerror(errno));
	case GIT_EINVALIDSPEC:
		return report_error("Invalid branch name '%s'", info->branch.c_str());
	case GIT_ENOTFOUND: /* We'll happily create it */
		ref = NULL;
		parent = try_to_find_parent(saved_git_id.c_str(), info->repo);
		break;
	case 0:
		if (git_reference_peel(&parent, ref, GIT_OBJ_COMMIT))
			return report_error("Unable to look up parent in branch '%s'", info->branch.c_str());

		if (!saved_git_id.empty()) {
			if (!existing_filename.empty() && verbose)
				report_info("existing filename %s\n", existing_filename.c_str());
			const git_oid *id = git_commit_id((const git_commit *) parent);
			/* if we are saving to the same git tree we got this from, let's make
			 * sure there is no confusion */
			if (existing_filename == info->url && git_oid_strcmp(id, saved_git_id.c_str()))
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
		membuffer commit_msg;

		create_commit_message(&commit_msg, create_empty);
		if (git_commit_create_v(&commit_id, info->repo, NULL, author, author, NULL, mb_cstring(&commit_msg), tree, parent != NULL, parent)) {
			git_signature_free(author);
			return report_error("Git commit create failed (%s)", strerror(errno));
		}

		if (git_commit_lookup(&commit, info->repo, &commit_id)) {
			git_signature_free(author);
			return report_error("Could not look up newly created commit");
		}
	}

	git_signature_free(author);

	if (!ref) {
		if (git_branch_create(&ref, info->repo, info->branch.c_str(), commit, 0))
			return report_error("Failed to create branch '%s'", info->branch.c_str());
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
				info->branch.c_str(), errstr);
		}
	}

	if (git_reference_set_target(&ref, ref, &commit_id, "Subsurface save event"))
		return report_error("Failed to update branch '%s'", info->branch.c_str());

	/*
	 * if this was the empty commit to initialize a new repo, don't remember the
	 * commit_id, otherwise we'll think that the cache is valid and fail when building
	 * the tree when we actually try to store the dive data
	 */
	if (! create_empty)
		set_git_id(&commit_id);

	return 0;
}

static int write_git_tree(git_repository *repo, const struct dir *tree, git_oid *result)
{
	int ret;

	/* Write out our subdirectories, add them to the treebuilder, and free them */
	for (auto &subdir: tree->subdirs) {
		git_oid id;

		if (!write_git_tree(repo, subdir.get(), &id))
			tree_insert(tree->files, subdir->name.c_str(), subdir->unique, &id, GIT_FILEMODE_TREE);
	};

	/* .. write out the resulting treebuilder */
	ret = git_treebuilder_write(result, tree->files);
	if (ret && verbose) {
		const git_error *gerr = giterr_last();
		if (gerr)
			report_info("tree_insert failed with return %d error %s\n", ret, gerr->message);
	}

	return ret;
}

int do_git_save(struct git_info *info, bool select_only, bool create_empty)
{
	struct dir tree;
	git_oid id;
	bool cached_ok;

	if (!info->repo)
		return report_error("Unable to open git repository '%s[%s]'", info->url.c_str(), info->branch.c_str());

	if (verbose)
		report_info("git storage: do git save\n");

	if (!create_empty) // so we are actually saving the dives
		git_storage_update_progress(translate("gettextFromC", "Preparing to save data"));

	/*
	 * Check if we can do the cached writes - we need to
	 * have the original git commit we loaded in the repo
	 */
	cached_ok = try_to_find_parent(saved_git_id.c_str(), info->repo);

	/* Start with an empty tree: no subdirectories, no files */
	if (git_treebuilder_new(&tree.files, info->repo, NULL))
		return report_error("git treebuilder failed");

	if (!create_empty)
		/* Populate our tree data structure */
		if (create_git_tree(info->repo, &tree, select_only, cached_ok))
			return -1;

	if (verbose)
		report_info("git storage, write git tree\n");

	if (write_git_tree(info->repo, &tree, &id))
		return report_error("git tree write failed");

	/* And save the tree! */
	if (create_new_commit(info, &id, create_empty))
		return report_error("creating commit failed");

	/* now sync the tree with the remote server */
	if (!info->url.empty() && !git_local_only)
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
	if (!git_repository_open(&info->repo, info->localdir.c_str()))
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
		return report_error(translate("gettextFromC", "Failed to save dives to %s[%s] (%s)"), info->url.c_str(), info->branch.c_str(), strerror(errno));

	return do_git_save(info, select_only, false);
}
