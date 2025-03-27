// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPARSEPERFORMANCE_H
#define TESTPARSEPERFORMANCE_H

#include "testbase.h"

class TestParsePerformance : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void init();
	void cleanup();

	void parseSsrf();
	void parseGit();
};

#endif
