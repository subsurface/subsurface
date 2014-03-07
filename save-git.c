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
	/* The first entry is a dummy, because people don't understand pointers to pointers */
	tags = tags->next;
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
	if (!dive->airtemp.mkelvin && !dive->watertemp.mkelvin)
		return;
	if (dive->airtemp.mkelvin == dc_airtemp(&dive->dc) && dive->watertemp.mkelvin == dc_watertemp(&dive->dc))
		return;

	put_string(b, "divetemperature");
	if (dive->airtemp.mkelvin != dc_airtemp(&dive->dc))
		put_temperature(b, dive->airtemp, " air=", "°C");
	if (dive->watertemp.mkelvin != dc_watertemp(&dive->dc))
		put_temperature(b, dive->watertemp, " water=", "°C'");
	put_string(b, "\n");
}

static void save_depths(struct membuffer *b, struct divecomputer *dc)
{
	/* What's the point of this dive entry again? */
	if (!dc->maxdepth.mm && !dc->meandepth.mm)
		return;

	put_string(b, "  depth");
	put_depth(b, dc->maxdepth, " max=", "m");
	put_depth(b, dc->meandepth, " mean=", "m");
	put_string(b, "\n");
}

static void save_temperatures(struct membuffer *b, struct divecomputer *dc)
{
	if (!dc->airtemp.mkelvin && !dc->watertemp.mkelvin)
		return;
	put_string(b, "  temperature");
	put_temperature(b, dc->airtemp, " air=", "°C");
	put_temperature(b, dc->watertemp, " water=", "°C");
	put_string(b, "\n");
}

static void save_airpressure(struct membuffer *b, struct divecomputer *dc)
{
	if (!dc->surface_pressure.mbar)
		return;
	put_string(b, "  surface");
	put_pressure(b, dc->surface_pressure, " pressure=", "bar");
	put_string(b, "\n");
}

static void save_salinity(struct membuffer *b, struct divecomputer *dc)
{
	/* only save if we have a value that isn't the default of sea water */
	if (!dc->salinity || dc->salinity == SEAWATER_SALINITY)
		return;
	put_string(b, "  water");
	put_salinity(b, dc->salinity, " salinity=", "g/l");
	put_string(b, "\n");
}

