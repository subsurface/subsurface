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
	width: parent.width - MobileComponents.Units.gridUnit
	anchors.margins: MobileComponents.Units.gridUnit / 2
	objectName: "Log"

	ScrollView {
		anchors.fill: parent
		Flickable {
			id: logFlick
			anchors.fill: parent
			contentHeight: logContent.height
			clip: true
			ColumnLayout {
				width: logFlick.width
				spacing: MobileComponents.Units.smallSpacing
				MobileComponents.Heading {
					text: "Application Log"
				}
				MobileComponents.Label {
					id: logContent
					Layout.preferredWidth: parent.width
					Layout.maximumWidth: parent.width
					wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
					text: manager.logText
				}
			}
		}
	}
}
