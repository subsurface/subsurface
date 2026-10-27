// SPDX-License-Identifier: GPL-2.0
#ifndef TESTCYLINDERSMODELSENSORREFRESH_H
#define TESTCYLINDERSMODELSENSORREFRESH_H

#include "testbase.h"

class TestCylindersModelSensorRefresh : public TestBase {
	Q_OBJECT
private slots:
	void testDiveComputerEditedRefreshesOwnDive();
	void testDiveComputerEditedIgnoresOtherDive();
};

#endif
