import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

Button {
	style: ButtonStyle {
		padding {
			top: MobileComponents.Units.smallSpacing
			left: MobileComponents.Units.smallSpacing * 2
			right: MobileComponents.Units.smallSpacing * 2
			bottom: MobileComponents.Units.smallSpacing
		}
		background: Rectangle {
			border.width: 1
			radius: height / 3
			color: control.pressed ? subsurfaceTheme.shadedColor : subsurfaceTheme.accentColor
		}
		label: Text{
			text: control.text
			color: subsurfaceTheme.accentTextColor
			verticalAlignment: Text.AlignVCenter
			horizontalAlignment: Text.AlignHCenter
		}
	}
}
