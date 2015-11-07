#include "testgitstorage.h"
#include "dive.h"
#include "divelist.h"
#include "file.h"
#include "git2.h"
#include "prefs-macros.h"
#include "subsurfacestartup.h"
#include <QDir>
#include <QTextStream>
#include <QNetworkProxy>
#include <QSettings>
#include <QDebug>

// this is a local helper function in git-access.c
extern "C" char *get_local_dir(const char *remote, const char *branch);

void TestGitStorage::testSetup()
{
	// first, setup the preferences an proxy information
	copy_prefs(&default_prefs, &prefs);
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
	QSettings s;
	QVariant v;
	s.beginGroup("Network");
	GET_INT_DEF("proxy_type", proxy_type, QNetworkProxy::DefaultProxy);
	GET_TXT("proxy_host", proxy_host);
	GET_INT("proxy_port", proxy_port);
	GET_BOOL("proxy_auth", proxy_auth);
	GET_TXT("proxy_user", proxy_user);
	GET_TXT("proxy_pass", proxy_pass);
	s.endGroup();
	s.beginGroup("CloudStorage");
	GET_TXT("cloud_base_url", cloud_base_url);
	QString gitUrl(prefs.cloud_base_url);
	if (gitUrl.right(1) != "/")
		gitUrl += "/";
	prefs.cloud_git_url = strdup(qPrintable(gitUrl + "git"));
	s.endGroup();
	prefs.cloud_storage_email_encoded = strdup("ssrftest@hohndel.org");
	prefs.cloud_storage_password = strdup("geheim");
	prefs.cloud_background_sync = true;
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

void TestGitStorage::testGitStorageCloud()
{
	// test writing and reading back from cloud storage
	// connect to the ssrftest repository on the cloud server
	// and repeat the same test as before with the local git storage
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/SampleDivesV2.ssrf"), 0);
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
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
	clear_dive_file_data();
}

void TestGitStorage::testGitStorageCloudOfflineSync()
{
	// make a change to local cache repo (pretending that we did some offline changes)
	// and then open the remote one again and check that things were propagated correctly
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepo = localCacheDir + "[ssrftest@hohndel.org]";
	// read the local repo from the previous test and add dive 10
	QCOMPARE(parse_file(qPrintable(localCacheRepo)), 0);
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test10.xml"), 0);
	// calling process_dive() sorts the table, but calling it with
	// is_imported == true causes it to try to update the window title... let's not do that
	process_dives(false, false);
	// now save only to the local cache but not to the remote server
	QCOMPARE(save_dives(qPrintable(localCacheRepo)), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10local.ssrf"), 0);
	clear_dive_file_data();
	// open the cloud storage and compare
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
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
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
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
	clear_dive_file_data();
}

void TestGitStorage::testGitStorageCloudMerge()
{
	// now we need to mess with the local git repo to get an actual merge
	// first we add another dive to the "moved away" repository, pretending we did
	// another offline change there
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepoSave = localCacheDir + "save[ssrftest@hohndel.org]";
	QCOMPARE(parse_file(qPrintable(localCacheRepoSave)), 0);
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test11.xml"), 0);
	process_dives(false, false);
	QCOMPARE(save_dives(qPrintable(localCacheRepoSave)), 0);
	clear_dive_file_data();

	// now we open the cloud storage repo and add a different dive to it
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test12.xml"), 0);
	process_dives(false, false);
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// now we move the saved local cache into place and try to open the cloud repo
	// -> this forces a merge
	QDir localCacheDirectory(localCacheDir);
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
	QDir localCacheDirectorySave(localCacheDir + "save");
	QCOMPARE(localCacheDirectory.rename(localCacheDir + "save", localCacheDir), true);
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
	QCOMPARE(save_dives("./SapleDivesV3plus10-11-12-merged.ssrf"), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file("./SampleDivesV3plus10local.ssrf"), 0);
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test11.xml"), 0);
	process_dives(false, false);
	QCOMPARE(parse_file(SUBSURFACE_SOURCE "/dives/test12.xml"), 0);
	process_dives(false, false);
	QCOMPARE(save_dives("./SapleDivesV3plus10-11-12.ssrf"), 0);
	QFile org("./SapleDivesV3plus10-11-12-merged.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SapleDivesV3plus10-11-12.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();
}

void TestGitStorage::testGitStorageCloudMerge2()
{
	// delete a dive offline
	// edit the same dive in the cloud repo
	// merge
	QString cloudTestRepo("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org[ssrftest@hohndel.org]");
	QString localCacheDir(get_local_dir("https://cloud.subsurface-divelog.org/git/ssrftest@hohndel.org", "ssrftest@hohndel.org"));
	QString localCacheRepo = localCacheDir + "[ssrftest@hohndel.org]";
	QCOMPARE(parse_file(qPrintable(localCacheRepo)), 0);
	process_dives(false, false);
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
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
	process_dives(false, false);
	dive = get_dive(1);
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

	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
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
	clear_dive_file_data();
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
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
	process_dives(false, false);
	struct dive *dive = get_dive(0);
	QVERIFY(dive != 0);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	dive = get_dive(1);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	dive = get_dive(2);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	QCOMPARE(parse_file(qPrintable(localCacheRepo)), 0);
	process_dives(false, false);
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
	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
	process_dives(false, false);
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

	QCOMPARE(parse_file(qPrintable(cloudTestRepo)), 0);
	QCOMPARE(save_dives("./SampleDivesMerge3.ssrf"), 0);
	// we are not trying to compare this to a pre-determined result... what this test
	// checks is that there are no parsing errors with the merge
	clear_dive_file_data();
}

QTEST_MAIN(TestGitStorage)
