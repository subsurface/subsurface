// SPDX-License-Identifier: GPL-2.0
#include "testgitstorage.h"
#include "git2.h"

#include "core/divesite.h"
#include "core/file.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPrefProxy.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/trip.h"

#include <QDir>
#include <QTextStream>
#include <QNetworkProxy>
#include <QTextCodec>
#include <QDebug>

// this is a local helper function in git-access.c
extern "C" char *get_local_dir(const char *remote, const char *branch);

void TestGitStorage::initTestCase()
{
	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences an proxy information
	copy_prefs(&default_prefs, &prefs);
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
	qPrefProxy::load();
	qPrefCloudStorage::load();

	QString gitUrl(prefs.cloud_base_url);
	if (gitUrl.right(1) != "/")
		gitUrl += "/";
	prefs.cloud_git_url = copy_qstring(gitUrl + "git");
	prefs.cloud_storage_email_encoded = strdup("ssrftest@hohndel.org");
	prefs.cloud_storage_password = strdup("geheim");
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(prefs.proxy_host);
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(prefs.proxy_user);
		proxy.setPassword(prefs.proxy_pass);
	}
	QNetworkProxy::setApplicationProxy(proxy);

	// now cleanup the cache dir in case there's something weird from previous runs
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QDir localCacheDirectory(localCacheDir);
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
}

void TestGitStorage::cleanup()
{
	clear_dive_file_data();
}

void TestGitStorage::testGitStorageLocal_data()
{
	// Test different paths we may encounter (since storage depends on user name)
	// as well as with and without "file://" URL prefix.
	QTest::addColumn<QString>("testDirName");
	QTest::addColumn<QString>("prefixRead");
	QTest::addColumn<QString>("prefixWrite");
	QTest::newRow("ASCII path") << "./gittest" << "" << "";
	QTest::newRow("Non ASCII path") << "./gittest_éèêôàüäößíñóúäåöø" << "" << "";
	QTest::newRow("ASCII path with file:// prefix on read") << "./gittest2" << "file://" << "";
	QTest::newRow("Non ASCII path with file:// prefix on read") << "./gittest2_éèêôàüäößíñóúäåöø" << "" << "file://";
	QTest::newRow("ASCII path with file:// prefix on write") << "./gittest3" << "file://" << "";
	QTest::newRow("Non ASCII path with file:// prefix on write") << "./gittest3_éèêôàüäößíñóúäåöø" << "" << "file://";
}

