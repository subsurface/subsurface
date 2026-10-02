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
	void testSurfaceAirCylinderDataAccess();
	void testImportMissingCylinderDepth();
	void testImportedCylinderDepthPreserved();
	void testCylinderDepthInput();
	void testStoredZeroCylinderDepthDisplay();
	// AI-generated (Claude)
	void testDecoSwitchDepthValidation();
	// AI-generated (Claude)
	void testZeroDepthExcludesDecoGas();
	// AI-generated (Claude)
	void testRecreationalPlanSaveAllowed();
	// AI-generated (Claude)
	void testMobilePlannerSurfacePressureRetained();
	// AI-generated (Claude)
	void testMobilePlannerSurfacePressureRetainedWithSave();
	// AI-generated (Claude)
	void testMobilePlannerAboveSeaLevelPressure();
};

#endif // TESTDIVEPLANNERMODEL_H
