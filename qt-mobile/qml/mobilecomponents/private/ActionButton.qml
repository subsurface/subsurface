/*
 *   Copyright 2015 Marco Martin <mart@kde.org>
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.1
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import org.kde.plasma.mobilecomponents 0.2

MouseArea {
    id: button
    property alias iconSource: icon.source
    property bool checkable: false
    property bool checked: false
    //either Action or QAction should work here
    property QtObject action
    Layout.minimumWidth: Units.iconSizes.large
    Layout.maximumWidth: Layout.minimumWidth
    implicitWidth: Units.iconSizes.large
    implicitHeight: width
    drag {
        target: button
        axis: Drag.XAxis
        minimumX: parent.width/2 - width/2 - (contextDrawer && contextDrawer.enabled ? contextDrawer.contentItem.width : 0)
        maximumX: parent.width/2 - width/2 + (globalDrawer && globalDrawer.enabled ?  globalDrawer.contentItem.width : 0)
    }
    function toggleVisibility() {
        showAnimation.running = false;
        if (translateTransform.y < button.height) {
            showAnimation.to = button.height;
        } else {
            showAnimation.to = 0;
        }
        showAnimation.running = true;
    }

    transform: Translate {
        id: translateTransform
    }
    onReleased: {
        if (globalDrawer && x > Math.min(parent.width/4*3, parent.width/2 + globalDrawer.contentItem.width/2)) {
            globalDrawer.open();
            contextDrawer.close();
        } else if (contextDrawer && x < Math.max(parent.width/4, parent.width/2 - contextDrawer.contentItem.width/2)) {
            if (contextDrawer) {
                contextDrawer.open();
            }
            if (globalDrawer) {
                globalDrawer.close();
            }
        } else {
            if (globalDrawer) {
                globalDrawer.close();
            }
            if (contextDrawer) {
                contextDrawer.close();
            }
        }
    }
    onClicked: {
        if (checkable) {
            checked = !checked;
        }

        //if an action has been assigned, trigger it
        if (button.action && button.action.trigger) {
            button.action.trigger();
        }
    }
    Connections {
        target: globalDrawer
        onPositionChanged: {
            if (!button.pressed) {
                button.x = globalDrawer.contentItem.width * globalDrawer.position + button.parent.width/2 - button.width/2;
            }
        }
    }
    Connections {
        target: contextDrawer
        onPositionChanged: {
            if (!button.pressed) {
                button.x = button.parent.width/2 - button.width/2 - contextDrawer.contentItem.width * contextDrawer.position;
            }
        }
    }
    Connections {
        target: button.parent
        onWidthChanged: button.x = button.parent.width/2 - button.width/2
    }
    onXChanged: {
        if (button.pressed) {
            if (globalDrawer) {
                globalDrawer.position = Math.min(1, Math.max(0, (x - button.parent.width/2 + button.width/2)/globalDrawer.contentItem.width));
            }
            if (contextDrawer) {
                contextDrawer.position = Math.min(1, Math.max(0, (button.parent.width/2 - button.width/2 - x)/contextDrawer.contentItem.width));
            }
        }
    }

    NumberAnimation {
        id: showAnimation
        target: translateTransform
        properties: "y"
        duration: Units.longDuration
        easing.type: Easing.InOutQuad
    }
    Item {
        id: background
        anchors {
            fill: parent
            leftMargin: -Units.gridUnit
            rightMargin: -Units.gridUnit
        }
        Rectangle {
            radius: width/2
            anchors.centerIn: parent
            height: parent.height - Units.smallSpacing*2
            width: height
            color: button.pressed || button.checked ? Theme.highlightColor : Theme.backgroundColor
            Icon {
                id: icon
                anchors {
                    fill: parent
                    margins: Units.smallSpacing
                }
            }
            ActionButtonArrow {
                anchors {
                    right: parent.left
                    rightMargin: Units.smallSpacing
                }
                visible: contextDrawer && contextDrawer.enabled
                inverted: true
            }
            ActionButtonArrow {
                anchors {
                    left: parent.right
                    leftMargin: Units.smallSpacing
                }
                visible: globalDrawer && globalDrawer.enabled
            }
            Behavior on color {
                ColorAnimation {
                    duration: Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
            Behavior on x {
                NumberAnimation {
                    duration: Units.longDuration
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
    DropShadow {
        anchors.fill: background
        horizontalOffset: 0
        verticalOffset: Units.smallSpacing/3
        radius: Units.gridUnit / 3.5
        samples: 16
        color: button.pressed ? "transparent" : Qt.rgba(0, 0, 0, 0.5)
        source: background
    }
}

