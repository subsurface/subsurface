// SPDX-License-Identifier: GPL-2.0
#include "testqPrefMedia.h"

#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefMedia.h"
#include "core/settings/qPref.h"

#include <QTest>
#include <QSignalSpy>

void TestQPrefMedia::initTestCase()
{
	TestBase::initTestCase();

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("SubsurfaceTestQPrefMedia");
	qPref::registerQML(NULL);
}

void TestQPrefMedia::test_struct_get()
{
	// Test struct pref -> get func.

	auto tst = qPrefMedia::instance();

	prefs.auto_recalculate_thumbnails = true;
	prefs.extract_video_thumbnails = true;
	prefs.extract_video_thumbnails_position = 15;
	prefs.ffmpeg_executable = "new base16";

	QCOMPARE(tst->auto_recalculate_thumbnails(), prefs.auto_recalculate_thumbnails);
	QCOMPARE(tst->extract_video_thumbnails(), prefs.extract_video_thumbnails);
	QCOMPARE(tst->extract_video_thumbnails_position(), prefs.extract_video_thumbnails_position);
	QCOMPARE(tst->ffmpeg_executable(), QString::fromStdString(prefs.ffmpeg_executable));
}

void TestQPrefMedia::test_set_struct()
{
	// Test set func -> struct pref

	auto tst = qPrefMedia::instance();

	tst->set_auto_recalculate_thumbnails(false);
	tst->set_extract_video_thumbnails(false);
	tst->set_extract_video_thumbnails_position(25);
	tst->set_ffmpeg_executable("new base26");

	QCOMPARE(prefs.auto_recalculate_thumbnails, false);
	QCOMPARE(prefs.extract_video_thumbnails, false);
	QCOMPARE(prefs.extract_video_thumbnails_position, 25);
	QCOMPARE(QString::fromStdString(prefs.ffmpeg_executable), QString("new base26"));
}

void TestQPrefMedia::test_set_load_struct()
{
	// test set func -> load -> struct pref

	auto tst = qPrefMedia::instance();

	tst->set_auto_recalculate_thumbnails(true);
	tst->set_extract_video_thumbnails(true);
	tst->set_extract_video_thumbnails_position(35);
	tst->set_ffmpeg_executable("new base36");

	prefs.auto_recalculate_thumbnails = false;
	prefs.extract_video_thumbnails = false;
	prefs.extract_video_thumbnails_position = 15;
	prefs.ffmpeg_executable = "error";

	tst->load();
	QCOMPARE(prefs.auto_recalculate_thumbnails, true);
	QCOMPARE(prefs.extract_video_thumbnails, true);
	QCOMPARE(prefs.extract_video_thumbnails_position, 35);
	QCOMPARE(QString::fromStdString(prefs.ffmpeg_executable), QString("new base36"));
}

void TestQPrefMedia::test_struct_disk()
{
	// test struct prefs -> disk

	auto tst = qPrefMedia::instance();

	prefs.auto_recalculate_thumbnails = true;
	prefs.extract_video_thumbnails = true;
	prefs.extract_video_thumbnails_position = 45;
	prefs.ffmpeg_executable = "base46";

	tst->sync();
	prefs.auto_recalculate_thumbnails = false;
	prefs.extract_video_thumbnails = false;
	prefs.extract_video_thumbnails_position = 15;
	prefs.ffmpeg_executable = "error";

	tst->load();
	QCOMPARE(prefs.auto_recalculate_thumbnails, true);
	QCOMPARE(prefs.extract_video_thumbnails, true);
	QCOMPARE(prefs.extract_video_thumbnails_position, 45);
	QCOMPARE(QString::fromStdString(prefs.ffmpeg_executable), QString("base46"));
}

#define TEST(METHOD, VALUE)      \
	QCOMPARE(METHOD, VALUE); \
	media->sync();           \
	media->load();           \
	QCOMPARE(METHOD, VALUE);

void TestQPrefMedia::test_signals()
{
	QSignalSpy spy1(qPrefMedia::instance(), &qPrefMedia::auto_recalculate_thumbnailsChanged);
	QSignalSpy spy6(qPrefMedia::instance(), &qPrefMedia::extract_video_thumbnailsChanged);
	QSignalSpy spy7(qPrefMedia::instance(), &qPrefMedia::extract_video_thumbnails_positionChanged);
	QSignalSpy spy8(qPrefMedia::instance(), &qPrefMedia::ffmpeg_executableChanged);

	prefs.auto_recalculate_thumbnails = true;
	qPrefMedia::set_auto_recalculate_thumbnails(false);

	qPrefMedia::set_extract_video_thumbnails(false);
	qPrefMedia::set_extract_video_thumbnails_position(25);
	qPrefMedia::set_ffmpeg_executable("new base26");

	QCOMPARE(spy1.count(), 1);

	QVERIFY(spy1.takeFirst().at(0).toBool() == false);

	qPrefMedia::set_extract_video_thumbnails(false);
	qPrefMedia::set_extract_video_thumbnails_position(25);
	qPrefMedia::set_ffmpeg_executable("new base26");
}

QTEST_MAIN(TestQPrefMedia)
