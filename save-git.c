#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <git2.h>

#include "dive.h"
#include "divelist.h"
#include "device.h"
#include "membuffer.h"
#include "git-access.h"
#include "version.h"
#include "qthelperfromc.h"

/*
 * handle libgit2 revision 0.20 and earlier
 */
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR <= 20 && !defined(USE_LIBGIT21_API)
  #define GIT_CHECKOUT_OPTIONS_INIT GIT_CHECKOUT_OPTS_INIT
  #define git_checkout_options git_checkout_opts
  #define git_branch_create(out,repo,branch_name,target,force,sig,msg) \
	git_branch_create(out,repo,branch_name,target,force)
  #define git_reference_set_target(out,ref,target,signature,log_message) \
	git_reference_set_target(out,ref,target)
#endif
/*
 * api break in libgit2 revision 0.22
 */
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR < 22
  #define git_treebuilder_new(out, repo, source) git_treebuilder_create(out, source)
#else
  #define git_treebuilder_write(id, repo, bld)   git_treebuilder_write(id, bld)
#endif
/*
 * api break introduced in libgit2 master after 0.22 - let's guess this is the v0.23 API
 */
#if USE_LIBGIT23_API
  #define git_branch_create(out, repo, branch_name, target, force, signature, log_message) \
	git_branch_create(out, repo, branch_name, target, force)
  #define git_reference_set_target(out, ref, id, author, log_message) \
	git_reference_set_target(out, ref, id, log_message)
#endif

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

static void cond_put_format(int cond, struct membuffer *b, const char *fmt, ...)
{
	if (cond) {
		VA_BUF(b, fmt);
	}
}

#define SAVE(str, x) cond_put_format(dive->x, b, str " %d\n", dive->x)

static void show_gps(struct membuffer *b, degrees_t latitude, degrees_t longitude)
{
	if (latitude.udeg || longitude.udeg) {
		put_degrees(b, latitude, "gps ", " ");
		put_degrees(b, longitude, "", "\n");
	}
}

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
	show_utf8(b, "divemaster ", dive->divemaster, "\n");
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

static void put_gasmix(struct membuffer *b, struct gasmix *mix)
{
	int o2 = mix->o2.permille;
	int he = mix->he.permille;

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
		cylinder_t *cylinder = dive->cylinder + i;
		int volume = cylinder->type.size.mliter;
		const char *description = cylinder->type.description;

		put_string(b, "cylinder");
		if (volume)
			put_milli(b, " vol=", volume, "l");
		put_pressure(b, cylinder->type.workingpressure, " workpressure=", "bar");
		show_utf8(b, " description=", description, "");
		strip_mb(b);
		put_gasmix(b, &cylinder->gasmix);
		put_pressure(b, cylinder->start, " start=", "bar");
		put_pressure(b, cylinder->end, " end=", "bar");
		if (cylinder->cylinder_use != OC_GAS)
			put_format(b, " use=%s", cylinderuse_text[cylinder->cylinder_use]);

		put_string(b, "\n");
	}
}

