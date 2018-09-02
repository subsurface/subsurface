// SPDX-License-Identifier: GPL-2.0
#include "testqPrefCloudStorage.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

void TestQPrefCloudStorage::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefCloudStorage");
}

void TestQPrefCloudStorage::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefCloudStorage::instance();

	prefs.cloud_base_url = copy_qstring("new url");
	prefs.cloud_git_url = copy_qstring("new again url");
	prefs.cloud_storage_email = copy_qstring("myEmail");
	prefs.cloud_storage_email_encoded = copy_qstring("encodedMyEMail");
	prefs.cloud_storage_newpassword = copy_qstring("secret");
	prefs.cloud_storage_password = copy_qstring("more secret");
	prefs.cloud_storage_pin = copy_qstring("a pin");
	prefs.cloud_timeout = 117;
	prefs.cloud_verification_status = qPref::CS_NOCLOUD;
	prefs.git_local_only = true;
	prefs.save_password_local = true;
	prefs.save_userid_local = true;
	prefs.userid = copy_qstring("my user");

	QCOMPARE(tst->cloud_base_url(), QString(prefs.cloud_base_url));
	QCOMPARE(tst->cloud_git_url(), QString(prefs.cloud_git_url));
	QCOMPARE(tst->cloud_storage_email(), QString(prefs.cloud_storage_email));
	QCOMPARE(tst->cloud_storage_email_encoded(), QString(prefs.cloud_storage_email_encoded));
	QCOMPARE(tst->cloud_storage_newpassword(), QString(prefs.cloud_storage_newpassword));
	QCOMPARE(tst->cloud_storage_password(), QString(prefs.cloud_storage_password));
	QCOMPARE(tst->cloud_storage_pin(), QString(prefs.cloud_storage_pin));
	QCOMPARE(tst->cloud_timeout(), prefs.cloud_timeout);
	QCOMPARE(tst->cloud_verification_status(), prefs.cloud_verification_status);
	QCOMPARE(tst->git_local_only(), prefs.git_local_only);
	QCOMPARE(tst->save_password_local(), prefs.save_password_local);
	QCOMPARE(tst->save_userid_local(), prefs.save_userid_local);
	QCOMPARE(tst->userid(), QString(prefs.userid));
}

void TestQPrefCloudStorage::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefCloudStorage::instance();

	tst->set_cloud_base_url("t2 base");
	tst->set_cloud_storage_email("t2 email");
	tst->set_cloud_storage_email_encoded("t2 email2");
	tst->set_cloud_storage_newpassword("t2 pass1");
	tst->set_cloud_storage_password("t2 pass2");
	tst->set_cloud_storage_pin("t2 pin");
	tst->set_cloud_timeout(123);
	tst->set_cloud_verification_status(qPref::CS_VERIFIED);
	tst->set_git_local_only(false);
	tst->set_save_password_local(false);
	tst->set_save_userid_local(false);
	tst->set_userid("t2 user");

	QCOMPARE(QString(prefs.cloud_base_url), QString("t2 base"));
	QCOMPARE(QString(prefs.cloud_storage_email), QString("t2 email"));
	QCOMPARE(QString(prefs.cloud_storage_email_encoded), QString("t2 email2"));
	QCOMPARE(QString(prefs.cloud_storage_newpassword), QString("t2 pass1"));
	QCOMPARE(QString(prefs.cloud_storage_password), QString("t2 pass2"));
	QCOMPARE(QString(prefs.cloud_storage_pin), QString("t2 pin"));
	QCOMPARE(prefs.cloud_timeout, 123);
	QCOMPARE(prefs.cloud_verification_status, (int)qPrefCloudStorage::CS_VERIFIED);
	QCOMPARE(prefs.git_local_only, false);
	QCOMPARE(prefs.save_password_local, false);
	QCOMPARE(prefs.save_userid_local, false);
	QCOMPARE(QString(prefs.userid), QString("t2 user"));

	// remark is set with set_base_url
	QCOMPARE(QString(prefs.cloud_git_url), QString("t2 base/git"));
}