static void show_date(struct membuffer *b, timestamp_t when)
{
	struct tm tm;

	utc_mkdate(when, &tm);

	put_format(b, " date=%04u-%02u-%02u",
		   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	put_format(b, " time=%02u:%02u:%02u",
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
	put_format(b, "  %3u:%02umin", FRACTION(sample->time.seconds, 60));
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
		put_format(b, " ndl=%u:%02umin", FRACTION(sample->ndl.seconds, 60));
		old->ndl = sample->ndl;
	}
	if (sample->in_deco != old->in_deco) {
		put_format(b, " in_deco=%d", sample->in_deco ? 1 : 0);
		old->in_deco = sample->in_deco;
	}
	if (sample->stoptime.seconds != old->stoptime.seconds) {
		put_format(b, " stoptime=%u:%02umin", FRACTION(sample->stoptime.seconds, 60));
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
	put_format(b, "  event %d:%02dmin", FRACTION(ev->time.seconds, 60));
	show_index(b, ev->type, "type=", "");
	show_index(b, ev->flags, "flags=", "");
	show_index(b, ev->value, "value=", "");
	show_utf8(b, ev->name, " name=", "");
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
	put_format(b, "divecomputer");
	show_utf8(b, " model=", dc->model, "");
	if (dc->deviceid)
		put_format(b, " deviceid=%08x", dc->deviceid);
	if (dc->diveid)
		put_format(b, " diveid=%08x", dc->diveid);
	if (dc->when && dc->when != dive->when)
		show_date(b, dc->when);
	if (dc->duration.seconds && dc->duration.seconds != dive->dc.duration.seconds)
		put_duration(b, dc->duration, " duration=", "min");
	put_string(b, "\n");

	save_depths(b, dc);
	save_temperatures(b, dc);
	save_airpressure(b, dc);
	save_salinity(b, dc);
	put_duration(b, dc->surfacetime, "  surfacetime", "min\n");

	save_events(b, dc->events);
	save_samples(b, dc->samples, dc->sample);
}

/*
 * Note that we don't save the date and time or dive
 * number: they are encoded in the filename.
 */
static void create_dive_buffer(struct dive *dive, struct membuffer *b)
{
	struct divecomputer *dc;

	put_format(b, "duration %u:%02u min\n", FRACTION(dive->dc.duration.seconds, 60));
	SAVE("rating", rating);
	SAVE("visibility", visibility);
	cond_put_format(dive->tripflag == NO_TRIP, b, "notrip\n");
	save_tags(b, dive->tag_list);

	save_overview(b, dive);
	save_cylinder_info(b, dive);
	save_weightsystem_info(b, dive);
	save_dive_temperature(b, dive);

	/* Save the dive computer data */
	dc = &dive->dc;
	do {
		save_dc(b, dive, dc);
		dc = dc->next;
	} while (dc);
}

static int git_save_error(const char *fmt, ...)
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
	char name[1];
};

/*
 * The name of a dive is the date plus a hash of the contents.
 */
static void generate_name(struct dive *dive, git_oid *id, struct membuffer *name)
{
	struct tm tm;

	utc_mkdate(dive->when, &tm);
	put_format(name, "%04u-%02u-%02u-%02u:%02u:%02u",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	cond_put_format(dive->number, name, "-%d", dive->number);
	put_format(name, "-%02x%02x%02x%02x%c",
		id->id[0], id->id[1], id->id[2], id->id[3], 0);
}

static int save_one_dive(git_repository *repo, struct dir *tree, struct dive *dive)
{
	struct membuffer buf = { 0 }, name = { 0 };
	git_oid blob_id;
	int ret;

	create_dive_buffer(dive, &buf);
	ret = git_blob_create_frombuffer(&blob_id, repo, buf.buffer, buf.len);
	free_buffer(&buf);
	if (ret)
		return git_save_error("git blob creation failed");

	generate_name(dive, &blob_id, &name);
	ret = git_treebuilder_insert(NULL, tree->files, name.buffer, &blob_id, 0100644);
	free_buffer(&name);
	if (ret)
		return git_save_error("git tree insert failed");

	return 0;
}

/*
 * This does *not* make sure the new subdirectory doesn't
 * alias some existing name. Currently we always create
 * unique subdirectory names ("tripXYZ"), but ..
 */
static struct dir *new_directory(struct dir *parent, const char *fmt, ...)
{
	struct membuffer name = { 0 };
	struct dir *subdir;

	VA_BUF(&name, fmt);
	subdir = malloc(sizeof(*subdir)+name.len);

	/*
	 * It starts out empty: no subdirectories of its own,
	 * and an empty treebuilder list of files.
	 */
	subdir->subdirs = NULL;
	git_treebuilder_create(&subdir->files, NULL);
	memcpy(subdir->name, name.buffer, name.len);
	subdir->name[name.len] = 0;

	/* Add it to the list of subdirs of the parent */
	subdir->sibling = parent->subdirs;
	parent->subdirs = subdir;

	return subdir;
}

static int save_one_trip(git_repository *repo, struct dir *tree, dive_trip_t *trip, int idx)
{
	int i;
	struct dive *dive;
	struct dir *subdir;

	subdir = new_directory(tree, "trip%03d", idx);
	for_each_dive(i, dive) {
		if (dive->divetrip == trip)
			save_one_dive(repo, subdir, dive);
	}

	return 0;
}

#define TRIPNAME_SIZE (10)

static int create_git_tree(git_repository *repo, struct dir *tree, bool select_only)
{
	int i, tripidx;
	struct dive *dive;
	dive_trip_t *trip;

	tripidx = 0;
	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;

	/* save the dives */
	for_each_dive(i, dive) {
		trip = dive->divetrip;

		if (select_only) {
			if (!dive->selected)
				continue;
			/* We don't save trips when doing selected dive saves */
			trip = NULL;
		}

		if (trip) {
			/* Did we already save this trip? */
			if (trip->index)
				continue;
			trip->index = ++tripidx;

			/* Pass that new subdirectory in for save-trip */
			save_one_trip(repo, tree, trip, tripidx);
			continue;
		}

		save_one_dive(repo, tree, dive);
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

	ret = git_branch_lookup(&ref, repo, branch, GIT_BRANCH_LOCAL);
	switch (ret) {
	default:
		return git_save_error("Bad branch '%s' (%s)", branch, strerror(errno));
	case GIT_EINVALIDSPEC:
		return git_save_error("Invalid branch name '%s'", branch);
	case GIT_ENOTFOUND: /* We'll happily create it */
		ref = NULL; parent = NULL;
		break;
	case 0:
		if (git_reference_peel(&parent, ref, GIT_OBJ_COMMIT))
			return git_save_error("Unable to look up parent in branch '%s'", branch);
		/* all good */
		break;
	}

	if (git_tree_lookup(&tree, repo, tree_id))
		return git_save_error("Could not look up newly created tree");

	/* git_signature_default() is too recent */
	if (git_signature_now(&author, "Subsurface", "subsurface@hohndel.org"))
		return git_save_error("No user name configuration in git repo");

	if (git_commit_create_v(&commit_id, repo, NULL, author, author, NULL, "subsurface commit", tree, parent != NULL, parent))
		return git_save_error("Git commit create failed (%s)", strerror(errno));

	if (git_commit_lookup(&commit, repo, &commit_id))
		return git_save_error("Could not look up newly created commit");

	if (!ref) {
		if (git_branch_create(&ref, repo, branch, commit, 0, author, "Create branch"))
			return git_save_error("Failed to create branch '%s'", branch);
	}
	if (git_reference_set_target(&ref, ref, &commit_id, author, "Subsurface save event"))
		return git_save_error("Failed to update branch '%s'", branch);

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
			git_treebuilder_insert(NULL, tree->files, subdir->name, &id, 0040000);
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
		return git_save_error("git treebuilder failed");

	/* Populate our tree data structure */
	if (create_git_tree(repo, &tree, select_only))
		return -1;

	if (write_git_tree(repo, &tree, &id))
		return git_save_error("git tree write failed");

	/* And save the tree! */
	create_new_commit(repo, branch, &id);
	return 0;
}

int git_save_dives(int fd, bool select_only)
{
	int len, ret;
	struct stat st;
	static char buffer[2048];
	git_repository *repo;
	char *loc, *branch;

	if (fstat(fd, &st) < 0)
		return 0;
	if (st.st_size >= sizeof(buffer))
		return 0;
	len = st.st_size;
	if (read(fd, buffer, len) != len)
		return 0;
	buffer[len] = 0;
	if (len < 4)
		return 0;
	if (memcmp(buffer, "git ", 4))
		return 0;

	/* Ok, it's a git pointer, we're going to follow it */
	close(fd);

	/* Trim any whitespace at the end */
	while (isspace(buffer[len-1]))
		buffer[--len] = 0;

	/* skip the "git" part and any whitespace from the beginning */
	loc = buffer+3;
	len -= 3;
	while (isspace(*loc)) {
		loc++;
		len--;
	}

	if (!len)
		return git_save_error("Invalid git pointer");

	/*
	 * The result should be a git directory and branch name, like
	 *  "/home/torvalds/scuba:linus"
	 */
	branch = strrchr(loc, ':');
	if (branch)
		*branch++ = 0;

	if (git_repository_open(&repo, loc))
		return git_save_error("Unable to open git repository at '%s' (branch '%s')", loc, branch);

	ret = do_git_save(repo, branch, select_only);
	git_repository_free(repo);
	return ret < 0 ? ret : 1;
}
