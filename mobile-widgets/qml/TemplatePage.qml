// SPDX-License-Identifier: GPL-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
	width: rootItem.colWidth
	height: parent.height
	visible: false
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
}