void TestQPrefCloudStorage::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefCloudStorage::instance();

	tst->set_cloud_base_url("t3 base");
	tst->set_cloud_storage_email("t3 email");
	tst->set_cloud_storage_email_encoded("t3 email2");
	tst->set_cloud_storage_newpassword("t3 pass1");
	tst->set_save_password_local(true);
	tst->set_cloud_storage_password("t3 pass2");
	tst->set_cloud_storage_pin("t3 pin");
	tst->set_cloud_timeout(321);
	tst->set_cloud_verification_status(qPref::CS_NOCLOUD);
	tst->set_git_local_only(true);
	tst->set_save_userid_local(true);
	tst->set_userid("t3 user");

	prefs.cloud_base_url = copy_qstring("error1");
	prefs.cloud_git_url = copy_qstring("error1");
	prefs.cloud_storage_email = copy_qstring("error1");
	prefs.cloud_storage_email_encoded = copy_qstring("error1");
	prefs.cloud_storage_newpassword = copy_qstring("my new password");
	prefs.cloud_storage_password = copy_qstring("error1");
	prefs.cloud_storage_pin = copy_qstring("error1");
	prefs.cloud_timeout = 324;
	prefs.cloud_verification_status = qPref::CS_VERIFIED;
	prefs.git_local_only = false;
	prefs.save_password_local = false;
	prefs.save_userid_local = false;
	prefs.userid = copy_qstring("error1");

	tst->load();
	QCOMPARE(QString(prefs.cloud_base_url), QString("t3 base"));
	QCOMPARE(QString(prefs.cloud_storage_email), QString("t3 email"));
	QCOMPARE(QString(prefs.cloud_storage_email_encoded), QString("t3 email2"));
	QCOMPARE(QString(prefs.cloud_storage_password), QString("t3 pass2"));
	QCOMPARE(QString(prefs.cloud_storage_pin), QString("t3 pin"));
	QCOMPARE(prefs.cloud_timeout, 321);
	QCOMPARE(prefs.cloud_verification_status, (int)qPref::CS_NOCLOUD);
	QCOMPARE(prefs.git_local_only, true);
	QCOMPARE(prefs.save_password_local, true);
	QCOMPARE(prefs.save_userid_local, true);
	QCOMPARE(QString(prefs.userid), QString("t3 user"));

	// remark is set with set_base_url
	QCOMPARE(QString(prefs.cloud_git_url), QString("t3 base/git"));

	// warning new password is not saved on disk
	QCOMPARE(QString(prefs.cloud_storage_newpassword), QString("my new password"));
}

void TestQPrefCloudStorage::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefCloudStorage::instance();

	prefs.cloud_base_url = copy_qstring("t4 base");
	prefs.cloud_storage_email = copy_qstring("t4 email");
	prefs.cloud_storage_email_encoded = copy_qstring("t4 email2");
	prefs.cloud_storage_newpassword = copy_qstring("t4 pass1");
	prefs.save_password_local = true;
	prefs.cloud_storage_password = copy_qstring("t4 pass2");
	prefs.cloud_storage_pin = copy_qstring("t4 pin");
	prefs.cloud_timeout = 123;
	prefs.cloud_verification_status = qPref::CS_VERIFIED;
	prefs.git_local_only = true;
	prefs.save_userid_local = true;
	prefs.userid = copy_qstring("t4 user");

	tst->sync();

	prefs.cloud_base_url = copy_qstring("error1");
	prefs.cloud_git_url = copy_qstring("error1");
	prefs.cloud_storage_email = copy_qstring("error1");
	prefs.cloud_storage_email_encoded = copy_qstring("error1");
	prefs.cloud_storage_newpassword = copy_qstring("my new password");
	prefs.cloud_storage_password = copy_qstring("error1");
	prefs.cloud_storage_pin = copy_qstring("error1");
	prefs.cloud_timeout = 324;
	prefs.cloud_verification_status = qPref::CS_VERIFIED;
	prefs.git_local_only = false;
	prefs.save_password_local = false;
	prefs.save_userid_local = false;
	prefs.userid = copy_qstring("error1");

	tst->load();

	QCOMPARE(QString(prefs.cloud_base_url), QString("t4 base"));
	QCOMPARE(QString(prefs.cloud_storage_email), QString("t4 email"));
	QCOMPARE(QString(prefs.cloud_storage_email_encoded), QString("t4 email2"));
	QCOMPARE(QString(prefs.cloud_storage_password), QString("t4 pass2"));
	QCOMPARE(QString(prefs.cloud_storage_pin), QString("t4 pin"));
	QCOMPARE(prefs.cloud_timeout, 123);
	QCOMPARE(prefs.cloud_verification_status, (int)qPref::CS_VERIFIED);
	QCOMPARE(prefs.git_local_only, true);
	QCOMPARE(prefs.save_password_local, true);
	QCOMPARE(prefs.save_userid_local, true);
	QCOMPARE(QString(prefs.userid), QString("t4 user"));

	// remark is set with set_base_url
	QCOMPARE(QString(prefs.cloud_git_url), QString("t4 base/git"));

	// warning new password not stored on disk
	QCOMPARE(QString(prefs.cloud_storage_newpassword), QString("my new password"));
}

