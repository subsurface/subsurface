// SPDX-License-Identifier: GPL-2.0
#ifndef TESTPREFERENCESDEFAULTS_H
#define TESTPREFERENCESDEFAULTS_H

#include "testbase.h"

class TestPreferencesDefaults : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	void test_color_scheme_items();
	void test_color_scheme_refresh_and_sync();
	void test_color_scheme_applies();
	void test_color_scheme_always_selectable();
#endif
};

#endif // TESTPREFERENCESDEFAULTS_H
