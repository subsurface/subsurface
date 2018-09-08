// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDisplay.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDisplay.h"

#include <QDate>
#include <QTest>
#include <QSignalSpy>

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

	prefs.animation_speed = 17;
	prefs.display_invalid_dives = true;
	prefs.divelist_font = copy_qstring("comic");
	prefs.font_size = 12.0;
	prefs.show_developer = false;

	QCOMPARE(display->animation_speed(), prefs.animation_speed);
	QCOMPARE(display->display_invalid_dives(), prefs.display_invalid_dives);
	QCOMPARE(display->divelist_font(), QString(prefs.divelist_font));
	QCOMPARE(display->font_size(), prefs.font_size);
	QCOMPARE(display->show_developer(), prefs.show_developer);
}

void TestQPrefDisplay::test_set_struct()
{
	// Test set func -> struct pref

	auto display = qPrefDisplay::instance();

	display->set_animation_speed(27);
	display->set_display_invalid_dives(false);
	display->set_divelist_font("doNotCareAtAll");
	display->set_font_size(12.0);
	display->set_show_developer(false);
	display->set_theme("myTheme");
	display->set_lastDir("test1");
	display->set_tooltip_position(QPointF(512, 3));
	display->set_UserSurvey("my1");
	display->set_mainSplitter("main1");
	display->set_topSplitter("top1");
	display->set_bottomSplitter("bottom1");
	display->set_maximized(false);
	display->set_geometry("geo1");
	display->set_windowState("win1");
	display->set_lastState(17);

	QCOMPARE(prefs.animation_speed, 27);
	QCOMPARE(prefs.display_invalid_dives, false);
	QCOMPARE(prefs.divelist_font, "doNotCareAtAll");
	QCOMPARE(prefs.font_size, 12.0);
	QCOMPARE(prefs.show_developer, false);
	QCOMPARE(display->theme(), QString("myTheme"));
	QCOMPARE(display->lastDir(), QString("test1"));
	QCOMPARE(display->tooltip_position(), QPointF(512, 3));
	QCOMPARE(display->UserSurvey(), QString("my1"));
	QCOMPARE(display->mainSplitter(), QByteArray("main1"));
	QCOMPARE(display->topSplitter(), QByteArray("top1"));
	QCOMPARE(display->bottomSplitter(), QByteArray("bottom1"));
	QCOMPARE(display->maximized(), false);
	QCOMPARE(display->geometry(), QByteArray("geo1"));
	QCOMPARE(display->windowState(), QByteArray("win1"));
	QCOMPARE(display->lastState(), 17);
}

void TestQPrefDisplay::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto display = qPrefDisplay::instance();

	display->set_animation_speed(33);
	display->set_display_invalid_dives(true);
	display->set_divelist_font("doNotCareString");
	display->set_font_size(15.0);
	display->set_show_developer(true);
	display->set_theme("myTheme2");
	display->set_lastDir("test2");
	display->set_tooltip_position(QPointF(612, 3));
	display->set_UserSurvey("my2");
	display->set_mainSplitter("main2");
	display->set_topSplitter("top2");
	display->set_bottomSplitter("bottom2");
	display->set_maximized(true);
	display->set_geometry("geo2");
	display->set_windowState("win2");
	display->set_lastState(27);

	prefs.animation_speed = 17;
	prefs.display_invalid_dives = false;
	prefs.divelist_font = copy_qstring("doNotCareAtAll");
	prefs.font_size = 12.0;
	prefs.show_developer = false;

	display->load();
	QCOMPARE(prefs.animation_speed, 33);
	QCOMPARE(prefs.display_invalid_dives, true);
	QCOMPARE(prefs.divelist_font, "doNotCareString");
	QCOMPARE(prefs.font_size, 15.0);
	QCOMPARE(prefs.show_developer, true);
	QCOMPARE(display->theme(), QString("myTheme2"));
	QCOMPARE(display->lastDir(), QString("test2"));
	QCOMPARE(display->tooltip_position(), QPointF(612, 3));
	QCOMPARE(display->UserSurvey(), QString("my2"));
	QCOMPARE(display->mainSplitter(), QByteArray("main2"));
	QCOMPARE(display->topSplitter(), QByteArray("top2"));
	QCOMPARE(display->bottomSplitter(), QByteArray("bottom2"));
	QCOMPARE(display->maximized(), true);
	QCOMPARE(display->geometry(), QByteArray("geo2"));
	QCOMPARE(display->windowState(), QByteArray("win2"));
	QCOMPARE(display->lastState(), 27);
}

void TestQPrefDisplay::test_struct_disk()
{
	// test struct prefs -> disk

	auto display = qPrefDisplay::instance();

	prefs.animation_speed = 27;
	prefs.display_invalid_dives = true;
	prefs.divelist_font = copy_qstring("doNotCareAtAll");
	prefs.font_size = 17.0;
	prefs.show_developer = false;

	display->sync();
	prefs.animation_speed = 35;
	prefs.display_invalid_dives = false;
	prefs.divelist_font = copy_qstring("noString");
	prefs.font_size = 11.0;
	prefs.show_developer = true;

	display->load();
	QCOMPARE(prefs.animation_speed, 27);
	QCOMPARE(prefs.display_invalid_dives, true);
	QCOMPARE(prefs.divelist_font, "doNotCareAtAll");
	QCOMPARE(prefs.font_size, 17.0);
	QCOMPARE(prefs.show_developer, false);
}

void TestQPrefDisplay::test_multiple()
{
	// test multiple instances have the same information
	auto display_direct = qPrefDisplay::instance();
	prefs.divelist_font = copy_qstring("comic");

	auto display = qPrefDisplay::instance();
	prefs.font_size = 15.0;

	QCOMPARE(display->divelist_font(), display_direct->divelist_font());
	QCOMPARE(display->divelist_font(), QString("comic"));
	QCOMPARE(display->font_size(), display_direct->font_size());
	QCOMPARE(display->font_size(), 15.0);
}

QTEST_MAIN(TestQPrefDisplay)
