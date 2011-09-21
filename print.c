#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <gtk/gtk.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"

/* Why doesn't pango/gtk have these quoting functions? */
static inline int add_char(char *buffer, size_t size, int len, char c)
{
	if (len < size)
		buffer[len++] = c;
	return len;
}

/* Add an escape string "atomically" - all or nothing */
static int add_str(char *buffer, size_t size, int len, const char *s)
{
	int oldlen = len;
	char c;
	while ((c = *s++) != 0) {
		if (len >= size)
			return oldlen;
		buffer[len++] = c;
	}
	return len;
}

static int add_quoted_string(char *buffer, size_t size, int len, const char *s)
{
	if (!s)
		return len;

	/* Room for '\0' */
	size--;
	for (;;) {
		const char *escape;
		unsigned char c = *s++;
		switch(c) {
		default:
			len = add_char(buffer, size, len, c);
			continue;
		case 0:
			escape = "\n";
			break;
		case '&':
			escape = "&amp;";
			break;
		case '>':
			escape = "&gt;";
			break;
		case '<':
			escape = "&lt;";
			break;
		}
		len = add_str(buffer, size, len, escape);
		if (c)
			continue;
		buffer[len] = 0;
		return len;
	}
}

/*
 * You know what? Maybe somebody can do a real Pango layout thing.
 * This is hacky.
 */
static void show_dive_text(struct dive *dive, cairo_t *cr, double w, double h)
{
	int len;
	PangoLayout *layout;
	struct tm *tm;
	char buffer[1024], divenr[20];

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_width(layout, w * PANGO_SCALE);
	pango_layout_set_height(layout, h * PANGO_SCALE * 0.9);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

	*divenr = 0;
	if (dive->number)
		snprintf(divenr, sizeof(divenr), "Dive #%d - ", dive->number);


	tm = gmtime(&dive->when);
	len = snprintf(buffer, sizeof(buffer),
		"<span size=\"large\">%s%s, %s %d, %d   %d:%02d</span>\n",
		divenr,
		weekday(tm->tm_wday),
		monthname(tm->tm_mon),
		tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min);

	len = add_quoted_string(buffer, sizeof(buffer), len, dive->location);
	len = add_quoted_string(buffer, sizeof(buffer), len, dive->notes);

	pango_layout_set_markup(layout, buffer, -1);

	cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
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

	/* Plus 5% on all sides */
	cairo_translate(cr, w/20, h/20);
	w *= 0.9; h *= 0.9;

	/* We actually want to scale the text and the lines now */
	cairo_scale(cr, 0.5, 0.5);

	/* Dive plot in the upper two thirds - note the scaling */
	show_dive_profile(dive, cr, w*2, h*1.33);

	/* Dive information in the lower third */
	cairo_translate(cr, 0, h*1.33);

	show_dive_text(dive, cr, w*2, h*0.67);

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
	print(nr+4, cr, 0, 2*h, w, h);
	print(nr+5, cr, w, 2*h, w, h);
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

	repaint_dive();
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