void TestGitStorage::testGitStorageLocal()
{
	// test writing and reading back from local git storage
	git_repository *repo;
	git_libgit2_init();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &dive_table, &trip_table, &dive_site_table), 0);
	QFETCH(QString, testDirName);
	QFETCH(QString, prefixRead);
	QFETCH(QString, prefixWrite);
	QDir testDir(testDirName);
	QCOMPARE(testDir.removeRecursively(), true);
	QCOMPARE(QDir().mkdir(testDirName), true);
	QString repoNameRead = prefixRead + testDirName;
	QString repoNameWrite = prefixWrite + testDirName;
	QCOMPARE(git_repository_init(&repo, qPrintable(testDirName), false), 0);
	QCOMPARE(save_dives(qPrintable(repoNameWrite + "[test]")), 0);
	QCOMPARE(save_dives("./SampleDivesV3.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(qPrintable(repoNameRead + "[test]"), &dive_table, &trip_table, &dive_site_table), 0);
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
}

void TestGitStorage::testGitStorageCloud()
{
	// test writing and reading back from cloud storage
	// connect to the ssrftest repository on the cloud server
	// and repeat the same test as before with the local git storage
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives("./SampleDivesV3viacloud.ssrf"), 0);
	QFile org("./SampleDivesV3.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3viacloud.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudOfflineSync()
{
	// make a change to local cache repo (pretending that we did some offline changes)
	// and then open the remote one again and check that things were propagated correctly
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepo = localCacheDir + "[ssrftest@hohndel.org]";
	// read the local repo from the previous test and add dive 10
	QCOMPARE(parse_file(qPrintable(localCacheRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &dive_table, &trip_table, &dive_site_table), 0);
	// calling process_loaded_dives() sorts the table, but calling add_imported_dives()
	// causes it to try to update the window title... let's not do that
	process_loaded_dives();
	// now save only to the local cache but not to the remote server
	QCOMPARE(save_dives(qPrintable(localCacheRepo)), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10local.ssrf"), 0);
	clear_dive_file_data();
	// open the cloud storage and compare
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10viacloud.ssrf"), 0);
	QFile org("./SampleDivesV3plus10local.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3plus10viacloud.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	// write back out to cloud storage, move away the local cache, open again and compare
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();
	QDir localCacheDirectory(localCacheDir);
	QDir localCacheDirectorySave(localCacheDir + "save");
	QCOMPARE(localCacheDirectorySave.removeRecursively(), true);
	QCOMPARE(localCacheDirectory.rename(localCacheDir, localCacheDir + "save"), true);
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10fromcloud.ssrf"), 0);
	org.close();
	org.open(QFile::ReadOnly);
	QFile out2("./SampleDivesV3plus10fromcloud.ssrf");
	out2.open(QFile::ReadOnly);
	QTextStream orgS2(&org);
	QTextStream outS2(&out2);
	readin = orgS2.readAll();
	written = outS2.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge()
{
	// now we need to mess with the local git repo to get an actual merge
	// first we add another dive to the "moved away" repository, pretending we did
	// another offline change there
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepoSave = localCacheDir + "save[ssrftest@hohndel.org]";
	QCOMPARE(parse_file(qPrintable(localCacheRepoSave), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test11.xml", &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives(qPrintable(localCacheRepoSave)), 0);
	clear_dive_file_data();

	// now we open the cloud storage repo and add a different dive to it
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test12.xml", &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// now we move the saved local cache into place and try to open the cloud repo
	// -> this forces a merge
	QDir localCacheDirectory(localCacheDir);
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
	QDir localCacheDirectorySave(localCacheDir + "save");
	QCOMPARE(localCacheDirectory.rename(localCacheDir + "save", localCacheDir), true);
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12-merged.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file("./SampleDivesV3plus10local.ssrf", &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test11.xml", &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test12.xml", &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12.ssrf"), 0);
	QFile org("./SampleDivesV3plus10-11-12-merged.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3plus10-11-12.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge2()
{
	// delete a dive offline
	// edit the same dive in the cloud repo
	// merge
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepo = localCacheDir + "[ssrftest@hohndel.org]";
	QCOMPARE(parse_file(qPrintable(localCacheRepo), &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	struct dive *dive = get_dive(1);
	delete_single_dive(1);
	QCOMPARE(save_dives("./SampleDivesMinus1.ssrf"), 0);
	QCOMPARE(save_dives(qPrintable(localCacheRepo)), 0);
	clear_dive_file_data();

	// move the local cache away
	{ // scope for variables
		QDir localCacheDirectory(localCacheDir);
		QDir localCacheDirectorySave(localCacheDir + "save");
		QCOMPARE(localCacheDirectorySave.removeRecursively(), true);
		QCOMPARE(localCacheDirectory.rename(localCacheDir, localCacheDir + "save"), true);
	}
	// now we open the cloud storage repo and modify that first dive
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	dive = get_dive(1);
	QVERIFY(dive != NULL);
	free(dive->notes);
	dive->notes = strdup("These notes have been modified by TestGitStorage");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// now we move the saved local cache into place and try to open the cloud repo
	// -> this forces a merge
	QDir localCacheDirectory(localCacheDir);
	QDir localCacheDirectorySave(localCacheDir + "save");
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
	QCOMPARE(localCacheDirectorySave.rename(localCacheDir + "save", localCacheDir), true);

	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives("./SampleDivesMinus1-merged.ssrf"), 0);
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	QFile org("./SampleDivesMinus1-merged.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesMinus1.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge3()
{
	// create multi line notes and store them to the cloud repo and local cache
	// edit dive notes offline
	// edit the same dive notes in the cloud repo
	// merge
	clear_dive_file_data();
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepo = localCacheDir + "[ssrftest@hohndel.org]";
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	struct dive *dive = get_dive(0);
	QVERIFY(dive != 0);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	dive = get_dive(1);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	dive = get_dive(2);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	QCOMPARE(parse_file(qPrintable(localCacheRepo), &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	dive = get_dive(0);
	dive->notes = strdup("Create multi line dive notes\nDifferent line 2 and removed 3-5\n\nThat should be enough");
	dive = get_dive(1);
	dive->notes = strdup("Line 2\nLine 3\nLine 4\nLine 5"); // keep the middle, remove first and last");
	dive = get_dive(2);
	dive->notes = strdup("single line dive notes");
	QCOMPARE(save_dives(qPrintable(localCacheRepo)), 0);
	clear_dive_file_data();

	// move the local cache away
	{ // scope for variables
		QDir localCacheDirectory(localCacheDir);
		QDir localCacheDirectorySave(localCacheDir + "save");
		QCOMPARE(localCacheDirectorySave.removeRecursively(), true);
		QCOMPARE(localCacheDirectory.rename(localCacheDir, localCacheDir + "save"), true);
	}
	// now we open the cloud storage repo and modify those first dive notes differently
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	process_loaded_dives();
	dive = get_dive(0);
	dive->notes = strdup("Completely different dive notes\nBut also multi line");
	dive = get_dive(1);
	dive->notes = strdup("single line dive notes");
	dive = get_dive(2);
	dive->notes = strdup("Line 2\nLine 3\nLine 4\nLine 5"); // keep the middle, remove first and last");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// now we move the saved local cache into place and try to open the cloud repo
	// -> this forces a merge
	QDir localCacheDirectory(localCacheDir);
	QDir localCacheDirectorySave(localCacheDir + "save");
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
	QCOMPARE(localCacheDirectorySave.rename(localCacheDir + "save", localCacheDir), true);

	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table, &dive_site_table), 0);
	QCOMPARE(save_dives("./SampleDivesMerge3.ssrf"), 0);
	// we are not trying to compare this to a pre-determined result... what this test
	// checks is that there are no parsing errors with the merge
}

QTEST_GUILESS_MAIN(TestGitStorage)
