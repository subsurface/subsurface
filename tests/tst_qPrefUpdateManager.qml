// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefUpdateManager"

	function test_variables() {
		var x1 = PrefUpdateManager.dont_check_for_updates
		PrefUpdateManager.dont_check_for_updates = true
		compare(PrefUpdateManager.dont_check_for_updates, true)

		var x2 = PrefUpdateManager.dont_check_exists
		PrefUpdateManager.dont_check_exists = true
		compare(PrefUpdateManager.dont_check_exists, true)

		var x3 = PrefUpdateManager.last_version_used
		PrefUpdateManager.last_version_used = "jan again"
		compare(PrefUpdateManager.last_version_used, "jan again")

		var x4 = PrefUpdateManager.next_check
		var x4_date = Date.fromLocaleString(Qt.locale(), "01-01-2001", "dd-MM-yyyy")
		PrefUpdateManager.next_check = x4_date
//TBD		compare(PrefUpdateManager.next_check,  x4_date)

		var x5 = PrefUpdateManager.uuidString
		PrefUpdateManager.uuidString = "jan again"
		compare(PrefUpdateManager.uuidString, "jan again")
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		property bool spy2 : false
		property bool spy3 : false
		property bool spy4 : false
		property bool spy5 : false

		Connections {
			target: PrefUpdateManager
			onDont_check_for_updatesChanged: {spyCatcher.spy1 = true }
			onDont_check_existsChanged: {spyCatcher.spy2 = true }
			onLast_version_usedChanged: {spyCatcher.spy3 = true }
			onNext_checkChanged: {spyCatcher.spy4 = true }
			onUuidStringChanged: {spyCatcher.spy5 = true }
		}
	}

	function test_signals() {
		PrefUpdateManager.dont_check_for_updates = ! PrefUpdateManager.dont_check_for_updates
		PrefUpdateManager.dont_check_exists = ! PrefUpdateManager.dont_check_exists
		PrefUpdateManager.last_version_used = "qml"
		var x4_date = Date.fromLocaleString(Qt.locale(), "01-01-2010", "dd-MM-yyyy")
		PrefUpdateManager.next_check = x4_date
		PrefUpdateManager.uuidString = "qml"

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy2, true)
		compare(spyCatcher.spy3, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
	}
}
