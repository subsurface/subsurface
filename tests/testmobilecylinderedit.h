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
// See mobile-widgets/notes/mobile-cylinder-edit-index-mapping.md for the
// full design rationale.
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
};

#endif // TESTMOBILECYLINDEREDIT_H
