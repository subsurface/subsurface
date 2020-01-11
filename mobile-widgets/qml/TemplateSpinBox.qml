// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

SpinBox {
	id: control
	editable: true
	font.pointSize: subsurfaceTheme.regularPointSize

	contentItem: TextInput {
		z: 2
		text: control.textFromValue(control.value, control.locale)

		font: control.font
		color: control.enabled ? "black" : "lightgrey"
		horizontalAlignment: Qt.AlignHCenter
		verticalAlignment: Qt.AlignVCenter

		readOnly: !control.editable
		validator: control.validator
		inputMethodHints: Qt.ImhFormattedNumbersOnly
	}

	up.indicator: Rectangle {
		x: control.mirrored ? 0 : parent.width - width
		height: control.height
		implicitWidth: 30
		implicitHeight: 30
		color: control.enabled ? "grey" : "lightgrey"
		border.color: control.enabled ? "grey" : "lightgrey"

		Text {
			text: "+"
			font.pixelSize: control.font.pixelSize * 2
			color: control.enabled ? "black" : "lightgrey"
			anchors.fill: parent
			fontSizeMode: Text.Fit
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
		}
	}

	down.indicator: Rectangle {
		x: control.mirrored ? parent.width - width : 0
		height: control.height
		implicitWidth: 30
		implicitHeight: 30
		color: control.enabled ? "grey" : "lightgrey"
		border.color: control.enabled ? "grey" : "lightgrey"

		Text {
			text: "-"
			font.pixelSize: control.font.pixelSize * 2
			color: control.enabled ? "black" : "lightgrey"
			anchors.fill: parent
			fontSizeMode: Text.Fit
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
		}
	}

	background: Rectangle {
		implicitWidth: 140
		color: subsurfaceTheme.backgroundColor
		border.color: subsurfaceTheme.backgroundColor
	}
}
