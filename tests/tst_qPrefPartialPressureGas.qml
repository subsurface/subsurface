// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefPartialPressureGas"

	function test_variables() {
		var x1 = PrefPartialPressureGas.phe = true
		PrefPartialPressureGas.phe = true
		compare(PrefPartialPressureGas.phe, true)

		var x2 = PrefPartialPressureGas.phe_threshold = 21.2
		PrefPartialPressureGas.phe_threshold = 21.2
		compare(PrefPartialPressureGas.phe_threshold, 21.2)

		var x3 = PrefPartialPressureGas.pn2 = true
		PrefPartialPressureGas.pn2 = true
		compare(PrefPartialPressureGas.pn2, true)

		var x4 = PrefPartialPressureGas.pn2_threshold = 21.3
		PrefPartialPressureGas.pn2_threshold = 21.3
		compare(PrefPartialPressureGas.pn2_threshold, 21.3)

		var x5 = PrefPartialPressureGas.po2 = true
		PrefPartialPressureGas.po2 = true
		compare(PrefPartialPressureGas.po2, true)

		var x6 = PrefPartialPressureGas.po2_threshold_max = 21.4
		PrefPartialPressureGas.po2_threshold_max = 21.4
		compare(PrefPartialPressureGas.po2_threshold_max, 21.4)

		var x7 = PrefPartialPressureGas.po2_threshold_min = 21.5
		PrefPartialPressureGas.po2_threshold_min = 21.5
		compare(PrefPartialPressureGas.po2_threshold_min, 21.5)
	}
}
