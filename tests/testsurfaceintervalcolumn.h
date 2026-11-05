// SPDX-License-Identifier: GPL-2.0
#ifndef TESTSURFACEINTERVALCOLUMN_H
#define TESTSURFACEINTERVALCOLUMN_H

#include "testbase.h"

class TestSurfaceIntervalColumn : public TestBase {
	Q_OBJECT
private slots:
	void init();
	void cleanup();
	void testLookupMatchesLinearSearch();
	void testLookupWithManyIdenticalStartTimes();
	void testLookupValues();
	void testLookupForDiveNotInTable();
	void testColumnPosition();
	void testListModelCells();
	void testTreeModelCells();
	void testWholeLogWithHiddenDive();
	void testListSorting();
	void testSuccessorRefreshList();
	void testSuccessorRefreshTree();
	void testSuccessorRefreshAddDeleteMoveList();
	void testSuccessorRefreshAddDeleteMoveTree();
};

#endif // TESTSURFACEINTERVALCOLUMN_H
