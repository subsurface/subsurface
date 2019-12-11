// SPDX-License-Identifier: GPL-2.0
#include "testqPrefLog.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefLog.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefLog::initTestCase()
{
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefLog");
	qPref::registerQML(NULL);
}

void TestQPrefLog::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefLog::instance();

	prefs.default_filename = copy_qstring("new base12");
	prefs.default_file_behavior = UNDEFINED_DEFAULT_FILE;
	prefs.use_default_file = true;
	prefs.show_average_depth = true;
	prefs.extraEnvironmentalDefault = true;

	QCOMPARE(tst->default_filename(), QString(prefs.default_filename));
	QCOMPARE(tst->default_file_behavior(), prefs.default_file_behavior);
	QCOMPARE(tst->use_default_file(), prefs.use_default_file);
	QCOMPARE(tst->show_average_depth(), prefs.show_average_depth);
	QCOMPARE(tst->extraEnvironmentalDefault(), prefs.extraEnvironmentalDefault);
}

void TestQPrefLog::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefLog::instance();

	tst->set_default_filename("new base22");
	tst->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	tst->set_use_default_file(false);
	tst->set_show_average_depth(false);
	tst->set_extraEnvironmentalDefault(false);

	QCOMPARE(QString(prefs.default_filename), QString("new base22"));
	QCOMPARE(prefs.default_file_behavior, LOCAL_DEFAULT_FILE);
	QCOMPARE(prefs.use_default_file, false);
	QCOMPARE(prefs.show_average_depth, false);
	QCOMPARE(prefs.extraEnvironmentalDefault, false);
}

void TestQPrefLog::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefLog::instance();

	tst->set_default_filename("new base32");
	tst->set_default_file_behavior(NO_DEFAULT_FILE);
	tst->set_use_default_file(true);
	tst->set_show_average_depth(true);
	tst->set_extraEnvironmentalDefault(true);

	prefs.default_filename = copy_qstring("error");
	prefs.default_file_behavior = UNDEFINED_DEFAULT_FILE;
	prefs.use_default_file = false;
	prefs.show_average_depth = false;
	prefs.extraEnvironmentalDefault = false;

	tst->load();
	QCOMPARE(QString(prefs.default_filename), QString("new base32"));
	QCOMPARE(prefs.default_file_behavior, NO_DEFAULT_FILE);
	QCOMPARE(prefs.use_default_file, true);
	QCOMPARE(prefs.show_average_depth, true);
	QCOMPARE(prefs.extraEnvironmentalDefault, true);
}

void TestQPrefLog::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefLog::instance();

	prefs.default_filename = copy_qstring("base42");
	prefs.default_file_behavior = CLOUD_DEFAULT_FILE;
	prefs.use_default_file = true;
	prefs.show_average_depth = true;
	prefs.extraEnvironmentalDefault = true;

	tst->sync();
	prefs.default_filename = copy_qstring("error");
	prefs.default_file_behavior = UNDEFINED_DEFAULT_FILE;
	prefs.use_default_file = false;
	prefs.show_average_depth = false;
	prefs.extraEnvironmentalDefault = false;

	tst->load();
	QCOMPARE(QString(prefs.default_filename), QString("base42"));
	QCOMPARE(prefs.default_file_behavior, CLOUD_DEFAULT_FILE);
	QCOMPARE(prefs.use_default_file, true);
	QCOMPARE(prefs.show_average_depth, true);
	QCOMPARE(prefs.extraEnvironmentalDefault, true);
}

void TestQPrefLog::test_multiple()
{
	// test multiple instances have the same information
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	log->sync();           \
	log->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefLog::test_oldPreferences()
{
	auto log = qPrefLog::instance();

	log->set_default_filename("filename");
	log->set_default_file_behavior(LOCAL_DEFAULT_FILE);
	log->set_use_default_file(true);
	log->set_show_average_depth(true);
	log->set_extraEnvironmentalDefault(true);

	TEST(log->default_filename(), QStringLiteral("filename"));
	TEST(log->default_file_behavior(), LOCAL_DEFAULT_FILE); // since we have a default file, here it returns
	TEST(log->use_default_file(), true);
	TEST(log->show_average_depth(), true);
	TEST(log->extraEnvironmentalDefault(), true);

	//TODO: Change this to a enum.
	log->set_default_filename("no_file_name");
	log->set_default_file_behavior(CLOUD_DEFAULT_FILE);
	log->set_use_default_file(false);
	log->set_show_average_depth(false);
	log->set_extraEnvironmentalDefault(false);

	TEST(log->default_filename(), QStringLiteral("no_file_name"));
	TEST(log->default_file_behavior(), CLOUD_DEFAULT_FILE);
	TEST(log->use_default_file(), false);
	TEST(log->extraEnvironmentalDefault(), false);
	TEST(log->show_average_depth(), false);
}

void TestQPrefLog::test_signals()
{
	QSignalSpy spy1(qPrefLog::instance(), &qPrefLog::default_filenameChanged);
	QSignalSpy spy2(qPrefLog::instance(), &qPrefLog::default_file_behaviorChanged);
	QSignalSpy spy3(qPrefLog::instance(), &qPrefLog::use_default_fileChanged);
	QSignalSpy spy4(qPrefLog::instance(), &qPrefLog::show_average_depthChanged);
	QSignalSpy spy5(qPrefLog::instance(), &qPrefLog::extraEnvironmentalDefaultChanged);

	qPrefLog::set_default_filename("new base22");
	qPrefLog::set_default_file_behavior(LOCAL_DEFAULT_FILE);
	qPrefLog::set_use_default_file(false);
	qPrefLog::set_show_average_depth(false);
	qPrefLog::set_extraEnvironmentalDefault(false);

	qPrefLog::set_default_filename("new base22");
	qPrefLog::set_default_file_behavior(LOCAL_DEFAULT_FILE);
	prefs.use_default_file = true;
	qPrefLog::set_use_default_file(false);
	prefs.extraEnvironmentalDefault = true;
	qPrefLog::set_extraEnvironmentalDefault(false);
	prefs.show_average_depth = true;
	qPrefLog::set_show_average_depth(false);

	QCOMPARE(spy1.count(), 1);
	QCOMPARE(spy2.count(), 1);
	QCOMPARE(spy3.count(), 1);
	QCOMPARE(spy4.count(), 1);
	QCOMPARE(spy5.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toBool() == false);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
	QVERIFY(spy2.takeFirst().at(0).toBool() == false);
	QVERIFY(spy4.takeFirst().at(0).toBool() == false);
	QVERIFY(spy5.takeFirst().at(0).toBool() == false);

}


QTEST_MAIN(TestQPrefLog)
