// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDiveComputer.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDiveComputer.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefDiveComputer::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDiveComputer");
}

void TestQPrefDiveComputer::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefDiveComputer::instance();

	prefs.dive_computer.device = "my device";
	prefs.dive_computer.device_name = "my device name";
	prefs.dive_computer.product = "my product";
	prefs.dive_computer.vendor = "my vendor";

	QCOMPARE(tst->device(), QString::fromStdString(prefs.dive_computer.device));
	QCOMPARE(tst->device_name(), QString::fromStdString(prefs.dive_computer.device_name));
	QCOMPARE(tst->product(), QString::fromStdString(prefs.dive_computer.product));
	QCOMPARE(tst->vendor(), QString::fromStdString(prefs.dive_computer.vendor));
}

void TestQPrefDiveComputer::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefDiveComputer::instance();

	tst->set_device("t2 device");
	tst->set_device_name("t2 device name");
	tst->set_product("t2 product");
	tst->set_vendor("t2 vendor");

	QCOMPARE(QString::fromStdString(prefs.dive_computer.device), QString("t2 device"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.device_name), QString("t2 device name"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.product), QString("t2 product"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.vendor), QString("t2 vendor"));
}

void TestQPrefDiveComputer::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefDiveComputer::instance();

	tst->set_device("t3 device");
	tst->set_device_name("t3 device name");
	tst->set_product("t3 product");
	tst->set_vendor("t3 vendor");

	prefs.dive_computer.device = "error1";
	prefs.dive_computer.device_name = "error2";
	prefs.dive_computer.product = "error3";
	prefs.dive_computer.vendor = "error4";

	tst->load();
	QCOMPARE(QString::fromStdString(prefs.dive_computer.device), QString("t3 device"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.device_name), QString("t3 device name"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.product), QString("t3 product"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.vendor), QString("t3 vendor"));
}

void TestQPrefDiveComputer::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefDiveComputer::instance();

	prefs.dive_computer.device = "t4 device";
	prefs.dive_computer.device_name = "t4 device name";
	prefs.dive_computer.product = "t4 product";
	prefs.dive_computer.vendor = "t4 vendor";

	tst->sync();

	prefs.dive_computer.device = "error";
	prefs.dive_computer.device_name = "error";
	prefs.dive_computer.product = "error";
	prefs.dive_computer.vendor = "error";

	tst->load();

	QCOMPARE(QString::fromStdString(prefs.dive_computer.device), QString("t4 device"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.device_name), QString("t4 device name"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.product), QString("t4 product"));
	QCOMPARE(QString::fromStdString(prefs.dive_computer.vendor), QString("t4 vendor"));
}

void TestQPrefDiveComputer::test_multiple()
{
	// test multiple instances have the same information

	auto tst = qPrefDiveComputer::instance();
	prefs.dive_computer.device = "mine";

	QCOMPARE(tst->device(), qPrefDiveComputer::device());
	QCOMPARE(tst->device(), QString("mine"));
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	dc->sync();              \
	dc->load();              \
	QCOMPARE(METHOD, VALUE);

void TestQPrefDiveComputer::test_oldPreferences()
{
	auto dc = qPrefDiveComputer::instance();

	dc->set_device("TomazComputer");
	TEST(dc->device(), QStringLiteral("TomazComputer"));
	dc->set_device("Deepwater");
	TEST(dc->device(), QStringLiteral("Deepwater"));

	dc->set_product("Thingy1");
	TEST(dc->product(), QStringLiteral("Thingy1"));
	dc->set_product("Thingy2");
	TEST(dc->product(), QStringLiteral("Thingy2"));

	dc->set_vendor("Sharewater");
	TEST(dc->vendor(), QStringLiteral("Sharewater"));
	dc->set_vendor("OSTS");
	TEST(dc->vendor(), QStringLiteral("OSTS"));
}

void TestQPrefDiveComputer::test_signals()
{
	QSignalSpy spy1(qPrefDiveComputer::instance(), &qPrefDiveComputer::deviceChanged);
	QSignalSpy spy2(qPrefDiveComputer::instance(), &qPrefDiveComputer::device_nameChanged);
	QSignalSpy spy4(qPrefDiveComputer::instance(), &qPrefDiveComputer::productChanged);
	QSignalSpy spy5(qPrefDiveComputer::instance(), &qPrefDiveComputer::vendorChanged);

	qPrefDiveComputer::set_device("t_signal device");
	qPrefDiveComputer::set_device_name("t_signal device name");
	qPrefDiveComputer::set_product("t_signal product");
	qPrefDiveComputer::set_vendor("t_signal vendor");

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toString() == "t_signal device");
	QVERIFY(spy2.takeFirst().at(0).toString() == "t_signal device name");
	QVERIFY(spy4.takeFirst().at(0).toString() == "t_signal product");
	QVERIFY(spy5.takeFirst().at(0).toString() == "t_signal vendor");
}

QTEST_MAIN(TestQPrefDiveComputer)
