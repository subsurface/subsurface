// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Button {
	id: root
	property double fontSize: subsurfaceTheme.regularPointSize

	// AI-generated (Claude): horizontal padding added to the label's natural
	// width when computing the implicit width. The default (half a line height
	// per side) is the historical look; a caller packing several buttons into
	// a constrained row can tighten it. This affects the implicit size only --
	// the content item always spans the full background width.
	property double labelPadding: buttonMetrics.height / 2

	// AI-generated (Claude): measure the label independently of the rendered
	// Text item. Taking the background's implicit size from buttonText while
	// buttonText's size is bound back to the background is a circular binding;
	// TextMetrics reports the natural size whatever the rendered label does.
	TextMetrics {
		id: buttonMetrics
		font.pointSize: root.fontSize
		text: root.text
	}

	// implicitWidth/implicitHeight (not width/height) so Control's own sizing
	// still anchors this background to the button's actual allocated size when
	// a Layout constrains it narrower or wider than the label.
	background: Rectangle {
		id: buttonBackground
		color: root.enabled? (root.pressed ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.primaryColor) : "gray"
		antialiasing: true
		radius: Kirigami.Units.smallSpacing * 2
		implicitHeight: buttonMetrics.height * 2
		implicitWidth: buttonMetrics.width + root.labelPadding * 2
	}

	// AI-generated (Claude): shrink-to-fit computed here rather than delegated
	// to fontSizeMode: Text.Fit, which acts as a one-way latch inside a Layout
	// -- the first pass hands the Text a near-zero width, Fit drops to
	// minimumPointSize, and it never grows back once the column gets its real
	// width. Scaling fontSize by available/natural width is a plain binding
	// and re-evaluates in both directions. smallPointSize is the legibility
	// floor: below it the label stops shrinking, and a caller that is still
	// too narrow has to grant more width instead.
	contentItem: Text {
		id: buttonText
		readonly property double availableWidth: buttonBackground.width - root.labelPadding * 2
		readonly property double fittedSize: buttonMetrics.width <= 0 || availableWidth >= buttonMetrics.width ?
			root.fontSize :
			root.fontSize * availableWidth / buttonMetrics.width
		text: root.text
		font.pointSize: Math.max(subsurfaceTheme.smallPointSize, fittedSize)
		width: buttonBackground.width
		height: buttonBackground.height
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		anchors.centerIn: buttonBackground
		color: root.pressed ? subsurfaceTheme.darkerPrimaryTextColor :subsurfaceTheme.primaryTextColor
	}
}
