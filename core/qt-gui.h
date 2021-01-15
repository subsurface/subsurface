// SPDX-License-Identifier: GPL-2.0
#ifndef QT_GUI_H
#define QT_GUI_H

void init_qt_late();
void init_ui();

void exit_ui();
void set_non_bt_addresses();

#if defined(SUBSURFACE_MOBILE)
#include <QQuickWindow>
void run_mobile_ui(double initial_font_size);
#else
void run_ui();
#endif

#endif // QT_GUI_H
