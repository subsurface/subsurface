// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFEQUIPMENT_H
#define TESTQPREFEQUIPMENT_H

#include "testbase.h"

class TestQPrefEquipment : public TestBase {
	Q_OBJECT

private slots:
	void initTestCase();
	void test_struct_get();
	void test_set_struct();
	void test_set_load_struct();
	void test_struct_disk();
	void test_oldPreferences();
	void test_signals();
};

#endif // TESTQPREFEQUIPMENT_H
