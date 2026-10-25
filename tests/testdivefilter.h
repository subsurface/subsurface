// SPDX-License-Identifier: GPL-2.0
#ifndef TESTDIVEFILTER_H
#define TESTDIVEFILTER_H

#include "testbase.h"

class TestDiveFilter : public TestBase {
	Q_OBJECT
private slots:
	void testDiveSiteFilterInvalidDives();
};

#endif // TESTDIVEFILTER_H
