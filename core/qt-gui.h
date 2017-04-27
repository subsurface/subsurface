// SPDX-License-Identifier: GPL-2.0
#ifndef QT_GUI_H
#define QT_GUI_H

void init_qt_late();
void init_ui();

void run_ui();
void exit_ui();

#if defined(SUBSURFACE_MOBILE)
#include <QQuickWindow>
extern QObject *qqWindowObject;
#endif

#endif // QT_GUI_H
