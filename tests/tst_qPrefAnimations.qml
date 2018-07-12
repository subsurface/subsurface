// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefAnimations"

	SsrfAnimationsPrefs {
		id: tst
	}

	function test_variables() {
		var x1 = tst.animation_speed;
		tst.animation_speed = 37
		compare(tst.animation_speed, 37)
	}

}
