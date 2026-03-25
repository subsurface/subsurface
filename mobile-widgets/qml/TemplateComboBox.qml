// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ComboBox {
	id: cb
	editable: false
	Layout.fillWidth: true
	inputMethodHints: Qt.ImhNoPredictiveText
	font.pointSize: subsurfaceTheme.regularPointSize

	FontMetrics {
		id: comboFontMetrics
		font: cb.font
	}
	Layout.preferredHeight: Math.max(Kirigami.Units.gridUnit * 2.0,
		Math.ceil(comboFontMetrics.height) + topPadding + bottomPadding)
	rightPadding: Kirigami.Units.smallSpacing
	property var flickable // used to ensure the combobox is visible on screen

	// measure popup item widths to auto-size the dropdown
	TextMetrics {
		id: popupMetrics
		font.pointSize: subsurfaceTheme.smallPointSize
	}

	delegate: ItemDelegate {
		width: cb.popup.width - 2
		contentItem: Text {
			text: modelData
			color: subsurfaceTheme.textColor
			font.pointSize: subsurfaceTheme.smallPointSize
			verticalAlignment: Text.AlignVCenter
		}
		highlighted: cb.highlightedIndex === index
	}

	indicator: Canvas {
		id: canvas
		x: cb.width - width - cb.rightPadding
		y: cb.topPadding + (cb.availableHeight - height) / 2
		width: Kirigami.Units.gridUnit * 0.8
		height: width * 0.66
		contextType: "2d"
		Connections {
			target: cb
			function onPressedChanged() { canvas.requestPaint(); }
		}
		// if the theme changes, we need to force a repaint of the indicator
		property color sttc: subsurfaceTheme.textColor
		onSttcChanged: { requestPaint() }
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

	contentItem: SsrfTextField {
		inComboBox: cb.editable
		readOnly: !cb.editable
		anchors.right: indicator.left
		anchors.left: cb.left
		flickable: cb.flickable
		leftPadding: Kirigami.Units.smallSpacing
		rightPadding: Kirigami.Units.smallSpacing
		text: readOnly ? cb.displayText : cb.editText
		font: cb.font
		color: subsurfaceTheme.textColor
		verticalAlignment: Text.AlignVCenter

		onPressed: {
			if (readOnly) {
				if (cb.popup.opened) {
					cb.popup.close()
				} else {
					cb.popup.open()
				}
			}
		}
	}

	background: Rectangle {
		border.color: cb.focus ? subsurfaceTheme.darkerPrimaryColor : subsurfaceTheme.backgroundColor
		border.width: cb.visualFocus ? 2 : 1
		color: Qt.darker(subsurfaceTheme.backgroundColor, 1.1)
		radius: Kirigami.Units.smallSpacing
	}

	popup: Popup {
		y: cb.height - 1
		width: calculatedWidth
		property real calculatedWidth: cb.width
		onAboutToShow: {
			var maxW = cb.width;
			for (var i = 0; i < cb.count; i++) {
				popupMetrics.text = cb.textAt(i);
				var w = Math.ceil(popupMetrics.advanceWidth);
				if (w > maxW) maxW = w;
			}
			calculatedWidth = maxW + Kirigami.Units.gridUnit * 3;
		}
		implicitHeight: Math.min(contentItem.implicitHeight + 2, cb.Window.height * 0.4)
		padding: 1

		contentItem: ListView {
			clip: true
			implicitHeight: contentHeight
			model: cb.popup.visible ? cb.delegateModel : null
			currentIndex: cb.highlightedIndex

			ScrollBar.vertical: ScrollBar { }
		}

		background: Rectangle {
			border.color: subsurfaceTheme.darkerPrimaryColor
			color: subsurfaceTheme.backgroundColor
			radius: Kirigami.Units.smallSpacing
		}
	}
}
