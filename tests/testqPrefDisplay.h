// SPDX-License-Identifier: GPL-2.0
#ifndef TESTQPREFDISPLAY_H
#define TESTQPREFDISPLAY_H

#include "testbase.h"

#include <QPalette>
#include <QString>

class TestQPrefDisplay : public TestBase {
	Q_OBJECT
private slots:
	void initTestCase();
	void test_struct_get();
	void test_set_struct();
	void test_set_load_struct();
	void test_struct_disk();
	void test_multiple();
	void test_signals();
#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	void test_color_scheme();
	void test_color_scheme_palette();
	void test_color_scheme_startup();
#endif

private:
#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	QPalette systemPalette;
	QString systemStyle;
#endif
};

#endif // TESTQPREFDISPLAY_H
