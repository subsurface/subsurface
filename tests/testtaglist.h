// SPDX-License-Identifier: GPL-2.0
#ifndef TESTTAGLIST_H
#define TESTTAGLIST_H

#include <QtTest>

class TestTagList : public QObject {
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();

	void testGetTagstringNoTags();
	void testGetTagstringSingleTag();
	void testGetTagstringMultipleTags();
	void testGetTagstringWithAnEmptyTag();
	void testGetTagstringEmptyTagOnly();
	void testMergeTags();
};

#endif
