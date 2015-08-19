import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0

Rectangle {
	id: topBar
	color: "#2C4882"
	Layout.fillWidth: true
	Layout.margins: 0
	Layout.minimumHeight: prefsButton.height * 1.2
	RowLayout {
		anchors.bottom: topBar.bottom
		anchors.bottomMargin: prefsButton.height * 0.1
		anchors.left: topBar.left
		anchors.leftMargin: prefsButton.height * 0.1
		anchors.right: topBar.right
		anchors.rightMargin: prefsButton.height * 0.1
		Button {
			id: backButton
			Layout.maximumHeight: prefsButton.height
			Layout.minimumHeight: prefsButton.height
			Layout.preferredWidth: Screen.width * 0.1
			text: "\u2190"
			style: ButtonStyle {
				background: Rectangle {
					color: "#2C4882"
					implicitWidth: 50
				}
				label: Text {
					id: txt
					color: "white"
					font.pointSize: 18
					font.bold: true
					text: control.text
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
			}
			onClicked: {
				manager.commitChanges(
							dive_id,
							suit,
							buddy,
							divemaster,
							notes
							)
				stackView.pop();
			}
		}
		Text {
			text: qsTr("Subsurface mobile")
			font.pointSize: 18
			font.bold: true
			color: "white"
			anchors.horizontalCenter: parent.horizontalCenter
			horizontalAlignment: Text.AlignHCenter
		}
	}
}
