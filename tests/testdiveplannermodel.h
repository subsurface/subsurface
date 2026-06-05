// SPDX-License-Identifier: GPL-2.0
#ifndef TESTDIVEPLANNERMODEL_H
#define TESTDIVEPLANNERMODEL_H

#include "testbase.h"

class TestDivePlannerModel : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void testEmptyModelDataAccess();
	void testEmptyModelEmitDataChanged();
	void testInvalidCylinderIndex();
	void testInvalidRowIndex();
	void testNothingModeDataAccess();
};

#endif // TESTDIVEPLANNERMODEL_H
