// SPDX-License-Identifier: GPL-2.0
#include "testqPrefLanguage.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefLanguage.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefLanguage::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefLanguage");
}

void TestQPrefLanguage::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefLanguage::instance();

	prefs.date_format = "new date format";
	prefs.date_format_override = true;
	prefs.date_format_short = "new short format";
	prefs.locale.language = "new lang format";
	prefs.locale.lang_locale = "new loc lang format";
	prefs.time_format = "new time format";
	prefs.time_format_override = true;
	prefs.locale.use_system_language = true;

	QCOMPARE(tst->date_format(), QString::fromStdString(prefs.date_format));
	QCOMPARE(tst->date_format_override(), prefs.date_format_override);
	QCOMPARE(tst->date_format_short(), QString::fromStdString(prefs.date_format_short));
	QCOMPARE(tst->language(), QString::fromStdString(prefs.locale.language));
	QCOMPARE(tst->lang_locale(), QString::fromStdString(prefs.locale.lang_locale));
	QCOMPARE(tst->time_format(), QString::fromStdString(prefs.time_format));
	QCOMPARE(tst->time_format_override(), prefs.time_format_override);
	QCOMPARE(tst->use_system_language(), prefs.locale.use_system_language);
}

void TestQPrefLanguage::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefLanguage::instance();

	tst->set_date_format("new date2");
	tst->set_date_format_override(false);
	tst->set_date_format_short("new short2");
	tst->set_language("new lang format");
	tst->set_lang_locale("new loc lang2");
	tst->set_time_format("new time2");
	tst->set_time_format_override(false);
	tst->set_use_system_language(false);

	QCOMPARE(tst->date_format(), QString("new date2"));
	QCOMPARE(tst->date_format_override(), false);
	QCOMPARE(tst->date_format_short(), QString("new short2"));
	QCOMPARE(tst->language(), QString("new lang format"));
	QCOMPARE(tst->lang_locale(), QString("new loc lang2"));
	QCOMPARE(tst->time_format(), QString("new time2"));
	QCOMPARE(tst->time_format_override(), false);
	QCOMPARE(tst->use_system_language(), false);
}

void TestQPrefLanguage::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefLanguage::instance();

	tst->set_date_format("new date3");
	tst->set_date_format_override(true);
	tst->set_date_format_short("new short3");
	tst->set_language("new lang format3");
	tst->set_lang_locale("new loc lang3");
	tst->set_time_format("new time3");
	tst->set_time_format_override(true);
	tst->set_use_system_language(true);

	prefs.date_format = "error3";
	prefs.date_format_override = false;
	prefs.date_format_short = "error3";
	prefs.locale.language = "error3";
	prefs.locale.lang_locale = "error3";
	prefs.time_format = "error3";
	prefs.time_format_override = false;
	prefs.locale.use_system_language = false;

	tst->load();
	QCOMPARE(QString::fromStdString(prefs.date_format), QString("new date3"));
	QCOMPARE(prefs.date_format_override, true);
	QCOMPARE(QString::fromStdString(prefs.date_format_short), QString("new short3"));
	QCOMPARE(QString::fromStdString(prefs.locale.language), QString("new lang format3"));
	QCOMPARE(QString::fromStdString(prefs.locale.lang_locale), QString("new loc lang3"));
	QCOMPARE(QString::fromStdString(prefs.time_format), QString("new time3"));
	QCOMPARE(prefs.time_format_override, true);
	QCOMPARE(prefs.locale.use_system_language, true);
}

