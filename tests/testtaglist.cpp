// SPDX-License-Identifier: GPL-2.0
#include "testtaglist.h"
#include "core/dive.h"
#include "core/tag.h"

void TestTagList::initTestCase()
{
	TestBase::initTestCase();

	taglist_init_global();
}

void TestTagList::cleanupTestCase()
{
	g_tag_list.clear();
}

void TestTagList::testGetTagstringNoTags()
{
	tag_list tags;
	std::string tagstring = taglist_get_tagstring(tags);
	QVERIFY(tagstring.empty());
}

void TestTagList::testGetTagstringSingleTag()
{
	tag_list tags;
	taglist_add_tag(tags, "A new tag");
	std::string tagstring = taglist_get_tagstring(tags);
	QCOMPARE(QString::fromStdString(tagstring), QString::fromUtf8("A new tag"));
}

void TestTagList::testGetTagstringMultipleTags()
{
	tag_list tags;
	taglist_add_tag(tags, "A new tag");
	taglist_add_tag(tags, "A new tag 1");
	taglist_add_tag(tags, "A new tag 2");
	taglist_add_tag(tags, "A new tag 3");
	taglist_add_tag(tags, "A new tag 4");
	taglist_add_tag(tags, "A new tag 5");
	std::string tagstring = taglist_get_tagstring(tags);
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
	tag_list tags;
	taglist_add_tag(tags, "A new tag");
	taglist_add_tag(tags, "A new tag 1");
	taglist_add_tag(tags, "");
	std::string tagstring = taglist_get_tagstring(tags);
	QCOMPARE(QString::fromStdString(tagstring),
		 QString::fromUtf8(
			 "A new tag, "
			 "A new tag 1"));
}

void TestTagList::testGetTagstringEmptyTagOnly()
{
	tag_list tags;
	taglist_add_tag(tags, "");
	std::string tagstring = taglist_get_tagstring(tags);
	QCOMPARE(QString::fromStdString(tagstring),
		 QString::fromUtf8(""));
}

void TestTagList::testMergeTags()
{
	tag_list tags1, tags2;
	taglist_add_tag(tags1, "A new tag");
	taglist_add_tag(tags1, "A new tag 6");
	taglist_add_tag(tags1, "A new tag 1");
	taglist_add_tag(tags1, "A new tag 2");
	taglist_add_tag(tags1, "");
	taglist_add_tag(tags1, "A new tag 2");
	taglist_add_tag(tags1, "A new tag 3");
	taglist_add_tag(tags1, "A new tag");
	taglist_add_tag(tags2, "");
	taglist_add_tag(tags2, "A new tag 1");
	taglist_add_tag(tags2, "A new tag 4");
	taglist_add_tag(tags2, "A new tag 2");
	taglist_add_tag(tags2, "A new tag 5");
	tag_list tags3 = taglist_merge(tags1, tags2);
	std::string tagstring = taglist_get_tagstring(tags3);
	QCOMPARE(QString::fromStdString(tagstring),
		 QString::fromUtf8(
			 "A new tag, "
			 "A new tag 1, "
			 "A new tag 2, "
			 "A new tag 3, "
			 "A new tag 4, "
			 "A new tag 5, "
			 "A new tag 6"));
}

QTEST_GUILESS_MAIN(TestTagList)
