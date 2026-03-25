// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	width: rootItem.colWidth
	height: parent.height
	visible: false
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
}

