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

	/**
	 * set inComboBox if the TextField is used in an editable ComboBox
	 * this ensures that the baseline that is used to visually indicate that the user can
	 * edit the text as well as use the drop down is placed much closer to the actual text
	 */
	property bool inComboBox: false

	id: stf
	background: Item {
		Rectangle {
			width: parent.width - Kirigami.Units.smallSpacing
			x: inComboBox ? Kirigami.Units.smallSpacing : -1
			height: 1
			color: stf.focus ? subsurfaceTheme.primaryColor : Qt.darker(subsurfaceTheme.backgroundColor, 1.2)
			anchors.bottom: parent.bottom
			anchors.bottomMargin: inComboBox ? Kirigami.Units.largeSpacing : 1
			visible: !stf.readOnly
		}
	}

	// while we are at it, let's put some common settings here into the shared element
	font.pointSize: subsurfaceTheme.regularPointSize
	topPadding: 0
	bottomPadding: 0
	color: subsurfaceTheme.textColor
	onEditingFinished: {
		focus = false
		firstTime = true
	}

	// once a text input has focus, make sure it is visible
	// we do this via a timer to give the OS time to show a virtual keyboard
	onFocusChanged:	{
		if (focus && flickable !== undefined) {
			waitForKeyboard.start()
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
			var positionInFlickable = stf.mapToItem(flickable.contentItem, 0, 0)
			var stfY = positionInFlickable.y
			if (manager.verboseEnabebled)
				manager.appendTextToLog("position check: lower edge of view is " + (0 + flickable.contentY + flickable.height) + " and text field is at " + stfY)
			if (stfY + stf.height > flickable.contentY + flickable.height - 3 * Kirigami.Units.gridUnit || stfY < flickable.contentY)
				flickable.contentY = Math.max(0, 3 * Kirigami.Units.gridUnit + stfY + stf.height - flickable.height)
		}
	}
}
