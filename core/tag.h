// SPDX-License-Identifier: GPL-2.0
// Dive tag related structures and helpers
#ifndef TAG_H
#define TAG_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct divetag {
	/*
	 * The name of the divetag. If a translation is available, name contains
	 * the translated tag
	 */
	char *name;
	/*
	 * If a translation is available, we write the original tag to source.
	 * This enables us to write a non-localized tag to the xml file.
	 */
	char *source;
};

struct tag_entry {
	struct divetag *tag;
	struct tag_entry *next;
};

/*
 * divetags are only stored once, each dive only contains
 * a list of tag_entries which then point to the divetags
 * in the global g_tag_list
 */
extern struct tag_entry *g_tag_list;

struct divetag *taglist_add_tag(struct tag_entry **tag_list, const char *tag);

/*
 * Writes all divetags form tag_list into internally allocated buffer
 * Function returns pointer to allocated buffer
 * Buffer contains comma separated list of tags names or null terminated string
 *
 * NOTE! The returned buffer must be freed once used.
 */
char *taglist_get_tagstring(struct tag_entry *tag_list);

/* cleans up a list: removes empty tags and duplicates */
void taglist_cleanup(struct tag_entry **tag_list);

void taglist_init_global();
void taglist_free(struct tag_entry *tag_list);
struct tag_entry *taglist_copy(struct tag_entry *s);
void taglist_merge(struct tag_entry **dst, struct tag_entry *src1, struct tag_entry *src2);

#ifdef __cplusplus
}
#endif

#endif
