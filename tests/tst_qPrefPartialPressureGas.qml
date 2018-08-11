// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefPartialPressureGas"

	SsrfPartialPressureGasPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.phe = true
		tst.phe = true
		compare(tst.phe, true)

		var x2 = tst.phe_threshold = 21.2
		tst.phe_threshold = 21.2
		compare(tst.phe_threshold, 21.2)

		var x3 = tst.pn2 = true
		tst.pn2 = true
		compare(tst.pn2, true)

		var x4 = tst.pn2_threshold = 21.3
		tst.pn2_threshold = 21.3
		compare(tst.pn2_threshold, 21.3)

		var x5 = tst.po2 = true
		tst.po2 = true
		compare(tst.po2, true)

		var x6 = tst.po2_threshold_max = 21.4
		tst.po2_threshold_max = 21.4
		compare(tst.po2_threshold_max, 21.4)

		var x7 = tst.po2_threshold_min = 21.5
		tst.po2_threshold_min = 21.5
		compare(tst.po2_threshold_min, 21.5)
	}
}
