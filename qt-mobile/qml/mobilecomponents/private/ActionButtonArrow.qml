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

Canvas {
    id: canvas
    width: Units.gridUnit
    height: width
    property bool inverted
    property color color: parent.color
    anchors.verticalCenter: parent.verticalCenter

    onColorChanged: requestPaint()

    onPaint: {
        var ctx = canvas.getContext("2d");
        ctx.fillStyle = canvas.color;
        ctx.beginPath();
        if (inverted) {
            ctx.moveTo(canvas.width, 0);
            ctx.bezierCurveTo(canvas.width-canvas.width/8, 0,
                              canvas.width-canvas.width/8, canvas.height,
                              canvas.width, canvas.height);
            ctx.lineTo(0, canvas.height/2);
        } else {
            ctx.moveTo(0, 0);
            ctx.bezierCurveTo(canvas.width/8, 0,
                              canvas.width/8, canvas.height,
                              0, canvas.height);
            ctx.lineTo(canvas.width, canvas.height/2);
            //ctx.lineTo(0, canvas.height);
        }
        ctx.fill();
    }
}

