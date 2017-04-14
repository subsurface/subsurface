import QtQuick 2.6
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.0 as Kirigami

TextField {
	id: root
	z: focus ? 999 : 0
	property alias model: hintsView.model
	property alias currentIndex: hintsView.currentIndex
	inputMethodHints: Qt.ImhNoPredictiveText
	onTextChanged: {
		textUpdateTimer.restart();
	}
	Keys.onPressed: frame.shouldShow = !frame.shouldShow
	Keys.onUpPressed: {
		hintsView.currentIndex--;
	}
	Keys.onDownPressed: {
		hintsView.currentIndex++;
	}
	Timer {
		id: textUpdateTimer
		interval: 300
		onTriggered: {
			if (root.text.length == 0) {
				return;
			}
			for (var i = 0; i < hintsView.count; ++i) {
				var m = model[i].match(root.text);
				if (m !== null && model[i].startsWith(root.text)) {
					hintsView.currentIndex = i;
					root.text = model[i];
					root.select(m[0].length, model[i].length);
					textUpdateTimer.running = false;
					break;
				}
			}
		}
	}
	Frame {
		id: frame
		property bool shouldShow
		z: 9000
		y: -height - Kirigami.Units.gridUnit
		opacity: root.focus && shouldShow ? 1 : 0
		visible: opacity > 0
		width: root.width
		leftPadding: 0
		rightPadding: 0
		topPadding: 2
		bottomPadding: 2
		height: Math.min(Kirigami.Units.gridUnit * 14, Math.max(Kirigami.Units.gridUnit*4, hintsView.contentHeight + topPadding + bottomPadding))
		Behavior on opacity {
			NumberAnimation {
				duration: 200
				easing.type: Easing.OutQuad
			}
		}
		background: Rectangle {
			color: Kirigami.Theme.viewBackgroundColor
			radius: 2
			layer.enabled: true
			layer.effect: DropShadow {
				anchors.fill: parent
				anchors.bottomMargin: -Kirigami.Units.gridUnit * 2
				horizontalOffset: 0
				verticalOffset: 1
				radius: Kirigami.Units.gridUnit
				samples: 32
				color: Qt.rgba(0, 0, 0, 0.5)
			}
			Rectangle {
				color: Kirigami.Theme.viewBackgroundColor
				width: Kirigami.Units.gridUnit
				height: width
				rotation: 45
				anchors {
					verticalCenter: parent.bottom
					left: parent.left
					leftMargin: width
				}
			}
		}
		ListView {
			id: hintsView
			anchors.fill: parent
			clip: true
			onCurrentIndexChanged: root.text = currentIndex === -1 ? "" : model[currentIndex];

			delegate: Kirigami.BasicListItem {
				label: modelData
				topPadding: 0
				bottomPadding: 0
				leftPadding: 0
				rightPadding: 0
				implicitHeight: Kirigami.Units.gridUnit*2
				checked: hintsView.currentIndex == index
				onClicked: {
					hintsView.currentIndex = index
					root.text = modelData
					frame.shouldShow = false;
				}
			}
			ScrollBar.vertical: ScrollBar { }
		}
	}
}
