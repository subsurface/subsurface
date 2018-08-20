// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2
import org.subsurfacedivelog.mobile 1.0

TestCase {
	name: "qPrefDisplay"

	SsrfDisplayPrefs {
		id: display
	}

	function test_variables() {
		var x0 = display.animation_speed
		display.animation_speed = 37
		compare(display.animation_speed, 37)

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

//TBD		var x6 = display.tooltip_position
//TBD		display.tooltip_position = ??
//TBD		compare(display.tooltip_position, ??)

		var x7 = display.lastDir
		display.lastDir = "myDir"
		compare(display.lastDir, "myDir")

		var x8 = display.UserSurvey
		display.UserSurvey = "yes"
		compare(display.UserSurvey, "yes")

//TBD		var x9 = display.mainSplitter
//TBD		display.mainSplitter =  ???
//TBD		compare(display.mainSplitter, ???)

//TBD		var x10 = display.topSplitter
//TBD		display.topSplitter =  ???
//TBD		compare(display.topSplitter, ???)

//TBD		var x11 = display.bottomSplitter
//TBD		display.bottomSplitter =  ???
//TBD		compare(display.bottomSplitter, ???)

		var x12 = display.maximized
		display.maximized = true
		compare(display.maximized, true)

//TBD		var x13 = display.geometry
//TBD		display.geometry =  ???
//TBD		compare(display.geometry, ???)

//TBD		var x14 = display.windowState
//TBD		display.windowState =  ???
//TBD		compare(display.windowState, ???)

		var x15 = display.lastState
		display.lastState = 17
		compare(display.lastState, 17)
	}
}