void TestQPrefCloudStorage::test_multiple()
{
	// test multiple instances have the same information

	prefs.userid = copy_qstring("my user");
	auto tst_direct = new qPrefCloudStorage;

	prefs.cloud_timeout = 25;
	auto tst = qPrefCloudStorage::instance();

	QCOMPARE(tst->cloud_timeout(), tst_direct->cloud_timeout());
	QCOMPARE(tst->userid(), tst_direct->userid());
	QCOMPARE(tst_direct->cloud_timeout(), 25);
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
	TEST(cloud->cloud_base_url(), QStringLiteral("test_one"));
	cloud->set_cloud_base_url("test_two");
	TEST(cloud->cloud_base_url(), QStringLiteral("test_two"));

	cloud->set_cloud_storage_email("tomaz@subsurface.com");
	TEST(cloud->cloud_storage_email(), QStringLiteral("tomaz@subsurface.com"));
	cloud->set_cloud_storage_email("tomaz@gmail.com");
	TEST(cloud->cloud_storage_email(), QStringLiteral("tomaz@gmail.com"));

	cloud->set_git_local_only(true);
	TEST(cloud->git_local_only(), true);
	cloud->set_git_local_only(false);
	TEST(cloud->git_local_only(), false);

	// Why there's new password and password on the prefs?
	cloud->set_cloud_storage_newpassword("ABCD");
	TEST(cloud->cloud_storage_newpassword(), QStringLiteral("ABCD"));
	cloud->set_cloud_storage_newpassword("ABCDE");
	TEST(cloud->cloud_storage_newpassword(), QStringLiteral("ABCDE"));

	cloud->set_cloud_storage_password("ABCDE");
	TEST(cloud->cloud_storage_password(), QStringLiteral("ABCDE"));
	cloud->set_cloud_storage_password("ABCABC");
	TEST(cloud->cloud_storage_password(), QStringLiteral("ABCABC"));

	cloud->set_save_password_local(true);
	TEST(cloud->save_password_local(), true);
	cloud->set_save_password_local(false);
	TEST(cloud->save_password_local(), false);

	cloud->set_save_userid_local(1);
	TEST(cloud->save_userid_local(), true);
	cloud->set_save_userid_local(0);
	TEST(cloud->save_userid_local(), false);

	cloud->set_userid("Tomaz");
	TEST(cloud->userid(), QStringLiteral("Tomaz"));
	cloud->set_userid("Zamot");
	TEST(cloud->userid(), QStringLiteral("Zamot"));

	cloud->set_cloud_verification_status(0);
	TEST(cloud->cloud_verification_status(), 0);
	cloud->set_cloud_verification_status(1);
	TEST(cloud->cloud_verification_status(), 1);
}

void TestQPrefCloudStorage::test_loadFromCloud_var()
{
	auto cloud = qPrefCloudStorage::instance();

	cloud->set_loadFromCloud("mail1", true);
	cloud->set_loadFromCloud("mail2", false);
	cloud->set_loadFromCloud("mail3", true);

	QCOMPARE(cloud->loadFromCloud("mail1"), true);
	QCOMPARE(cloud->loadFromCloud("mail2"), false);
	QCOMPARE(cloud->loadFromCloud("mail3"), true);
	QCOMPARE(cloud->loadFromCloud("mail_unknown"), false);
}

QTEST_MAIN(TestQPrefCloudStorage)
