/*
 *   Copyright 2010 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Layouts 1.2
import org.kde.plasma.mobilecomponents 0.2

/**
 * An item delegate for the primitive ListView component.
 *
 * It's intended to make all listviews look coherent.
 *
 * @inherit QtQuick.Item
 */
ListItem {
    id: listItem

    property string label
    property var icon

    RowLayout {
        height: implicitHeight + Units.smallSpacing * 2
        anchors {
            left: parent.left
            margins: Units.largeSpacing
        }
        Icon {
            Layout.minimumHeight: Units.iconSizes.smallMedium
            Layout.minimumWidth: height
            source: listItem.icon
        }
        Label {
            text: listItem.label
        }
    }
}
