// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPref"

	function test_register() {
		var x = Pref.mobile_version
		compare(x, Pref.mobile_version)
	}
}
