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

import QtQuick 2.0
import QtGraphicalEffects 1.0
import org.kde.plasma.mobilecomponents 0.2

Item {
    id: root
    property string source
    property alias smooth: image.smooth
    property bool active: false
    property bool valid: image.status == Image.Ready 
    implicitWidth: Math.min(image.sourceSize.width, Units.iconSizes.medium)
    implicitHeight: Math.min(image.sourceSize.height, Units.iconSizes.medium)

    Image {
        id: image
        anchors.fill: parent
        source: root.source != "" ? (root.source.indexOf(".") === -1 ? "icons/" + root.source + ".svg" : root.source) : root.source
    }
    GammaAdjust {
        anchors.fill: image
        source: image
        gamma: root.active ? 3.0 : 1
    }
}
