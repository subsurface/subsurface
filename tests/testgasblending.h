// SPDX-License-Identifier: GPL-2.0
#ifndef TESTGASBLENDER_H
#define TESTGASBLENDER_H

#include "testbase.h"

class TestGasBlender : public TestBase {
	Q_OBJECT
private slots:
	void setup();
	void testIntersection();
	void test2DInterpolation();
	void testGetCompressibilityFactor();
	void testCalculateCylinderVolumes();
	void testSign();
	void testTriangularInterpolation();
	void testPointInTriangle();
	void testCalculateSourceMix();
	void testBlendStruct();
	void testCorrectPressures();
	void testCalculatePressure();
	void testInterpolateBlends();
	void testCalculateBlends();
	void testConvexHull();
	void testGasSourceTriangle();
	void testGenerateExhaustiveBlends();
};

#endif
