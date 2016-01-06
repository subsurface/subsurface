/*
 *   Copyright 2012 Marco Martin <mart@kde.org>
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
import QtGraphicalEffects 1.0
import org.kde.plasma.mobilecomponents 0.2
import "private"

/**Documented API

Imports:
        QtQuick 2.1

Description:
        Overlay Drawers are used to expose additional UI elements needed for
        small secondary tasks for which the main UI elements are not needed.
        For example in Okular Active, an Overlay Drawer is used to display
        thumbnails of all pages within a document along with a search field.
        This is used for the distinct task of navigating to another page.

Properties:
        bool opened:
        If true the drawer is open showing the contents of the "drawer"
        component.

        Item page:
        It's the default property. it's the main content of the drawer page,
        the part that is always shown

        Item contentItem:
        It's the part that can be pulled in and out, will act as a sidebar.
**/
AbstractDrawer {
    id: root
    anchors.fill: parent
    z: 9999

    default property alias page: mainPage.data
    property Item contentItem
    property alias opened: mainFlickable.open
    property int edge: Qt.LeftEdge
    property real position: 0
    onPositionChanged: {
        if (!mainFlickable.flicking && !mainFlickable.dragging && !mainAnim.running) {
            switch (root.edge) {
            case Qt.RightEdge:
                mainFlickable.contentX = drawerPage.width * position;
                break;
            case Qt.LeftEdge:
                mainFlickable.contentX = drawerPage.width * (1-position);
                break;
            case Qt.BottomEdge:
                mainFlickable.contentY = drawerPage.height * position;
                break;
            }
        }
    }
    onContentItemChanged: {
        contentItem.parent = drawerPage
        contentItem.anchors.fill = drawerPage
    }


    function open () {
        mainAnim.to = 0;
        switch (root.edge) {
        case Qt.RightEdge:
            mainAnim.to = drawerPage.width;
            break;
        case Qt.BottomEdge:
            mainAnim.to = drawerPage.height;
            break;
        case Qt.LeftEdge:
        case Qt.TopEdge:
        default:
            mainAnim.to = 0;
            break;
        }
        mainAnim.running = true;
        mainFlickable.open = true;
    }
    function close () {
        switch (root.edge) {
        case Qt.RightEdge:
        case Qt.BottomEdge:
            mainAnim.to = 0;
            break;
        case Qt.LeftEdge:
            mainAnim.to = drawerPage.width;
            break;
        case Qt.TopEdge:
            mainAnim.to = drawerPage.height;
            break;
        }
        mainAnim.running = true;
        mainFlickable.open = false;
    }

    Item {
        id: mainPage
        anchors.fill: parent
        onChildrenChanged: mainPage.children[0].anchors.fill = mainPage
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.6 * mainFlickable.internalPosition
    }


    NumberAnimation {
        id: mainAnim
        target: mainFlickable
        properties: (root.edge == Qt.RightEdge || root.edge == Qt.LeftEdge) ? "contentX" : "contentY"
        duration: Units.longDuration
        easing.type: Easing.InOutQuad
    }

    MouseArea {
        id: edgeMouse
        anchors {
            right: root.edge == Qt.LeftEdge ? undefined : parent.right
            left: root.edge == Qt.RightEdge ? undefined : parent.left
            top: root.edge == Qt.BottomEdge ? undefined : parent.top
            bottom: root.edge == Qt.TopEdge ? undefined : parent.bottom
        }
        z: 99
        width: Units.smallSpacing * 2
        height: Units.smallSpacing * 2
        property int startMouseX
        property real oldMouseX
        property int startMouseY
        property real oldMouseY
        property bool startDragging: false

        onPressed: {
            if (drawerPage.children.length == 0) {
                mouse.accepted = false;
                return;
            }

            speedSampler.restart();
            mouse.accepted = true;
            oldMouseX = startMouseX = mouse.x;
            oldMouseY = startMouseY = mouse.y;
            startDragging = false;
        }

        onPositionChanged: {
            if (!root.contentItem) {
                mouse.accepted = false;
                return;
            }

            if (Math.abs(mouse.x - startMouseX) > root.width / 5 ||
                Math.abs(mouse.y - startMouseY) > root.height / 5) {
                startDragging = true;
            }
            if (startDragging) {
                switch (root.edge) {
                case Qt.RightEdge:
                    mainFlickable.contentX = Math.min(mainItem.width - root.width, mainFlickable.contentX + oldMouseX - mouse.x);
                    break;
                case Qt.LeftEdge:
                    mainFlickable.contentX = Math.max(0, mainFlickable.contentX + oldMouseX - mouse.x);
                    break;
                case Qt.BottomEdge:
                    mainFlickable.contentY = Math.min(mainItem.height - root.height, mainFlickable.contentY + oldMouseY - mouse.y);
                    break;
                case Qt.TopEdge:
                    mainFlickable.contentY = Math.max(0, mainFlickable.contentY + oldMouseY - mouse.y);
                    break;
                }
            }
            oldMouseX = mouse.x;
            oldMouseY = mouse.y;
        }
        onReleased: {
            speedSampler.running = false;
            if (speedSampler.speed != 0) {
                if (root.edge == Qt.RightEdge || root.edge == Qt.LeftEdge) {
                    mainFlickable.flick(speedSampler.speed, 0);
                } else {
                    mainFlickable.flick(0, speedSampler.speed);
                }
            } else {
                if (mainFlickable.internalPosition > 0.5) {
                    root.open();
                } else {
                    root.close();
                }
            }
        }
    }

    Timer {
        id: speedSampler
        interval: 100
        repeat: true
        property real speed
        property real oldContentX
        property real oldContentY
        onTriggered: {
            if (root.edge == Qt.RightEdge || root.edge == Qt.LeftEdge) {
                speed = (mainFlickable.contentX - oldContentX) * 10;
                oldContentX = mainFlickable.contentX;
            } else {
                speed = (mainFlickable.contentY - oldContentY) * 10;
                oldContentY = mainFlickable.contentY;
            }
        }
        onRunningChanged: {
            if (running) {
                speed = 0;
                oldContentX = mainFlickable.contentX;
                oldContentY = mainFlickable.contentY;
            } else {
                if (root.edge == Qt.RightEdge || root.edge == Qt.LeftEdge) {
                    speed = (oldContentX - mainFlickable.contentX) * 10;
                } else {
                    speed = (oldContentY - mainFlickable.contentY) * 10;
                }
            }
        }
    }

    MouseArea {
        id: mainMouseArea
        anchors.fill: parent
        drag.filterChildren: true
        onClicked: {
            if ((root.edge == Qt.LeftEdge && mouse.x < drawerPage.width) ||
                (root.edge == Qt.RightEdge && mouse.x > drawerPage.x) ||
                (root.edge == Qt.TopEdge && mouse.y < drawerPage.height) ||
                (root.edge == Qt.BottomEdge && mouse.y > drawerPage.y)) {
                return;
            }
            root.clicked();
            root.close();
        }
        enabled: (root.edge == Qt.LeftEdge && !mainFlickable.atXEnd) ||
                 (root.edge == Qt.RightEdge && !mainFlickable.atXBeginning) ||
                 (root.edge == Qt.TopEdge && !mainFlickable.atYEnd) ||
                 (root.edge == Qt.BottomEdge && !mainFlickable.atYBeginning) ||
                 mainFlickable.moving

        Flickable {
            id: mainFlickable
            property bool open
            anchors.fill: parent

            onOpenChanged: {
                if (open) {
                    root.open();
                } else {
                    root.close();
                }
            }
            enabled: parent.enabled
            flickableDirection: root.edge == Qt.LeftEdge || root.edge == Qt.RightEdge ? Flickable.HorizontalFlick : Flickable.VerticalFlick
            contentWidth: mainItem.width
            contentHeight: mainItem.height
            boundsBehavior: Flickable.StopAtBounds
            property real internalPosition: {
                switch (root.edge) {
                case Qt.RightEdge:
                    return mainFlickable.contentX/drawerPage.width;
                case Qt.LeftEdge:
                    return 1 - (mainFlickable.contentX/drawerPage.width);
                case Qt.BottomEdge:
                    return mainFlickable.contentY/drawerPage.height;
                case Qt.TopEdge:
                    return 1 - (mainFlickable.contentY/drawerPage.height);
                }
            }
            onInternalPositionChanged: {
                root.position = internalPosition;
            }

            onFlickingChanged: {
                if (!flicking) {
                    if (internalPosition > 0.5) {
                        root.open();
                    } else {
                        root.close();
                    }
                }
            }
            onMovingChanged: {
                if (!moving) {
                    flickingChanged();
                }
            }

            Item {
                id: mainItem
                width: root.width + ((root.edge == Qt.RightEdge || root.edge == Qt.LeftEdge) ? drawerPage.width : 0)
                height: root.height + ((root.edge == Qt.TopEdge || root.edge == Qt.BottomEdge) ? drawerPage.height : 0)
                onWidthChanged: {
                    if (root.edge == Qt.LeftEdge) {
                        mainFlickable.contentX = drawerPage.width;
                    }
                }
                onHeightChanged: {
                    if (root.edge == Qt.TopEdge) {
                        mainFlickable.contentY = drawerPage.Height;
                    }
                }

                Rectangle {
                    id: drawerPage
                    anchors {
                        left: root.edge != Qt.RightEdge ? parent.left : undefined
                        right: root.edge != Qt.LeftEdge ? parent.right : undefined
                        top: root.edge != Qt.BottomEdge ? parent.top : undefined
                        bottom: root.edge != Qt.TopEdge ? parent.bottom : undefined
                    }
                    color: Theme.viewBackgroundColor
                    clip: true
                    width: root.contentItem ? Math.min(root.contentItem.implicitWidth, root.width - Units.gridUnit * 2) : 0
                    height: root.contentItem ? Math.min(root.contentItem.implicitHeight, root.height - Units.gridUnit * 2) : 0
                }
                LinearGradient {
                    width: Units.gridUnit/2
                    height: Units.gridUnit/2
                    anchors {
                        right: root.edge == Qt.RightEdge ? drawerPage.left : (root.edge == Qt.LeftEdge ? undefined : parent.right)
                        left: root.edge == Qt.LeftEdge ? drawerPage.right : (root.edge == Qt.RightEdge ? undefined : parent.left)
                        top: root.edge == Qt.TopEdge ? drawerPage.bottom : (root.edge == Qt.BottomEdge ? undefined : parent.top)
                        bottom: root.edge == Qt.BottomEdge ? drawerPage.top : (root.edge == Qt.TopEdge ? undefined : parent.bottom)
                    }

                    opacity: root.position == 0 ? 0 : 1
                    start: Qt.point(0, 0)
                    end: (root.edge == Qt.RightEdge || root.edge == Qt.LeftEdge) ? Qt.point(Units.gridUnit/2, 0) : Qt.point(0, Units.gridUnit/2)
                    gradient: Gradient {
                        GradientStop {
                            position: 0.0
                            color: root.edge == Qt.LeftEdge ? Qt.rgba(0, 0, 0, 0.3) : "transparent"
                        }
                        GradientStop {
                            position: root.edge == Qt.LeftEdge ? 0.3 : 0.7
                            color: Qt.rgba(0, 0, 0, 0.15)
                        }
                        GradientStop {
                            position: 1.0
                            color: root.edge == Qt.LeftEdge ? "transparent" : Qt.rgba(0, 0, 0, 0.3)
                        }
                    }
                    Behavior on opacity {
                        NumberAnimation {
                            duration: Units.longDuration
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
        }
    }
}
