import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.ScrollablePage {
	id: logWindow
	width: parent.width - Kirigami.Units.gridUnit
	anchors.margins: Kirigami.Units.gridUnit / 2
	objectName: "Log"
	title: "Application Log"

/* this can be done by hitting the back key
	contextualActions: [
		Action {
			text: "Close Log"
			iconName: "dialog-cancel"
			onTriggered: {
				stackView.pop()
				contextDrawer.close()
			}
		}
	]
 */

	Flickable {
		id: logFlick
		anchors.fill: parent
		contentHeight: logContent.height
		clip: true
		ColumnLayout {
			width: logFlick.width
			spacing: Kirigami.Units.smallSpacing
			Kirigami.Heading {
				text: "Application Log"
			}
			Kirigami.Label {
				id: logContent
				Layout.preferredWidth: parent.width
				Layout.maximumWidth: parent.width
				wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
				text: manager.logText
			}
		}
	}
}
