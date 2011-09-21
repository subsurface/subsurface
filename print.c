#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <gtk/gtk.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"

static void show_text(cairo_t *cr, int size, double x, double y, const char *fmt, ...)
{
	va_list args;
	char buffer[256], *p;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	cairo_set_font_size(cr, size);

	p = buffer;
	do {
		char *n = strchr(p, '\n');
		if (n)
			*n++ = 0;
		cairo_move_to(cr, x, y);
		cairo_show_text(cr, p);
		p = n;
		y += size;
	} while (p);
}

/*
 * You know what? Maybe somebody can do a real Pango layout thing.
 *
 * I'm going to do this with the cairo engine instead. I can only learn so
 * many new interfaces.
 */
static void show_dive_text(struct dive *dive, cairo_t *cr, double w, double h)
{
	struct tm *tm;

	tm = gmtime(&dive->when);
	show_text(cr, 16, 0, 2, "Dive #%d - %s, %s %d, %d    %d:%02d",
		dive->number,
		weekday(tm->tm_wday),
		monthname(tm->tm_mon),
		tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min);

	show_text(cr, 10, w*0.6, 0,
		"Max depth: %d ft\nDuration: %d:%02d",
		to_feet(dive->maxdepth),
		dive->duration.seconds / 60,
		dive->duration.seconds % 60);

	show_text(cr, 10, 0, 20, "%s", dive->location ?: "");
	show_text(cr, 10, 0, 30, "%s", dive->notes ?: "");
}

static void show_dive_profile(struct dive *dive, cairo_t *cr, double w, double h)
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

	/* We actually want to scale the text and the lines now */
	cairo_scale(cr, 0.5, 0.5);

	/* Dive plot in the upper 75% - note the scaling */
	show_dive_profile(dive, cr, w*2, h*1.5);

	/* Dive information in the lower 25% */
	cairo_translate(cr, 0, h*1.5);
	show_dive_text(dive, cr, w*2, h*0.5);

	cairo_restore(cr);
}

static void draw_page(GtkPrintOperation *operation,
			GtkPrintContext *context,
			gint page_nr,
			gpointer user_data)
{
	int nr;
	cairo_t *cr;
	double w, h;

	cr = gtk_print_context_get_cairo_context(context);

	w = gtk_print_context_get_width(context)/2;
	h = gtk_print_context_get_height(context)/3;

	nr = page_nr*6;
	print(nr+0, cr, 0,   0, w, h);
	print(nr+1, cr, w,   0, w, h);
	print(nr+2, cr, 0,   h, w, h);
	print(nr+3, cr, w,   h, w, h);
	print(nr+2, cr, 0, 2*h, w, h);
	print(nr+3, cr, w, 2*h, w, h);
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
	pages = (dive_table.nr + 5) / 6;
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
