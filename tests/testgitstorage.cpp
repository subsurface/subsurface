#include "testgitstorage.h"
#include "dive.h"
#include "divelist.h"
#include "file.h"
#include "git2.h"
#include "prefs-macros.h"
#include <QDir>
#include <QTextStream>
#include <QNetworkProxy>
#include <QSettings>
#include <QDebug>

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
	// first, setup the preferences an proxy information
	prefs = default_prefs;
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

	// now connect to the ssrftest repository on the cloud server
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

QTEST_MAIN(TestGitStorage)
