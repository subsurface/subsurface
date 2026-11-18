// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDisplay.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDisplay.h"

#include <QDate>
#include <QSettings>
#include <QTest>
#include <QSignalSpy>
#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QWidget>
#endif

void TestQPrefDisplay::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDisplay");

#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	// What the application looks like before any colour scheme override
	QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance());
	if (app) {
		systemPalette = QApplication::palette();
		systemStyle = QApplication::style()->name();
	}
#endif
}

void TestQPrefDisplay::test_struct_get()
{
	// Test struct pref -> get func.

	auto display = qPrefDisplay::instance();

	prefs.animation_speed = 17;
	prefs.display_invalid_dives = true;
	prefs.divelist_font = "comic";
	prefs.font_size = 12.0;
	prefs.show_developer = false;

	QCOMPARE(display->animation_speed(), prefs.animation_speed);
	QCOMPARE(display->display_invalid_dives(), prefs.display_invalid_dives);
	QCOMPARE(display->divelist_font(), QString::fromStdString(prefs.divelist_font));
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
	display->set_mainSplitter("main2");
	display->set_topSplitter("top2");
	display->set_bottomSplitter("bottom2");
	display->set_maximized(true);
	display->set_geometry("geo2");
	display->set_windowState("win2");
	display->set_lastState(27);

	prefs.animation_speed = 17;
	prefs.display_invalid_dives = false;
	prefs.divelist_font = "doNotCareAtAll";
	prefs.font_size = 12.0;
	prefs.show_developer = false;

	display->load();
#ifndef SUBSURFACE_MOBILE
	// Font size is never stored on disk, but qPref grabs the system default
	QCOMPARE(prefs.font_size, 15.0);
#endif
	QCOMPARE(prefs.animation_speed, 33);
	QCOMPARE(prefs.display_invalid_dives, true);
	QCOMPARE(prefs.divelist_font, "doNotCareString");
	QCOMPARE(prefs.show_developer, true);
	QCOMPARE(display->theme(), QString("myTheme2"));
	QCOMPARE(display->lastDir(), QString("test2"));
	QCOMPARE(display->tooltip_position(), QPointF(612, 3));
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
	prefs.divelist_font = "doNotCareAtAll";
	prefs.font_size = 17.0;
	prefs.show_developer = false;

	display->sync();
	prefs.animation_speed = 35;
	prefs.display_invalid_dives = false;
	prefs.divelist_font = "noString";
	prefs.font_size = 11.0;
	prefs.show_developer = true;

	display->load();
	QCOMPARE(prefs.animation_speed, 27);
	QCOMPARE(prefs.display_invalid_dives, true);
	QCOMPARE(prefs.divelist_font, "doNotCareAtAll");
#ifndef SUBSURFACE_MOBILE
	// Font size is never stored on disk, but qPref grabs the system default
	QCOMPARE(prefs.font_size, 17.0);
#endif
	QCOMPARE(prefs.show_developer, false);
}

void TestQPrefDisplay::test_multiple()
{
	// test multiple instances have the same information
	prefs.divelist_font = "comic";
	auto display = qPrefDisplay::instance();
	prefs.font_size = 15.0;

	QCOMPARE(display->divelist_font(), qPrefDisplay::divelist_font());
	QCOMPARE(display->divelist_font(), QString("comic"));
	QCOMPARE(display->font_size(), qPrefDisplay::font_size());
	QCOMPARE(display->font_size(), 15.0);
}

