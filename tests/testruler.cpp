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
#include "core/format.h"
#include "core/subsurface-string.h"
#include "core/event.h"
#include "core/sample.h"
#include "core/tanksensormapping.h"
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

void TestRuler::testSacCoverage()
{
	dive d;
	d.dcs[0].surface_pressure = 1000_mbar;
	for (int c = 0; c < 2; ++c)
		d.get_or_create_cylinder(c)->type.size.mliter = 12000;
	plot_info pi;
	pi.nr_cylinders = 2;
	pi.entry.resize(5);
	pi.pressures.resize(10);
	for (int i = 0; i < 5; ++i) {
		pi.entry[i].sec = i * 60;
		pi.entry[i].depth = 10_m;
		pi.entry[i].ruler_cylinder = i < 2 ? 0 : 1;
	}
	auto pressure = [&](int i, int c, int bar) {
		set_plot_pressure_data(pi, i, SENSOR_PR, c, bar * 1000);
	};
	pressure(0, 0, 200);
	pressure(1, 0, 190);
	pressure(2, 0, 180); // Measured pressure on the outgoing cylinder at the switch.
	pressure(2, 1, 200);
	pressure(3, 1, 190);
	pressure(4, 1, 180);
	const auto text = [&](int from, int to) {
		return compare_samples(&d, pi, from, to, false)[1];
	};
	const int use = (d.get_cylinder(0)->gas_volume(200_bar) - d.get_cylinder(0)->gas_volume(180_bar)).mliter * 2;
	int precision;
	const char *unit;
	const double value = get_volume_units(lrint(use / d.depth_to_atm(10_m) * 60 / 240), &precision, &unit);
	const std::string expected = casprintf_loc("SAC:%.*f%s/min", precision, value, unit);
	QVERIFY(text(0, 4).find(expected) != std::string::npos);
	QCOMPARE(text(0, 4), text(4, 0));
	QVERIFY(text(0, 2).find("SAC:") != std::string::npos);
	QVERIFY(text(2, 4).find("SAC:") != std::string::npos);
	// Gas mixes can be identical; only the selected cylinder index counts.
	pi.entry[2].ruler_cylinder = -1;
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);
	pi.entry[2].ruler_cylinder = 1;

	// An unrelated cylinder's changing pressure is not gas consumption.
	pressure(0, 1, 250);
	QVERIFY(text(0, 4).find(expected) != std::string::npos);
	// A zero at the switch cannot be treated as a full cylinder depletion.
	pressure(2, 0, 0);
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);
	QVERIFY(text(0, 4).find("↓V:") != std::string::npos);
	QVERIFY(text(0, 1).find("SAC:") != std::string::npos);
	pressure(2, 0, 180);
	pressure(2, 1, 0);
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);
	pressure(2, 1, 200);
	pressure(3, 1, 0);
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);
	pressure(3, 1, 190);

	// A held or unbounded interpolated value is not an observed switch pressure.
	pressure(2, 0, 0);
	set_plot_pressure_data(pi, 2, INTERPOLATED_PR, 0, 180000);
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);
	set_plot_pressure_data(pi, 2, INTERPOLATED_PR, 0, 0);
	pressure(2, 0, 180);
	pi.pressures[2 * pi.nr_cylinders].held = true;
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);
	pi.pressures[2 * pi.nr_cylinders].held = false;
	// A missing interior sensor sample may be interpolated between observations.
	pressure(1, 0, 0);
	set_plot_pressure_data(pi, 1, INTERPOLATED_PR, 0, 190000);
	QVERIFY(text(0, 4).find(expected) != std::string::npos);
	set_plot_pressure_data(pi, 1, INTERPOLATED_PR, 0, 0);
	QVERIFY(text(0, 4).find("SAC:") == std::string::npos);

	// Selection boundaries inside a continuously measured gas-use span
	// can be interpolated from observations on both sides of the selection.
	for (auto &entry: pi.entry)
		entry.ruler_cylinder = 0;
	pressure(1, 0, 0);
	pressure(2, 0, 0);
	pressure(4, 0, 180);
	set_plot_pressure_data(pi, 1, INTERPOLATED_PR, 0, 195000);
	set_plot_pressure_data(pi, 2, INTERPOLATED_PR, 0, 190000);
	set_plot_pressure_data(pi, 3, INTERPOLATED_PR, 0, 185000);
	QVERIFY(text(1, 3).find("SAC:") != std::string::npos);
	QCOMPARE(text(1, 3), text(3, 1));
	pressure(4, 0, 0);
	QVERIFY(text(1, 3).find("SAC:") == std::string::npos);

	// A later return to this cylinder cannot bound an interpolation across B.
	pi.entry[2].ruler_cylinder = 1;
	pressure(0, 0, 200);
	pressure(1, 0, 190);
	pressure(2, 0, 0);
	pressure(3, 0, 170);
	set_plot_pressure_data(pi, 2, INTERPOLATED_PR, 0, 180000);
	QVERIFY(text(0, 2).find("SAC:") == std::string::npos);
	QVERIFY(text(0, 2).find("ΔP:") == std::string::npos);
}

