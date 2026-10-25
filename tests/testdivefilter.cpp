// SPDX-License-Identifier: GPL-2.0
#include "testdivefilter.h"
#include "core/dive.h"
#include "core/divefilter.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/pref.h"

void TestDiveFilter::testDiveSiteFilterInvalidDives()
{
	prefs = default_prefs;
	divelog.clear();

	dive_site *siteA = divelog.sites.create("Site Alpha");
	dive_site *siteB = divelog.sites.create("Site Beta");

	// Dive 1: valid dive at Site Alpha
	auto d1 = std::make_unique<dive>();
	d1->number = 1;
	d1->when = 1000;
	d1->dive_site = siteA;
	d1->invalid = false;
	dive *pD1 = divelog.dives.register_dive(std::move(d1));

	// Dive 2: invalid dive at Site Alpha (regression target for PR #5055)
	auto d2 = std::make_unique<dive>();
	d2->number = 2;
	d2->when = 2000;
	d2->dive_site = siteA;
	d2->invalid = true;
	dive *pD2 = divelog.dives.register_dive(std::move(d2));

	// Dive 3: valid dive at Site Beta
	auto d3 = std::make_unique<dive>();
	d3->number = 3;
	d3->when = 3000;
	d3->dive_site = siteB;
	d3->invalid = false;
	dive *pD3 = divelog.dives.register_dive(std::move(d3));

	QCOMPARE(divelog.dives.size(), 3);

	// Initially enable display_invalid_dives so all 3 dives are shown in unfiltered state
	prefs.display_invalid_dives = true;
	DiveFilter *filter = DiveFilter::instance();
	filter->reset();
	QCOMPARE(filter->shownDives(), 3);
	QVERIFY(!pD1->hidden_by_filter);
	QVERIFY(!pD2->hidden_by_filter);
	QVERIFY(!pD3->hidden_by_filter);

	filter->startFilterDiveSites({ siteA });
	QVERIFY(filter->diveSiteMode());

	// Scenario 1: prefs.display_invalid_dives is true.
	// When display_invalid_dives is enabled, invalid dive at Site Alpha (pD2) is shown.
	prefs.display_invalid_dives = true;
	filter->updateAll();
	QVERIFY(!pD1->hidden_by_filter); // site matches -> shown
	QVERIFY(!pD2->hidden_by_filter); // site matches, invalid & display_invalid_dives=true -> shown
	QVERIFY(pD3->hidden_by_filter);  // site does not match -> hidden
	QCOMPARE(filter->shownDives(), 2);

	// Scenario 2: prefs.display_invalid_dives is false (default).
	// With pD2 currently shown, calling updateAll() with display_invalid_dives=false
	// actively filters and hides pD2, verifying that updateAll() performs the hiding.
	prefs.display_invalid_dives = false;
	filter->updateAll();
	QVERIFY(!pD1->hidden_by_filter); // site matches, valid -> shown
	QVERIFY(pD2->hidden_by_filter);  // site matches, invalid & display_invalid_dives=false -> hidden
	QVERIFY(pD3->hidden_by_filter);  // site does not match -> hidden
	QCOMPARE(filter->shownDives(), 1);

	// Scenario 3: prefs.display_invalid_dives toggled back to true.
	// With pD2 currently hidden, calling updateAll() with display_invalid_dives=true
	// actively reveals pD2, verifying that updateAll() performs the reveal path.
	prefs.display_invalid_dives = true;
	filter->updateAll();
	QVERIFY(!pD1->hidden_by_filter);
	QVERIFY(!pD2->hidden_by_filter);
	QVERIFY(pD3->hidden_by_filter);
	QCOMPARE(filter->shownDives(), 2);

	// Scenario 4: Verify incremental update() toggles invalid dive visibility
	// First hide pD2 (currently visible from Scenario 3) to test that update() actively hides it
	prefs.display_invalid_dives = false;
	filter->update({ pD2 });
	QVERIFY(pD2->hidden_by_filter);
	QCOMPARE(filter->shownDives(), 1);

	// Next reveal pD2 back using incremental update() to test that update() actively reveals it
	prefs.display_invalid_dives = true;
	filter->update({ pD2 });
	QVERIFY(!pD2->hidden_by_filter);
	QCOMPARE(filter->shownDives(), 2);

	// Cleanup
	filter->stopFilterDiveSites();
	QVERIFY(!filter->diveSiteMode());
	divelog.clear();
	prefs = default_prefs;
	filter->reset();
	QCOMPARE(filter->shownDives(), 0);
}

QTEST_GUILESS_MAIN(TestDiveFilter)
