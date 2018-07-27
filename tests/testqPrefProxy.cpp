// SPDX-License-Identifier: GPL-2.0
#include "testqPrefProxy.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

void TestQPrefProxy::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefProxy");
}

void TestQPrefProxy::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefProxy::instance();

	prefs.proxy_auth = true;
	prefs.proxy_host = copy_qstring("t1 host");
	prefs.proxy_pass = copy_qstring("t1 pass");
	prefs.proxy_port = 512;
	prefs.proxy_type = 3;
	prefs.proxy_user = copy_qstring("t1 user");

	QCOMPARE(tst->proxy_auth(), true);
	QCOMPARE(tst->proxy_host(), QString(prefs.proxy_host));
	QCOMPARE(tst->proxy_pass(), QString(prefs.proxy_pass));
	QCOMPARE(tst->proxy_port(), 512);
	QCOMPARE(tst->proxy_type(), 3);
	QCOMPARE(tst->proxy_user(), QString(prefs.proxy_user));
}

void TestQPrefProxy::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefProxy::instance();

	tst->set_proxy_auth(false);
	tst->set_proxy_host("t2 host");
	tst->set_proxy_pass("t2 pass");
	tst->set_proxy_port(524);
	tst->set_proxy_type(2);
	tst->set_proxy_user("t2 user");

	QCOMPARE(prefs.proxy_auth, false);
	QCOMPARE(QString(prefs.proxy_host), QString("t2 host"));
	QCOMPARE(QString(prefs.proxy_pass), QString("t2 pass"));
	QCOMPARE(prefs.proxy_port, 524);
	QCOMPARE(prefs.proxy_type, 2);
	QCOMPARE(QString(prefs.proxy_user), QString("t2 user"));
}

void TestQPrefProxy::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefProxy::instance();

	tst->set_proxy_auth(true);
	tst->set_proxy_host("t3 host");
	tst->set_proxy_pass("t3 pass");
	tst->set_proxy_port(532);
	tst->set_proxy_type(1);
	tst->set_proxy_user("t3 user");

	tst->sync();
	prefs.proxy_auth = false;
	prefs.proxy_host = copy_qstring("error1");
	prefs.proxy_pass = copy_qstring("error1");
	prefs.proxy_port = 128;
	prefs.proxy_type = 0;
	prefs.proxy_user = copy_qstring("error1");

	tst->load();
	QCOMPARE(prefs.proxy_auth, true);
	QCOMPARE(QString(prefs.proxy_host), QString("t3 host"));
	QCOMPARE(QString(prefs.proxy_pass), QString("t3 pass"));
	QCOMPARE(prefs.proxy_port, 532);
	QCOMPARE(prefs.proxy_type, 1);
	QCOMPARE(QString(prefs.proxy_user), QString("t3 user"));
}

void TestQPrefProxy::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefProxy::instance();

	prefs.proxy_auth = false;
	prefs.proxy_host = copy_qstring("t4 host");
	prefs.proxy_pass = copy_qstring("t4 pass");
	prefs.proxy_port = 544;
	prefs.proxy_type = 4;
	prefs.proxy_user = copy_qstring("t4 user");

	tst->sync();
	prefs.proxy_auth = true;
	prefs.proxy_host = copy_qstring("error1");
	prefs.proxy_pass = copy_qstring("error1");
	prefs.proxy_port = 128;
	prefs.proxy_type = 5;
	prefs.proxy_user = copy_qstring("error1");

	tst->load();
	QCOMPARE(prefs.proxy_auth, false);
	QCOMPARE(QString(prefs.proxy_host), QString("t4 host"));
	QCOMPARE(QString(prefs.proxy_pass), QString("t4 pass"));
	QCOMPARE(prefs.proxy_port, 544);
	QCOMPARE(prefs.proxy_type, 4);
	QCOMPARE(QString(prefs.proxy_user), QString("t4 user"));
}

void TestQPrefProxy::test_multiple()
{
	// test multiple instances have the same information

	prefs.proxy_port= 37;
	auto tst_direct = new qPrefProxy;

	prefs.proxy_type = 25;
	auto tst = qPrefProxy::instance();

	QCOMPARE(tst->proxy_port(), tst_direct->proxy_port());
	QCOMPARE(tst->proxy_port(), 37);
	QCOMPARE(tst->proxy_type(), tst_direct->proxy_type());
	QCOMPARE(tst->proxy_type(), 25);
}

QTEST_MAIN(TestQPrefProxy)
