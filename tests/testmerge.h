// SPDX-License-Identifier: GPL-2.0
#ifndef TESTMERGE_H
#define TESTMERGE_H

#include "testbase.h"

class TestMerge : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanup();

	void testMergeEmpty();
	void testMergeBackwards();
};

#endif
