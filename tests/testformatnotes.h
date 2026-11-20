// SPDX-License-Identifier: GPL-2.0

#include "testbase.h"

class TestFormatNotes : public TestBase {
	Q_OBJECT
private slots:
	void test_empty();
	void test_angle_brackets();
	void test_ampersands();
	void test_literal_entity();
	void test_multiline();
	void test_special_characters_multiline_round_trip();
	void test_planner_notes();
	void test_planner_escapes_literal_angle_bracket();
};
