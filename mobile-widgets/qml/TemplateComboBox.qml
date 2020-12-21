// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.kde.kirigami 2.4 as Kirigami

ComboBox {
	id: cb
	Layout.fillWidth: true
	Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
	inputMethodHints: Qt.ImhNoPredictiveText
	font.pointSize: subsurfaceTheme.regularPointSize
	delegate: ItemDelegate {
		width: cb.width
		contentItem: Text {
			text: modelData
			color: subsurfaceTheme.textColor
			font: cb.font
			elide: Text.ElideRight
			verticalAlignment: Text.AlignVCenter
		}
		highlighted: cb.highlightedIndex === index
	}

	indicator: Canvas {
		id: canvas
		x: cb.width - width - cb.rightPadding
		y: cb.topPadding + (cb.availableHeight - height) / 2
		width: Kirigami.Units.gridUnit
		height: width * 0.66
		contextType: "2d"

		Connections {
			target: cb
			function onPressedChanged() { canvas.requestPaint(); }
		}

		onPaint: {
			context.reset();
			context.moveTo(0, 0);
			context.lineTo(width, 0);
			context.lineTo(width / 2, height);
			context.closePath();
			context.fillStyle = subsurfaceTheme.textColor;
			context.fill();
		}
	}

	contentItem: Text {
		leftPadding: Kirigami.Units.smallSpacing
		rightPadding: cb.indicator.width + cb.spacing
		text: cb.displayText
		font: cb.font
		color: subsurfaceTheme.textColor
		verticalAlignment: Text.AlignVCenter
		elide: Text.ElideRight
	}

	background: Rectangle {
		border.color: subsurfaceTheme.darkerPrimaryColor
		border.width: cb.visualFocus ? 2 : 1
		color: subsurfaceTheme.backgroundColor
		radius: 2
	}

	popup: Popup {
		y: cb.height - 1
		width: cb.width
		implicitHeight: contentItem.implicitHeight
		padding: 1

		contentItem: ListView {
			clip: true
			implicitHeight: contentHeight
			model: cb.popup.visible ? cb.delegateModel : null
			currentIndex: cb.highlightedIndex

			ScrollIndicator.vertical: ScrollIndicator { }
		}

		background: Rectangle {
			border.color: subsurfaceTheme.darkerPrimaryColor
			color: subsurfaceTheme.backgroundColor
			radius: 2
		}
	}
}
