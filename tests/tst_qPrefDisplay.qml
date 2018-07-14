// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPref"

	SsrfDisplayPrefs {
		id: display
	}

	function test_variables() {
		var x1 = display.divelist_font
		display.divelist_font = "helvitica"
		compare(display.divelist_font, "helvitica")
		var x2 = display.font_size
		display.font_size = 12.0
		compare(display.font_size, 12.0)
		var x3 = display.display_invalid_dives
		display.display_invalid_dives = !x3
		compare(display.display_invalid_dives, !x3)
		var x4 = display.show_developer
		display.show_developer = !x4
		compare(display.show_developer, !x4)
		var x5 = display.theme
		display.theme = "myColor"
		compare(display.theme, "myColor")
	}

}
