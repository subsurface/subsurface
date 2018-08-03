// SPDX-License-Identifier: GPL-2.0
#include "testqPrefUpdateManager.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

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
	prefs.update_manager.next_check = copy_qstring(QString("11/09/1957"));

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
	prefs.update_manager.next_check = copy_qstring(QString("11/09/1957"));

	QCOMPARE(prefs.update_manager.dont_check_for_updates, false);
	QCOMPARE(prefs.update_manager.dont_check_exists, false);
	QCOMPARE(QString(prefs.update_manager.last_version_used), QString("last_version2"));
	QCOMPARE(QDate::fromString(QString(prefs.update_manager.next_check), "dd/MM/yyyy"), QDate::fromString("11/09/1957", "dd/MM/yyyy")); 
}

void TestQPrefUpdateManager::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefUpdateManager::instance();

	// secure set_ stores on disk
	prefs.update_manager.dont_check_for_updates = true;
	prefs.update_manager.dont_check_exists = true;
	prefs.update_manager.next_check = copy_qstring(QString("value1"));

	tst->set_dont_check_for_updates(false);
	tst->set_dont_check_exists(false);
	tst->set_last_version_used("last_version2");
	tst->set_next_check(QDate::fromString("11/09/1957", "dd/MM/yyyy")); 

	prefs.update_manager.dont_check_for_updates = true;
	prefs.update_manager.dont_check_exists = true;
	prefs.update_manager.last_version_used = copy_qstring("last_version");
	prefs.update_manager.next_check = copy_qstring(QString("01/01/2018"));

	tst->load();
	QCOMPARE(prefs.update_manager.dont_check_for_updates, false);
	QCOMPARE(QString(prefs.update_manager.last_version_used), QString("last_version2"));
	QCOMPARE(QDate::fromString(QString(prefs.update_manager.next_check),"dd/MM/yyyy"), QDate::fromString("11/09/1957", "dd/MM/yyyy")); 

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
	prefs.update_manager.next_check = copy_qstring("11/09/1957"); 

	tst->sync();
	prefs.update_manager.dont_check_for_updates = false;
	prefs.update_manager.dont_check_exists = false;
	prefs.update_manager.last_version_used = copy_qstring("");
	prefs.update_manager.next_check = copy_qstring("01/09/2057"); 

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

QTEST_MAIN(TestQPrefUpdateManager)
