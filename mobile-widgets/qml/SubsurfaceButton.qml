import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import org.kde.kirigami 1.0 as Kirigami

Button {
	style: ButtonStyle {
		padding {
			top: Kirigami.Units.smallSpacing * 2
			left: Kirigami.Units.smallSpacing * 4
			right: Kirigami.Units.smallSpacing * 4
			bottom: Kirigami.Units.smallSpacing * 2
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
