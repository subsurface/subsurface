// SPDX-License-Identifier: GPL-2.0

#include "tag.h"
#include "structured_list.h"
#include "subsurface-string.h"
#include "membuffer.h"
#include "gettext.h"

#include <stdlib.h>

struct tag_entry *g_tag_list = NULL;

static const char *default_tags[] = {
	QT_TRANSLATE_NOOP("gettextFromC", "boat"), QT_TRANSLATE_NOOP("gettextFromC", "shore"), QT_TRANSLATE_NOOP("gettextFromC", "drift"),
	QT_TRANSLATE_NOOP("gettextFromC", "deep"), QT_TRANSLATE_NOOP("gettextFromC", "cavern"), QT_TRANSLATE_NOOP("gettextFromC", "ice"),
	QT_TRANSLATE_NOOP("gettextFromC", "wreck"), QT_TRANSLATE_NOOP("gettextFromC", "cave"), QT_TRANSLATE_NOOP("gettextFromC", "altitude"),
	QT_TRANSLATE_NOOP("gettextFromC", "pool"), QT_TRANSLATE_NOOP("gettextFromC", "lake"), QT_TRANSLATE_NOOP("gettextFromC", "river"),
	QT_TRANSLATE_NOOP("gettextFromC", "night"), QT_TRANSLATE_NOOP("gettextFromC", "fresh"), QT_TRANSLATE_NOOP("gettextFromC", "student"),
	QT_TRANSLATE_NOOP("gettextFromC", "instructor"), QT_TRANSLATE_NOOP("gettextFromC", "photo"), QT_TRANSLATE_NOOP("gettextFromC", "video"),
	QT_TRANSLATE_NOOP("gettextFromC", "deco")
};

/* copy an element in a list of tags */
static void copy_tl(struct tag_entry *st, struct tag_entry *dt)
{
	dt->tag = malloc(sizeof(struct divetag));
	dt->tag->name = copy_string(st->tag->name);
	dt->tag->source = copy_string(st->tag->source);
}

static bool tag_seen_before(struct tag_entry *start, struct tag_entry *before)
{
	while (start && start != before) {
		if (same_string(start->tag->name, before->tag->name))
			return true;
		start = start->next;
	}
	return false;
}

/* remove duplicates and empty nodes */
void taglist_cleanup(struct tag_entry **tag_list)
{
	struct tag_entry **tl = tag_list;
	while (*tl) {
		/* skip tags that are empty or that we have seen before */
		if (empty_string((*tl)->tag->name) || tag_seen_before(*tag_list, *tl)) {
			*tl = (*tl)->next;
			continue;
		}
		tl = &(*tl)->next;
	}
}

char *taglist_get_tagstring(struct tag_entry *tag_list)
{
	bool first_tag = true;
	struct membuffer b = { 0 };
	struct tag_entry *tmp = tag_list;
	while (tmp != NULL) {
		if (!empty_string(tmp->tag->name)) {
			if (first_tag) {
				put_format(&b, "%s", tmp->tag->name);
				first_tag = false;
			} else {
				put_format(&b, ", %s", tmp->tag->name);
			}
		}
		tmp = tmp->next;
	}
	/* Ensures we do return null terminated empty string for:
	 *  - empty tag list
	 *  - tag list with empty tag only
	 */
	return detach_cstring(&b);
}

static inline void taglist_free_divetag(struct divetag *tag)
{
	if (tag->name != NULL)
		free(tag->name);
	if (tag->source != NULL)
		free(tag->source);
	free(tag);
}

/* Add a tag to the tag_list, keep the list sorted */
static struct divetag *taglist_add_divetag(struct tag_entry **tag_list, struct divetag *tag)
{
	struct tag_entry *next, *entry;

	while ((next = *tag_list) != NULL) {
		int cmp = strcmp(next->tag->name, tag->name);

		/* Already have it? */
		if (!cmp)
			return next->tag;
		/* Is the entry larger? If so, insert here */
		if (cmp > 0)
			break;
		/* Continue traversing the list */
		tag_list = &next->next;
	}

	/* Insert in front of it */
	entry = malloc(sizeof(struct tag_entry));
	entry->next = next;
	entry->tag = tag;
	*tag_list = entry;
	return tag;
}

struct divetag *taglist_add_tag(struct tag_entry **tag_list, const char *tag)
{
	size_t i = 0;
	int is_default_tag = 0;
	struct divetag *ret_tag, *new_tag;
	const char *translation;
	new_tag = malloc(sizeof(struct divetag));

	for (i = 0; i < sizeof(default_tags) / sizeof(char *); i++) {
		if (strcmp(default_tags[i], tag) == 0) {
			is_default_tag = 1;
			break;
		}
	}
	/* Only translate default tags */
	if (is_default_tag) {
		translation = translate("gettextFromC", tag);
		new_tag->name = malloc(strlen(translation) + 1);
		memcpy(new_tag->name, translation, strlen(translation) + 1);
		new_tag->source = malloc(strlen(tag) + 1);
		memcpy(new_tag->source, tag, strlen(tag) + 1);
	} else {
		new_tag->source = NULL;
		new_tag->name = malloc(strlen(tag) + 1);
		memcpy(new_tag->name, tag, strlen(tag) + 1);
	}
	/* Try to insert new_tag into g_tag_list if we are not operating on it */
	if (tag_list != &g_tag_list) {
		ret_tag = taglist_add_divetag(&g_tag_list, new_tag);
		/* g_tag_list already contains new_tag, free the duplicate */
		if (ret_tag != new_tag)
			taglist_free_divetag(new_tag);
		ret_tag = taglist_add_divetag(tag_list, ret_tag);
	} else {
		ret_tag = taglist_add_divetag(tag_list, new_tag);
		if (ret_tag != new_tag)
			taglist_free_divetag(new_tag);
	}
	return ret_tag;
}

void taglist_free(struct tag_entry *entry)
{
	STRUCTURED_LIST_FREE(struct tag_entry, entry, free)
}

struct tag_entry *taglist_copy(struct tag_entry *s)
{
	struct tag_entry *res;
	STRUCTURED_LIST_COPY(struct tag_entry, s, res, copy_tl);
	return res;
}

/* Merge src1 and src2, write to *dst */
void taglist_merge(struct tag_entry **dst, struct tag_entry *src1, struct tag_entry *src2)
{
	struct tag_entry *entry;

	for (entry = src1; entry; entry = entry->next)
		taglist_add_divetag(dst, entry->tag);
	for (entry = src2; entry; entry = entry->next)
		taglist_add_divetag(dst, entry->tag);
}

void taglist_init_global()
{
	size_t i;

	for (i = 0; i < sizeof(default_tags) / sizeof(char *); i++)
		taglist_add_tag(&g_tag_list, default_tags[i]);
}
