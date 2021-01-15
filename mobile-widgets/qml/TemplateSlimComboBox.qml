// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.kde.kirigami 2.4 as Kirigami

TemplateComboBox {
	id: cb
	Layout.fillWidth: false
	Layout.preferredHeight: Kirigami.Units.gridUnit * 2
	Layout.minimumWidth: Kirigami.Units.gridUnit * 8
	contentItem: Text {
		text: cb.displayText
		font.pointSize: subsurfaceTheme.regularPointSize
		anchors.right: indicator.left
		anchors.left: cb.left
		color: subsurfaceTheme.textColor
		leftPadding: Kirigami.Units.smallSpacing * 0.5
		rightPadding: Kirigami.Units.smallSpacing * 0.5
		verticalAlignment: Text.AlignVCenter
	}
}