static void save_weightsystem_info(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_weightsystems(dive);
	for (i = 0; i < nr; i++) {
		weightsystem_t *ws = dive->weightsystem + i;
		int grams = ws->weight.grams;
		const char *description = ws->description;

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
	/* only save if we have a value that isn't the default of sea water */
	if (!dc->salinity || dc->salinity == SEAWATER_SALINITY)
		return;
	put_salinity(b, dc->salinity, "salinity ", "g/l\n");
}

static void show_date(struct membuffer *b, timestamp_t when)
{
	struct tm tm;

	utc_mkdate(when, &tm);

	put_format(b, "date %04u-%02u-%02u\n",
		   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	put_format(b, "time %02u:%02u:%02u\n",
		   tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void show_index(struct membuffer *b, int value, const char *pre, const char *post)
{
	if (value)
		put_format(b, " %s%d%s", pre, value, post);
}

/*
 * Samples are saved as densely as possible while still being readable,
 * since they are the bulk of the data.
 *
 * For parsing, look at the units to figure out what the numbers are.
 */
static void save_sample(struct membuffer *b, struct sample *sample, struct sample *old)
{
	put_format(b, "%3u:%02u", FRACTION(sample->time.seconds, 60));
	put_milli(b, " ", sample->depth.mm, "m");
	put_temperature(b, sample->temperature, " ", "°C");
	put_pressure(b, sample->cylinderpressure, " ", "bar");
	put_pressure(b, sample->o2cylinderpressure," o2pressure=","bar");

	/*
	 * We only show sensor information for samples with pressure, and only if it
	 * changed from the previous sensor we showed.
	 */
	if (sample->cylinderpressure.mbar && sample->sensor != old->sensor) {
		put_format(b, " sensor=%d", sample->sensor);
		old->sensor = sample->sensor;
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

	if (sample->setpoint.mbar != old->setpoint.mbar) {
		put_milli(b, " po2=", sample->setpoint.mbar, "bar");
		old->setpoint = sample->setpoint;
	}
	show_index(b, sample->heartbeat, "heartbeat=", "");
	show_index(b, sample->bearing.degrees, "bearing=", "°");
	put_format(b, "\n");
}

static void save_samples(struct membuffer *b, int nr, struct sample *s)
{
	struct sample dummy = {};

	while (--nr >= 0) {
		save_sample(b, s, &dummy);
		s++;
	}
}

static void save_one_event(struct membuffer *b, struct event *ev)
{
	put_format(b, "event %d:%02d", FRACTION(ev->time.seconds, 60));
	show_index(b, ev->type, "type=", "");
	show_index(b, ev->flags, "flags=", "");
	show_index(b, ev->value, "value=", "");
	show_utf8(b, " name=", ev->name, "");
	if (event_is_gaschange(ev)) {
		if (ev->gas.index >= 0) {
			show_index(b, ev->gas.index, "cylinder=", "");
			put_gasmix(b, &ev->gas.mix);
		} else if (!event_gasmix_redundant(ev))
			put_gasmix(b, &ev->gas.mix);
	}
	put_string(b, "\n");
}

static void save_events(struct membuffer *b, struct event *ev)
{
	while (ev) {
		save_one_event(b, ev);
		ev = ev->next;
	}
}

static void save_dc(struct membuffer *b, struct dive *dive, struct divecomputer *dc)
{
	show_utf8(b, "model ", dc->model, "\n");
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
	save_events(b, dc->events);
	save_samples(b, dc->samples, dc->sample);
}

/*
 * Note that we don't save the date and time or dive
 * number: they are encoded in the filename.
 */
static void create_dive_buffer(struct dive *dive, struct membuffer *b)
{
	put_format(b, "duration %u:%02u min\n", FRACTION(dive->dc.duration.seconds, 60));
	SAVE("rating", rating);
	SAVE("visibility", visibility);
	cond_put_format(dive->tripflag == NO_TRIP, b, "notrip\n");
	save_tags(b, dive->tag_list);
	cond_put_format(dive->dive_site_uuid, b, "divesiteid %08x\n", dive->dive_site_uuid);

	save_overview(b, dive);
	save_cylinder_info(b, dive);
	save_weightsystem_info(b, dive);
	save_dive_temperature(b, dive);
}

static struct membuffer error_string_buffer = { 0 };

/*
 * Note that the act of "getting" the error string
 * buffer doesn't de-allocate the buffer, but it does
 * set the buffer length to zero, so that any future
 * error reports will overwrite the string rather than
 * append to it.
 */
const char *get_error_string(void)
{
	const char *str;

	if (!error_string_buffer.len)
		return "";
	str = mb_cstring(&error_string_buffer);
	error_string_buffer.len = 0;
	return str;
}

int report_error(const char *fmt, ...)
{
	struct membuffer *buf = &error_string_buffer;

	/* Previous unprinted errors? Add a newline in between */
	if (buf->len)
		put_bytes(buf, "\n", 1);
	VA_BUF(buf, fmt);
	mb_cstring(buf);
	return -1;
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
		put_format(name, "%04u-", tm.tm_year + 1900);
	if (tm.tm_mon != dirtm->tm_mon)
		put_format(name, "%02u-", tm.tm_mon+1);

	/* a colon is an illegal char in a file name on Windows - use an '=' instead */
	put_format(name, "%02u-%s-%02u=%02u=%02u",
		tm.tm_mday, weekday[tm.tm_wday],
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

/* Write file at filepath to the git repo with given filename */
static int blob_insert_fromdisk(git_repository *repo, struct dir *tree, const char *filepath, const char *filename)
{
	int ret;
	git_oid blob_id;

	ret = git_blob_create_fromdisk(&blob_id, repo, filepath);
	if (ret)
		return ret;
	return tree_insert(tree->files, filename, 1, &blob_id, GIT_FILEMODE_BLOB);
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
	int error;

	show_utf8(&buf, "filename ", pic->filename, "\n");
	show_gps(&buf, pic->latitude, pic->longitude);
	show_utf8(&buf, "hash ", pic->hash, "\n");

	/* Picture loading will load even negative offsets.. */
	if (offset < 0) {
		offset = -offset;
		sign = '-';
	}

	/* Use full hh:mm:ss format to make it all sort nicely */
	h = offset / 3600;
	offset -= h *3600;
	error = blob_insert(repo, dir, &buf, "%c%02u=%02u=%02u",
		sign, h, FRACTION(offset, 60));
	if (!error) {
		/* next store the actual picture; we prefix all picture names
		 * with "PIC-" to make things easier on the parsing side */
		struct membuffer namebuf = { 0 };
		const char *localfn = local_file_path(pic);
		put_format(&namebuf, "PIC-%s", pic->hash);
		error = blob_insert_fromdisk(repo, dir, localfn, mb_cstring(&namebuf));
		free((void *)localfn);
	}
	return error;
}

static int save_pictures(git_repository *repo, struct dir *dir, struct dive *dive)
{
	if (dive->picture_list) {
		dir = mktree(repo, dir, "Pictures");
		FOR_EACH_PICTURE(dive) {
			save_one_picture(repo, dir, picture);
		}
	}
	return 0;
}

static int save_one_dive(git_repository *repo, struct dir *tree, struct dive *dive, struct tm *tm)
{
	struct divecomputer *dc;
	struct membuffer buf = { 0 }, name = { 0 };
	struct dir *subdir;
	int ret, nr;

	/* Create dive directory */
	create_dive_name(dive, &name, tm);
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
		   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
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

static int save_one_trip(git_repository *repo, struct dir *tree, dive_trip_t *trip, struct tm *tm)
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
			save_one_dive(repo, subdir, dive, tm);
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
		put_format(b, "units PERSONALIZE %s %s %s %s %s %s",
			   prefs.units.length == METERS ? "METERS" : "FEET",
			   prefs.units.volume == LITER ? "LITER" : "CUFT",
			   prefs.units.pressure == BAR ? "BAR" : prefs.units.pressure == PSI ? "PSI" : "PASCAL",
			   prefs.units.temperature == CELSIUS ? "CELSIUS" : prefs.units.temperature == FAHRENHEIT ? "FAHRENHEIT" : "KELVIN",
			   prefs.units.weight == KG ? "KG" : "LBS",
			   prefs.units.vertical_speed_time == SECONDS ? "SECONDS" : "MINUTES");
}

static void save_userid(void *_b)
{
	struct membuffer *b = _b;
	if (prefs.save_userid_local)
		put_format(b, "userid %30s\n", prefs.userid);
}

static void save_one_device(void *_b, const char *model, uint32_t deviceid,
	const char *nickname, const char *serial, const char *firmware)
{
	struct membuffer *b = _b;

	if (nickname && !strcmp(model, nickname))
		nickname = NULL;
	if (serial && !*serial) serial = NULL;
	if (firmware && !*firmware) firmware = NULL;
	if (nickname && !*nickname) nickname = NULL;
	if (!nickname && !serial && !firmware)
		return;

	show_utf8(b, "divecomputerid ", model, "");
	put_format(b, " deviceid=%08x", deviceid);
	show_utf8(b, " serial=", serial, "");
	show_utf8(b, " firmware=", firmware, "");
	show_utf8(b, " nickname=", nickname, "");
	put_string(b, "\n");
}

static void save_settings(git_repository *repo, struct dir *tree)
{
	struct membuffer b = { 0 };

	put_format(&b, "version %d\n", DATAFORMAT_VERSION);
	save_userid(&b);
	call_for_each_dc(&b, save_one_device, false);
	cond_put_format(autogroup, &b, "autogroup\n");
	save_units(&b);

	blob_insert(repo, tree, &b, "00-Subsurface");
}

static void save_divesites(git_repository *repo, struct dir *tree)
{
	struct dir *subdir;
	struct membuffer dirname = { 0 };
	put_format(&dirname, "01-Divesites");
	subdir = new_directory(repo, tree, &dirname);

	for (int i = 0; i < dive_site_table.nr; i++) {
		struct membuffer b = { 0 };
		struct dive_site *ds = get_dive_site(i);
		if (dive_site_is_empty(ds)) {
			int j;
			struct dive *d;
			for_each_dive(j, d) {
				if (d->dive_site_uuid == ds->uuid)
					d->dive_site_uuid = 0;
			}
			delete_dive_site(get_dive_site(i)->uuid);
			i--; // since we just deleted that one
			continue;
		}
		int size = sizeof("Site-012345678");
		char name[size];
		snprintf(name, size, "Site-%08x", ds->uuid);
		show_utf8(&b, "name ", ds->name, "\n");
		show_utf8(&b, "description ", ds->description, "\n");
		show_utf8(&b, "notes ", ds->notes, "\n");
		show_gps(&b, ds->latitude, ds->longitude);
		blob_insert(repo, subdir, &b, name);
	}
}

static int create_git_tree(git_repository *repo, struct dir *root, bool select_only)
{
	int i;
	struct dive *dive;
	dive_trip_t *trip;

	save_settings(repo, root);

	save_divesites(repo, root);

	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;

	/* save the dives */
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
		utc_mkdate(trip ? trip->when : dive->when, &tm);
		tree = mktree(repo, root, "%04d", tm.tm_year + 1900);
		tree = mktree(repo, tree, "%02d", tm.tm_mon + 1);

		if (trip) {
			/* Did we already save this trip? */
			if (trip->index)
				continue;
			trip->index = 1;

			/* Pass that new subdirectory in for save-trip */
			save_one_trip(repo, tree, trip, &tm);
			continue;
		}

		save_one_dive(repo, tree, dive, &tm);
	}
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

static int update_git_checkout(git_repository *repo, git_object *parent, git_tree *tree)
{
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;

	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	opts.notify_flags = GIT_CHECKOUT_NOTIFY_CONFLICT | GIT_CHECKOUT_NOTIFY_DIRTY;
	opts.notify_cb = notify_cb;
	opts.baseline = get_git_tree(repo, parent);
	return git_checkout_tree(repo, (git_object *) tree, &opts);
}

static int get_authorship(git_repository *repo, git_signature **authorp)
{
#if LIBGIT2_VER_MAJOR || LIBGIT2_VER_MINOR >= 20
	if (git_signature_default(authorp, repo) == 0)
		return 0;
#endif
	/* Default name information, with potential OS overrides */
	struct user_info user = {
		.name = "Subsurface",
		.email = "subsurace@subsurface-divelog.org"
	};

	subsurface_user_info(&user);

	/* git_signature_default() is too recent */
	return git_signature_now(authorp, user.name, user.email);
}

static void create_commit_message(struct membuffer *msg)
{
	int nr = dive_table.nr;
	struct dive *dive = get_dive(nr-1);

	if (dive) {
		dive_trip_t *trip = dive->divetrip;
		const char *location = get_dive_location(dive) ? : "no location";
		struct divecomputer *dc = &dive->dc;
		const char *sep = "\n";

		if (dive->number)
			nr = dive->number;

		put_format(msg, "dive %d: %s", nr, location);
		if (trip && trip->location && *trip->location && strcmp(trip->location, location))
			put_format(msg, " (%s)", trip->location);
		put_format(msg, "\n");
		do {
			if (dc->model && *dc->model) {
				put_format(msg, "%s%s", sep, dc->model);
				sep = ", ";
			}
		} while ((dc = dc->next) != NULL);
		put_format(msg, "\n");
	}
	put_format(msg, "Created by subsurface %s\n", subsurface_version());
}

static int create_new_commit(git_repository *repo, const char *remote, const char *branch, git_oid *tree_id)
{
	int ret;
	git_reference *ref;
	git_object *parent;
	git_oid commit_id;
	git_signature *author;
	git_commit *commit;
	git_tree *tree;

	ret = git_branch_lookup(&ref, repo, branch, GIT_BRANCH_LOCAL);
	switch (ret) {
	default:
		return report_error("Bad branch '%s' (%s)", branch, strerror(errno));
	case GIT_EINVALIDSPEC:
		return report_error("Invalid branch name '%s'", branch);
	case GIT_ENOTFOUND: /* We'll happily create it */
		ref = NULL;
		parent = try_to_find_parent(saved_git_id, repo);
		break;
	case 0:
		if (git_reference_peel(&parent, ref, GIT_OBJ_COMMIT))
			return report_error("Unable to look up parent in branch '%s'", branch);

		if (saved_git_id) {
			if (existing_filename)
				fprintf(stderr, "existing filename %s\n", existing_filename);
			const git_oid *id = git_commit_id((const git_commit *) parent);
			/* if we are saving to the same git tree we got this from, let's make
			 * sure there is no confusion */
			if (same_string(existing_filename, remote) && git_oid_strcmp(id, saved_git_id))
				return report_error("The git branch does not match the git parent of the source");
		}

		/* all good */
		break;
	}

	if (git_tree_lookup(&tree, repo, tree_id))
		return report_error("Could not look up newly created tree");

	if (get_authorship(repo, &author))
		return report_error("No user name configuration in git repo");

	/* If the parent commit has the same tree ID, do not create a new commit */
	if (parent && git_oid_equal(tree_id, git_commit_tree_id((const git_commit *) parent))) {
		/* If the parent already came from the ref, the commit is already there */
		if (ref)
			return 0;
		/* Else we do want to create the new branch, but with the old commit */
		commit = (git_commit *) parent;
	} else {
		struct membuffer commit_msg = { 0 };

		create_commit_message(&commit_msg);
		if (git_commit_create_v(&commit_id, repo, NULL, author, author, NULL, mb_cstring(&commit_msg), tree, parent != NULL, parent))
			return report_error("Git commit create failed (%s)", strerror(errno));
		free_buffer(&commit_msg);

		if (git_commit_lookup(&commit, repo, &commit_id))
			return report_error("Could not look up newly created commit");
	}

	if (!ref) {
		if (git_branch_create(&ref, repo, branch, commit, 0, author, "Create branch"))
			return report_error("Failed to create branch '%s'", branch);
	}
	/*
	 * If it's a checked-out branch, try to also update the working
	 * tree and index. If that fails (dirty working tree or whatever),
	 * this is not technically a save error (we did save things in
	 * the object database), but it can cause extreme confusion, so
	 * warn about it.
	 */
	if (git_branch_is_head(ref) && !git_repository_is_bare(repo)) {
		if (update_git_checkout(repo, parent, tree)) {
			const git_error *err = giterr_last();
			const char *errstr = err ? err->message : strerror(errno);
			report_error("Git branch '%s' is checked out, but worktree is dirty (%s)",
				branch, errstr);
		}
	}

	if (git_reference_set_target(&ref, ref, &commit_id, author, "Subsurface save event"))
		return report_error("Failed to update branch '%s'", branch);
	set_git_id(&commit_id);

	git_signature_free(author);

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
	ret = git_treebuilder_write(result, repo, tree->files);

	/* .. and free the now useless treebuilder */
	git_treebuilder_free(tree->files);

	return ret;
}

int do_git_save(git_repository *repo, const char *branch, const char *remote, bool select_only, bool create_empty)
{
	struct dir tree;
	git_oid id;

	/* Start with an empty tree: no subdirectories, no files */
	tree.name[0] = 0;
	tree.subdirs = NULL;
	if (git_treebuilder_new(&tree.files, repo, NULL))
		return report_error("git treebuilder failed");

	if (!create_empty)
		/* Populate our tree data structure */
		if (create_git_tree(repo, &tree, select_only))
			return -1;

	if (write_git_tree(repo, &tree, &id))
		return report_error("git tree write failed");

	/* And save the tree! */
	if (create_new_commit(repo, remote, branch, &id))
		return report_error("creating commit failed");

	if (remote && prefs.cloud_background_sync) {
		/* now sync the tree with the cloud server */
		if (strstr(remote, prefs.cloud_git_url)) {
			return sync_with_remote(repo, remote, branch, RT_HTTPS);
		}
	}
	return 0;
}

int git_save_dives(struct git_repository *repo, const char *branch, const char *remote, bool select_only)
{
	int ret;

	if (repo == dummy_git_repository)
		return report_error("Unable to open git repository '%s'", branch);
	ret = do_git_save(repo, branch, remote, select_only, false);
	git_repository_free(repo);
	free((void *)branch);
	return ret;
}
