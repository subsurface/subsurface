// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.12
import QtQuick.Dialogs 1.3
import org.subsurfacedivelog.mobile 1.0
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	title: qsTr("Dive planner manager")

	ColumnLayout {
		width: parent.width
		spacing: 1
		Layout.margins: Kirigami.Units.gridUnit

		Text {
			text: "Dive planner manager"
		}
	}
}
