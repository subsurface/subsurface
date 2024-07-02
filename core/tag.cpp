// SPDX-License-Identifier: GPL-2.0

#include "tag.h"
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

divetag::divetag(std::string name, std::string source) :
	name(std::move(name)), source(std::move(source))
{
}

/* remove duplicates and empty tags */
void taglist_cleanup(tag_list &list)
{
	// Remove empty tags
	list.erase(std::remove_if(list.begin(), list.end(), [](const divetag *tag) { return tag->name.empty(); }),
		   list.end());

	// Sort (should be a NOP, because we add in a sorted way, but let's make sure)
	std::sort(list.begin(), list.end());

	// Remove duplicates
	list.erase(std::unique(list.begin(), list.end(),
			       [](const divetag *tag1, const divetag *tag2) { return tag1->name == tag2->name; }),
		   list.end());
}

std::string taglist_get_tagstring(const tag_list &list)
{
	std::string res;
	for (const divetag *tag: list) {
		if (tag->name.empty())
			continue;
		if (!res.empty())
			res += ", ";
		res += tag->name;
	}
	return res;
}

/* Add a tag to the tag_list, keep the list sorted */
static void taglist_add_divetag(tag_list &list, const struct divetag *tag)
{
	// Use binary search to enter at sorted position
	auto it = std::lower_bound(list.begin(), list.end(), tag,
				   [](const struct divetag *tag1, const struct divetag *tag2)
				   { return tag1->name < tag2->name; });
	// Don't add if it already exists
	if (it == list.end() || (*it)->name != tag->name)
		list.insert(it, tag);
}

static const divetag *register_tag(std::string s, std::string source)
{
	// binary search
	auto it = std::lower_bound(g_tag_list.begin(), g_tag_list.end(), s,
				   [](const std::unique_ptr<divetag> &tag, const std::string &s)
				   { return tag->name < s; });
	if (it == g_tag_list.end() || (*it)->name != s)
		it = g_tag_list.insert(it, std::make_unique<divetag>(std::move(s), std::move(source)));
	return it->get();
}

void taglist_add_tag(tag_list &list, const std::string &tag)
{
	bool is_default_tag = std::find_if(std::begin(default_tags), std::end(default_tags),
			[&tag] (const char *default_tag) { return tag == default_tag; });

	/* Only translate default tags */
	/* TODO: Do we really want to translate user-supplied tags if they happen to be known!? */
	std::string translation = is_default_tag ? translate("gettextFromC", tag.c_str()) : tag;
	std::string source = is_default_tag ? tag : std::string();
	const struct divetag *d_tag = register_tag(std::move(translation), std::move(source));

	taglist_add_divetag(list, d_tag);
}

/* Merge src1 and src2, write to *dst */
tag_list taglist_merge(const tag_list &src1, const tag_list &src2)
{
	tag_list dst;

	for (const divetag *t: src1)
		taglist_add_divetag(dst, t);
	for (const divetag *t: src2)
		taglist_add_divetag(dst, t);
	return dst;
}

void taglist_init_global()
{
	for (const char *s: default_tags)
		register_tag(translate("gettextFromC", s), s);
}
