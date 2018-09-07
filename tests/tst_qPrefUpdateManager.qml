// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefUpdateManager"

	function test_variables() {
		var x1 = PrefUpdateManager.dont_check_for_updates
		PrefUpdateManager.dont_check_for_updates = true;
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
}
