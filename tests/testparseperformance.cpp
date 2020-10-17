// SPDX-License-Identifier: GPL-2.0
#include "testparseperformance.h"
#include "core/device.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/git-access.h"
#include "core/settings/qPrefProxy.h"
#include "core/settings/qPrefCloudStorage.h"
#include <QFile>
#include <QDebug>
#include <QNetworkProxy>

#define LARGE_TEST_REPO "https://github.com/Subsurface/large-anonymous-sample-data"

void TestParsePerformance::initTestCase()
{
	/* we need to manually tell that the resource exists, because we are using it as library. */
	Q_INIT_RESOURCE(subsurface);

	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences an proxy information
	copy_prefs(&default_prefs, &prefs);
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
	qPrefProxy::load();
	qPrefCloudStorage::load();

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
	QString localCacheDir(get_local_dir(LARGE_TEST_REPO, "git"));
	QDir localCacheDirectory(localCacheDir);
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
		qDebug() << "missing large sample data file - available at " LARGE_TEST_REPO;
		qDebug() << "clone the repo, uncompress the file and copy it to " SUBSURFACE_TEST_DATA "/dives/large-anon.ssrf";
		return;
	}
	QBENCHMARK {
		parse_file(SUBSURFACE_TEST_DATA "/dives/large-anon.ssrf", &dive_table, &trip_table,
			   &dive_site_table, &device_table, &filter_preset_table);
	}
}

void TestParsePerformance::parseGit()
{
	// some more necessary setup
	git_libgit2_init();

	// first parse this once to populate the local cache - this way network
	// effects don't dominate the parse time
	parse_file(LARGE_TEST_REPO "[git]", &dive_table, &trip_table, &dive_site_table,
		   &device_table, &filter_preset_table);

	cleanup();

	QBENCHMARK {
		parse_file(LARGE_TEST_REPO "[git]", &dive_table, &trip_table, &dive_site_table,
			   &device_table, &filter_preset_table);
	}
}

QTEST_GUILESS_MAIN(TestParsePerformance)
