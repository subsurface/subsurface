// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

SpinBox {
	id: control
	editable: true
	font.pointSize: subsurfaceTheme.regularPointSize

	contentItem: TextInput {
		z: 2
		text: control.textFromValue(control.value, control.locale)
		font: control.font
		color: control.enabled ? subsurfaceTheme.textColor : subsurfaceTheme.disabledTextColor
		horizontalAlignment: Qt.AlignHCenter
		verticalAlignment: Qt.AlignVCenter

		readOnly: !control.editable
		validator: control.validator
		inputMethodHints: Qt.ImhFormattedNumbersOnly
	}

	up.indicator: Rectangle {
		x: control.mirrored ? 0 : parent.width - width
		height: Kirigami.Units.gridUnit * 2
		implicitWidth: Kirigami.Units.gridUnit * 1.5
		implicitHeight: Kirigami.Units.gridUnit * 1.5
		color: control.enabled ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
		border.color: control.enabled ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
		Text {
			text: "+"
			font.pixelSize: control.font.pixelSize * 2
			color: control.enabled ? subsurfaceTheme.primaryTextColor : subsurfaceTheme.disabledTextColor
			anchors.fill: parent
			fontSizeMode: Text.Fit
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
		}
	}

	down.indicator: Rectangle {
		x: control.mirrored ? parent.width - width : 0
		height: Kirigami.Units.gridUnit * 2
		implicitWidth: Kirigami.Units.gridUnit * 1.5
		implicitHeight: Kirigami.Units.gridUnit * 1.5
		color: control.enabled ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
		border.color: control.enabled ? subsurfaceTheme.primaryColor : subsurfaceTheme.backgroundColor
		Text {
			text: "-"
			font.pixelSize: control.font.pixelSize * 2
			color: control.enabled ? subsurfaceTheme.primaryTextColor : subsurfaceTheme.disabledTextColor
			anchors.fill: parent
			fontSizeMode: Text.Fit
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
		}
	}

	background: Rectangle {
		implicitWidth: 5* Kirigami.Units.gridUnit
		color: subsurfaceTheme.backgroundColor
		border.color: subsurfaceTheme.backgroundColor
	}
}
