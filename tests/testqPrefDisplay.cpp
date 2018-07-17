// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDisplay.h"

#include "core/settings/qPref.h"
#include "core/pref.h"
#include "core/qthelper.h"

#include <QDate>
#include <QTest>

void TestQPrefDisplay::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDisplay");
}

void TestQPrefDisplay::test_struct_get()
{
	// Test struct pref -> get func.

	auto display = qPrefDisplay::instance();

	prefs.display_invalid_dives = true;
	prefs.divelist_font = copy_qstring("comic");
	prefs.font_size = 12.0;
	prefs.show_developer = false;
 	prefs.theme = copy_qstring("myTheme");

	QCOMPARE(display->display_invalid_dives(), prefs.display_invalid_dives);
	QCOMPARE(display->divelist_font(), QString(prefs.divelist_font));
	QCOMPARE(display->font_size(), prefs.font_size);
	QCOMPARE(display->show_developer(), prefs.show_developer);
	QCOMPARE(display->theme(), QString(prefs.theme));
}

void TestQPrefDisplay::test_set_struct()
{
	// Test set func -> struct pref

	auto display = qPrefDisplay::instance();

	display->set_display_invalid_dives(true);
	display->set_divelist_font("doNotCareAtAll");
	display->set_font_size(12.0);
	display->set_show_developer(false);
 	display->set_theme("myTheme");

	QCOMPARE(prefs.display_invalid_dives, true);
	QCOMPARE(prefs.divelist_font, "doNotCareAtAll");
	QCOMPARE(prefs.font_size, 12.0);
	QCOMPARE(prefs.show_developer, false);
	QCOMPARE(prefs.theme, "myTheme");
}

void TestQPrefDisplay::test_set_load_struct()
{
	// test set func -> load -> struct pref 

	auto display = qPrefDisplay::instance();

	display->set_display_invalid_dives(false);
	display->set_divelist_font("doNotCareString");
	display->set_font_size(15.0);
	display->set_show_developer(true);
 	display->set_theme("myTheme2");

	prefs.display_invalid_dives = true;
	prefs.divelist_font = copy_qstring("doNotCareAtAll");
	prefs.font_size = 12.0;
	prefs.show_developer = false;
 	prefs.theme = copy_qstring("myTheme");

	display->load();
	QCOMPARE(prefs.display_invalid_dives, false);
	QCOMPARE(prefs.divelist_font, "doNotCareString");
	QCOMPARE(prefs.font_size, 15.0);
	QCOMPARE(prefs.show_developer, true);
	QCOMPARE(prefs.theme, "myTheme2");
}

void TestQPrefDisplay::test_struct_disk()
{
	// test struct prefs -> disk

	auto display = qPrefDisplay::instance();

	prefs.display_invalid_dives = true;
	prefs.divelist_font = copy_qstring("doNotCareAtAll");
	prefs.font_size = 17.0;
	prefs.show_developer = false;
 	prefs.theme = copy_qstring("myTheme3");

	display->sync();
	prefs.display_invalid_dives = false;
	prefs.divelist_font = copy_qstring("noString");
	prefs.font_size = 11.0;
	prefs.show_developer = true;
 	prefs.theme = copy_qstring("myTheme");

	display->load();
	QCOMPARE(prefs.display_invalid_dives, true);
	QCOMPARE(prefs.divelist_font, "doNotCareAtAll");
	QCOMPARE(prefs.font_size, 17.0);
	QCOMPARE(prefs.show_developer, false);
	QCOMPARE(prefs.theme, "myTheme3");
}

void TestQPrefDisplay::test_multiple()
{
	// test multiple instances have the same information

	prefs.display_invalid_dives = false;
	prefs.divelist_font = copy_qstring("comic");
	prefs.font_size = 11.0;
	prefs.show_developer = true;
 	prefs.theme = copy_qstring("myTheme");
	auto display_direct = new qPrefDisplay;

	prefs.display_invalid_dives = true;
	prefs.divelist_font = copy_qstring("multipleCharsInString");
	prefs.font_size = 15.0;
	prefs.show_developer = false;
 	prefs.theme = copy_qstring("myTheme8");
	auto display = qPrefDisplay::instance();

	QCOMPARE(display->display_invalid_dives(), display_direct->display_invalid_dives());
	QCOMPARE(display->divelist_font(), display_direct->divelist_font());
	QCOMPARE(display->font_size(), display_direct->font_size());
	QCOMPARE(display->show_developer(), display_direct->show_developer());
	QCOMPARE(display->theme(), display_direct->theme());
}

QTEST_MAIN(TestQPrefDisplay)
