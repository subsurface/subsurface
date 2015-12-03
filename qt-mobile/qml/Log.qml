import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.plasma.mobilecomponents 0.2 as MobileComponents

MobileComponents.Page {
	id: logWindow
	width: parent.width
	objectName: "Log"
	flickable: logFlick
	ScrollView {
		anchors.fill: parent
		Flickable {
			id: logFlick
			anchors.fill: parent
			contentHeight: logContent.height
			clip: true
			Item {
				id: logContent
				width: logFlick.width
				height: childrenRect.height + MobileComponents.Units.smallSpacing * 2

				ColumnLayout {
					anchors {
						left: parent.left
						right: parent.right
						top: parent.top
						margins: MobileComponents.Units.smallSpacing
					}
					spacing: MobileComponents.Units.smallSpacing

					Text {
						wrapMode: Text.WrapAnywhere
						text: manager.logText
					}
					Item {
						height: MobileComponents.Units.gridUnit * 3
						width: height
					}
				}
			}
		}
	}
}
