// SPDX-License-Identifier: GPL-2.0
#ifndef TESTRENUMBER_H
#define TESTRENUMBER_H

#include "testbase.h"

class TestRenumber : public TestBase {
	Q_OBJECT
private slots:
	void setup();
	void testMerge();
	void testMergeAndAppend();
};

#endif
