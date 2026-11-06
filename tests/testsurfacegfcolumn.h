// SPDX-License-Identifier: GPL-2.0
#ifndef TESTSURFACEGFCOLUMN_H
#define TESTSURFACEGFCOLUMN_H
#include "testbase.h"

class TestSurfaceGfColumn : public TestBase {
	Q_OBJECT
private slots:
	void init();
	void cleanup();
	void testDisplaySortAndRefresh();
	void testHiddenDiveAndTree();
	void testAddDeleteAndTimeShift();
	void testDecoModelTooltip();
};
#endif
