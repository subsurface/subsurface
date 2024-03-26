// SPDX-License-Identifier: GPL-2.0
#include "testtaglist.h"
#include "core/dive.h"
#include "core/tag.h"

void TestTagList::initTestCase()
{
	taglist_init_global();
}

void TestTagList::cleanupTestCase()
{
	g_tag_list.clear();
}

void TestTagList::testGetTagstringNoTags()
{
	struct tag_entry *tag_list = NULL;
	std::string tagstring = taglist_get_tagstring(tag_list);
	QVERIFY(tagstring.empty());
}

void TestTagList::testGetTagstringSingleTag()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "A new tag");
	std::string tagstring = taglist_get_tagstring(tag_list);
	QCOMPARE(QString::fromStdString(tagstring), QString::fromUtf8("A new tag"));
}

void TestTagList::testGetTagstringMultipleTags()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "A new tag");
	taglist_add_tag(&tag_list, "A new tag 1");
	taglist_add_tag(&tag_list, "A new tag 2");
	taglist_add_tag(&tag_list, "A new tag 3");
	taglist_add_tag(&tag_list, "A new tag 4");
	taglist_add_tag(&tag_list, "A new tag 5");
	std::string tagstring = taglist_get_tagstring(tag_list);
	QCOMPARE(QString::fromStdString(tagstring),
		 QString::fromUtf8(
			 "A new tag, "
			 "A new tag 1, "
			 "A new tag 2, "
			 "A new tag 3, "
			 "A new tag 4, "
			 "A new tag 5"));
}

void TestTagList::testGetTagstringWithAnEmptyTag()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "A new tag");
	taglist_add_tag(&tag_list, "A new tag 1");
	taglist_add_tag(&tag_list, "");
	std::string tagstring = taglist_get_tagstring(tag_list);
	QCOMPARE(QString::fromStdString(tagstring),
		 QString::fromUtf8(
			 "A new tag, "
			 "A new tag 1"));
}

void TestTagList::testGetTagstringEmptyTagOnly()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "");
	std::string tagstring = taglist_get_tagstring(tag_list);
	QCOMPARE(QString::fromStdString(tagstring),
		 QString::fromUtf8(""));
}

QTEST_GUILESS_MAIN(TestTagList)
