// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#ifndef TESTMOBILECYLINDEREDIT_H
#define TESTMOBILECYLINDEREDIT_H

#include "testbase.h"

// Regression tests for the mobile cylinder-edit index-mapping fix.
//
// These tests exercise the core cylinder write logic at the dive level,
// independent of QMLManager and the UI.  They verify that cylinder data
// (type description, start/end pressures, gas mix) is correctly written for
// dives where no cylinder was previously "used" (no pressure delta, no
// gas-change events).
//
class TestMobileCylinderEdit : public TestBase {
	Q_OBJECT
private slots:
	// TC1: zero-cylinder dive -- adding a cylinder on edit must persist.
	void test_zero_cylinder_dive_edit_adds_cylinder();

	// TC2: unused-cylinder dive -- editing an existing but unused cylinder
	// must persist (regression against the old is_cylinder_used guard).
	void test_unused_cylinder_dive_edit_persists();

	// TC3: two-cylinder dive with gas-change event -- editing pressures must
	// not conflate cylinder slots (guards against #2913 regression).
	void test_multi_cylinder_index_mapping_preserved();

	// TC4: notes-only edit of zero-cylinder dive must not create a spurious
	// cylinder (regression against the [""] normalisation fix).
	void test_notes_only_edit_does_not_create_cylinder();

	// TC5: multi-cylinder dive with a gap (unused cylinder in the middle).
	// QML compressed list index must map to the correct physical cylinder,
	// skipping the unused slot.
	void test_multi_cylinder_gap_mapping();
};

#endif // TESTMOBILECYLINDEREDIT_H
