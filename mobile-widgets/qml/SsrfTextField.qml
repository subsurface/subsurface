// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.2
import QtQuick.Controls 2.2 as Controls
import org.kde.kirigami 2.4 as Kirigami

Controls.TextField {
	/**
	 * set the flickable property to the Flickable this TextField is part of and
	 * when the user starts editing the text this should stay visible
	 */
	property var flickable
	property bool firstTime: true

	id: stf

	// while we are at it, let's put some common settings here into the shared element
	color: subsurfaceTheme.textColor
	onEditingFinished: {
		focus = false
		firstTime = true
	}

	// that's when a user taps on the field to start entering text
	onPressed: {
		if (flickable !== undefined) {
			waitForKeyboard.start()
		} else {
			console.log("flickable is undefined")
		}
	}

	// give the OS enough time to actually resize the flickable
	Timer {
		id: waitForKeyboard
		interval: 300 // 300ms seems like FOREVER, but even that sometimes isn't long enough on Android

		onTriggered: {
			if (!Qt.inputMethod.visible) {
				if (firstTime) {
					firstTime = false
					restart()
				}
				return
			}
			// make sure there's enough space for the input field above the keyboard and action button (and that it's not too far up, either)
			if (stf.y + stf.height > flickable.contentY + flickable.height - 3 * Kirigami.Units.gridUnit || y < flickable.contentY)
				ensureVisible(Math.max(0, 3 * Kirigami.Units.gridUnit + stf.y + stf.height - flickable.height))
		}
	}

	// scroll the flickable to the desired position if the keyboard has shown up
	// this didn't work when setting it from within the Timer, but calling this function works.
	// go figure.
	function ensureVisible(yDest) {
		flickable.contentY = yDest
	}
}
