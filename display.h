#ifndef DISPLAY_H
#define DISPLAY_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

extern GtkWidget *main_window;

extern void import_dialog(GtkWidget *, gpointer);
extern void report_error(GError* error);

extern GtkWidget *dive_profile_widget(void);
extern GtkWidget *dive_info_frame(void);
extern GtkWidget *extended_dive_info_widget(void);
extern GtkWidget *equipment_widget(void);

extern void repaint_dive(void);
extern void do_print(void);

/*
 * Cairo scaling really is horribly horribly mis-designed.
 *
 * Which is sad, because I really like Cairo otherwise. But
 * the fact that the line width is scaled with the same scale
 * as the coordinate system is a f*&%ing disaster. So we
 * can't use it, and instead have this butt-ugly wrapper thing..
 */
struct graphics_context {
	cairo_t *cr;
	double maxx, maxy;
	double leftx, rightx;
	double topy, bottomy;
};

extern void plot(struct graphics_context *gc, int w, int h, struct dive *dive);

#endif
