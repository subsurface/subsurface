// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefUnits"

	SsrfUnitPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.coordinates_traditional
		tst.coordinates_traditional = true
		compare(tst.coordinates_traditional, true)

		var x2 = tst.duration_units
//TBD		tst.duration_units = ??
//TBD		compare(tst.duration_units,  ??)

		var x3 = tst.length
//TBD		tst.=length = ??
//TBD		compare(tst.length,  ??)

		var x4 = tst.pressure
//TBD		tst.pressure = ??
//TBD		compare(tst.pressure, ??)

		var x5 = tst.show_units_table
		tst.show_units_table = true
		compare(tst.show_units_table, true)

		var x6 = tst.temperature
//TBD		tst.temperature = ??
//TBD		compare(tst.temperature, ??)

		var x7 = tst.unit_system
		tst.unit_system = "metric" 
		compare(tst.unit_system, "metric")

		var x8 = tst.vertical_speed_time
//TBD		tst.vertical_speed_time = ??
//TBD		compare(tst.vertical_speed_time, ??)

		var x9 = tst.volume
//TBD		tst.volume = ??
//TBD		compare(tst.volume, ??)

		var x10 = tst.weight
//TBD		tst.weight = ??
//TBD		compare(tst.weight, ??)
	}
}
