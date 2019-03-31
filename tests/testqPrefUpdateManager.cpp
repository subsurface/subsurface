// SPDX-License-Identifier: GPL-2.0
#include "testqPrefUpdateManager.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUpdateManager.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefUpdateManager::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefUpdateManager");
}

void TestQPrefUpdateManager::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefUpdateManager::instance();

	prefs.update_manager.dont_check_for_updates = true;
	prefs.update_manager.dont_check_exists = true;
	prefs.update_manager.last_version_used = copy_qstring("last_version");
	prefs.update_manager.next_check = QDate::fromString("11/09/1957", "dd/MM/yyyy").toJulianDay();

	QCOMPARE(tst->dont_check_for_updates(), true);
	QCOMPARE(tst->dont_check_exists(), true);
	QCOMPARE(tst->last_version_used(), QString("last_version"));
	QCOMPARE(tst->next_check(), QDate::fromString("11/09/1957", "dd/MM/yyyy")); 
}

void TestQPrefUpdateManager::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefUpdateManager::instance();

	tst->set_dont_check_for_updates(false);
	tst->set_dont_check_exists(false);
	tst->set_last_version_used("last_version2");
	tst->set_next_check(QDate::fromString("11/09/1957", "dd/MM/yyyy")); 
	tst->set_uuidString("uuid");

	QCOMPARE(prefs.update_manager.dont_check_for_updates, false);
	QCOMPARE(prefs.update_manager.dont_check_exists, false);
	QCOMPARE(QString(prefs.update_manager.last_version_used), QString("last_version2"));
	QCOMPARE(QDate::fromJulianDay(prefs.update_manager.next_check), QDate::fromString("11/09/1957", "dd/MM/yyyy")); 
	QCOMPARE(tst->uuidString(), QString("uuid"));
}

void TestQPrefUpdateManager::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefUpdateManager::instance();

	// secure set_ stores on disk
	prefs.update_manager.dont_check_for_updates = true;
	prefs.update_manager.dont_check_exists = true;
	prefs.update_manager.next_check = 100;

	tst->set_dont_check_for_updates(false);
	tst->set_dont_check_exists(false);
	tst->set_last_version_used("last_version2");
	tst->set_next_check(QDate::fromString("11/09/1957", "dd/MM/yyyy")); 
	tst->set_uuidString("uuid2");

	prefs.update_manager.dont_check_for_updates = true;
	prefs.update_manager.dont_check_exists = true;
	prefs.update_manager.last_version_used = copy_qstring("last_version");
	prefs.update_manager.next_check = 1000;

	tst->load();
	QCOMPARE(prefs.update_manager.dont_check_for_updates, false);
	QCOMPARE(QString(prefs.update_manager.last_version_used), QString("last_version2"));
	QCOMPARE(QDate::fromJulianDay(prefs.update_manager.next_check), QDate::fromString("11/09/1957", "dd/MM/yyyy")); 
	QCOMPARE(tst->uuidString(), QString("uuid2"));

	// dont_check_exists is NOT stored on disk
	QCOMPARE(prefs.update_manager.dont_check_exists, true);
}

void TestQPrefUpdateManager::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefUpdateManager::instance();

	prefs.update_manager.dont_check_for_updates = true;
	prefs.update_manager.dont_check_exists = true;
	prefs.update_manager.last_version_used = copy_qstring("last_version");
	prefs.update_manager.next_check = QDate::fromString("11/09/1957", "dd/MM/yyyy").toJulianDay();

	tst->sync();
	prefs.update_manager.dont_check_for_updates = false;
	prefs.update_manager.dont_check_exists = false;
	prefs.update_manager.last_version_used = copy_qstring("");
	prefs.update_manager.next_check = 1000;

	tst->load();
	QCOMPARE(tst->dont_check_for_updates(), true);
	QCOMPARE(tst->last_version_used(), QString("last_version"));
	QCOMPARE(tst->next_check(), QDate::fromString("11/09/1957", "dd/MM/yyyy"));

	// dont_check_exists is NOT stored on disk
	QCOMPARE(tst->dont_check_exists(), false);
}

void TestQPrefUpdateManager::test_multiple()
{
	// test multiple instances have the same information

	prefs.update_manager.dont_check_for_updates = false;
	auto tst_direct = new qPrefUpdateManager;

	prefs.update_manager.dont_check_exists = false;
	auto tst = qPrefUpdateManager::instance();

	QCOMPARE(tst->dont_check_for_updates(), tst_direct->dont_check_for_updates());
	QCOMPARE(tst->dont_check_for_updates(), false);
	QCOMPARE(tst->dont_check_exists(), tst_direct->dont_check_exists());
	QCOMPARE(tst_direct->dont_check_exists(), false);
}

void TestQPrefUpdateManager::test_next_check()
{
	auto tst = qPrefUpdateManager::instance();

	prefs.update_manager.next_check = QDate::fromString("11/09/1957", "dd/MM/yyyy").toJulianDay();
	prefs.update_manager.next_check++;

	QCOMPARE(tst->next_check(), QDate::fromString("12/09/1957", "dd/MM/yyyy")); 
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	update->sync();           \
	update->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefUpdateManager::test_oldPreferences()
{
	auto update = qPrefUpdateManager::instance();
	QDate date = QDate::currentDate();

	update->set_dont_check_for_updates(true);
	update->set_last_version_used("tomaz-1");
	update->set_next_check(date);

	TEST(update->dont_check_for_updates(), true);
	TEST(update->last_version_used(), QStringLiteral("tomaz-1"));
	TEST(update->next_check(), date);

	date = date.addDays(3);
	update->set_dont_check_for_updates(false);
	update->set_last_version_used("tomaz-2");
	update->set_next_check(date);

	TEST(update->dont_check_for_updates(), false);
	TEST(update->last_version_used(), QStringLiteral("tomaz-2"));
	TEST(update->next_check(), date);
}

void TestQPrefUpdateManager::test_signals()
{
	QSignalSpy spy1(qPrefUpdateManager::instance(), &qPrefUpdateManager::dont_check_for_updatesChanged);
	QSignalSpy spy2(qPrefUpdateManager::instance(), &qPrefUpdateManager::dont_check_existsChanged);
	QSignalSpy spy3(qPrefUpdateManager::instance(), &qPrefUpdateManager::last_version_usedChanged);
	QSignalSpy spy4(qPrefUpdateManager::instance(), &qPrefUpdateManager::next_checkChanged);
	QSignalSpy spy5(qPrefUpdateManager::instance(), &qPrefUpdateManager::uuidStringChanged);

	prefs.update_manager.dont_check_for_updates = true;
	qPrefUpdateManager::set_dont_check_for_updates(false);
	prefs.update_manager.dont_check_exists = true;
	qPrefUpdateManager::set_dont_check_exists(false);
	qPrefUpdateManager::set_last_version_used("signal last_version2");
	qPrefUpdateManager::set_next_check(QDate::fromString("11/09/1967", "dd/MM/yyyy")); 
	qPrefUpdateManager::set_uuidString("signal uuid");

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toBool() == false);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
	QVERIFY(spy3.takeFirst().at(0).toString() == "signal last_version2");
	QVERIFY(spy4.takeFirst().at(0).toDate() == QDate::fromString("11/09/1967", "dd/MM/yyyy")); 
	QVERIFY(spy5.takeFirst().at(0).toString() == "signal uuid");
}


QTEST_MAIN(TestQPrefUpdateManager)
