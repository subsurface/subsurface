#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

extern int selected_dive;
#define current_dive (get_dive(selected_dive))

extern GtkWidget *dive_profile_frame(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_frame(void);
extern GtkWidget *create_dive_list(void);
extern void update_dive_info(struct dive *dive);
extern void repaint_dive(void);

#endif
