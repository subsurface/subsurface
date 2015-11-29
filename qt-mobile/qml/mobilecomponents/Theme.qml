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

import QtQuick 2.4

pragma Singleton

/*!
   \qmltype Theme
   \inqmlmodule Material 0.1

   \brief Provides access to standard colors that follow the Material Design specification.

   See \l {http://www.google.com/design/spec/style/color.html#color-ui-color-application} for
   details about choosing a color scheme for your application.
 */
QtObject {
    id: theme

    property color textColor: Qt.rgba(0,0,0, 0.54)
    
    property color highlightColor: "#2196F3"
    property color backgroundColor: "#f3f3f3"
    property color linkColor: "#2196F3"
    property color visitedLinkColor: "#2196F3"

    property color buttonTextColor: Qt.rgba(0,0,0, 0.54)
    property color buttonBackgroundColor: "#f3f3f3"
    property color buttonHoverColor: "#2196F3"
    property color buttonFocusColor: "#2196F3"

    property color viewTextColor: Qt.rgba(0,0,0, 0.54)
    property color viewBackgroundColor: "#f3f3f3"
    property color viewHoverColor: "#2196F3"
    property color viewFocusColor: "#2196F3"

    property color complementaryTextColor: "#f3f3f3"
    property color complementaryBackgroundColor: Qt.rgba(0,0,0, 0.54)
    property color complementaryHoverColor: "#2196F3"
    property color complementaryFocusColor: "#2196F3"

    property font defaultFont: fontMetrics.font

    property variant fontMetrics: TextMetrics {}

}
