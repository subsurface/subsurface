// SPDX-License-Identifier: GPL-2.0
#ifndef TESTTANKINFOMODEL_H
#define TESTTANKINFOMODEL_H

#include "testbase.h"

// Regression / unit tests for the cylinder-type management operations
// (add / rename / resize / remove) added to TankInfoModel to back the
// "manage cylinder types" preferences editor. There is no manual reordering
// (removed as adding no real UX value): the list is instead always kept
// sorted alphabetically by name, automatically.
class TestTankInfoModel : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void init();    // run before each test: reset tank_info_table to a known state
	void cleanup(); // run after each test

	void testAddInsertsEntry();
	void testAddTwiceProducesUniqueNames();
	void testAddThenSizeBeforePressureRetainsSize();
	void testRenameDescription();
	void testRenameRejectsEmpty();
	void testRenameRejectsDuplicate();
	void testResizeSize();
	void testResizeRejectsInvalidInput();
	void testResizeWorkingPressure();
	void testResizeUnchangedWorkingPressureIsNoop();
	void testResizeUnchangedValueIsNoop();
	void testResizeUnchangedCuftValueAvoidsRoundingDrift();
	void testRemoveEntry();
	void testRemoveDefaultEntryClearsDefaultCylinder();
	void testRemoveOutOfRangeIsNoop();
	void testListStaysSortedAfterAdd();
	void testRenameResortsListWhenOrderChanges();
	void testSortIsCaseInsensitive();
	void testEditsPersistAcrossRestart();
	void testRenameCascadesToDiveCylinders();

	void testAddMirrorsIntoBaseline();
	void testRemoveMirrorsIntoBaselineOnlyIfPresent();
	void testResizeMirrorsIntoBaselineOnlyIfPresent();
	void testFileOnlyTypeNotPromotedToBaseline();

	void testRenameUndoRedoKeepsTableAndBaselineInSync();
	void testRenameFileOnlyTypeLeavesBaselineUntouched();
	void testRenameUndoPersistsRevertedNameImmediately();

	void testAddAcrossReloadNeverDuplicatesBaselineName();
	void testRenameRejectsNameOnlyInBaseline();

	void testRenameEmitsCylinderTypeRenamedSignal();
};

#endif
