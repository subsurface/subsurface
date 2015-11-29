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
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0
import org.kde.plasma.mobilecomponents 0.2

OverlayDrawer {
    id: root
    edge: Qt.LeftEdge

    default property alias content: mainContent.data

    property alias title: heading.text
    property alias titleIcon: headingIcon.source
    property alias bannerImageSource: bannerImage.source
    property list<Action> actions

    contentItem: ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        spacing: 0
        implicitWidth: Units.gridUnit * 12

        Image {
            id: bannerImage
            Layout.fillWidth: true

            Layout.preferredWidth: title.implicitWidth
            Layout.preferredHeight: bannerImageSource != "" ? Math.max(title.implicitHeight, Math.floor(width / (sourceSize.width/sourceSize.height))) : title.implicitHeight
            Layout.minimumHeight: Math.max(headingIcon.height, heading.height) + Units.smallSpacing*2

            fillMode: Image.PreserveAspectCrop

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }

            LinearGradient {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }
                visible: bannerImageSource != ""
                height: title.height * 1.3
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: Qt.rgba(0, 0, 0, 0.8)
                    }
                    GradientStop {
                        position: 1.0
                        color: "transparent"
                    }
                }
            }

            RowLayout {
                id: title
                anchors {
                    left: parent.left
                    top: parent.top
                    margins: Units.smallSpacing
                }
                Icon {
                    id: headingIcon
                    Layout.minimumWidth: Units.iconSizes.large
                    Layout.minimumHeight: width
                }
                Heading {
                    id: heading
                    level: 1
                    color: bannerImageSource != "" ? "white" : Theme.textColor
                }
                Item {
                    height: parent.height
                    Layout.minimumWidth: height
                }
            }
        }

        Rectangle {
            color: Theme.textColor
            opacity: 0.2
            Layout.fillWidth: true
            Layout.minimumHeight: 1
        }

        StackView {
            id: pageRow
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: menuComponent
        }

        ColumnLayout {
            id: mainContent
            Layout.alignment: Qt.AlignHCenter
            Layout.minimumWidth: parent.width - Units.smallSpacing*2
            Layout.maximumWidth: Layout.minimumWidth
            Layout.fillWidth: false
            Layout.fillHeight: true
        }
        Item {
            Layout.minimumWidth: Units.smallSpacing
            Layout.minimumHeight: Units.smallSpacing
        }

        Component {
            id: menuComponent
            ListView {
                id: optionMenu
                clip: true

                model: actions
                property int level: 0

                footer: ListItem {
                    visible: level > 0
                    enabled: true
                    RowLayout {
                        anchors {
                            left: parent.left
                        }
                        Icon {
                            Layout.maximumWidth: height
                            Layout.fillHeight: true
                            source: "go-previous"
                        }
                        Label {
							// Weird, this doesn't work
                            //text: (typeof(i18n) != undefined) ? i18n("Back") : "Back"
							text: "Back"
                        }
                    }
                    onClicked: pageRow.pop()
                }
                delegate: ListItem {
                    enabled: true
                    RowLayout {
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                        Icon {
                            Layout.maximumWidth: height
                            Layout.fillHeight: true
                            source: modelData.iconName
                        }
                        Label {
                            Layout.fillWidth: true
                            text: modelData.text
                        }
                        Icon {
                            Layout.maximumWidth: height
                            Layout.fillHeight: true
                            source: "go-next"
                            visible: modelData.children != undefined
                        }
                    }
                    onClicked: {
                        if (modelData.children) {
                            pageRow.push(menuComponent, {"model": modelData.children, "level": level + 1});
                        } else {
                            modelData.trigger();
                            pageRow.pop(pageRow.initialPage);
                            root.opened = false;
                        }
                    }
                }
            }
        }
    }
}

