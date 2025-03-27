// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFUPDATEMANAGER_H
#define TESTQPREFUPDATEMANAGER_H

#include "testbase.h"

class TestQPrefUpdateManager : public TestBase {
	Q_OBJECT

private slots:
	void initTestCase();
	void test_struct_get();
	void test_set_struct();
	void test_set_load_struct();
	void test_struct_disk();
	void test_multiple();
	void test_oldPreferences();
	void test_next_check();
	void test_signals();
};

#endif // TESTQPREFUPDATEMANAGER_H
