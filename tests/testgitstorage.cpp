// SPDX-License-Identifier: GPL-2.0
#include "testgitstorage.h"
#include "git2.h"

#include "core/device.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/file.h"
#include "core/qthelper.h"
#include "core/subsurfacestartup.h"
#include "core/settings/qPrefProxy.h"
#include "core/settings/qPrefCloudStorage.h"
#include "core/trip.h"
#include "core/git-access.h"

#include <QDir>
#include <QTextStream>
#include <QNetworkProxy>
#include <QTextCodec>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif

// provide declarations for two local helper functions in git-access.c
extern "C" char *get_local_dir(const char *remote, const char *branch);
extern "C" void delete_remote_branch(git_repository *repo, const char *remote, const char *branch);

QString email;
QString gitUrl;
QString cloudTestRepo;
QString localCacheDir;
QString localCacheRepo;
QString randomBranch;

static void moveDir(QString oldName, QString newName)
{
	QDir oldDir(oldName);
	QDir newDir(newName);
	QCOMPARE(newDir.removeRecursively(), true);
	QCOMPARE(oldDir.rename(oldName, newName), true);
}

static void localRemoteCleanup()
{
	// cleanup the local cache dir
	QDir localCacheDirectory(localCacheDir);
	QCOMPARE(localCacheDirectory.removeRecursively(), true);

	// get the remote/branch information as parsed by our git tooling
	const char *branch, *remote;
	struct git_repository *git_repo;

	// when this is first executed, we expect the branch not to exist on the remote server;
	// if that's true, this will print a harmless error to stderr
	git_repo = is_git_repository(qPrintable(cloudTestRepo), &branch, &remote, false);

	// this odd comparison is used to tell that we were able to connect to the remote repo;
	// in the error case we get the full cloudTestRepo name back as "branch"
	if (branch != randomBranch) {
		// dang, we weren't able to connect to the server - let's not fail the test
		// but just give up
		QSKIP("wasn't able to connect to server");
	}

	// force delete any remote branch of that name on the server (and ignore any errors)
	delete_remote_branch(git_repo, remote, branch);

	// and since this will have created a local repo, remove that one, again so the tests start clean
	QCOMPARE(localCacheDirectory.removeRecursively(), true);

	free((void *)branch);
	free((void *)remote);
	git_repository_free(git_repo);
}

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

	// setup our cloud test repo / credentials but allow the user to pick a different account by
	// setting these environment variables
	// Of course that email needs to exist as cloud storage account and have the given password
	//
	// To reduce the risk of collisions on the server, we have ten accounts set up for this purpose
	// please don't use them for other reasons as they will get deleted regularly
	email = qgetenv("SSRF_USER_EMAIL");
	QString password = qgetenv("SSRF_USER_PASSWORD");

	if (email.isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		email = QString("gitstorage%1@hohndel.org").arg(QRandomGenerator::global()->bounded(10));
#else
		// on Qt 5.9 we go back to using qsrand()/qrand()
		qsrand(time(NULL));
		email = QString("gitstorage%1@hohndel.org").arg(qrand() % 10);
#endif
	}
	if (password.isEmpty())
		password = "please-only-use-this-in-the-git-tests";
	gitUrl = prefs.cloud_base_url;
	if (gitUrl.right(1) != "/")
		gitUrl += "/";
	gitUrl += "git";
	prefs.cloud_git_url = copy_qstring(gitUrl);
	prefs.cloud_storage_email_encoded = copy_qstring(email);
	prefs.cloud_storage_password = copy_qstring(password);
	gitUrl += "/" + email;
	// all user storage for historical reasons always uses the user's email both as
	// repo name and as branch. To allow us to keep testing and not step on parallel
	// runs we'll use actually random branch names - yes, this still has a chance of
	// conflict, but I'm not going to implement a distributed locak manager for this
	if (email.startsWith("gitstorage")) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
		randomBranch = QString::number(QRandomGenerator::global()->bounded(0x1000000), 16) +
				QString::number(QRandomGenerator::global()->bounded(0x1000000), 16);
