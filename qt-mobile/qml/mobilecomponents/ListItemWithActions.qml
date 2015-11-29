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
import QtQuick.Layouts 1.3
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
    height: paddingItem.childrenRect.height + Units.smallSpacing*2

    property int implicitHeight: paddingItem.childrenRect.height + Units.smallSpacing*2


    Rectangle {
        id : background
        color: Theme.backgroundColor

        width: parent.width
        height: parent.height
        MouseArea {
            anchors.fill: parent
            onClicked: itemMouse.x = 0;
        }
        RowLayout {
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                rightMargin: y
            }
            height: Math.min( parent.height/1.5, Units.iconSizes.medium)
            property bool exclusive: false
            property Item checkedButton
            spacing: 0
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
                delegate: ToolButton {
                    Layout.fillHeight: true
                    Layout.minimumWidth: height
                    iconName: modelData.iconName
                    property bool flat: false
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
    InnerShadow {
        anchors.fill: parent
        radius: Units.smallSpacing*2
        samples: 16
        horizontalOffset: 0
        verticalOffset: Units.smallSpacing/2
        color: Qt.rgba(0, 0, 0, 0.3)
        source: background
    }
    LinearGradient {
        id: shadow
        //TODO: depends from app layout
        property bool inverse: true
        width: Units.smallSpacing*2
        anchors {
            right: shadow.inverse ? undefined : itemMouse.left
            left: shadow.inverse ? itemMouse.right : undefined
            top: itemMouse.top
            bottom: itemMouse.bottom
            rightMargin: -1
        }
        start: Qt.point(0, 0)
        end: Qt.point(Units.smallSpacing*2, 0)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: shadow.inverse ? Qt.rgba(0, 0, 0, 0.3) : "transparent"
            }
            GradientStop {
                position: shadow.inverse ? 0.3 : 0.7
                color: Qt.rgba(0, 0, 0, 0.15)
            }
            GradientStop {
                position: 1.0
                color: shadow.inverse ? "transparent" : Qt.rgba(0, 0, 0, 0.3)
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

            MouseArea {
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
                }
                onReleased: {
                    if (itemMouse.x > -itemMouse.width/2) {
                        itemMouse.x = 0;
                    } else {
                        itemMouse.x = -itemMouse.width + width*2
                    }
                }
                onClicked: {
                    if (itemMouse.x < -itemMouse.width/2) {
                        itemMouse.x = 0;
                    } else {
                        itemMouse.x = -itemMouse.width + width*2
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
