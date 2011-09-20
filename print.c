#include <gtk/gtk.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"

static void show_one_dive(struct dive *dive, cairo_t *cr, double w, double h)
{
	struct graphics_context gc = {
		.printer = 1,
		.cr = cr
	};
	plot(&gc, w, h, dive);
}

static void print(int divenr, cairo_t *cr, double x, double y, double w, double h)
{
	struct dive *dive;

	dive = get_dive(divenr);
	if (!dive)
		return;
	cairo_save(cr);
	cairo_translate(cr, x, y);

	/* Dive plot in the upper half */
	show_one_dive(dive, cr, w, h/2);

	/* Dive information in the lower half */

	cairo_restore(cr);
}

static void draw_page(GtkPrintOperation *operation,
			GtkPrintContext *context,
			gint page_nr,
			gpointer user_data)
{
	int nr;
	cairo_t *cr;
	PangoLayout *layout;
	double w, h;

	cr = gtk_print_context_get_cairo_context(context);
	layout=gtk_print_context_create_pango_layout(context);

	w = gtk_print_context_get_width(context) / 2;
	h = gtk_print_context_get_height(context) / 2;

	nr = page_nr*4;
	print(nr+0, cr, 0, 0, w, h);
	print(nr+1, cr, w, 0, w, h);
	print(nr+2, cr, 0, h, w, h);
	print(nr+3, cr, w, h, w, h);

	pango_cairo_show_layout(cr,layout);
	g_object_unref(layout);
}

static void begin_print(GtkPrintOperation *operation, gpointer user_data)
{
}

static GtkPrintSettings *settings = NULL;

void do_print(void)
{
	int pages;
	GtkPrintOperation *print;
	GtkPrintOperationResult res;

	print = gtk_print_operation_new();
	if (settings != NULL)
		gtk_print_operation_set_print_settings(print, settings);
	pages = (dive_table.nr + 3) / 4;
	gtk_print_operation_set_n_pages(print, pages);
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
