import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls
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
	height: Math.round(Kirigami.Units.gridUnit * 1.8)
	padding: 0

	property real detailsOpacity : 0
	property color textColor: subsurfaceTheme.secondaryTextColor

	Row {
		id: downloadedDive
		width: parent.width
		spacing: Kirigami.Units.largeSpacing
		padding: 0
		anchors.leftMargin: Math.round (Kirigami.Units.largeSpacing / 2)
		anchors.fill: parent
		add: Transition {
			NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
			NumberAnimation { property: "scale"; from: 0; to: 1.0; duration: 400 }
		}
		SsrfCheckBox {
			id: diveIsSelected
			checked: innerListItem.selected;
			width: childrenRect.width - Kirigami.Units.smallSpacing;
			height: childrenRect.heigh - Kirigami.Units.smallSpacing;
			anchors.verticalCenter: parent.verticalCenter
			onClicked: {
				console.log("Clicked on the checkbox of item " + index)
				importModel.selectRow(index)
			}
		}
		Controls.Label {
			id: dateLabel
			text: innerListItem.datetime
			anchors.verticalCenter: parent.verticalCenter
			width: Math.round(parent.width * 0.35)
			font.pointSize: subsurfaceTheme.smallPointSize
			color: textColor
		}
		Controls.Label {
			text: innerListItem.depth + ' / ' + innerListItem.duration
			anchors.verticalCenter: parent.verticalCenter
			width: Math.round(parent.width * 0.35)
			font.pointSize: subsurfaceTheme.smallPointSize
			color: textColor
		}
	}
}
