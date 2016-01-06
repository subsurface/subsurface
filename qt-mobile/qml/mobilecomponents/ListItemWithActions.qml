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
import QtQuick.Controls 1.0
import org.kde.plasma.mobilecomponents 0.2
import QtGraphicalEffects 1.0

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

    /**
     * Defines the actions for the page: at most 4 buttons will
     * contain the actions at the bottom of the page, if the main
     * item of the page is a Flickable or a ScrllArea, it will
     * control the visibility of the actions.
     */
    property alias actions: internalActions.data

    Item {
        id: internalActions
    }

    width: parent ? parent.width : childrenRect.width
    height: paddingItem.childrenRect.height + Units.smallSpacing * 2

    property int implicitHeight: paddingItem.childrenRect.height + Units.smallSpacing * 2


    Rectangle {
        id: shadowHolder
        color: Theme.backgroundColor
        x: itemMouse.x + itemMouse.width
        width: parent.width
        height: parent.height
    }
    InnerShadow {
        anchors.fill: shadowHolder
        radius: Units.smallSpacing * 2
        samples: 16
        horizontalOffset: verticalOffset
        verticalOffset: Units.smallSpacing / 2
        color: Qt.rgba(0, 0, 0, 0.3)
        source: shadowHolder
    }

    MouseArea {
        anchors.fill: parent
        drag {
            target: itemMouse
            axis: Drag.XAxis
            maximumX: 0
        }
        onClicked: itemMouse.x = 0;
        onPressed: handleArea.mouseDown(mouse);
        onPositionChanged: handleArea.positionChanged(mouse);
        onReleased: handleArea.released(mouse);
    }
    RowLayout {
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            rightMargin: y
        }
        height: Math.min( parent.height / 1.5, Units.iconSizes.medium)
        property bool exclusive: false
        property Item checkedButton
        spacing: Units.largeSpacing
        Repeater {
            model: {
                if (listItem.actions.length == 0) {
                    return null;
                } else {
                    return listItem.actions[0].text !== undefined &&
                        listItem.actions[0].trigger !== undefined ?
                            listItem.actions :
                            listItem.actions[0];
                }
            }
            delegate: Icon {
                Layout.fillHeight: true
                Layout.minimumWidth: height
                source: modelData.iconName
                MouseArea {
                    anchors {
                        fill: parent
                        margins: -Units.smallSpacing
                    }
                    onClicked: {
                        if (modelData && modelData.trigger !== undefined) {
                            modelData.trigger();
                        // assume the model is a list of QAction or Action
                        } else if (toolbar.model.length > index) {
                            toolbar.model[index].trigger();
                        } else {
                            console.log("Don't know how to trigger the action")
                        }
                        itemMouse.x = 0;
                    }
                }
            }
        }
    }


    MouseArea {
        id: itemMouse
        property bool changeBackgroundOnPress: !listItem.checked && !listItem.sectionDelegate
        width: parent.width
        height: parent.height
        enabled: false
        hoverEnabled: true

        onClicked: listItem.clicked()
        onPressAndHold: listItem.pressAndHold()

        Behavior on x {
            NumberAnimation {
                duration: Units.longDuration
                easing.type: Easing.InOutQuad
            }
        }

        Rectangle {
            id : item
            color: listItem.checked || (itemMouse.pressed && itemMouse.changeBackgroundOnPress) ? Theme.highlightColor : Theme.viewBackgroundColor
            anchors.fill: parent

            visible: listItem.ListView.view ? listItem.ListView.view.highlight === null : true
            Behavior on color {
                ColorAnimation { duration: Units.longDuration }
            }

            Item {
                id: paddingItem
                anchors {
                    fill: parent
                    margins: Units.smallSpacing
                }
            }

            Timer {
                id: speedSampler
                interval: 100
                repeat: true
                property real speed
                property real oldItemMouseX
                onTriggered: {
                    speed = itemMouse.x - oldItemMouseX;
                    oldItemMouseX = itemMouse.x;
                }
                onRunningChanged: {
                    if (running) {
                        speed = 0;
                    } else {
                        speed = itemMouse.x - oldItemMouseX;
                    }
                    oldItemMouseX = itemMouse.x;
                }
            }
            MouseArea {
                id: handleArea
                width: Units.iconSizes.smallMedium
                height: width
                preventStealing: true
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    rightMargin: y
                }
                drag {
                    target: itemMouse
                    axis: Drag.XAxis
                    maximumX: 0
                    minimumX: -listItem.width
                }
                function mouseDown(mouse) {
                    speedSampler.speed = 0;
                    speedSampler.running = true;
                }
                onPressed: mouseDown(mouse);
                onCanceled: speedSampler.running = false;
                onReleased: {
                    speedSampler.running = false;

                    if (speedSampler.speed < -Units.gridUnit * 3) {
                        itemMouse.x = -itemMouse.width + width * 2;
                    } else if (speedSampler.speed > Units.gridUnit * 3 || itemMouse.x > -itemMouse.width/3) {
                        itemMouse.x = 0;
                    } else {
                        itemMouse.x = -itemMouse.width + width * 2;
                    }
                }
                onClicked: {
                    if (itemMouse.x < -itemMouse.width/2) {
                        itemMouse.x = 0;
                    } else {
                        itemMouse.x = -itemMouse.width + width * 2
                    }
                }
                Icon {
                    anchors.fill: parent
                    source: "application-menu"
                }
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
