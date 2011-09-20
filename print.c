#include <gtk/gtk.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"

static void draw_page(GtkPrintOperation *operation,
			GtkPrintContext *context,
			gint page_nr,
			gpointer user_data)
{
	cairo_t *cr;
	PangoLayout *layout;
	double w, h;
	struct graphics_context gc = { .printer = 1 };

	cr = gtk_print_context_get_cairo_context(context);
	gc.cr = cr;

	layout=gtk_print_context_create_pango_layout(context);

	w = gtk_print_context_get_width(context);
	h = gtk_print_context_get_height(context);

	/* Do the profile on the top half of the page.. */
	if (current_dive)
		plot(&gc, w, h/2, current_dive);

	pango_cairo_show_layout(cr,layout);
	g_object_unref(layout);
}

static void begin_print(GtkPrintOperation *operation, gpointer user_data)
{
}

static GtkPrintSettings *settings = NULL;

void do_print(void)
{
	GtkPrintOperation *print;
	GtkPrintOperationResult res;

	print = gtk_print_operation_new();
	if (settings != NULL)
		gtk_print_operation_set_print_settings(print, settings);
	gtk_print_operation_set_n_pages(print, 1);
	g_signal_connect(print, "begin_print", G_CALLBACK(begin_print), NULL);
	g_signal_connect(print, "draw_page", G_CALLBACK(draw_page), NULL);
	res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
					 GTK_WINDOW(main_window), NULL);
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (settings != NULL)
			g_object_unref(settings);
		settings = g_object_ref(gtk_print_operation_get_print_settings(print));
	}
	g_object_unref(print);
}
