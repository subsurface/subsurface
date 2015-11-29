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
        Overlay Drawers are used to expose additional UI elements needed for small secondary tasks for which the main UI elements are not needed. For example in Okular Active, an Overlay Drawer is used to display thumbnails of all pages within a document along with a search field. This is used for the distinct task of navigating to another page.

Properties:
        bool opened:
        If true the drawer is open showing the contents of the "drawer" component.

        Item page:
        It's the default property. it's the main content of the drawer page, the part that is always shown

        Item contentItem:
        It's the part that can be pulled in and out, will act as a sidebar.
**/
AbstractDrawer {
    id: root
    anchors.fill: parent
    z: 9999

    default property alias page: mainPage.data
    property Item contentItem
    property alias opened: browserFrame.open
    property int edge: Qt.LeftEdge
    property real position: 0

    onContentItemChanged: contentItem.parent = drawerPage
    onPositionChanged: {
        if (!enabled) {
            return;
        }
        if (root.edge == Qt.LeftEdge) {
            browserFrame.x = -browserFrame.width + position * browserFrame.width;
        } else {
            browserFrame.x = root.width - (position * browserFrame.width);
        }
    }
    function open () {
        mouseEventListener.startBrowserFrameX = browserFrame.x;
        browserFrame.state = "Dragging";
        browserFrame.state = "Open";
    }
    function close () {
        mouseEventListener.startBrowserFrameX = browserFrame.x;
        browserFrame.state = "Dragging";
        browserFrame.state = "Closed";
    }

    Item {
        id: mainPage
        anchors.fill: parent
        onChildrenChanged: mainPage.children[0].anchors.fill = mainPage
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.6 * (root.edge == Qt.LeftEdge
            ? ((browserFrame.x + browserFrame.width) / browserFrame.width)
            : (1 - browserFrame.x / root.width))
    }

    MouseArea {
        anchors {
            right: root.edge == Qt.LeftEdge ? undefined : parent.right
            left: root.edge == Qt.LeftEdge ? parent.left : undefined
            top: parent.top
            bottom: parent.bottom
        }
        z: 99
        width: Units.smallSpacing
        onPressed: mouseEventListener.managePressed(mouse)
        onPositionChanged: mouseEventListener.positionChanged(mouse)
        onReleased: mouseEventListener.released(mouse)
    }
    MouseArea {
        id: mouseEventListener
        anchors.fill: parent
        drag.filterChildren: true
        property int startBrowserFrameX
        property int startMouseX
        property real oldMouseX
        property bool startDragging: false
        property string startState
        enabled: browserFrame.state != "Closed"

        onPressed: managePressed(mouse)
        function managePressed(mouse) {
            if (drawerPage.children.length == 0) {
                mouse.accepted = false;
                return;
            }

            mouse.accepted = true;
            startBrowserFrameX = browserFrame.x;
            oldMouseX = startMouseX = mouse.x;
            startDragging = false;
            startState = browserFrame.state;
            browserFrame.state = "Dragging";
            browserFrame.x = startBrowserFrameX;
        }

        onPositionChanged: {
            if (drawerPage.children.length == 0) {
                mouse.accepted = false;
                return;
            }

            if (mouse.x < Units.gridUnit ||
                Math.abs(mouse.x - startMouseX) > root.width / 5) {
                startDragging = true;
            }
            if (startDragging) {
                browserFrame.x = root.edge == Qt.LeftEdge
                    ? Math.min(0, browserFrame.x + mouse.x - oldMouseX)
                    : Math.max(root.width - browserFrame.width, browserFrame.x + mouse.x - oldMouseX);
            }
            oldMouseX = mouse.x;
        }

        onReleased: {
            if (drawerPage.children.length == 0) {
                mouse.accepted = false;
                return;
            }

            if (root.edge == Qt.LeftEdge) {
                if (mouse.x < Units.gridUnit) {
                    browserFrame.state = "Closed";
                } else if (browserFrame.x - startBrowserFrameX > browserFrame.width / 3) {
                    browserFrame.state = "Open";
                } else if (startBrowserFrameX - browserFrame.x > browserFrame.width / 3) {
                    browserFrame.state = "Closed";
                } else {
                    browserFrame.state = startState
                }

            } else {
                if (mouse.x > width - Units.gridUnit) {
                    browserFrame.state = "Closed";
                } else if (browserFrame.x - startBrowserFrameX > browserFrame.width / 3) {
                    browserFrame.state = "Closed";
                } else if (startBrowserFrameX - browserFrame.x > browserFrame.width / 3) {
                    browserFrame.state = "Open";
                } else {
                    browserFrame.state = startState;
                }
            }
        }
        onCanceled: {
            if (oldMouseX > width - Units.gridUnit) {
                browserFrame.state = "Closed";
            } else if (Math.abs(browserFrame.x - startBrowserFrameX) > browserFrame.width / 3) {
                browserFrame.state = startState == "Open" ? "Closed" : "Open";
            } else {
                browserFrame.state = "Open";
            }
        }
        onClicked: {
            if (Math.abs(startMouseX - mouse.x) > Units.gridUnit) {
                return;
            }
            if ((root.edge == Qt.LeftEdge && mouse.x > browserFrame.width) ||
                (root.edge != Qt.LeftEdge && mouse.x < browserFrame.x)) {
                browserFrame.state = startState == "Open" ? "Closed" : "Open";
                root.clicked();
            }
        }
        Rectangle {
            id: browserFrame
            z: 100
            color: Theme.viewBackgroundColor
            anchors {
                top: parent.top
                bottom: parent.bottom
            }

            width: {
                if (drawerPage.children.length > 0 && drawerPage.children[0].implicitWidth > 0) {
                    return Math.min( parent.width - Units.gridUnit, drawerPage.children[0].implicitWidth)
                } else {
                    return parent.width - Units.gridUnit * 3
                }
            }

            onXChanged: {
                root.position = root.edge == Qt.LeftEdge ? 1 + browserFrame.x/browserFrame.width : (root.width - browserFrame.x)/browserFrame.width;
            }

            state: "Closed"
            onStateChanged: open = (state != "Closed")
            property bool open: false
            onOpenChanged: {
                if (drawerPage.children.length == 0) {
                    return;
                }

                if (open) {
                    browserFrame.state = "Open";
                } else {
                    browserFrame.state = "Closed";
                }
            }

            LinearGradient {
                width: Units.gridUnit/2
                anchors {
                    right: root.edge == Qt.LeftEdge ? undefined : parent.left
                    left: root.edge == Qt.LeftEdge ? parent.right : undefined
                    top: parent.top
                    bottom: parent.bottom
                }
                opacity: root.position == 0 ? 0 : 1
                start: Qt.point(0, 0)
                end: Qt.point(Units.gridUnit/2, 0)
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


            MouseArea {
                id: drawerPage
                anchors {
                    fill: parent
                    //leftMargin: Units.gridUnit
                }
                clip: true
                onChildrenChanged: drawerPage.children[0].anchors.fill = drawerPage
            }

            states: [
                State {
                    name: "Open"
                    PropertyChanges {
                        target: browserFrame
                        x: root.edge == Qt.LeftEdge ? 0 : root.width - browserFrame.width
                    }
                },
                State {
                    name: "Dragging"
                    //workaround for a quirkiness of the state machine
                    //if no x binding gets defined in this state x will be set to whatever last x it had last time it was in this state
                    PropertyChanges {
                        target: browserFrame
                        x: mouseEventListener.startBrowserFrameX
                    }
                },
                State {
                    name: "Closed"
                    PropertyChanges {
                        target: browserFrame
                        x: root.edge == Qt.LeftEdge ? -browserFrame.width : root.width
                    }
                }
            ]

            transitions: [
                Transition {
                    //Exclude Dragging
                    to: "Open,Closed"
                    NumberAnimation {
                        id: transitionAnim
                        properties: "x"
                        duration: Units.longDuration
                        easing.type: Easing.InOutQuad
                    }
                }
            ]
        }
    }
}

