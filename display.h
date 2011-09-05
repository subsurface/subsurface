#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern void update_dive_info(struct dive *dive);
extern void repaint_dive(void);

#endif
