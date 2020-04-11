// SPDX-License-Identifier: GPL-2.0
#include "picture.h"
#include "table.h"
#include "subsurface-string.h"
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
static MAKE_REMOVE_FROM(picture_table, pictures)
//MAKE_SORT(picture_table, struct picture, pictures, comp_pictures)
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

// Return true if picture was found and deleted
bool remove_picture(struct picture_table *t, const char *filename)
{
	for (int i = 0; i < t->nr; ++i) {
		if (same_string(t->pictures[i].filename, filename)) {
			remove_from_picture_table(t, i);
			return true;
		}
	}
	return false;
}
