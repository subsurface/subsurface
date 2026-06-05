// SPDX-License-Identifier: GPL-2.0
#include "testcylinderconfig.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/equipment.h"
#include "core/file.h"
#include "core/pref.h"

void TestCylinderConfig::initTestCase()
{
	TestBase::initTestCase();
	prefs = default_prefs;
}

void TestCylinderConfig::cleanup()
{
	clear_dive_file_data();
}

void TestCylinderConfig::testEnsureO2CylinderAddsForCcr()
{
	// A CCR dive with only a diluent cylinder should get an O2 cylinder added
	struct dive d;
	d.dcs[0].divemode = CCR;
	cylinder_t dil;
	dil.gasmix.o2 = 210_permille;
	dil.cylinder_use = DILUENT;
	d.cylinders.push_back(std::move(dil));

	QCOMPARE((int)d.cylinders.size(), 1);
	d.ensure_o2_cylinder();
	QCOMPARE((int)d.cylinders.size(), 2);
	QCOMPARE(d.cylinders[1].cylinder_use, OXYGEN);
	QCOMPARE(d.cylinders[1].gasmix.o2.permille, 1000);
}

void TestCylinderConfig::testEnsureO2CylinderSkipsForOc()
{
	// An OC dive should not get an O2 cylinder added
	struct dive d;
	d.dcs[0].divemode = OC;
	cylinder_t cyl;
	cyl.gasmix.o2 = 320_permille;
	cyl.cylinder_use = OC_GAS;
	d.cylinders.push_back(std::move(cyl));

	d.ensure_o2_cylinder();
	QCOMPARE((int)d.cylinders.size(), 1);
}

void TestCylinderConfig::testEnsureO2CylinderIdempotent()
{
	// If an O2 cylinder already exists, don't add another
	struct dive d;
	d.dcs[0].divemode = CCR;
	cylinder_t o2;
	o2.gasmix.o2 = 1000_permille;
	o2.cylinder_use = OXYGEN;
	d.cylinders.push_back(std::move(o2));
	cylinder_t dil;
	dil.gasmix.o2 = 210_permille;
	dil.cylinder_use = DILUENT;
	d.cylinders.push_back(std::move(dil));

	QCOMPARE((int)d.cylinders.size(), 2);
	d.ensure_o2_cylinder();
	QCOMPARE((int)d.cylinders.size(), 2);
}

void TestCylinderConfig::testParsedCcrDiveGetsO2Cylinder()
{
	// Load a file with CCR dives and verify every CCR dive has an O2 cylinder.
	// The test file abitofeverything.ssrf has CCR dives — dive 35 has only
	// a diluent cylinder. After fixup, ensure_o2_cylinder should have added one.
	//
	// Note: ensure_o2_cylinder is not currently called from the XML parse path,
	// so we call it explicitly here to test the invariant.
	QCOMPARE(parse_file(SUBSURFACE_TEST_DATA "/dives/abitofeverything.ssrf", &divelog), 0);
	QVERIFY(!divelog.dives.empty());

	int ccr_count = 0;
	for (auto &dive : divelog.dives) {
		if (dive->dcs.empty() || dive->dcs[0].divemode != CCR)
			continue;
		ccr_count++;

		dive->ensure_o2_cylinder();

		bool has_o2 = false;
		for (auto &cyl : dive->cylinders) {
			if (cyl.cylinder_use == OXYGEN) {
				has_o2 = true;
				QCOMPARE(cyl.gasmix.o2.permille, 1000);
				break;
			}
		}
		QVERIFY2(has_o2,
			qPrintable(QString("CCR dive %1 has no OXYGEN cylinder").arg(dive->number)));
	}
	QVERIFY2(ccr_count >= 2, "Expected at least 2 CCR dives in test data");
}

QTEST_GUILESS_MAIN(TestCylinderConfig)
