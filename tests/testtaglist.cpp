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
	taglist_free(g_tag_list);
	g_tag_list = NULL;
}

void TestTagList::testGetTagstringNoTags()
{
	struct tag_entry *tag_list = NULL;
	char *tagstring = taglist_get_tagstring(tag_list);
	QVERIFY(tagstring != NULL);
	QCOMPARE(*tagstring, '\0');
}

void TestTagList::testGetTagstringSingleTag()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "A new tag");
	char *tagstring = taglist_get_tagstring(tag_list);
	QVERIFY(tagstring != NULL);
	QCOMPARE(QString::fromUtf8(tagstring), QString::fromUtf8("A new tag"));
	free(tagstring);
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
	char *tagstring = taglist_get_tagstring(tag_list);
	QVERIFY(tagstring != NULL);
	QCOMPARE(QString::fromUtf8(tagstring),
		 QString::fromUtf8(
			 "A new tag, "
			 "A new tag 1, "
			 "A new tag 2, "
			 "A new tag 3, "
			 "A new tag 4, "
			 "A new tag 5"));
	free(tagstring);
}

void TestTagList::testGetTagstringWithAnEmptyTag()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "A new tag");
	taglist_add_tag(&tag_list, "A new tag 1");
	taglist_add_tag(&tag_list, "");
	char *tagstring = taglist_get_tagstring(tag_list);
	QVERIFY(tagstring != NULL);
	QCOMPARE(QString::fromUtf8(tagstring),
		 QString::fromUtf8(
			 "A new tag, "
			 "A new tag 1"));
	free(tagstring);
}

void TestTagList::testGetTagstringEmptyTagOnly()
{
	struct tag_entry *tag_list = NULL;
	taglist_add_tag(&tag_list, "");
	char *tagstring = taglist_get_tagstring(tag_list);
	QVERIFY(tagstring != NULL);
	QCOMPARE(QString::fromUtf8(tagstring),
		 QString::fromUtf8(""));
	free(tagstring);
}

QTEST_GUILESS_MAIN(TestTagList)
