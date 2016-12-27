import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.0 as Kirigami

Kirigami.ScrollablePage {
	id: logWindow
	anchors.margins: Kirigami.Units.gridUnit / 2
	objectName: "Log"
	title: qsTr("Application Log")

	property int pageWidth: subsurfaceTheme.columnWidth - Kirigami.Units.smallSpacing

	ColumnLayout {
		width: logWindow.width - logWindow.leftPadding - logWindow.rightPadding - 2 * Kirigami.Units.smallSpacing
		spacing: Kirigami.Units.smallSpacing
		Kirigami.Heading {
			text: qsTr("Application Log")
		}
		Kirigami.Label {
			id: logContent
			width: parent.width
			Layout.preferredWidth: parent.width
			Layout.maximumWidth: parent.width
			wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
			text: manager.logText
		}
		Rectangle {
			color: "transparent"
			height: Kirigami.Units.gridUnit * 2
			width: pageWidth
		}
	}
}
