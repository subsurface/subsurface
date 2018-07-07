// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPref"

	SsrfPrefs {
		id: prefs
	}

	function test_register() {
		var x = prefs.mobile_version
		compare(x, prefs.mobile_version)
	}
}