void TestQPrefDisplay::test_signals()
{
	QSignalSpy spy1(qPrefDisplay::instance(), &qPrefDisplay::animation_speedChanged);
	QSignalSpy spy2(qPrefDisplay::instance(), &qPrefDisplay::display_invalid_divesChanged);
	QSignalSpy spy3(qPrefDisplay::instance(), &qPrefDisplay::divelist_fontChanged);
	QSignalSpy spy4(qPrefDisplay::instance(), &qPrefDisplay::font_sizeChanged);
	QSignalSpy spy5(qPrefDisplay::instance(), &qPrefDisplay::show_developerChanged);
	QSignalSpy spy6(qPrefDisplay::instance(), &qPrefDisplay::themeChanged);
	QSignalSpy spy7(qPrefDisplay::instance(), &qPrefDisplay::lastDirChanged);
	QSignalSpy spy8(qPrefDisplay::instance(), &qPrefDisplay::tooltip_positionChanged);
	QSignalSpy spy10(qPrefDisplay::instance(), &qPrefDisplay::mainSplitterChanged);
	QSignalSpy spy11(qPrefDisplay::instance(), &qPrefDisplay::topSplitterChanged);
	QSignalSpy spy12(qPrefDisplay::instance(), &qPrefDisplay::bottomSplitterChanged);
	QSignalSpy spy13(qPrefDisplay::instance(), &qPrefDisplay::maximizedChanged);
	QSignalSpy spy14(qPrefDisplay::instance(), &qPrefDisplay::geometryChanged);
	QSignalSpy spy15(qPrefDisplay::instance(), &qPrefDisplay::windowStateChanged);
	QSignalSpy spy16(qPrefDisplay::instance(), &qPrefDisplay::lastStateChanged);

	qPrefDisplay::set_animation_speed(1);
	qPrefDisplay::set_display_invalid_dives(false);
	qPrefDisplay::set_divelist_font("signal doNotCareAtAll");
	qPrefDisplay::set_font_size(2.0);
	qPrefDisplay::set_show_developer(true);
	qPrefDisplay::set_theme("signal myTheme");
	qPrefDisplay::set_lastDir("signal test1");
	qPrefDisplay::set_tooltip_position(QPointF(12, 3));
	qPrefDisplay::set_mainSplitter("signal main1");
	qPrefDisplay::set_topSplitter("signal top1");
	qPrefDisplay::set_bottomSplitter("signal bottom1");
	qPrefDisplay::set_maximized(false);
	qPrefDisplay::set_geometry("signal geo1");
	qPrefDisplay::set_windowState("signal win1");
	qPrefDisplay::set_lastState(17);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);
	QCOMPARE(spy6.count(), 1);
	QCOMPARE(spy7.count(), 1);
	QCOMPARE(spy8.count(), 1);
	QCOMPARE(spy10.count(), 1);
	QCOMPARE(spy11.count(), 1);
	QCOMPARE(spy12.count(), 1);
	QCOMPARE(spy13.count(), 1);
	QCOMPARE(spy14.count(), 1);
	QCOMPARE(spy15.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toInt() == 1);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
	QVERIFY(spy3.takeFirst().at(0).toString() == "signal doNotCareAtAll");
	QVERIFY(spy4.takeFirst().at(0).toDouble() == 2.0);
	QVERIFY(spy5.takeFirst().at(0).toBool() == true);
	QVERIFY(spy6.takeFirst().at(0).toString() == "signal myTheme");
	QVERIFY(spy7.takeFirst().at(0).toString() == "signal test1");
	QVERIFY(spy8.takeFirst().at(0).toPointF() == QPointF(12, 3));
	QVERIFY(spy10.takeFirst().at(0).toByteArray() == QByteArray("signal main1"));
	QVERIFY(spy11.takeFirst().at(0).toByteArray() == QByteArray("signal top1"));
	QVERIFY(spy12.takeFirst().at(0).toByteArray() == QByteArray("signal bottom1"));
	QVERIFY(spy13.takeFirst().at(0).toBool() == false);
	QVERIFY(spy14.takeFirst().at(0).toByteArray() == QByteArray("signal geo1"));
	QVERIFY(spy15.takeFirst().at(0).toByteArray() == QByteArray("signal win1"));
}

#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
void TestQPrefDisplay::test_color_scheme()
{
	// This covers saving and reloading the preference. It does not depend on
	// the platform theme honouring the override.
	const QString key = QStringLiteral("Display/color_scheme");
	QSettings().remove(key);

	qPrefDisplay::load();
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("system"));
	qPrefDisplay::set_theme(QStringLiteral("mobileTheme"));

	QSignalSpy spy(qPrefDisplay::instance(), &qPrefDisplay::color_schemeChanged);
	qPrefDisplay::set_color_scheme(QStringLiteral("light"));
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("light"));
	QCOMPARE(QSettings().value(key).toString(), QStringLiteral("light"));
	QCOMPARE(qPrefDisplay::theme(), QStringLiteral("mobileTheme"));
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("light"));

	qPrefDisplay::set_color_scheme(QStringLiteral("dark"));
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("dark"));
	QCOMPARE(QSettings().value(key).toString(), QStringLiteral("dark"));

	// Forget the in-memory value, then reload it from the saved settings
	qPrefDisplay::set_color_scheme(QStringLiteral("system"));
	QSettings().setValue(key, QStringLiteral("dark"));
	qPrefDisplay::load();
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("dark"));

	QSettings().setValue(key, QStringLiteral("invalid"));
	qPrefDisplay::load();
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("system"));

	// Selecting the default again removes the saved value
	qPrefDisplay::set_color_scheme(QStringLiteral("light"));
	QCOMPARE(QSettings().value(key).toString(), QStringLiteral("light"));
	qPrefDisplay::set_color_scheme(QStringLiteral("system"));
	QVERIFY(!QSettings().contains(key));
}

static bool paletteIsLight(const QPalette &palette, QPalette::ColorGroup group = QPalette::Active)
{
	return palette.color(group, QPalette::Window).lightnessF() > 0.5 &&
	       palette.color(group, QPalette::Base).lightnessF() > 0.5 &&
	       palette.color(group, QPalette::WindowText).lightnessF() < 0.5 &&
	       palette.color(group, QPalette::Text).lightnessF() < 0.5 &&
	       palette.color(group, QPalette::ButtonText).lightnessF() < 0.5;
}

