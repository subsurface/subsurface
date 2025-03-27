// SPDX-License-Identifier: GPL-2.0
#include "testqPrefCloudStorage.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefCloudStorage.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefCloudStorage::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefCloudStorage");
}

void TestQPrefCloudStorage::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefCloudStorage::instance();

	prefs.cloud_auto_sync = true;
	prefs.cloud_base_url = "new url";
	prefs.cloud_storage_email = "myEmail";
	prefs.cloud_storage_email_encoded = "encodedMyEMail";
	prefs.cloud_storage_password = "more secret";
	prefs.cloud_storage_pin = "a pin";
	prefs.cloud_timeout = 117;
	prefs.cloud_verification_status = qPrefCloudStorage::CS_NOCLOUD;
	prefs.save_password_local = true;

	QCOMPARE(tst->cloud_auto_sync(), prefs.cloud_auto_sync);
	QCOMPARE(tst->cloud_base_url(), QString::fromStdString(prefs.cloud_base_url));
	QCOMPARE(tst->cloud_storage_email(), QString::fromStdString(prefs.cloud_storage_email));
	QCOMPARE(tst->cloud_storage_email_encoded(), QString::fromStdString(prefs.cloud_storage_email_encoded));
	QCOMPARE(tst->cloud_storage_password(), QString::fromStdString(prefs.cloud_storage_password));
	QCOMPARE(tst->cloud_storage_pin(), QString::fromStdString(prefs.cloud_storage_pin));
	QCOMPARE(tst->cloud_timeout(), (int)prefs.cloud_timeout);
	QCOMPARE(tst->cloud_verification_status(), (int)prefs.cloud_verification_status);
	QCOMPARE(tst->save_password_local(), prefs.save_password_local);
}

void TestQPrefCloudStorage::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefCloudStorage::instance();

	tst->set_cloud_auto_sync(false);
	tst->set_cloud_base_url("t2 base");
	tst->set_cloud_storage_email("t2 email");
	tst->set_cloud_storage_email_encoded("t2 email2");
	tst->set_cloud_storage_password("t2 pass2");
	tst->set_cloud_storage_pin("t2 pin");
	tst->set_cloud_timeout(123);
	tst->set_cloud_verification_status(qPrefCloudStorage::CS_VERIFIED);
	tst->set_save_password_local(false);

	QCOMPARE(prefs.cloud_auto_sync, false);
	QCOMPARE(QString::fromStdString(prefs.cloud_base_url), QString("t2 base"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_email), QString("t2 email"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_email_encoded), QString("t2 email2"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_password), QString("t2 pass2"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_pin), QString("t2 pin"));
	QCOMPARE((int)prefs.cloud_timeout, 123);
	QCOMPARE((int)prefs.cloud_verification_status, (int)qPrefCloudStorage::CS_VERIFIED);
	QCOMPARE(prefs.save_password_local, false);
}

void TestQPrefCloudStorage::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefCloudStorage::instance();

	tst->set_cloud_base_url("t3 base");
	tst->store_cloud_base_url("t3 base"); // the base URL is no longer automatically saved to disk
	tst->set_cloud_storage_email("t3 email");
	tst->set_cloud_storage_email_encoded("t3 email2");
	tst->set_save_password_local(true);
	tst->set_cloud_auto_sync(true);
	tst->set_cloud_storage_password("t3 pass2");
	tst->set_cloud_storage_pin("t3 pin");
	tst->set_cloud_timeout(321);
	tst->set_cloud_verification_status(qPrefCloudStorage::CS_NOCLOUD);

	prefs.cloud_auto_sync = false;
	prefs.cloud_base_url = "error1";
	prefs.cloud_storage_email = "error1";
	prefs.cloud_storage_email_encoded = "error1";
	prefs.cloud_storage_password = "error1";
	prefs.cloud_storage_pin = "error1";
	prefs.cloud_timeout = 324;
	prefs.cloud_verification_status = qPrefCloudStorage::CS_VERIFIED;
	prefs.save_password_local = false;

	tst->load();
	QCOMPARE(prefs.cloud_auto_sync, true);
	QCOMPARE(QString::fromStdString(prefs.cloud_base_url), QString("t3 base"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_email), QString("t3 email"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_email_encoded), QString("t3 email2"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_password), QString("t3 pass2"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_pin), QString("t3 pin"));
	QCOMPARE((int)prefs.cloud_timeout, 321);
	QCOMPARE((int)prefs.cloud_verification_status, (int)qPrefCloudStorage::CS_NOCLOUD);
	QCOMPARE(prefs.save_password_local, true);
}