#else
		// on Qt 5.9 we go back to using qsrand()/qrand() -- if we get to this code, qsrand() was already called
		// even on a 32bit system RAND_MAX is at least 32767 so this will also give us 12 random hex digits
		randomBranch = QString::number(qrand() % 0x1000, 16) + QString::number(qrand() % 0x1000, 16) +
				QString::number(qrand() % 0x1000, 16) + QString::number(qrand() % 0x1000, 16);
#endif
	} else {
		// user supplied their own credentials, fall back to the usual "email is branch" pattern
		randomBranch = email;
	}
	cloudTestRepo = gitUrl + QStringLiteral("[%1]").arg(randomBranch);
	localCacheDir = get_local_dir(qPrintable(gitUrl), qPrintable(randomBranch));
	localCacheRepo = localCacheDir + QStringLiteral("[%1]").arg(randomBranch);
	qDebug() << "repo used:" << cloudTestRepo;
	qDebug() << "local cache:" << localCacheRepo;

	// make sure we deal with any proxy settings that are needed
	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(prefs.proxy_host);
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(prefs.proxy_user);
		proxy.setPassword(prefs.proxy_pass);
	}
	QNetworkProxy::setApplicationProxy(proxy);

	// we will keep switching between online and offline mode below; let's always start online
	git_local_only = false;

	// initialize libgit2
	git_libgit2_init();

	// cleanup local and remote branches
	localRemoteCleanup();
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
}

