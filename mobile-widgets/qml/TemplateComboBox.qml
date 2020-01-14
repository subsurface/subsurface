// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.kde.kirigami 2.4 as Kirigami

ComboBox {
	Layout.fillWidth: true
	Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
	inputMethodHints: Qt.ImhNoPredictiveText
	font.pointSize: subsurfaceTheme.regularPointSize
}