void TestQPrefLanguage::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefLanguage::instance();

	prefs.date_format = "new date format";
	prefs.date_format_override = true;
	prefs.date_format_short = "new short format";
	prefs.locale.language = "new lang format";
	prefs.locale.lang_locale = "new loc lang format";
	prefs.time_format = "new time format";
	prefs.time_format_override = true;
	prefs.locale.use_system_language = true;

	tst->sync();
	prefs.date_format = "error3";
	prefs.date_format_override = false;
	prefs.date_format_short = "error3";
	prefs.locale.language = "error3";
	prefs.locale.lang_locale = "error3";
	prefs.time_format = "error3";
	prefs.time_format_override = false;
	prefs.locale.use_system_language = false;

	tst->load();
	QCOMPARE(QString::fromStdString(prefs.date_format), QString("new date format"));
	QCOMPARE(prefs.date_format_override, true);
	QCOMPARE(QString::fromStdString(prefs.date_format_short), QString("new short format"));
	QCOMPARE(QString::fromStdString(prefs.locale.language), QString("new lang format"));
	QCOMPARE(QString::fromStdString(prefs.locale.lang_locale), QString("new loc lang format"));
	QCOMPARE(QString::fromStdString(prefs.time_format), QString("new time format"));
	QCOMPARE(prefs.time_format_override, true);
	QCOMPARE(prefs.locale.use_system_language, true);
}

