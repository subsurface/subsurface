// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefUpdateManager"

	SsrfUpdateManagerPrefs {
		id: tst
	}

	SsrfPrefs {
		id: prefs
	}

	function test_variables() {
		var x1 = tst.dont_check_for_updates
		tst.dont_check_for_updates = true;
		compare(tst.dont_check_for_updates, true)

		var x2 = tst.dont_check_exists
		tst.dont_check_exists = true
		compare(tst.dont_check_exists, true)

		var x3 = tst.last_version_used
		tst.last_version_used = "jan again"
		compare(tst.last_version_used, "jan again")

		var x4 = tst.next_check
		var x4_date = Date.fromLocaleString(Qt.locale(), "01-01-2001", "dd-MM-yyyy")
		tst.next_check = x4_date
		compare(tst.next_check,  x4_date)

		var x5 = tst.uuidString
		tst.uuidString = "jan again"
		compare(tst.uuidString, "jan again")
	}
}
