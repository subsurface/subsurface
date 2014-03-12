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
#include "device.h"
#include "membuffer.h"
#include "ssrf-version.h"

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

static void cond_put_format(int cond, struct membuffer *b, const char *fmt, ...)
{
	if (cond) {
		VA_BUF(b, fmt);
	}
}

#define SAVE(str, x) cond_put_format(dive->x, b, str " %d\n", dive->x)

static void put_degrees(struct membuffer *b, degrees_t value, const char sep)
{
	int udeg = value.udeg;
	const char *sign = "";

	if (udeg < 0) {
		udeg = -udeg;
		sign = "-";
	}
	put_format(b,"%s%u.%06u%c", sign, FRACTION(udeg, 1000000), sep);
}

static void show_gps(struct membuffer *b, degrees_t latitude, degrees_t longitude)
{
	if (latitude.udeg || longitude.udeg) {
		put_string(b, "gps ");
		put_degrees(b, latitude, ' ');
		put_degrees(b, longitude, '\n');
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
	show_gps(b, dive->latitude, dive->longitude);
	show_utf8(b, "location ", dive->location, "\n");
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

static void save_cylinder_info(struct membuffer *b, struct dive *dive)
{
	int i, nr;

	nr = nr_cylinders(dive);
	for (i = 0; i < nr; i++) {
		cylinder_t *cylinder = dive->cylinder + i;
		int volume = cylinder->type.size.mliter;
		const char *description = cylinder->type.description;
		int o2 = cylinder->gasmix.o2.permille;
		int he = cylinder->gasmix.he.permille;

		put_string(b, "cylinder");
		if (volume)
			put_milli(b, " vol=", volume, "l");
		put_pressure(b, cylinder->type.workingpressure, " workpressure=", "bar");
		show_utf8(b, " description=", description, "");
		strip_mb(b);
		if (o2) {
			put_format(b, " o2=%u.%u%%", FRACTION(o2, 10));
			if (he)
				put_format(b, " he=%u.%u%%", FRACTION(he, 10));
		}
		put_pressure(b, cylinder->start, " start=", "bar");
		put_pressure(b, cylinder->end, " end=", "bar");
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

	if (sample->po2 != old->po2) {
		put_milli(b, " po2=", sample->po2, "bar");
		old->po2 = sample->po2;
	}
	show_index(b, sample->heartbeat, "heartbeat=", "");
	show_index(b, sample->bearing, "bearing=", "°");
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

	save_depths(b, dc);
	save_temperatures(b, dc);
	save_airpressure(b, dc);
	save_salinity(b, dc);
	put_duration(b, dc->surfacetime, "surfacetime ", "min\n");

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

	save_overview(b, dive);
	save_cylinder_info(b, dive);
	save_weightsystem_info(b, dive);
	save_dive_temperature(b, dive);
}

int report_error(const char *fmt, ...)
{
	struct membuffer b = { 0 };
	VA_BUF(&b, fmt);

	/* We should do some UI element thing describing the failure */
	put_bytes(&b, "\n", 1);
	flush_buffer(&b, stderr);
	free_buffer(&b);

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
static struct dir *new_directory(struct dir *parent, struct membuffer *namebuf)
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
	git_treebuilder_create(&subdir->files, NULL);
	memcpy(subdir->name, name, len);
	subdir->unique = 0;
	subdir->name[len] = 0;

	/* Add it to the list of subdirs of the parent */
	subdir->sibling = parent->subdirs;
	parent->subdirs = subdir;

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
 */
static void create_dive_name(struct dive *dive, struct membuffer *name, struct tm *dirtm)
{
	struct tm tm;

	utc_mkdate(dive->when, &tm);
	if (tm.tm_year != dirtm->tm_year)
		put_format(name, "%04u-", tm.tm_year + 1900);
	if (tm.tm_mon != dirtm->tm_mon)
		put_format(name, "%02u-", tm.tm_mon+1);

	put_format(name, "%02u-%s-%02u:%02u:%02u",
		tm.tm_mday, weekday(tm.tm_wday),
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

static int save_one_dive(git_repository *repo, struct dir *tree, struct dive *dive, struct tm *tm)
{
	struct divecomputer *dc;
	struct membuffer buf = { 0 }, name = { 0 };
	struct dir *subdir;
	int ret, nr;

	/* Create dive directory */
	create_dive_name(dive, &name, tm);
	subdir = new_directory(tree, &name);
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
	subdir = new_directory(tree, &name);
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

static struct dir *mktree(struct dir *dir, const char *fmt, ...)
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
		subdir = new_directory(dir, &buf);
	free_buffer(&buf);
	return subdir;
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

#define VERSION 2

static void save_settings(git_repository *repo, struct dir *tree)
{
	struct membuffer b = { 0 };

	put_format(&b, "version %d\n", VERSION);
	call_for_each_dc(&b, save_one_device);
	cond_put_format(autogroup, &b, "autogroup\n");

	blob_insert(repo, tree, &b, "00-Subsurface");
}

static int create_git_tree(git_repository *repo, struct dir *root, bool select_only)
{
	int i;
	struct dive *dive;
	dive_trip_t *trip;

	save_settings(repo, root);

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
		tree = mktree(root, "%04d", tm.tm_year + 1900);
		tree = mktree(tree, "%02d", tm.tm_mon + 1);

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
 * libgit2 revision 0.20 and earlier do not have the signature and
 * message log arguments.
 */
#if !LIBGIT2_VER_MAJOR && LIBGIT2_VER_MINOR <= 20 && !defined(USE_LIBGIT21_API)
  #define git_branch_create(out,repo,branch_name,target,force,sig,msg) \
	git_branch_create(out,repo,branch_name,target,force)
  #define git_reference_set_target(out,ref,target,signature,log_message) \
	git_reference_set_target(out,ref,target)
#endif

static int create_new_commit(git_repository *repo, const char *branch, git_oid *tree_id)
{
	int ret;
	git_reference *ref;
	git_object *parent;
	git_oid commit_id;
	git_signature *author;
	git_commit *commit;
	git_tree *tree;
	struct membuffer commit_msg = { 0 };

	ret = git_branch_lookup(&ref, repo, branch, GIT_BRANCH_LOCAL);
	switch (ret) {
	default:
		return report_error("Bad branch '%s' (%s)", branch, strerror(errno));
	case GIT_EINVALIDSPEC:
		return report_error("Invalid branch name '%s'", branch);
	case GIT_ENOTFOUND: /* We'll happily create it */
		ref = NULL; parent = NULL;
		break;
	case 0:
		if (git_reference_peel(&parent, ref, GIT_OBJ_COMMIT))
			return report_error("Unable to look up parent in branch '%s'", branch);

		/* If the parent commit has the same tree ID, do nothing */
		if (git_oid_equal(tree_id, git_commit_tree_id((const git_commit *) parent)))
			return 0;

		/* all good */
		break;
	}

	if (git_tree_lookup(&tree, repo, tree_id))
		return report_error("Could not look up newly created tree");

	/* git_signature_default() is too recent */
	if (git_signature_now(&author, "Subsurface", "subsurface@hohndel.org"))
		return report_error("No user name configuration in git repo");

	put_format(&commit_msg, "Created by subsurface %s\n", VERSION_STRING);
	if (git_commit_create_v(&commit_id, repo, NULL, author, author, NULL, mb_cstring(&commit_msg), tree, parent != NULL, parent))
		return report_error("Git commit create failed (%s)", strerror(errno));

	if (git_commit_lookup(&commit, repo, &commit_id))
		return report_error("Could not look up newly created commit");

	if (!ref) {
		if (git_branch_create(&ref, repo, branch, commit, 0, author, "Create branch"))
			return report_error("Failed to create branch '%s'", branch);
	}
	if (git_reference_set_target(&ref, ref, &commit_id, author, "Subsurface save event"))
		return report_error("Failed to update branch '%s'", branch);

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

static int do_git_save(git_repository *repo, const char *branch, bool select_only)
{
	struct dir tree;
	git_oid id;

	/* Start with an empty tree: no subdirectories, no files */
	tree.name[0] = 0;
	tree.subdirs = NULL;
	if (git_treebuilder_create(&tree.files, NULL))
		return report_error("git treebuilder failed");

	/* Populate our tree data structure */
	if (create_git_tree(repo, &tree, select_only))
		return -1;

	if (write_git_tree(repo, &tree, &id))
		return report_error("git tree write failed");

	/* And save the tree! */
	create_new_commit(repo, branch, &id);
	return 0;
}

/*
 * If it's not a git repo, return NULL. Be very conservative.
 */
struct git_repository *is_git_repository(const char *filename, const char **branchp)
{
	int flen, blen, ret;
	struct stat st;
	git_repository *repo;
	char *loc, *branch;

	flen = strlen(filename);
	if (!flen || filename[--flen] != ']')
		return NULL;

	/* Find the matching '[' */
	blen = 0;
	while (flen && filename[--flen] != '[')
		blen++;

	if (!flen)
		return NULL;

	loc = malloc(flen+1);
	if (!loc)
		return NULL;
	memcpy(loc, filename, flen);
	loc[flen] = 0;

	branch = malloc(blen+1);
	if (!branch) {
		free(loc);
		return NULL;
	}
	memcpy(branch, filename+flen+1, blen);
	branch[blen] = 0;

	if (stat(loc, &st) < 0 || !S_ISDIR(st.st_mode)) {
		free(loc);
		return NULL;
	}

	ret = git_repository_open(&repo, loc);
	free(loc);
	if (ret < 0) {
		free(branch);
		return NULL;
	}
	*branchp = branch;
	return repo;
}

int git_save_dives(struct git_repository *repo, const char *branch, bool select_only)
{
	int ret = do_git_save(repo, branch, select_only);
	git_repository_free(repo);
	free((void *)branch);
	return ret;
}
