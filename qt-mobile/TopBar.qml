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
	color: theme.accentColor
	Layout.fillWidth: true
	Layout.margins: 0
	Layout.minimumHeight: prefsButton.height + units.spacing * 2
	RowLayout {
		anchors.bottom: topBar.bottom
		anchors.bottomMargin: units.spacing
		anchors.left: topBar.left
		anchors.leftMargin: units.spacing
		anchors.right: topBar.right
		anchors.rightMargin: units.spacing
		Button {
			id: backButton
			Layout.maximumHeight: prefsButton.height
			Layout.minimumHeight: prefsButton.height
			Layout.preferredWidth: units.gridUnit * 2
			text: "\u2190"
			style: ButtonStyle {
				background: Rectangle {
					color: theme.accentColor
					implicitWidth: units.gridUnit * 2
				}
				label: Text {
					id: txt
					color: theme.accentTextColor
					font.pointSize: 18
					font.bold: true
					text: control.text
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
				}
			}
			onClicked: {
				if (stackView.currentItem.objectName == "DiveDetails")
				{
				manager.commitChanges(
							dive_id,
							suit,
							buddy,
							divemaster,
							notes
							)
				}
				stackView.pop();
			}
		}
		Text {
			text: qsTr("Subsurface mobile")
			font.pointSize: 18
			color: theme.accentTextColor
			anchors.left: backButton.right
			anchors.leftMargin: units.spacing
			//horizontalAlignment: Text.AlignHCenter
		}
	}
}