void TestGitStorage::cleanupTestCase()
{
	localRemoteCleanup();
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
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
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
	QCOMPARE(parse_file(qPrintable(repoNameRead + "[test]"), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
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
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/SampleDivesV2.ssrf", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
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
	// read the local repo from the previous test and add dive 10
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test10.xml", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	// calling process_loaded_dives() sorts the table, but calling add_imported_dives()
	// causes it to try to update the window title... let's not do that
	process_loaded_dives();
	// now save only to the local cache but not to the remote server
	git_local_only = true;
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	QCOMPARE(save_dives("./SampleDivesV3plus10local.ssrf"), 0);
	clear_dive_file_data();
	// now pretend that we are online again and open the cloud storage and compare
	git_local_only = false;
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
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
	moveDir(localCacheDir, localCacheDir + "save");
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
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
	// we want to test a merge - in order to do so we need to make changes to the cloud
	// repo from two clients - but since we have only one client here, we have to cheat
	// a little:
	// the local cache with the 'save' extension will serve as our second client;
	//
	// (1) first we make a change and save it to the cloud
	// (2) then we switch to the second client (i.e., we move that cache back in place)
	// (3) on that second client we make a different change while offline
	// (4) now we take that second client back online and get the merge
	// (5) let's make sure that we have the expected data on the second client
	// (6) go back to the first client and ensure we have the same data there after sync

	// (1) open the repo, add dive test11 and save to the cloud
	git_local_only = false;
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test11.xml", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// (2) switch to the second client by moving the old cache back in place
	moveDir(localCacheDir, localCacheDir + "client1");
	moveDir(localCacheDir + "save", localCacheDir);

	// (3) open the repo from the old cache and add dive test12 while offline
	git_local_only = true;
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test12.xml", &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// (4) now take things back online
	git_local_only = false;
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
			    &dive_site_table, &device_table, &filter_preset_table), 0);
	clear_dive_file_data();

	// (5) now we should have all the dives in our repo on the second client
	// first create the reference data from the xml files:
	QCOMPARE(parse_file("./SampleDivesV3plus10local.ssrf", &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test11.xml", &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test12.xml", &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12.ssrf"), 0);
	// then load from the cloud
	clear_dive_file_data();
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12-merged.ssrf"), 0);
	// finally compare what we have
	QFile org("./SampleDivesV3plus10-11-12-merged.ssrf");
	org.open(QFile::ReadOnly);
	QFile out("./SampleDivesV3plus10-11-12.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);
	clear_dive_file_data();

	// (6) move ourselves back to the first client and compare data there
	moveDir(localCacheDir + "client1", localCacheDir);
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QCOMPARE(save_dives("./SampleDivesV3plus10-11-12-merged-client1.ssrf"), 0);
	QFile client1("./SampleDivesV3plus10-11-12-merged-client1.ssrf");
	client1.open(QFile::ReadOnly);
	QTextStream client1S(&client1);
	readin = client1S.readAll();
	QCOMPARE(readin, written);
}

void TestGitStorage::testGitStorageCloudMerge2()
{
	// delete a dive offline
	// edit the same dive in the cloud repo
	// merge
	// (1) open repo, delete second dive, save offline
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	struct dive *dive = get_dive(1);
	delete_single_dive(1);
	QCOMPARE(save_dives("./SampleDivesMinus1.ssrf"), 0);
	git_local_only = true;
	QCOMPARE(save_dives(qPrintable(localCacheRepo)), 0);
	git_local_only = false;
	clear_dive_file_data();

	// (2) move cache out of the way
	moveDir(localCacheDir, localCacheDir + "save");

	// (3) now we open the cloud storage repo and modify that second dive
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	dive = get_dive(1);
	QVERIFY(dive != NULL);
	free(dive->notes);
	dive->notes = strdup("These notes have been modified by TestGitStorage");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// (4) move the saved local cache  backinto place and try to open the cloud repo
	//     -> this forces a merge
	moveDir(localCacheDir + "save", localCacheDir);
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
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


	// (1) open repo, edit notes of first three dives
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	struct dive *dive;
	QVERIFY((dive = get_dive(0)) != 0);
	free(dive->notes);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	QVERIFY((dive = get_dive(1)) != 0);
	free(dive->notes);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	QVERIFY((dive = get_dive(2)) != 0);
	free(dive->notes);
	dive->notes = strdup("Create multi line dive notes\nLine 2\nLine 3\nLine 4\nLine 5\nThat should be enough");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// (2) make different edits offline
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QVERIFY((dive = get_dive(0)) != 0);
	free(dive->notes);
	dive->notes = strdup("Create multi line dive notes\nDifferent line 2 and removed 3-5\n\nThat should be enough");
	QVERIFY((dive = get_dive(1)) != 0);
	free(dive->notes);
	dive->notes = strdup("Line 2\nLine 3\nLine 4\nLine 5"); // keep the middle, remove first and last");
	QVERIFY((dive = get_dive(2)) != 0);
	free(dive->notes);
	dive->notes = strdup("single line dive notes");
	git_local_only = true;
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	git_local_only = false;
	clear_dive_file_data();

	// (3) simulate a second system by moving the cache away and open the cloud storage repo and modify
	//     those first dive notes differently while online
	moveDir(localCacheDir, localCacheDir + "save");
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	process_loaded_dives();
	QVERIFY((dive = get_dive(0)) != 0);
	free(dive->notes);
	dive->notes = strdup("Completely different dive notes\nBut also multi line");
	QVERIFY((dive = get_dive(1)) != 0);
	free(dive->notes);
	dive->notes = strdup("single line dive notes");
	QVERIFY((dive = get_dive(2)) != 0);
	free(dive->notes);
	dive->notes = strdup("Line 2\nLine 3\nLine 4\nLine 5"); // keep the middle, remove first and last");
	QCOMPARE(save_dives(qPrintable(cloudTestRepo)), 0);
	clear_dive_file_data();

	// (4) move the saved local cache back into place and open the cloud repo -> this forces a merge
	moveDir(localCacheDir + "save", localCacheDir);
	QCOMPARE(parse_file(qPrintable(cloudTestRepo), &dive_table, &trip_table,
		 &dive_site_table, &device_table, &filter_preset_table), 0);
	QCOMPARE(save_dives("./SampleDivesMerge3.ssrf"), 0);
	// we are not trying to compare this to a pre-determined result... what this test
	// checks is that there are no parsing errors with the merge
}

QTEST_GUILESS_MAIN(TestGitStorage)