void TestQPrefCloudStorage::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefCloudStorage::instance();

	prefs.cloud_base_url = "t4 base";
	tst->store_cloud_base_url("t4 base"); // the base URL is no longer automatically saved to disk
	prefs.cloud_storage_email =("t4 email");
	prefs.cloud_storage_email_encoded = "t4 email2";
	prefs.save_password_local = true;
	prefs.cloud_auto_sync = true;
	prefs.cloud_storage_password = "t4 pass2";
	prefs.cloud_storage_pin = "t4 pin";
	prefs.cloud_timeout = 123;
	prefs.cloud_verification_status = qPrefCloudStorage::CS_VERIFIED;

	tst->sync();

	prefs.cloud_auto_sync = false;
	prefs.cloud_base_url = "error1";
	prefs.cloud_storage_email = "error1";
	prefs.cloud_storage_email_encoded = "error1";
	prefs.cloud_storage_password = "error1";
	prefs.cloud_storage_pin = "error1";
	prefs.cloud_timeout = 324;
	prefs.cloud_verification_status = qPrefCloudStorage::CS_VERIFIED;
	prefs.save_password_local = false;

	tst->load();

	QCOMPARE(prefs.cloud_auto_sync, true);
	QCOMPARE(QString::fromStdString(prefs.cloud_base_url), QString("t4 base"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_email), QString("t4 email"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_email_encoded), QString("t4 email2"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_password), QString("t4 pass2"));
	QCOMPARE(QString::fromStdString(prefs.cloud_storage_pin), QString("t4 pin"));
	QCOMPARE((int)prefs.cloud_timeout, 123);
	QCOMPARE((int)prefs.cloud_verification_status, (int)qPrefCloudStorage::CS_VERIFIED);
	QCOMPARE(prefs.save_password_local, true);
}

void TestQPrefCloudStorage::test_multiple()
{
	// test multiple instances have the same information

	prefs.cloud_timeout = 25;
	auto tst = qPrefCloudStorage::instance();

	QCOMPARE(tst->cloud_timeout(), qPrefCloudStorage::cloud_timeout());
	QCOMPARE(qPrefCloudStorage::cloud_timeout(), 25);
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	cloud->sync();           \
	cloud->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefCloudStorage::test_oldPreferences()
{
	auto cloud = qPrefCloudStorage::instance();

	cloud->set_cloud_base_url("test_one");
	cloud->store_cloud_base_url("test_one");
	TEST(cloud->cloud_base_url(), QStringLiteral("test_one"));
	cloud->set_cloud_base_url("test_two");
	cloud->store_cloud_base_url("test_two");
	TEST(cloud->cloud_base_url(), QStringLiteral("test_two"));

	cloud->set_cloud_storage_email("tomaz@subsurface.com");
	TEST(cloud->cloud_storage_email(), QStringLiteral("tomaz@subsurface.com"));
	cloud->set_cloud_storage_email("tomaz@gmail.com");
	TEST(cloud->cloud_storage_email(), QStringLiteral("tomaz@gmail.com"));

	cloud->set_cloud_storage_password("ABCDE");
	TEST(cloud->cloud_storage_password(), QStringLiteral("ABCDE"));
	cloud->set_cloud_storage_password("ABCABC");
	TEST(cloud->cloud_storage_password(), QStringLiteral("ABCABC"));

	cloud->set_cloud_auto_sync(true);
	TEST(cloud->cloud_auto_sync(), true);
	cloud->set_cloud_auto_sync(false);
	TEST(cloud->cloud_auto_sync(), false);

	cloud->set_save_password_local(true);
	TEST(cloud->save_password_local(), true);
	cloud->set_save_password_local(false);
	TEST(cloud->save_password_local(), false);

	cloud->set_cloud_verification_status(0);
	TEST(cloud->cloud_verification_status(), 0);
	cloud->set_cloud_verification_status(1);
	TEST(cloud->cloud_verification_status(), 1);
}

void TestQPrefCloudStorage::test_signals()
{
	QSignalSpy spy1(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_base_urlChanged);
	QSignalSpy spy2(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_storage_emailChanged);
	QSignalSpy spy3(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_storage_email_encodedChanged);
	QSignalSpy spy4(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_storage_passwordChanged);
	QSignalSpy spy5(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_storage_pinChanged);
	QSignalSpy spy6(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_timeoutChanged);
	QSignalSpy spy7(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_verification_statusChanged);
	QSignalSpy spy9(qPrefCloudStorage::instance(), &qPrefCloudStorage::save_password_localChanged);
	QSignalSpy spy10(qPrefCloudStorage::instance(), &qPrefCloudStorage::cloud_auto_syncChanged);

	qPrefCloudStorage::set_cloud_base_url("signal url");
	qPrefCloudStorage::set_cloud_storage_email("signal myEmail");
	qPrefCloudStorage::set_cloud_storage_email_encoded("signal encodedMyEMail");
	qPrefCloudStorage::set_cloud_storage_password("signal more secret");
	qPrefCloudStorage::set_cloud_storage_pin("signal a pin");
	qPrefCloudStorage::set_cloud_timeout(11);
	qPrefCloudStorage::set_cloud_verification_status(qPrefCloudStorage::CS_VERIFIED);
	qPrefCloudStorage::set_save_password_local(true);
	qPrefCloudStorage::set_cloud_auto_sync(true);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);
	QCOMPARE(spy6.count(), 1);
	QCOMPARE(spy7.count(), 1);
	QCOMPARE(spy9.count(), 1);
	QCOMPARE(spy10.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toString() == "signal url");
	QVERIFY(spy2.takeFirst().at(0).toString() == "signal myEmail");
	QVERIFY(spy3.takeFirst().at(0).toString() == "signal encodedMyEMail");
	QVERIFY(spy4.takeFirst().at(0).toString() == "signal more secret");
	QVERIFY(spy5.takeFirst().at(0).toString() == "signal a pin");
	QVERIFY(spy6.takeFirst().at(0).toInt() == 11);
	QVERIFY(spy7.takeFirst().at(0).toInt() == qPrefCloudStorage::CS_VERIFIED);
	QVERIFY(spy9.takeFirst().at(0).toBool() == true);
	QVERIFY(spy10.takeFirst().at(0).toBool() == true);
}

QTEST_MAIN(TestQPrefCloudStorage)