void TestRuler::testGasSwitchPressure()
{
	dive d;
	divecomputer &dc = d.dcs[0];
	dc.surface_pressure = 1000_mbar;
	for (int c = 0; c < 2; ++c) {
		cylinder_t *cyl = d.get_or_create_cylinder(c);
		cyl->type.size.mliter = 12000;
		cyl->start = 200_bar;
		cyl->end = 180_bar;
		dc.tank_sensor_mappings.push_back({static_cast<int16_t>(c), static_cast<unsigned int>(c)});
	}
	add_gas_switch_event(&d, &dc, 20, 1);
	for (int t = 0; t <= 40; t += 10) {
		sample *s = prepare_sample(&dc);
		s->time.seconds = t;
		s->depth = 10_m;
		if (t <= 30)
			s->pressure[0].mbar = (200 - t) * 1000;
		if (t >= 20) {
			s->sensor[1] = 1;
			s->pressure[1].mbar = (220 - t) * 1000;
		}
	}
	const plot_info pi = create_plot_info_new(&d, &dc, nullptr);
	int from = -1, at_switch = -1, to = -1;
	for (int i = 0; i < pi.nr; ++i) {
		if (pi.entry[i].sec == 10)
			from = i;
		if (pi.entry[i].sec == 20)
			at_switch = i;
		if (pi.entry[i].sec == 30)
			to = i;
	}
	QVERIFY(from >= 0 && at_switch >= 0 && to >= 0);
	QCOMPARE(pi.entry[at_switch].ruler_cylinder, 1);
	QCOMPARE(get_plot_sensor_pressure(pi, at_switch, 0), 0);
	QCOMPARE(pi.entry[at_switch].ruler_outgoing_pressure, 180000);
	QCOMPARE(get_plot_sensor_pressure(pi, to, 0), 0);
	const std::string covered = compare_samples(&d, pi, from, to, false)[1];
	QVERIFY(covered.find("SAC:") != std::string::npos);
	QVERIFY(covered.find("ΔP:") != std::string::npos);
	plot_info missing = pi;
	missing.entry[at_switch].ruler_outgoing_pressure = 0;
	const std::string uncovered = compare_samples(&d, missing, from, to, false)[1];
	QVERIFY(uncovered.find("SAC:") == std::string::npos);
	QVERIFY(uncovered.find("ΔP:") == std::string::npos);
}

void TestRuler::testSmallSpanConsumption()
{
	dive d;
	d.dcs[0].surface_pressure = 1000_mbar;
	for (int c = 0; c < 2; ++c)
		d.get_or_create_cylinder(c)->type.size.mliter = 12000;
	plot_info pi;
	pi.nr_cylinders = 2;
	pi.entry.resize(5);
	pi.pressures.resize(10);
	for (int i = 0; i < 5; ++i) {
		pi.entry[i].sec = i * 60;
		pi.entry[i].depth = 10_m;
		pi.entry[i].ruler_cylinder = i < 2 ? 0 : 1;
	}
	set_plot_pressure_data(pi, 0, SENSOR_PR, 0, 200000);
	set_plot_pressure_data(pi, 1, SENSOR_PR, 0, 199500);
	set_plot_pressure_data(pi, 2, SENSOR_PR, 0, 199100);
	set_plot_pressure_data(pi, 2, SENSOR_PR, 1, 200000);
	set_plot_pressure_data(pi, 3, SENSOR_PR, 1, 199500);
	set_plot_pressure_data(pi, 4, SENSOR_PR, 1, 198900);

	const int use = (d.get_cylinder(0)->gas_volume(200_bar) -
			 d.get_cylinder(0)->gas_volume(pressure_t::from_base(199100)) +
			 d.get_cylinder(1)->gas_volume(200_bar) -
			 d.get_cylinder(1)->gas_volume(pressure_t::from_base(198900))).mliter;
	int precision;
	const char *unit;
	const double value = get_volume_units(lrint(use / d.depth_to_atm(10_m) * 60 / 240), &precision, &unit);
	const std::string expected = casprintf_loc("SAC:%.*f%s/min", precision, value, unit);
	const auto text = [&] { return compare_samples(&d, pi, 0, 4, false)[1]; };
	QVERIFY(text().find(expected) != std::string::npos);
	QVERIFY(text().find("ΔP:2bar") != std::string::npos);

	// A small contributing span still needs a known cylinder size.
	d.get_or_create_cylinder(0)->type.size.mliter = 0;
	QVERIFY(text().find("SAC:") == std::string::npos);
	QVERIFY(text().find("ΔP:") == std::string::npos);
	d.get_or_create_cylinder(0)->type.size.mliter = 12000;
	// Preserve the existing noise threshold, but apply it to the total.
	set_plot_pressure_data(pi, 4, SENSOR_PR, 1, 199900);
	QVERIFY(text().find("SAC:") == std::string::npos);
	QVERIFY(text().find("ΔP:") == std::string::npos);
}

QTEST_GUILESS_MAIN(TestRuler)
