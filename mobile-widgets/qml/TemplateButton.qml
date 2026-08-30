// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Button {
	id: root
	property double fontSize: subsurfaceTheme.regularPointSize
	// AI-generated (Claude): implicitWidth/implicitHeight (not width/height)
	// so Control's own sizing still anchors this background to the button's
	// actual allocated size when a Layout constrains it narrower or wider
	// than the label -- a fixed width/height left the background mismatched
	// against the real control bounds whenever a parent RowLayout/GridLayout
	// resized the button (surfaced by M001/S03/T09's offset quick-adjust row).
	background: Rectangle {
		id: buttonBackground
		color: root.enabled? (root.pressed ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor) : "gray"
		antialiasing: true
		radius: Kirigami.Units.smallSpacing * 2
		implicitHeight: buttonText.implicitHeight * 2
		implicitWidth: buttonText.implicitWidth + buttonText.implicitHeight
	}
	// AI-generated (Claude): explicit width/height plus fontSizeMode: Text.Fit
	// so the label shrinks to whatever space a parent Layout actually grants
	// the button, instead of the button being forced wide enough for the
	// label at full size. A fixed row of many narrow buttons (M001/S03/T09's
	// offset quick-adjust grid) can end up narrower than the labels' natural
	// width would need; without Fit the overflow labels were clipped past the
	// screen edge instead of shrinking. implicitWidth/Height above stay
	// unaffected (Text's implicit size is always the natural, unconstrained
	// size), so normal callers that already have room are unchanged.
	// minimumPointSize is a legibility floor (D016): shrinking without one
	// produced labels that fit but were too small to read. Below the floor
	// Fit stops shrinking, so a caller that is still too narrow must give
	// the button more width rather than trading away readability.
	contentItem: Text {
		id: buttonText
		text: root.text
		font.pointSize: root.fontSize
		width: buttonBackground.width
		height: buttonBackground.height
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		fontSizeMode: Text.Fit
		minimumPointSize: subsurfaceTheme.smallPointSize
		anchors.centerIn: buttonBackground
		color: root.pressed ? subsurfaceTheme.darkerPrimaryTextColor :subsurfaceTheme.primaryTextColor
	}
}