static bool paletteIsDark(const QPalette &palette, QPalette::ColorGroup group = QPalette::Active)
{
	return palette.color(group, QPalette::Window).lightnessF() < 0.5 &&
	       palette.color(group, QPalette::Base).lightnessF() < 0.5 &&
	       palette.color(group, QPalette::WindowText).lightnessF() > 0.5 &&
	       palette.color(group, QPalette::Text).lightnessF() > 0.5 &&
	       palette.color(group, QPalette::ButtonText).lightnessF() > 0.5;
}

// An explicit Light or Dark choice must be visible whatever the platform theme
// does with QStyleHints::setColorScheme(), and "Use system" must bring back the
// appearance the application had before.
void TestQPrefDisplay::test_color_scheme_palette()
{
	QVERIFY2(qobject_cast<QApplication *>(QCoreApplication::instance()), "needs a widgets application");

	QSettings().remove(QStringLiteral("Display/color_scheme"));
	qPrefDisplay::load();
	QCOMPARE(QApplication::palette(), systemPalette);
	QCOMPARE(QApplication::palette().resolveMask(), systemPalette.resolveMask());

	// A widget that exists before the change must follow it. Qt delivers the
	// palette change to existing widgets from the event loop, hence QTRY_*.
	QWidget widget;

	qPrefDisplay::set_color_scheme(QStringLiteral("dark"));
	QVERIFY(paletteIsDark(QApplication::palette()));
	QVERIFY(paletteIsDark(QApplication::palette(), QPalette::Inactive));
	QTRY_VERIFY(paletteIsDark(widget.palette()));
	// Disabled text must stay readable against the dark background, but dimmer than enabled text
	const QPalette dark = QApplication::palette();
	QVERIFY(dark.color(QPalette::Disabled, QPalette::Text).lightnessF() < dark.color(QPalette::Active, QPalette::Text).lightnessF());
	QVERIFY(dark.color(QPalette::Disabled, QPalette::Text).lightnessF() > dark.color(QPalette::Active, QPalette::Base).lightnessF());

	qPrefDisplay::set_color_scheme(QStringLiteral("light"));
	QVERIFY(paletteIsLight(QApplication::palette()));
	QVERIFY(paletteIsLight(QApplication::palette(), QPalette::Inactive));
	QTRY_VERIFY(paletteIsLight(widget.palette()));
	const QPalette light = QApplication::palette();
	QVERIFY(light.color(QPalette::Disabled, QPalette::Text).lightnessF() > light.color(QPalette::Active, QPalette::Text).lightnessF());
	QVERIFY(light.color(QPalette::Disabled, QPalette::Text).lightnessF() < light.color(QPalette::Active, QPalette::Base).lightnessF());

	// Switching directly between overrides does not leave anything of the previous one behind
	qPrefDisplay::set_color_scheme(QStringLiteral("dark"));
	QCOMPARE(QApplication::palette(), dark);

	// Use system removes the override and restores the earlier palette and style
	qPrefDisplay::set_color_scheme(QStringLiteral("system"));
	QCOMPARE(QApplication::palette(), systemPalette);
	QCOMPARE(QApplication::palette().resolveMask(), systemPalette.resolveMask());
	QTRY_COMPARE(widget.palette(), systemPalette);
	QCOMPARE(QApplication::style()->name(), systemStyle);

	// The widget style is not touched by an override either
	qPrefDisplay::set_color_scheme(QStringLiteral("dark"));
	QCOMPARE(QApplication::style()->name(), systemStyle);
	qPrefDisplay::set_color_scheme(QStringLiteral("system"));
}

// The saved choice is applied when the preferences are loaded at startup
void TestQPrefDisplay::test_color_scheme_startup()
{
	const QString key = QStringLiteral("Display/color_scheme");

	QSettings().setValue(key, QStringLiteral("dark"));
	qPrefDisplay::load();
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("dark"));
	QVERIFY(paletteIsDark(QApplication::palette()));

	QSettings().setValue(key, QStringLiteral("light"));
	qPrefDisplay::load();
	QVERIFY(paletteIsLight(QApplication::palette()));

	QSettings().setValue(key, QStringLiteral("system"));
	qPrefDisplay::load();
	QCOMPARE(QApplication::palette(), systemPalette);

	// An unusable saved value falls back to the system appearance
	QSettings().setValue(key, QStringLiteral("dark"));
	qPrefDisplay::load();
	QSettings().setValue(key, QStringLiteral("invalid"));
	qPrefDisplay::load();
	QCOMPARE(qPrefDisplay::color_scheme(), QStringLiteral("system"));
	QCOMPARE(QApplication::palette(), systemPalette);

	QSettings().remove(key);
	qPrefDisplay::load();
}
#endif

QTEST_MAIN(TestQPrefDisplay)
