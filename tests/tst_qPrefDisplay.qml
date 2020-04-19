// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.6
import QtTest 1.2

TestCase {
	name: "qPrefDisplay"

	function test_variables() {
		var x1 = PrefDisplay.animation_speed
		PrefDisplay.animation_speed = 37
		compare(PrefDisplay.animation_speed, 37)

		// PrefDisplay.divelist_font is NOT permitted in QML
		// calling qApp->setFont in the setter crashes in Qt5.11
		// due to a conflict with the QML font system
//		var x2 = PrefDisplay.divelist_font
//		PrefDisplay.divelist_font = "helvitica"
//		compare(PrefDisplay.divelist_font, "helvitica")

		// PrefDisplay.font_size is NOT permitted in QML
		// calling qApp->setFont in the setter crashes in Qt5.11
		// due to a conflict with the QML font system
//		var x3 = PrefDisplay.font_size
//		PrefDisplay.font_size = 12.0
//		compare(PrefDisplay.font_size, 12.0)

		var x4 = PrefDisplay.PrefDisplay_invalid_dives
		PrefDisplay.PrefDisplay_invalid_dives = !x4
		compare(PrefDisplay.PrefDisplay_invalid_dives, !x4)

		var x5 = PrefDisplay.show_developer
		PrefDisplay.show_developer = !x5
		compare(PrefDisplay.show_developer, !x5)

		var x6 = PrefDisplay.theme
		PrefDisplay.theme = "myColor"
		compare(PrefDisplay.theme, "myColor")

//TBD		var x7 = PrefDisplay.tooltip_position
//TBD		PrefDisplay.tooltip_position = ??
//TBD		compare(PrefDisplay.tooltip_position, ??)

		var x8 = PrefDisplay.lastDir
		PrefDisplay.lastDir = "myDir"
		compare(PrefDisplay.lastDir, "myDir")

//TBD		var x10 = PrefDisplay.mainSplitter
//TBD		PrefDisplay.mainSplitter =  ???
//TBD		compare(PrefDisplay.mainSplitter, ???)

//TBD		var x11 = PrefDisplay.topSplitter
//TBD		PrefDisplay.topSplitter =  ???
//TBD		compare(PrefDisplay.topSplitter, ???)

//TBD		var x12 = PrefDisplay.bottomSplitter
//TBD		PrefDisplay.bottomSplitter =  ???
//TBD		compare(PrefDisplay.bottomSplitter, ???)

		var x13 = PrefDisplay.maximized
		PrefDisplay.maximized = true
		compare(PrefDisplay.maximized, true)

//TBD		var x14 = PrefDisplay.geometry
//TBD		PrefDisplay.geometry =  ???
//TBD		compare(PrefDisplay.geometry, ???)

//TBD		var x15 = PrefDisplay.windowState
//TBD		PrefDisplay.windowState =  ???
//TBD		compare(PrefDisplay.windowState, ???)

		var x16 = PrefDisplay.lastState
		PrefDisplay.lastState = 17
		compare(PrefDisplay.lastState, 17)
	}

	Item {
		id: spyCatcher

		property bool spy1 : false
		// no signals 2,3
		property bool spy4 : false
		property bool spy5 : false
		property bool spy6 : false
		// no signals 7	
		property bool spy8 : false
		property bool spy9 : false
		// no signals 10,11,12
		property bool spy13 : false
		// no signals 14,15
		property bool spy16 : false

		Connections {
			target: PrefDisplay
			onAnimation_speedChanged: {spyCatcher.spy1 = true }
			onDisplay_invalid_divesChanged: {spyCatcher.spy4 = true }
			onShow_developerChanged: {spyCatcher.spy5 = true }
			onThemeChanged: {spyCatcher.spy6 = true }
			onLastDirChanged: {spyCatcher.spy8 = true }
			onMaximizedChanged: {spyCatcher.spy13 = true }
			onLastStateChanged: {spyCatcher.spy16 = true }
		}
	}

	function test_signals() {
		PrefDisplay.animation_speed = -1157
		// 2,3 have no signal
		PrefDisplay.display_invalid_dives = ! PrefDisplay.display_invalid_dives
		PrefDisplay.show_developer = ! PrefDisplay.show_developer
		PrefDisplay.theme = "qml"
		// 7 has no signal
		PrefDisplay.lastDir = "qml"
		// 10, 11, 12 have no signal
		PrefDisplay.maximized = ! PrefDisplay.maximized
		// 14,15 have no signal
		PrefDisplay.lastState = -17

		compare(spyCatcher.spy1, true)
		compare(spyCatcher.spy4, true)
		compare(spyCatcher.spy5, true)
		compare(spyCatcher.spy6, true)
		compare(spyCatcher.spy8, true)
		compare(spyCatcher.spy9, true)
		compare(spyCatcher.spy13, true)
		compare(spyCatcher.spy16, true)
	}
}
