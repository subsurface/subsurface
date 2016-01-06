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
import org.kde.plasma.mobilecomponents 0.2
import "private"

Rectangle {
    id: root

    /**
     * type:PageStack
     * The page stack that this page is owned by.
     */
    property Item pageStack

    /**
     * Defines the toolbar contents for the page. If the page stack is set up
     * using a toolbar instance, it automatically shows the currently active
     * page's toolbar contents in the toolbar.
     *
     * The default value is null resulting in the page's toolbar to be
     * invisible when the page is active.
     */
    property Item tools: null

    /**
     * Defines the contextual actions for the page:
     * an easy way to assign actions in the right sliding panel
     */
    property alias contextualActions: internalContextualActions.data

    Item {
        id: internalContextualActions
    }

    Layout.fillWidth: true
    color: "transparent"
}
