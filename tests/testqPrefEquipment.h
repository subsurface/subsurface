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
	void test_tank_info_table_defaults_without_customisation();
	void test_tank_info_table_persists_across_restart();
	void test_tank_info_table_negative_count_falls_back_to_defaults();
	void test_tank_info_table_malformed_count_falls_back_to_defaults();
	void test_tank_info_table_close_without_changes_leaves_settings_unchanged();
	void test_tank_info_table_defaults_unchecked_does_not_overwrite_baseline();
};

#endif // TESTQPREFEQUIPMENT_H
