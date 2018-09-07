// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefLocationService"

	function test_variables() {
		var x1 = PrefLocationService.distance_threshold
		PrefLocationService.distance_threshold = 123
		compare(PrefLocationService.distance_threshold , 123)

		var x2 = PrefLocationService.time_threshold
		PrefLocationService.time_threshold = 12
		compare(PrefLocationService.time_threshold , 12)
	}
}
