// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefUnits"

	function test_variables() {
		var x1 = PrefUnits.coordinates_traditional
		PrefUnits.coordinates_traditional = true
		compare(PrefUnits.coordinates_traditional, true)

//TBD		var x2 = PrefUnits.duration_units
//TBD		PrefUnits.duration_units = ??
//TBD		compare(PrefUnits.duration_units,  ??)

//TBD		var x3 = PrefUnits.length
//TBD		PrefUnits.=length = ??
//TBD		compare(PrefUnits.length,  ??)

//TBD		var x4 = PrefUnits.pressure
//TBD		PrefUnits.pressure = ??
//TBD		compare(PrefUnits.pressure, ??)

		var x5 = PrefUnits.show_units_table
		PrefUnits.show_units_table = true
		compare(PrefUnits.show_units_table, true)

//TBD		var x6 = PrefUnits.temperature
//TBD		PrefUnits.temperature = ??
//TBD		compare(PrefUnits.temperature, ??)

		var x7 = PrefUnits.unit_system
		PrefUnits.unit_system = "metric" 
		compare(PrefUnits.unit_system, "metric")

//TBD		var x8 = PrefUnits.vertical_speed_time
//TBD		PrefUnits.vertical_speed_time = ??
//TBD		compare(PrefUnits.vertical_speed_time, ??)

//TBD		var x9 = PrefUnits.volume
//TBD		PrefUnits.volume = ??
//TBD		compare(PrefUnits.volume, ??)

//TBD		var x10 = PrefUnits.weight
//TBD		PrefUnits.weight = ??
//TBD		compare(PrefUnits.weight, ??)
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy5 : false
		property bool spy7 : false

		Connections {
			target: PrefUnits
			onCoordinates_traditionalChanged: {spyCatcher.spy1 = true }
			onShow_units_tableChanged: {spyCatcher.spy5 = true }
			onUnit_systemChanged: {spyCatcher.spy7 = true }
		}
	}

	function test_signals() {
		PrefUnits.coordinates_traditional = ! PrefUnits.coordinates_traditional
		PrefUnits.show_units_table = ! PrefUnits.show_units_table
		PrefUnits.unit_system = "qml" 

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy7, true)
	}
}
