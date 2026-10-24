// SPDX-License-Identifier: GPL-2.0
#include "testprofile.h"
#include "core/device.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/file.h"
#include "core/save-profiledata.h"
#include "core/pref.h"
#include "core/profile.h"
#include "core/sample.h"
#include "core/equipment.h"
#include "QTextCodec"

// This test compares the content of struct profile against a known reference version for a list
// of dives to prevent accidental regressions. Thus is you change anything in the profile this
// test will fail. If this change was intentional, run the test manually. Make sure only the
// indended fields change (for example by computing a diff between exportprofile.csv and
// ..dives/exportprofilereference.csv) and copy the former over the later and commit that change
// as well.

void TestProfile::init()
{
	setlocale(LC_ALL, "C");

	// Set UTF8 text codec as in real applications
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	// first, setup the preferences
	prefs = default_prefs;

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
}
void TestProfile::testProfileExport()
{
	prefs.planner_deco_mode = BUEHLMANN;
	parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog);
	save_profiledata("exportprofile.csv", false);
	QFile org(SUBSURFACE_TEST_DATA "/dives/exportprofilereference.csv");
	QCOMPARE(org.open(QFile::ReadOnly), true);
	QFile out("exportprofile.csv");
	QCOMPARE(out.open(QFile::ReadOnly), true);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);

}
void TestProfile::testProfileExportVPMB()
{
	prefs.planner_deco_mode = VPMB;
	parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog);
	save_profiledata("exportprofileVPMB.csv", false);
	QFile org(SUBSURFACE_TEST_DATA "/dives/exportprofilereferenceVPMB.csv");
	QCOMPARE(org.open(QFile::ReadOnly), true);
	QFile out("exportprofileVPMB.csv");
	QCOMPARE(out.open(QFile::ReadOnly), true);
	QTextStream orgS(&org);
	QTextStream outS(&out);
	QString readin = orgS.readAll();
	QString written = outS.readAll();
	QCOMPARE(readin, written);

}

void TestProfile::testEquivalentAirDepth()
{
	// EAD is relative to the nitrogen fraction used by fill_pressures(), not dry-air N2_IN_AIR.
	struct test_case { gasmix mix; int depth_mm; int expected_ead_mm; int expected_end_mm; int expected_eadd_mm; };
	const test_case cases[] = {
		{ gasmix_air, 50000, 50000, 50000, 50592 },
		{ gasmix_air, 55000, 55000, 55000, 55640 },
		{ { 32_percent, 0_percent }, 50000, 41576, 50000, 51523 }, // nitrox 32: ambient pressure * 680/791
		{ { 18_percent, 45_percent }, 50000, 18052, 22990, 27632 } // trimix 18/45: ambient pressure * 370/791
	};

	for (const auto &tc : cases) {
		clear_dive_file_data();
		auto d = std::make_unique<dive>();
		d->when = 1000000000;
		d->get_or_create_cylinder(0)->gasmix = tc.mix;
		auto &dc = d->dcs[0];
		dc.model = "test dc";
		dc.when = d->when;
		dc.surface_pressure = 1013_mbar;
		dc.divemode = OC;
		for (int i = 0; i < 3; i++) {
			auto *s = prepare_sample(&dc);
			s->time.seconds = i * 60;
			s->depth.mm = i == 2 ? 0 : tc.depth_mm;
		}
		dc.duration.seconds = 120;
		divelog.dives.record_dive(std::move(d));
		const auto *recorded = divelog.dives[0].get();
		const auto pi = create_plot_info_new(recorded, &recorded->dcs[0], nullptr);
		bool found = false;
		for (int i = 1; i < pi.nr; i++) {
			if (pi.entry[i].depth.mm != tc.depth_mm || pi.entry[i].sec != 60)
				continue;
			found = true;
			QVERIFY(std::abs(pi.entry[i].ead.mm - tc.expected_ead_mm) <= 100);
			QVERIFY(std::abs(pi.entry[i].end.mm - tc.expected_end_mm) <= 100);
			QVERIFY(std::abs(pi.entry[i].eadd.mm - tc.expected_eadd_mm) <= 100);
		}
		QVERIFY(found);
	}
}

QTEST_GUILESS_MAIN(TestProfile)
