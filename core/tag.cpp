// SPDX-License-Identifier: GPL-2.0

#include "tag.h"
#include "structured_list.h"
#include "subsurface-string.h"
#include "membuffer.h"
#include "gettext.h"

#include <stdlib.h>
#include <algorithm>
#include <QtGlobal> // for QT_TRANSLATE_NOOP

std::vector<std::unique_ptr<divetag>> g_tag_list;

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
	dt->tag = st->tag;
}

static bool tag_seen_before(struct tag_entry *start, struct tag_entry *before)
{
	while (start && start != before) {
		if (start->tag->name == before->tag->name)
			return true;
		start = start->next;
	}
	return false;
}

/* remove duplicates and empty nodes */
extern "C" void taglist_cleanup(struct tag_entry **tag_list)
{
	struct tag_entry **tl = tag_list;
	while (*tl) {
		/* skip tags that are empty or that we have seen before */
		if ((*tl)->tag->name.empty() || tag_seen_before(*tag_list, *tl)) {
			*tl = (*tl)->next;
			continue;
		}
		tl = &(*tl)->next;
	}
}

std::string taglist_get_tagstring(struct tag_entry *tag_list)
{
	bool first_tag = true;
	std::string res;
	for (struct tag_entry *tmp = tag_list; tmp != NULL; tmp = tmp->next) {
		if (tmp->tag->name.empty())
			continue;
		if (!first_tag)
			res += ", ";
		res += tmp->tag->name;
		first_tag = false;
	}
	return res;
}

/* Add a tag to the tag_list, keep the list sorted */
static void taglist_add_divetag(struct tag_entry **tag_list, const struct divetag *tag)
{
	struct tag_entry *next, *entry;

	while ((next = *tag_list) != NULL) {
		int cmp = next->tag->name.compare(tag->name);

		/* Already have it? */
		if (!cmp)
			return;
		/* Is the entry larger? If so, insert here */
		if (cmp > 0)
			break;
		/* Continue traversing the list */
		tag_list = &next->next;
	}

	/* Insert in front of it */
	entry = (tag_entry *)malloc(sizeof(struct tag_entry));
	entry->next = next;
	entry->tag = tag;
	*tag_list = entry;
}

static const divetag *register_tag(const char *s, const char *source)
{
	// binary search
	auto it = std::lower_bound(g_tag_list.begin(), g_tag_list.end(), s,
				   [](const std::unique_ptr<divetag> &tag, const char *s)
				   { return tag->name < s; });
	if (it == g_tag_list.end() || (*it)->name != s) {
		std::string source_s = empty_string(source) ? std::string() : std::string(source);
		it = g_tag_list.insert(it, std::make_unique<divetag>(s, source));
	}
	return it->get();
}

extern "C" void taglist_add_tag(struct tag_entry **tag_list, const char *tag)
{
	bool is_default_tag = std::find_if(std::begin(default_tags), std::end(default_tags),
			[&tag] (const char *default_tag) { return tag == default_tag; });

	/* Only translate default tags */
	/* TODO: Do we really want to translate user-supplied tags if they happen to be known!? */
	const char *translation = is_default_tag ? translate("gettextFromC", tag) : tag;
	const char *source = is_default_tag ? tag : nullptr;
	const struct divetag *d_tag = register_tag(translation, source);

	taglist_add_divetag(tag_list, d_tag);
}

extern "C" void taglist_free(struct tag_entry *entry)
{
	STRUCTURED_LIST_FREE(struct tag_entry, entry, free)
}

extern "C" struct tag_entry *taglist_copy(struct tag_entry *s)
{
	struct tag_entry *res;
	STRUCTURED_LIST_COPY(struct tag_entry, s, res, copy_tl);
	return res;
}

/* Merge src1 and src2, write to *dst */
extern "C" void taglist_merge(struct tag_entry **dst, struct tag_entry *src1, struct tag_entry *src2)
{
	struct tag_entry *entry;

	for (entry = src1; entry; entry = entry->next)
		taglist_add_divetag(dst, entry->tag);
	for (entry = src2; entry; entry = entry->next)
		taglist_add_divetag(dst, entry->tag);
}

extern "C" void taglist_init_global()
{
	for (const char *s: default_tags)
		register_tag(translate("gettextFromC", s), s);
}
