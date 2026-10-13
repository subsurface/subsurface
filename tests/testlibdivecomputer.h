// SPDX-License-Identifier: GPL-2.0
#ifndef TESTLIBDIVECOMPUTER_H
#define TESTLIBDIVECOMPUTER_H

#include "testbase.h"

class TestLibdivecomputer : public TestBase {
	Q_OBJECT
private slots:
	void cylinderUseFromTankUsage_data();
	void cylinderUseFromTankUsage();
};

#endif
