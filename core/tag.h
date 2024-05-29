// SPDX-License-Identifier: GPL-2.0
// Dive tag related structures and helpers
#ifndef TAG_H
#define TAG_H

#include <memory>
#include <string>
#include <vector>
#include <string>

struct divetag {
	/*
	 * The name of the divetag. If a translation is available, name contains
	 * the translated tag
	 */
	std::string name;
	/*
	 * If a translation is available, we write the original tag to source.
	 * This enables us to write a non-localized tag to the xml file.
	 */
	std::string source;
	divetag(std::string name, std::string source);
};

using tag_list = std::vector<const divetag *>;

void taglist_add_tag(tag_list &list, const std::string &tag);

/* cleans up a list: removes empty tags and duplicates */
void taglist_cleanup(tag_list &list);

void taglist_init_global();
tag_list taglist_merge(const tag_list &src1, const tag_list &src2);

/*
 * divetags are only stored once, each dive only contains
 * a list of tag_entries which then point to the divetags
 * in the global g_tag_list
 */
extern std::vector<std::unique_ptr<divetag>> g_tag_list;

/* Comma separated list of tags names or empty string */
std::string taglist_get_tagstring(const tag_list &tags);

#endif
