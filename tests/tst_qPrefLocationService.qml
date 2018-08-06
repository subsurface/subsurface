// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefLocationService"

	SsrfLocationServicePrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.distance_threshold
		tst.distance_threshold = 123
		compare(tst.distance_threshold , 123)

		var x2 = tst.time_threshold
		tst.time_threshold = 12
		compare(tst.time_threshold , 12)
	}
}