void TestQPrefLanguage::test_multiple()
{
	// test multiple instances have the same information

	prefs.locale.use_system_language = false;

	prefs.time_format_override = true;
	auto tst = qPrefLanguage::instance();

	QCOMPARE(tst->use_system_language(), qPrefLanguage::use_system_language());
	QCOMPARE(tst->time_format_override(), qPrefLanguage::time_format_override());
	QCOMPARE(qPrefLanguage::use_system_language(), false);
	QCOMPARE(qPrefLanguage::time_format_override(), true);
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	language->sync();           \
	language->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefLanguage::test_oldPreferences()
{
	auto language = qPrefLanguage::instance();
	language->set_lang_locale("en_US");
	language->set_language("en");
	language->set_time_format("hh:mm");
	language->set_date_format("dd/mm/yy");
	language->set_date_format_short("dd/mm");
	language->set_time_format_override(false);
	language->set_date_format_override(false);
	language->set_use_system_language(false);

	TEST(language->lang_locale(), QStringLiteral("en_US"));
	TEST(language->language(), QStringLiteral("en"));
	TEST(language->time_format(), QStringLiteral("hh:mm"));
	TEST(language->date_format(), QStringLiteral("dd/mm/yy"));
	TEST(language->date_format_short(), QStringLiteral("dd/mm"));
	TEST(language->time_format_override(), false);
	TEST(language->date_format_override(), false);
	TEST(language->use_system_language(), false);

	language->set_lang_locale("en_EN");
	language->set_language("br");
	language->set_time_format("mm:hh");
	language->set_date_format("yy/mm/dd");
	language->set_date_format_short("dd/yy");
	language->set_time_format_override(true);
	language->set_date_format_override(true);
	language->set_use_system_language(true);

	TEST(language->lang_locale(), QStringLiteral("en_EN"));
	TEST(language->language(), QStringLiteral("br"));
	TEST(language->time_format(), QStringLiteral("mm:hh"));
	TEST(language->date_format(), QStringLiteral("yy/mm/dd"));
	TEST(language->date_format_short(), QStringLiteral("dd/yy"));
	TEST(language->time_format_override(), true);
	TEST(language->date_format_override(), true);
	TEST(language->use_system_language(), true);

	// AI-generated (Claude): Verify the shared API and stable preset values.
	language->set_use_system_language(false);
	language->set_lang_locale("en_US");
	language->applyDateTimeFormats("ddd, d MMM yyyy", "d/M/yy", "HH:mm", true, true);
	QCOMPARE(language->effectiveDateFormat(), QStringLiteral("ddd, d MMM yyyy"));
	QCOMPARE(language->effectiveDateFormatShort(), QStringLiteral("d/M/yy"));
	QCOMPARE(language->effectiveTimeFormat(), QStringLiteral("HH:mm"));
	language->load();
	QCOMPARE(language->date_format(), QStringLiteral("ddd, d MMM yyyy"));
	QCOMPARE(language->date_format_short(), QStringLiteral("d/M/yy"));
	QCOMPARE(language->time_format(), QStringLiteral("HH:mm"));

	language->applyDatePreset("month-first");
	QCOMPARE(language->date_format(), QStringLiteral("MM/dd/yyyy"));
	QCOMPARE(language->date_format_short(), QStringLiteral("M/d/yy"));
	language->applyDatePreset("day-first");
	QCOMPARE(language->date_format(), QStringLiteral("dd.MM.yyyy"));
	QCOMPARE(language->date_format_short(), QStringLiteral("d.M.yy"));
	language->applyDatePreset("iso");
	QCOMPARE(language->date_format(), QStringLiteral("yyyy-MM-dd"));
	QCOMPARE(language->date_format_short(), QStringLiteral("yy-M-d"));
	language->applyTimePreset("24-hour");
	QCOMPARE(language->time_format(), QStringLiteral("hh:mm"));
	language->applyTimePreset("12-hour");
	QCOMPARE(language->time_format(), QStringLiteral("h:mm AP"));

	QSignalSpy formatsSpy(language, &qPrefLanguage::dateTimeFormatsChanged);
	language->restoreDateTimeDefaults();
	QCOMPARE(language->date_format_override(), false);
	QCOMPARE(language->time_format_override(), false);
	// When no override is active, the stored format is empty; the effective
	// format is resolved dynamically from the system locale at call time.
	// AI-generated (Claude): updated to match new behaviour where prefs store
	// empty string for system-default formats.
	QCOMPARE(language->date_format(), QString());
	QCOMPARE(language->date_format_short(), QString());
	QCOMPARE(language->time_format(), QString());
	QCOMPARE(formatsSpy.count(), 1);
	prefs.date_format = "error";
	prefs.date_format_short = "error";
	prefs.date_format_override = true;
	prefs.time_format = "error";
	prefs.time_format_override = true;
	language->load();
	QCOMPARE(language->date_format(), QString());
	QCOMPARE(language->date_format_short(), QString());
	QCOMPARE(language->date_format_override(), false);
	QCOMPARE(language->time_format_override(), false);

}

void TestQPrefLanguage::test_signals()
{
	QSignalSpy spy1(qPrefLanguage::instance(), &qPrefLanguage::date_formatChanged);
	QSignalSpy spy2(qPrefLanguage::instance(), &qPrefLanguage::date_format_overrideChanged);
	QSignalSpy spy3(qPrefLanguage::instance(), &qPrefLanguage::date_format_shortChanged);
	QSignalSpy spy4(qPrefLanguage::instance(), &qPrefLanguage::languageChanged);
	QSignalSpy spy5(qPrefLanguage::instance(), &qPrefLanguage::lang_localeChanged);
	QSignalSpy spy6(qPrefLanguage::instance(), &qPrefLanguage::time_formatChanged);
	QSignalSpy spy7(qPrefLanguage::instance(), &qPrefLanguage::time_format_overrideChanged);
	QSignalSpy spy8(qPrefLanguage::instance(), &qPrefLanguage::use_system_languageChanged);

	qPrefLanguage::set_date_format("signal date2");
	prefs.date_format_override = true;
	qPrefLanguage::set_date_format_override(false);
	qPrefLanguage::set_date_format_short("signal short2");
	qPrefLanguage::set_language("signal lang format");
	qPrefLanguage::set_lang_locale("signal loc lang2");
	qPrefLanguage::set_time_format("signal time2");
	prefs.time_format_override = true;
	qPrefLanguage::set_time_format_override(false);
	prefs.locale.use_system_language = true;
	qPrefLanguage::set_use_system_language(false);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);
	QCOMPARE(spy6.count(), 1);
	QCOMPARE(spy7.count(), 1);
	QCOMPARE(spy8.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toString() == "signal date2");
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
	QVERIFY(spy3.takeFirst().at(0).toString() == "signal short2");
	QVERIFY(spy4.takeFirst().at(0).toString() == "signal lang format");
	QVERIFY(spy5.takeFirst().at(0).toString() == "signal loc lang2");
	QVERIFY(spy6.takeFirst().at(0).toString() == "signal time2");
	QVERIFY(spy7.takeFirst().at(0).toBool() == false);
	QVERIFY(spy8.takeFirst().at(0).toBool() == false);
}


QTEST_MAIN(TestQPrefLanguage)
