// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPLANNERSHARED_H
#define TESTPLANNERSHARED_H

#include "testbase.h"

class TestPlannerShared : public TestBase {
	Q_OBJECT

private slots:
	void initTestCase();

	// test case grouping correspond to panels diveplanner window
	void test_planning();
	void test_gas();
	void test_notes();
};

#endif // TESTPLANNERSHARED_H
