/*
*   Copyright (C) 2011 by Marco Martin <mart@kde.org>
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
 * This is a label which uses the plasma Theme.
 *
 * The characteristics of the text will be automatically set according to the
 * plasma Theme. If you need a more customized text item use the Text component
 * from QtQuick.
 *
 * You can use all elements of the QML Text component, in particular the "text"
 * property to define the label text.
 *
 * @inherit QtQuick.Text
 */
Text {
    id: root

    height: Math.round(Math.max(paintedHeight, Units.gridUnit * 1.6))
    verticalAlignment: lineCount > 1 ? Text.AlignTop : Text.AlignVCenter

    activeFocusOnTab: false
    renderType: Text.NativeRendering

    font.capitalization: Theme.defaultFont.capitalization
    font.family: Theme.defaultFont.family
    font.italic: Theme.defaultFont.italic
    font.letterSpacing: Theme.defaultFont.letterSpacing
    font.pointSize: Theme.defaultFont.pointSize
    font.strikeout: Theme.defaultFont.strikeout
    font.underline: Theme.defaultFont.underline
    font.weight: Theme.defaultFont.weight
    font.wordSpacing: Theme.defaultFont.wordSpacing
    color: Theme.textColor

    opacity: enabled? 1 : 0.6

    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
