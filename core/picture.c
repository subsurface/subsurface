// SPDX-License-Identifier: GPL-2.0
#include "picture.h"
#include "dive.h"
#if !defined(SUBSURFACE_MOBILE)
#include "metadata.h"
#endif
#include "subsurface-string.h"
#include "table.h"
#include <stdlib.h>
#include <string.h>

static void free_picture(struct picture picture)
{
	free(picture.filename);
	picture.filename = NULL;
}

static int comp_pictures(struct picture a, struct picture b)
{
	if (a.offset.seconds < b.offset.seconds)
		return -1;
	if (a.offset.seconds > b.offset.seconds)
		return 1;
	return strcmp(a.filename ?: "", b.filename ?: "");
}

static bool picture_less_than(struct picture a, struct picture b)
{
	return comp_pictures(a, b) < 0;
}

/* picture table functions */
//static MAKE_GET_IDX(picture_table, struct picture, pictures)
static MAKE_GROW_TABLE(picture_table, struct picture, pictures)
static MAKE_GET_INSERTION_INDEX(picture_table, struct picture, pictures, picture_less_than)
MAKE_ADD_TO(picture_table, struct picture, pictures)
MAKE_REMOVE_FROM(picture_table, pictures)
MAKE_SORT(picture_table, struct picture, pictures, comp_pictures)
//MAKE_REMOVE(picture_table, struct picture, picture)
MAKE_CLEAR_TABLE(picture_table, pictures, picture)

/* Add a clone of a picture to the end of a picture table.
 * Cloned means that the filename-string is copied. */
static void add_cloned_picture(struct picture_table *t, struct picture pic)
{
	pic.filename = copy_string(pic.filename);
	int idx = picture_table_get_insertion_index(t, pic);
	add_to_picture_table(t, idx, pic);
}

void copy_pictures(const struct picture_table *s, struct picture_table *d)
{
	int i;
	clear_picture_table(d);
	for (i = 0; i < s->nr; i++)
		add_cloned_picture(d, s->pictures[i]);
}

void add_picture(struct picture_table *t, struct picture newpic)
{
	int idx = picture_table_get_insertion_index(t, newpic);
	add_to_picture_table(t, idx, newpic);
}

int get_picture_idx(const struct picture_table *t, const char *filename)
{
	for (int i = 0; i < t->nr; ++i) {
		if (same_string(t->pictures[i].filename, filename))
			return i;
	}
	return -1;
}

/* Return distance of timestamp to time of dive. Result is always positive, 0 means during dive. */
static timestamp_t time_from_dive(const struct dive *d, timestamp_t timestamp)
{
	timestamp_t end_time = dive_endtime(d);
	if (timestamp < d->when)
		return d->when - timestamp;
	else if (timestamp > end_time)
		return timestamp - end_time;
	else
		return 0;
}

/* Return dive closest selected dive to given timestamp or NULL if no dives are selected. */
static struct dive *nearest_selected_dive(timestamp_t timestamp)
{
	struct dive *d, *res = NULL;
	int i;
	timestamp_t offset, min = 0;

	for_each_dive(i, d) {
		if (!d->selected)
			continue;
		offset = time_from_dive(d, timestamp);
		if (!res || offset < min) {
			res = d;
			min = offset;
		}

		/* We suppose that dives are sorted chronologically. Thus
		 * if the offset starts to increase, we can end. This ignores
		 * pathological cases such as overlapping dives. In such a
		 * case the user will have to add pictures manually.
		 */
		if (offset == 0 || offset > min)
			break;
	}
	return res;
}

// only add pictures that have timestamps between 30 minutes before the dive and
// 30 minutes after the dive ends
#define D30MIN (30 * 60)
static bool dive_check_picture_time(const struct dive *d, timestamp_t timestamp)
{
	return time_from_dive(d, timestamp) < D30MIN;
}

#if !defined(SUBSURFACE_MOBILE)
/* Creates a picture and indicates the dive to which this picture should be added.
 * The caller is responsible for actually adding the picture to the dive.
 * If no appropriate dive was found, no picture is created and NULL is returned.
 */
struct picture *create_picture(const char *filename, timestamp_t shift_time, bool match_all, struct dive **dive)
{
	struct metadata metadata;
	timestamp_t timestamp;

	get_metadata(filename, &metadata);
	timestamp = metadata.timestamp + shift_time;
	*dive = nearest_selected_dive(timestamp);

	if (!*dive)
		return NULL;
	if (get_picture_idx(&(*dive)->pictures, filename) >= 0)
		return NULL;
	if (!match_all && !dive_check_picture_time(*dive, timestamp))
		return NULL;

	struct picture *picture = malloc(sizeof(struct picture));
	picture->filename = strdup(filename);
	picture->offset.seconds = metadata.timestamp - (*dive)->when + shift_time;
	picture->location = metadata.location;
	return picture;
}

bool picture_check_valid_time(timestamp_t timestamp, timestamp_t shift_time)
{
	int i;
	struct dive *dive;

	for_each_dive (i, dive)
		if (dive->selected && dive_check_picture_time(dive, timestamp + shift_time))
			return true;
	return false;
}
#endif
