// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.0

Item {
	Text {
		anchors.fill: parent
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
		color: "red"
		text: qsTr("MapWidget.qml failed to load!
The QML modules QtPositioning and QtLocation could be missing!")
	}
}
