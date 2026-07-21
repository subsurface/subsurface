// SPDX-License-Identifier: GPL-2.0
#include "testruler.h"
#include "core/profile.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/save-profiledata.h"
#include "core/pref.h"
#include "QTextCodec"

void TestRuler::init()
{
	QLocale::setDefault(QLocale::C);

	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences
	prefs = default_prefs;

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
}

/* This test loads a known synthetic dive and verifies the values
 * calculated by the ruler code against easily hand calculated values
 * to verify it's correctness.
 */
void TestRuler::testRuler()
{
	verbose = 1;

	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/test13.xml", &divelog), 0);

	QVERIFY(divelog.dives.size() == 1);
	const struct dive *dive = divelog.dives[0].get();
	const struct divecomputer *dc = &dive->dcs[0];

	const struct plot_info pi = create_plot_info_new(dive, dc, NULL);

	/* First string is the one with now good values.
	 * Speeds has rounding issues and ΔP / SAC has issues due to
	 * pressures being linearly applied as depth*time/pressure but
	 * volume calculations takes non linear compression into conciliation
	 */

	// Verify that we don't explode on a zero with measurement
	QVERIFY(compare_samples(dive, pi, 0, 0, false).size() == 0);

	// Verify that we don't explode on negative indices
	QVERIFY(compare_samples(dive, pi, -1, 0, false).size() == 0);
	QVERIFY(compare_samples(dive, pi, 0, -1, false).size() == 0);

	// Verify the edge case with only the first sample
	// ( We have a duplicate sample in the beginning, so somethings odd is going on)
	QVERIFY(compare_samples(dive, pi, 0, 1, false).size() == 0);

	// Verify the edge case with only the first real sample
	QCOMPARE(compare_samples(dive, pi, 1, 2, false)[0].c_str(),
			"ΔT:0:10min ΔD:0.8m ↓D:0.0m ↑D:0.8m øD:0.4m");

	// Verify that we get the right average for the whole linear decent
	auto idx_at_6_min = std::get<0>(get_plot_details_new(dive, pi, 6*60));
	QCOMPARE(compare_samples(dive, pi, 0, idx_at_6_min, false)[0].c_str(),
			"ΔT:6:00min ΔD:30.0m ↓D:0.0m ↑D:30.0m øD:15.0m");

	// Verify that it works the same with start and end reversed
	QCOMPARE(compare_samples(dive, pi, idx_at_6_min, 0, false)[0].c_str(),
			"ΔT:6:00min ΔD:30.0m ↓D:0.0m ↑D:30.0m øD:15.0m");

	// Verify that we get the right values for a single step
	QCOMPARE(compare_samples(dive, pi, idx_at_6_min - 1, idx_at_6_min, false)[0].c_str(),
			"ΔT:0:10min ΔD:0.8m ↓D:29.2m ↑D:30.0m øD:29.6m");

	// Verify that we get the right results for a straight and level bottom segment
	auto idx_at_13_00 = std::get<0>(get_plot_details_new(dive, pi, 13*60));
	QCOMPARE(compare_samples(dive, pi, idx_at_6_min, idx_at_13_00, false)[0].c_str(),
			"ΔT:7:00min ΔD:0.0m ↓D:30.0m ↑D:30.0m øD:30.0m");

	// Verify that we get the right average for a something containing both
	// a accent and a decent.
	auto idx_at_15_30 = std::get<0>(get_plot_details_new(dive, pi, 15*60 + 30));
	auto idx_at_17_00 = std::get<0>(get_plot_details_new(dive, pi, 17*60));
	QCOMPARE(compare_samples(dive, pi, idx_at_15_30, idx_at_17_00, false)[0].c_str(),
			"ΔT:1:30min ΔD:0.0m ↓D:5.0m ↑D:10.0m øD:7.5m");

	// Make sure that it works in the edge case, the last sample
	auto max_idx = pi.entry.size();
	QCOMPARE(compare_samples(dive, pi, max_idx - 2, max_idx - 1, false)[0].c_str(),
			"ΔT:0:01min ΔD:0.0m ↓D:0.0m ↑D:0.0m øD:0.0m");

	// Make sure we can calculate over all the samples
	QCOMPARE(compare_samples(dive, pi, 0, max_idx - 1, false)[0].c_str(),
			"ΔT:30:02min ΔD:0.0m ↓D:0.0m ↑D:30.0m øD:14.4m");

	// Verify that we don't explode on indexes larger than the amount of samples
	QVERIFY(compare_samples(dive, pi, max_idx - 1, max_idx, false).size() == 0);
}

QTEST_GUILESS_MAIN(TestRuler)
