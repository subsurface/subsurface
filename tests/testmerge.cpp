// SPDX-License-Identifier: GPL-2.0
#include "testmerge.h"
#include "core/device.h"
#include "core/dive.h" // for save_dives()
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/equipment.h"
#include "core/file.h"
#include "core/trip.h"
#include "core/pref.h"
#include <QTextStream>

void TestMerge::initTestCase()
{
	TestBase::initTestCase();
	prefs = default_prefs;
}

void TestMerge::cleanup()
{
	clear_dive_file_data();
}

void TestMerge::testMergeEmpty()
{
	/*
	 * check that we correctly merge mixed cylinder dives
	 */
	struct divelog log;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47.xml", &log), 0);
	divelog.add_imported_dives(log, import_flags::merge_all_trips);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test48.xml", &log), 0);
	divelog.add_imported_dives(log, import_flags::merge_all_trips);
	QCOMPARE(save_dives("./testmerge47+48.ssrf"), 0);
	QFile org(SUBSURFACE_TEST_DATA "/dives/test47+48.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testmerge47+48.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QStringList readin = orgS.readAll().split("\n");
	QStringList written = outS.readAll().split("\n");
	while (readin.size() && written.size())
		QCOMPARE(written.takeFirst().trimmed(), readin.takeFirst().trimmed());
}

void TestMerge::testMergeBackwards()
{
	/*
	 * check that we correctly merge mixed cylinder dives
	 */
	struct divelog log;
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test48.xml", &log), 0);
	divelog.add_imported_dives(log, import_flags::merge_all_trips);
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test47.xml", &log), 0);
	divelog.add_imported_dives(log, import_flags::merge_all_trips);
	QCOMPARE(save_dives("./testmerge47+48.ssrf"), 0);
	QFile org(SUBSURFACE_TEST_DATA "/dives/test48+47.xml");
	org.open(QFile::ReadOnly);
	QFile out("./testmerge47+48.ssrf");
	out.open(QFile::ReadOnly);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QStringList readin = orgS.readAll().split("\n");
	QStringList written = outS.readAll().split("\n");
	while (readin.size() && written.size())
		QCOMPARE(written.takeFirst().trimmed(), readin.takeFirst().trimmed());
}

void TestMerge::testMergeWeights()
{
	// Helper lambda to make a weightsystem_t
	auto make_ws = [](int grams, const char *description) -> weightsystem_t {
		return weightsystem_t { weight_t{ .grams = grams }, std::string(description), false };
	};

	// Helper lambda to count occurrences of a weightsystem in a table
	auto count_ws = [](const weightsystem_table &table, const weightsystem_t &ws) {
		return static_cast<int>(std::count(table.begin(), table.end(), ws));
	};

	// --- Test 1: Bug #4888 ---
	// Both dives have 3×1kg "Weight" → merged should have exactly 3, not 1.
	{
		dive a, b;
		for (int i = 0; i < 3; i++) {
			a.weightsystems.push_back(make_ws(1000, "belt"));
			b.weightsystems.push_back(make_ws(1000, "belt"));
		}
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(1000, "belt")), 3);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 3);
	}

	// --- Test 2: A has more than B ---
	// A: 3×1kg, B: 2×1kg → merged: 3×1kg (take max)
	{
		dive a, b;
		for (int i = 0; i < 3; i++)
			a.weightsystems.push_back(make_ws(1000, "belt"));
		for (int i = 0; i < 2; i++)
			b.weightsystems.push_back(make_ws(1000, "belt"));
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(1000, "belt")), 3);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 3);
	}

	// --- Test 3: B has more than A ---
	// A: 1×1kg, B: 3×1kg → merged: 3×1kg (take max)
	{
		dive a, b;
		a.weightsystems.push_back(make_ws(1000, "belt"));
		for (int i = 0; i < 3; i++)
			b.weightsystems.push_back(make_ws(1000, "belt"));
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(1000, "belt")), 3);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 3);
	}

	// --- Test 4: Distinct weights from each dive are all kept ---
	// A: 1×2kg ankle, B: 1×3kg belt → merged: both
	{
		dive a, b;
		a.weightsystems.push_back(make_ws(2000, "ankle"));
		b.weightsystems.push_back(make_ws(3000, "belt"));
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(2000, "ankle")), 1);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(3000, "belt")), 1);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 2);
	}

	// --- Test 5: Mixed – A has 1×1kg + 1×2kg, B has 2×1kg ---
	// Merged: 2×1kg + 1×2kg (the 1kg count is max(1,2)=2; 2kg is unique to A)
	{
		dive a, b;
		a.weightsystems.push_back(make_ws(1000, "belt"));
		a.weightsystems.push_back(make_ws(2000, "belt"));
		b.weightsystems.push_back(make_ws(1000, "belt"));
		b.weightsystems.push_back(make_ws(1000, "belt"));
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(1000, "belt")), 2);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(2000, "belt")), 1);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 3);
	}

	// --- Test 6: One dive has no weights ---
	// A: 2×1kg, B: empty → merged: 2×1kg
	{
		dive a, b;
		a.weightsystems.push_back(make_ws(1000, "belt"));
		a.weightsystems.push_back(make_ws(1000, "belt"));
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(count_ws(merged->weightsystems, make_ws(1000, "belt")), 2);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 2);
	}

	// --- Test 7: Both dives have no weights ---
	{
		dive a, b;
		auto merged = dive::create_merged_dive(a, b, 0, false);
		QCOMPARE(static_cast<int>(merged->weightsystems.size()), 0);
	}
}

QTEST_GUILESS_MAIN(TestMerge)
