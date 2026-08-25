// SPDX-License-Identifier: GPL-2.0
import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
	width: rootItem.colWidth
	// parent is still null at construction for pages pushed dynamically via
	// pageStack.push(Qt.resolvedUrl(...)) -- they're reparented into the
	// PageRow's ColumnView a tick after creation. Binding straight to
	// parent.height threw a TypeError that aborted placing the page in the
	// scene, leaving it permanently invisible.
	height: parent ? parent.height : 0
	visible: false
	background: Rectangle { color: subsurfaceTheme.backgroundColor }
}

