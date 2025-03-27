// SPDX-License-Identifier: GPL-2.0
#include "testparseperformance.h"
#include "core/device.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/git-access.h"
#include "core/settings/qPrefProxy.h"
#include "core/settings/qPrefCloudStorage.h"
#include <QFile>
#include <QNetworkProxy>
#include "QTextCodec"

#define LARGE_TEST_REPO "https://github.com/Subsurface/large-anonymous-sample-data"

void TestParsePerformance::initTestCase()
{
	TestBase::initTestCase();

	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences an proxy information
	prefs = default_prefs;
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
	qPrefProxy::load();
	qPrefCloudStorage::load();

	QNetworkProxy proxy;
	proxy.setType(QNetworkProxy::ProxyType(prefs.proxy_type));
	proxy.setHostName(QString::fromStdString(prefs.proxy_host));
	proxy.setPort(prefs.proxy_port);
	if (prefs.proxy_auth) {
		proxy.setUser(QString::fromStdString(prefs.proxy_user));
		proxy.setPassword(QString::fromStdString(prefs.proxy_pass));
	}
	QNetworkProxy::setApplicationProxy(proxy);

	// now cleanup the cache dir in case there's something weird from previous runs
	std::string localCacheDir = get_local_dir(LARGE_TEST_REPO, "git");
	QDir localCacheDirectory(localCacheDir.c_str());
	QCOMPARE(localCacheDirectory.removeRecursively(), true);
}

void TestParsePerformance::init()
{
}

void TestParsePerformance::cleanup()
{
	clear_dive_file_data();
}

void TestParsePerformance::parseSsrf()
{
	// parsing of a V2 file should work
	QFile largeSsrfFile(SUBSURFACE_TEST_DATA "/dives/large-anon.ssrf");
	if (!largeSsrfFile.exists()) {
		report_info("missing large sample data file - available at " LARGE_TEST_REPO);
		report_info("clone the repo, uncompress the file and copy it to " SUBSURFACE_TEST_DATA "/dives/large-anon.ssrf");
		return;
	}
	QBENCHMARK {
		parse_file(SUBSURFACE_TEST_DATA "/dives/large-anon.ssrf", &divelog);
	}
}

void TestParsePerformance::parseGit()
{
	// some more necessary setup
	git_libgit2_init();

	// first parse this once to populate the local cache - this way network
	// effects don't dominate the parse time
	parse_file(LARGE_TEST_REPO "[git]", &divelog);

	cleanup();

	QBENCHMARK {
		parse_file(LARGE_TEST_REPO "[git]", &divelog);
	}
}

QTEST_GUILESS_MAIN(TestParsePerformance)
