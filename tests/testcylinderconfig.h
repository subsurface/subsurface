// SPDX-License-Identifier: GPL-2.0
#ifndef TESTCYLINDERCONFIG_H
#define TESTCYLINDERCONFIG_H

#include "testbase.h"

class TestCylinderConfig : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanup();

	void testEnsureO2CylinderAddsForCcr();
	void testEnsureO2CylinderSkipsForOc();
	void testEnsureO2CylinderIdempotent();
	void testParsedCcrDiveGetsO2Cylinder();
};

#endif
