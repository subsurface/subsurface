#include "testgitstorage.h"
#include "dive.h"
#include "divelist.h"
#include "file.h"
#include "git2.h"
#include <QDir>
#include <QTextStream>

void TestGitStorage::testGitStorageLocal()
{
	// test writing and reading back from local git storage
	git_repository *repo;
	git_libgit2_init();
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/SampleDivesV2.ssrf"), 0);
	QString testDirName("./gittest");
	QDir testDir(testDirName);
	QCOMPARE(testDir.removeRecursively(), true);
	QCOMPARE(QDir().mkdir(testDirName), true);
	QCOMPARE(git_repository_init(&repo, qPrintable(testDirName), false), 0);
	QCOMPARE(save_dives(qPrintable(testDirName + "[test]")), 0);
	QCOMPARE(save_dives("./SampleDivesV3.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(qPrintable(testDirName + "[test]")), 0);
	QCOMPARE(save_dives("./SampleDivesV3viagit.ssrf"), 0);
	QFile org("./SampleDivesV3.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3viagit.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

QTEST_MAIN(TestGitStorage)
