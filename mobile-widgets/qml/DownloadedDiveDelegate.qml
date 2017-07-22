import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.AbstractListItem {
	id: innerListItem

	property string depth
	property string datetime
	property string duration
	property bool selected

	enabled: true
	supportsMouseEvents: true
	width: parent.width

	property real detailsOpacity : 0
	property int horizontalPadding: Kirigami.Units.gridUnit / 2 - Kirigami.Units.smallSpacing  + 1
	property color textColor: subsurfaceTheme.textColor

	Row {
		width: parent.width
		height: childrenRect.height + Kirigami.Units.smallSpacing
		spacing: horizontalPadding

		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
			NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
		}
		CheckBox {
			id: diveIsSelected
			checked: innerListItem.selected;
			width: childrenRect.width - Kirigami.Units.smallSpacing;
			height: childrenRect.heigh - Kirigami.Units.smallSpacing;
			indicator: Rectangle {
				visible: diveIsSelected
				implicitWidth: 20
				implicitHeight: 20
				//x: isBluetooth.leftPadding
				y: parent.height / 2 - height / 2
				radius: 4
				border.color: diveIsSelected.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
				color: subsurfaceTheme.backgroundColor

				Rectangle {
					width: 12
					height: 12
					x: 4
					y: 4
					radius: 3
					color: diveIsSelected.down ? subsurfaceTheme.primaryColor : subsurfaceTheme.darkerPrimaryColor
					visible: diveIsSelected && diveIsSelected.checked
				}
			}
			onClicked: {
				console.log("Clicked on the checkbox of item " + index)
				importModel.selectRow(index)
			}
		}
		Item {
			id: diveListEntry
			width: parent.width - diveIsSelected.width - Kirigami.Units.gridUnit * (innerListItem.deleteButtonVisible ? 3 : 1)
			height: childrenRect.height - Kirigami.Units.smallSpacing

			Kirigami.Label {
				id: depthLabel
				text: "Depth " + innerListItem.depth
				font.weight: Font.Light
				elide: Text.ElideRight
				maximumLineCount: 1 // needed for elide to work at all
				anchors {
					left: parent.left
					leftMargin: horizontalPadding
					top: parent.top
					right: dateLabel.left
				}
			}
			Kirigami.Label {
				id: dateLabel
				text: innerListItem.datetime
				font.pointSize: subsurfaceTheme.smallPointSize
				anchors {
					bottom: parent.bottom
					right: parent.right
					top: parent.top
				}
			}
			Row {
				anchors {
					top: depthLabel.bottom
					left: parent.left
					leftMargin: horizontalPadding
					right: parent.right
					rightMargin: horizontalPadding
					topMargin: - Kirigami.Units.smallSpacing * 2
				}
				Kirigami.Label {
					text: qsTr('Duration: ')
					font.pointSize: subsurfaceTheme.smallPointSize
				}
				Kirigami.Label {
					text: innerListItem.duration
					font.pointSize: subsurfaceTheme.smallPointSize
				}
			}
		}
	}
}
