// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefGeocoding"

	function test_variables() {
//TBD		var x1 = PrefGeocoding.first_taxonomy_category
//TBD		PrefGeocoding.first_taxonomy_category = ??
//TBD		compare(PrefGeocoding.first_taxonom_categroy, ??)

//TBD		var x2 = PrefGeocoding.second_taxonomy_category
//TBD		PrefGeocoding.second_taxonomy_category = ??
//TBD		compare(PrefGeocoding.second_taxonom_categroy, ??)

//TBD		var x3 = PrefGeocoding.third_taxonomy_category
//TBD		PrefGeocoding.third_taxonomy_category = ??
//TBD		compare(PrefGeocoding.third_taxonom_categroy, ??)
	}

	Item {
		id: spyCatcher

//TBD		property bool spy1 : false
//TBD		property bool spy2 : false
//TBD		property bool spy3 : false

		Connections {
			target: PrefGeocoding
//TBD			onChanged: {spyCatcher.spy1 = true }
		}
	}

	function test_signals() {
//TBD		PrefGeocoding.first_taxonomy_category = ??
//TBD		PrefGeocoding.second_taxonomy_category = ??
//TBD		PrefGeocoding.third_taxonomy_category = ??

//TBD		compare(spyCatcher.spy1, true)
//TBD		compare(spyCatcher.spy2, true)
//TBD		compare(spyCatcher.spy3, true)
	}
}
