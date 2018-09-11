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

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false

		Connections {
			target: PrefLocationService
			onDistance_thresholdChanged: {spyCatcher.spy1 = true }
			onTime_thresholdChanged: {spyCatcher.spy2 = true }
		}
	}

	function test_signals() {
		PrefLocationService.distance_threshold = -123
		PrefLocationService.time_threshold = -12

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
	}
}
