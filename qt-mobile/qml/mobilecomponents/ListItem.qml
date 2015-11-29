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
import org.kde.plasma.mobilecomponents 0.2

/**
 * An item delegate for the primitive ListView component.
 *
 * It's intended to make all listviews look coherent.
 *
 * @inherit QtQuick.Item
 */
Item {
    id: listItem
    default property alias content: paddingItem.data

    /**
     * type:bool Holds if the item emits signals related to mouse interaction.
     *
     * The default value is false.
     */
    property alias enabled: itemMouse.enabled
    //item has been clicked or pressed+hold

    /**
     * This signal is emitted when there is a click.
     *
     * This is disabled by default, set enabled to true to use it.
     * @see enabled
     */
    signal clicked


    /**
     * The user pressed the item with the mouse and didn't release it for a
     * certain amount of time.
     *
     * This is disabled by default, set enabled to true to use it.
     * @see enabled
     */
    signal pressAndHold

    /**
     * If true makes the list item look as checked or pressed. It has to be set
     * from the code, it won't change by itself.
     */
    //plasma extension
    //always look pressed?
    property bool checked: false

    /**
     * If true the item will be a delegate for a section, so will look like a
     * "title" for the otems under it.
     */
    //is this to be used as section delegate?
    property bool sectionDelegate: false

    /**
     * True if the list item contains mouse
     */
    property alias containsMouse: itemMouse.containsMouse

    width: parent ? parent.width : childrenRect.width
    height: paddingItem.childrenRect.height + Units.smallSpacing*2

    property int implicitHeight: paddingItem.childrenRect.height + Units.smallSpacing*2


    MouseArea {
        id: itemMouse
        property bool changeBackgroundOnPress: !listItem.checked && !listItem.sectionDelegate
        anchors.fill: parent
        enabled: false
        hoverEnabled: true

        onClicked: listItem.clicked()
        onPressAndHold: listItem.pressAndHold()

        Rectangle {
            id : background
            color: listItem.checked || (itemMouse.pressed && itemMouse.changeBackgroundOnPress) ? Theme.highlightColor : Theme.viewBackgroundColor

            anchors.fill: parent
            visible: listItem.ListView.view ? listItem.ListView.view.highlight === null : true
            opacity: itemMouse.containsMouse && !itemMouse.pressed ? 0.5 : 1
            Behavior on color {
                ColorAnimation { duration: Units.longDuration }
            }
            Behavior on opacity { NumberAnimation { duration: Units.longDuration } }
        }
        Item {
            id: paddingItem
            anchors {
                fill: parent
                margins: Units.smallSpacing
            }
        }
    }

    Rectangle {
        color: Theme.textColor
        opacity: 0.2
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 1
    }

    Accessible.role: Accessible.ListItem
}
