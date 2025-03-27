// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFDIVECOMPUTER_H
#define TESTQPREFDIVECOMPUTER_H

#include "testbase.h"

class TestQPrefDiveComputer : public TestBase {
	Q_OBJECT

private slots:
	void initTestCase();
	void test_struct_get();
	void test_set_struct();
	void test_set_load_struct();
	void test_struct_disk();
	void test_multiple();
	void test_oldPreferences();
	void test_signals();
};

#endif // TESTQPREFDIVECOMPUTER_H
