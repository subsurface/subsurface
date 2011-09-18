#ifndef DISPLAY_GTK_H
#define DISPLAY_GTK_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

extern GtkWidget *main_window;

extern void import_dialog(GtkWidget *, gpointer);
extern void report_error(GError* error);

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(void);

#endif
