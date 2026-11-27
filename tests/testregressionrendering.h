// SPDX-License-Identifier: GPL-2.0
// AI-generated (GPT-6 Sol)
#ifndef TEST_REGRESSION_RENDERING_H
#define TEST_REGRESSION_RENDERING_H

#include "testbase.h"

class TestRegressionRendering : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase() { TestBase::initTestCase(); }
	void twoPointsAndFeatures();
	void confidenceAndClipping();
	void invalidGeometry();
};

#endif
