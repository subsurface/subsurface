// SPDX-License-Identifier: GPL-2.0
#include "testqPrefFacebook.h"

#include "core/settings/qPrefFacebook.h"
#include "core/pref.h"
#include "core/qthelper.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefFacebook::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefFacebook");
}

void TestQPrefFacebook::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefFacebook::instance();

	prefs.facebook.access_token = copy_qstring("t1 token");
	prefs.facebook.album_id = copy_qstring("t1 album");
	prefs.facebook.user_id = copy_qstring("t1 user");

	QCOMPARE(tst->access_token(), QString(prefs.facebook.access_token));
	QCOMPARE(tst->album_id(), QString(prefs.facebook.album_id));
	QCOMPARE(tst->user_id(), QString(prefs.facebook.user_id));
}

void TestQPrefFacebook::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefFacebook::instance();

	tst->set_access_token("t2 token");
	tst->set_album_id("t2 album");
	tst->set_user_id("t2 user");

	QCOMPARE(QString(prefs.facebook.access_token), QString("t2 token"));
	QCOMPARE(QString(prefs.facebook.album_id), QString("t2 album"));
	QCOMPARE(QString(prefs.facebook.user_id), QString("t2 user"));
}

void TestQPrefFacebook::test_multiple()
{
	// test multiple instances have the same information
	prefs.facebook.access_token = copy_qstring("test 1");

	auto tst = qPrefFacebook::instance();
	prefs.facebook.album_id = copy_qstring("test 2");

	QCOMPARE(tst->access_token(), qPrefFacebook::access_token());
	QCOMPARE(tst->access_token(), QString("test 1"));
	QCOMPARE(tst->album_id(), qPrefFacebook::album_id());
	QCOMPARE(tst->album_id(), QString("test 2"));
}

#define TEST(METHOD, VALUE) \
QCOMPARE(METHOD, VALUE); \
fb->sync(); \
fb->load(); \
QCOMPARE(METHOD, VALUE);

void TestQPrefFacebook::test_oldPreferences()
{
	auto fb = qPrefFacebook::instance();
	fb->set_access_token("rand-access-token");
	fb->set_user_id("tomaz-user-id");
	fb->set_album_id("album-id");

	TEST(fb->access_token(),QStringLiteral("rand-access-token"));
	TEST(fb->user_id(),     QStringLiteral("tomaz-user-id"));
	TEST(fb->album_id(),    QStringLiteral("album-id"));

	fb->set_access_token("rand-access-token-2");
	fb->set_user_id("tomaz-user-id-2");
	fb->set_album_id("album-id-2");

	TEST(fb->access_token(),QStringLiteral("rand-access-token-2"));
	TEST(fb->user_id(),     QStringLiteral("tomaz-user-id-2"));
	TEST(fb->album_id(),    QStringLiteral("album-id-2"));
}

void TestQPrefFacebook::test_signals()
{
	QSignalSpy spy1(qPrefFacebook::instance(), SIGNAL(access_tokenChanged(QString)));
	QSignalSpy spy2(qPrefFacebook::instance(), SIGNAL(album_idChanged(QString)));
	QSignalSpy spy3(qPrefFacebook::instance(), SIGNAL(user_idChanged(QString)));

	qPrefFacebook::set_access_token("t_signal token");
	qPrefFacebook::set_album_id("t_signal album");
	qPrefFacebook::set_user_id("t_signal user");

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QVERIFY(spy1.takeFirst().at(0).toString() == "t_signal token");
	QVERIFY(spy2.takeFirst().at(0).toString() == "t_signal album");
	QVERIFY(spy3.takeFirst().at(0).toString() == "t_signal user");
}


QTEST_MAIN(TestQPrefFacebook)
