// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef TESTFITEXPORT_H
#define TESTFITEXPORT_H

#include "testbase.h"

class TestFitExport : public TestBase {
	Q_OBJECT
private slots:
	void testRoundTrip();
	void testNoSamplesFails();
	void testManufacturerMapping();
	void testTzResolution();
	void testTzOverrideAffectsBytes();
	void testGoldenByteVectors();
};

#endif
