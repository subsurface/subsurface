// SPDX-License-Identifier: GPL-2.0
#include "testqPrefDiveComputer.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPref.h"

#include <QTest>

void TestQPrefDiveComputer::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefDiveComputer");
}

void TestQPrefDiveComputer::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefDiveComputer::instance();

	prefs.dive_computer.device = copy_qstring("my device");
	prefs.dive_computer.device_name = copy_qstring("my device name");
	prefs.dive_computer.download_mode = 17;
	prefs.dive_computer.product = copy_qstring("my product");
	prefs.dive_computer.vendor = copy_qstring("my vendor");

	QCOMPARE(tst->device(), QString(prefs.dive_computer.device));
	QCOMPARE(tst->device_name(), QString(prefs.dive_computer.device_name));
	QCOMPARE(tst->download_mode(), prefs.dive_computer.download_mode);
	QCOMPARE(tst->product(), QString(prefs.dive_computer.product));
	QCOMPARE(tst->vendor(), QString(prefs.dive_computer.vendor));
}

void TestQPrefDiveComputer::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefDiveComputer::instance();

	tst->set_device("t2 device");
	tst->set_device_name("t2 device name");
	tst->set_download_mode(27);
	tst->set_product("t2 product");
	tst->set_vendor("t2 vendor");

	QCOMPARE(QString(prefs.dive_computer.device), QString("t2 device"));
	QCOMPARE(QString(prefs.dive_computer.device_name), QString("t2 device name"));
	QCOMPARE(prefs.dive_computer.download_mode, 27);
	QCOMPARE(QString(prefs.dive_computer.product), QString("t2 product"));
	QCOMPARE(QString(prefs.dive_computer.vendor), QString("t2 vendor"));
}

void TestQPrefDiveComputer::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefDiveComputer::instance();

	tst->set_device("t3 device");
	tst->set_device_name("t3 device name");
	tst->set_download_mode(37);
	tst->set_product("t3 product");
	tst->set_vendor("t3 vendor");

	prefs.dive_computer.device = copy_qstring("error1");
	prefs.dive_computer.device_name = copy_qstring("error2");
	prefs.dive_computer.download_mode = 38;
	prefs.dive_computer.product = copy_qstring("error3");
	prefs.dive_computer.vendor = copy_qstring("error4");

	tst->load();
	QCOMPARE(QString(prefs.dive_computer.device), QString("t3 device"));
	QCOMPARE(QString(prefs.dive_computer.device_name), QString("t3 device name"));
	QCOMPARE(prefs.dive_computer.download_mode, 37);
	QCOMPARE(QString(prefs.dive_computer.product), QString("t3 product"));
	QCOMPARE(QString(prefs.dive_computer.vendor), QString("t3 vendor"));
}

void TestQPrefDiveComputer::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefDiveComputer::instance();

	prefs.dive_computer.device = copy_qstring("t4 device");
	prefs.dive_computer.device_name = copy_qstring("t4 device name");
	prefs.dive_computer.download_mode = 47;
	prefs.dive_computer.product = copy_qstring("t4 product");
	prefs.dive_computer.vendor = copy_qstring("t4 vendor");

	tst->sync();

	prefs.dive_computer.device = copy_qstring("error");
	prefs.dive_computer.device_name = copy_qstring("error");
	prefs.dive_computer.download_mode = 48;
	prefs.dive_computer.product = copy_qstring("error");
	prefs.dive_computer.vendor = copy_qstring("error");

	tst->load();

	QCOMPARE(QString(prefs.dive_computer.device), QString("t4 device"));
	QCOMPARE(QString(prefs.dive_computer.device_name), QString("t4 device name"));
	QCOMPARE(prefs.dive_computer.download_mode, 47);
	QCOMPARE(QString(prefs.dive_computer.product), QString("t4 product"));
	QCOMPARE(QString(prefs.dive_computer.vendor), QString("t4 vendor"));
}

void TestQPrefDiveComputer::test_multiple()
{
	// test multiple instances have the same information
	auto tst_direct = new qPrefDiveComputer;
	prefs.dive_computer.download_mode = 57;

	auto tst = qPrefDiveComputer::instance();
	prefs.dive_computer.device = copy_qstring("mine");

	QCOMPARE(tst->device(), tst_direct->device());
	QCOMPARE(tst->device(), QString("mine"));
	QCOMPARE(tst->download_mode(), tst_direct->download_mode());
	QCOMPARE(tst->download_mode(), 57);
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

	dc->set_download_mode(0);
	TEST(dc->download_mode(), 0);
	dc->set_download_mode(1);
	TEST(dc->download_mode(), 1);

	dc->set_product("Thingy1");
	TEST(dc->product(), QStringLiteral("Thingy1"));
	dc->set_product("Thingy2");
	TEST(dc->product(), QStringLiteral("Thingy2"));

	dc->set_vendor("Sharewater");
	TEST(dc->vendor(), QStringLiteral("Sharewater"));
	dc->set_vendor("OSTS");
	TEST(dc->vendor(), QStringLiteral("OSTS"));
}

QTEST_MAIN(TestQPrefDiveComputer)
