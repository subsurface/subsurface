#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

extern int selected_dive;
extern GtkWidget *dive_profile_frame(void);
extern GtkWidget *create_dive_list(void);
extern void repaint_dive(void);

#endif
