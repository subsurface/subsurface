#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

extern GtkWidget *main_window;

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(void);

extern void repaint_dive(void);

#endif
