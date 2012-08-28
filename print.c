#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <gtk/gtk.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"

#define FONT_NORMAL (12)
#define FONT_SMALL (FONT_NORMAL / 1.2)
#define FONT_LARGE (FONT_NORMAL * 1.2)

static struct options print_options;

#define PRETTYOPTIONCALLBACK(name, option) \
static void name(GtkWidget *w, gpointer data) \
{ \
	option = GTK_TOGGLE_BUTTON(w)->active; \
}

PRETTYOPTIONCALLBACK(print_profiles_toggle, print_options.print_profiles)


static void set_font(PangoLayout *layout, PangoFontDescription *font,
	double size, int align)
{
	pango_font_description_set_size(font, size * PANGO_SCALE);
	pango_layout_set_font_description(layout, font);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_alignment(layout, align);

}

/*
 * You know what? Maybe somebody can do a real Pango layout thing.
 * This is hacky.
 */
static void show_dive_text(struct dive *dive, cairo_t *cr, double w,
	double h, PangoFontDescription *font)
{
	double depth;
	const char *unit;
	int len, decimals, width, height, maxwidth, maxheight;
	PangoLayout *layout;
	struct tm *tm;
	char buffer[80], divenr[20], *people;

	maxwidth = w * PANGO_SCALE;
	maxheight = h * PANGO_SCALE * 0.9;

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_width(layout, maxwidth);
	pango_layout_set_height(layout, maxheight);

	*divenr = 0;
	if (dive->number)
		snprintf(divenr, sizeof(divenr), "Dive #%d - ", dive->number);

	tm = gmtime(&dive->when);
	len = snprintf(buffer, sizeof(buffer),
		"%s%s, %s %d, %d   %d:%02d",
		divenr,
		weekday(tm->tm_wday),
		monthname(tm->tm_mon),
		tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min);

	set_font(layout, font, FONT_LARGE, PANGO_ALIGN_LEFT);
	pango_layout_set_text(layout, buffer, len);
	pango_layout_get_size(layout, &width, &height);

	cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr, layout);

	people = dive->buddy;
	if (!people || !*people) {
		people = dive->divemaster;
		if (!people)
			people = "";
	}

	depth = get_depth_units(dive->maxdepth.mm, &decimals, &unit);
	snprintf(buffer, sizeof(buffer),
		"Max depth: %.*f %s\n"
		"Duration: %d min\n"
		"%s",
		decimals, depth, unit,
		(dive->duration.seconds+59) / 60,
		people);

	set_font(layout, font, FONT_SMALL, PANGO_ALIGN_RIGHT);
	pango_layout_set_text(layout, buffer, -1);

	cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr, layout);

	/*
	 * Show the dive location
	 *
	 * .. or at least a space to get the size.
	 *
	 * Move down by the size of the date, and limit the
	 * width to the same width as the date string.
	 */
	cairo_translate(cr, 0, height / (double) PANGO_SCALE);
	maxheight -= height;
	pango_layout_set_height(layout, 1);
	pango_layout_set_width(layout, width);

	set_font(layout, font, FONT_NORMAL, PANGO_ALIGN_LEFT);
	pango_layout_set_text(layout, dive->location ? : " ", -1);

	cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr, layout);

	pango_layout_get_size(layout, &width, &height);

	/*
	 * Show the dive notes
	 */
	if (dive->notes) {
		/* Move down by the size of the location (x2) */
		height = height * 2;
		cairo_translate(cr, 0, height / (double) PANGO_SCALE);
		maxheight -= height;

		/* Use the full width and remaining height for notes */
		pango_layout_set_height(layout, maxheight);
		pango_layout_set_width(layout, maxwidth);
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_justify(layout, 1);
		pango_layout_set_text(layout, dive->notes, -1);

		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout);
	}
	g_object_unref(layout);
}

static void show_table_header(cairo_t *cr, double w, double h,
    PangoFontDescription *font)
{
	int len, width, height, maxwidth, maxheight;
	PangoLayout *layout;
	char buffer[160];

	maxwidth = w * PANGO_SCALE;
	maxheight = h * PANGO_SCALE * 0.9;

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_width(layout, maxwidth);
	pango_layout_set_height(layout, maxheight);

	len = snprintf(buffer, sizeof(buffer),
		"Dive# -  Date              - Depth - Time - Master"
        " Buddy -- Location");

	set_font(layout, font, FONT_LARGE, PANGO_ALIGN_LEFT);
	pango_layout_set_text(layout, buffer, len);
	pango_layout_get_size(layout, &width, &height);

	//cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr, layout);
}

static void show_dive_table(struct dive *dive, cairo_t *cr, double w,
	double h, PangoFontDescription *font)
{
	double depth;
	const char *unit;
	int len, decimals, width, height, maxwidth, maxheight;
	PangoLayout *layout;
	struct tm *tm;
	char buffer[160], divenr[20];

	maxwidth = w * PANGO_SCALE;
	maxheight = h * PANGO_SCALE * 0.9;

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_width(layout, maxwidth);
	pango_layout_set_height(layout, maxheight);

	*divenr = 0;
	if (dive->number)
		snprintf(divenr, sizeof(divenr), "#%d -", dive->number);

	depth = get_depth_units(dive->maxdepth.mm, &decimals, &unit);

	tm = gmtime(&dive->when);
	len = snprintf(buffer, sizeof(buffer),
		"%s %s, %s %d, %d   %dh%02d  - %.*f %s - %d min - %s %s --  %s",
		divenr,
		weekday(tm->tm_wday),
		monthname(tm->tm_mon),
		tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min,
		decimals,
		depth,
		unit,
		(dive->duration.seconds+59) / 60,
		dive->divemaster ? : " ",
		dive->buddy ? : " ",
		dive->location ? : " "
		);

	set_font(layout, font, FONT_NORMAL, PANGO_ALIGN_LEFT);
	pango_layout_set_text(layout, buffer, len);
	pango_layout_get_size(layout, &width, &height);

	cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr, layout);

	///*
	 //* Show the dive notes
	 //*/
	if (dive->notes) {
		/* Move down by the size of the location (x2) */
		height = height * 1.3;
		cairo_translate(cr, 20, height / (double) PANGO_SCALE);
		maxheight -= height;

		/* Use the full width and remaining height for notes */
		pango_layout_set_height(layout, maxheight);
		pango_layout_set_width(layout, maxwidth);
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_justify(layout, 1);
		pango_layout_set_text(layout, dive->notes, -1);

		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout);
	}
	g_object_unref(layout);
}

static void show_dive_profile(struct dive *dive, cairo_t *cr, double w,
	double h)
{
	cairo_rectangle_int_t drawing_area = { w/20.0, h/20.0, w, h};
	struct graphics_context gc = {
		.printer = 1,
		.cr = cr
	};
	cairo_save(cr);
	plot(&gc, &drawing_area, dive);
	cairo_restore(cr);
}

static void print(int divenr, cairo_t *cr, double x, double y, double w,
	double h, PangoFontDescription *font)
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

	show_dive_text(dive, cr, w*2, h*0.67, font);

	cairo_restore(cr);
}

static void print_pretty_table(int divenr, cairo_t *cr, double x, double y,
	double w, double h, PangoFontDescription *font)
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

	show_dive_text(dive, cr, w*2, h*2, font);

	cairo_restore(cr);
}

static void print_table_header(cairo_t *cr, double x, double y,
	double w, double h, PangoFontDescription *font)
{
    cairo_save(cr);
	cairo_translate(cr, x, y);

	/* Plus 5% on all sides */
	cairo_translate(cr, w/20, h/20);
	w *= 0.9; h *= 0.9;

	/* We actually want to scale the text and the lines now */
	cairo_scale(cr, 0.5, 0.5);

	show_table_header(cr, w*2, h*2, font);

	cairo_restore(cr);
}

static void print_table(int divenr, cairo_t *cr, double x, double y,
	double w, double h, PangoFontDescription *font)
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

	show_dive_table(dive, cr, w*2, h*2, font);

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
	PangoFontDescription *font;

	cr = gtk_print_context_get_cairo_context(context);
	font = pango_font_description_from_string("Sans");

	w = gtk_print_context_get_width(context)/2;
	h = gtk_print_context_get_height(context)/3;

	nr = page_nr*6;
	print(nr+0, cr, 0,   0, w, h, font);
	print(nr+1, cr, w,   0, w, h, font);
	print(nr+2, cr, 0,   h, w, h, font);
	print(nr+3, cr, w,   h, w, h, font);
	print(nr+4, cr, 0, 2*h, w, h, font);
	print(nr+5, cr, w, 2*h, w, h, font);

	pango_font_description_free(font);
}

static void draw_pretty_table(GtkPrintOperation *operation,
			GtkPrintContext *context,
			gint page_nr,
			gpointer user_data)
{
	int nr;
	cairo_t *cr;
	double w, h;
	PangoFontDescription *font;

	cr = gtk_print_context_get_cairo_context(context);
	font = pango_font_description_from_string("Sans");

	w = gtk_print_context_get_width(context);
	h = gtk_print_context_get_height(context)/15;

	nr = page_nr*15;
	int i;
	for (i = 0; i < 15; i++) {
		print_pretty_table(nr+i, cr, 0,   0+h*i, w, h, font);
	}

	pango_font_description_free(font);
}

static void draw_table(GtkPrintOperation *operation,
			GtkPrintContext *context,
			gint page_nr,
			gpointer user_data)
{
	int nr;
	int n_dive_per_page = 25;
	cairo_t *cr;
	double w, h;
	PangoFontDescription *font;

	cr = gtk_print_context_get_cairo_context(context);
	font = pango_font_description_from_string("Sans");

	w = gtk_print_context_get_width(context);
	h = gtk_print_context_get_height(context)/(n_dive_per_page+1);

	nr = page_nr*n_dive_per_page;
    print_table_header(cr, 0, 0+h, w, h, font);
	int i;
	for (i = 0; i < n_dive_per_page; i++) {
		print_table(nr+i, cr, 0,   h*1.5+h*i, w, h, font);
	}

	pango_font_description_free(font);
}

static void begin_print(GtkPrintOperation *operation, gpointer user_data)
{
	int dives_per_page = 1;
	if (print_options.type == PRETTY) {
		if (print_options.print_profiles){
			dives_per_page = 6;
		} else {
			dives_per_page = 15;
		}
	} else {
		dives_per_page = 25;
	}
	int pages;
	pages = (dive_table.nr + dives_per_page - 1) / dives_per_page;
		gtk_print_operation_set_n_pages(operation, pages);
}

static void update_print_window(GtkWidget *w) {
	if (print_options.type == TABLE) {
		// type == table - disable the profile option
		gtk_widget_set_sensitive(w, FALSE);
	} else {
		// type == pretty - enable the profile option
		gtk_widget_set_sensitive(w, TRUE);
	}
}

#define OPTIONCALLBACK(name, type, value) \
static void name(GtkWidget *w, gpointer data) \
{\
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) \
		print_options.type = value; \
	update_print_window(data); \
}

OPTIONCALLBACK(set_pretty, type, PRETTY)
OPTIONCALLBACK(set_table, type, TABLE)

static GtkWidget *print_dialog(GtkPrintOperation *operation, gpointer user_data)
{
	GtkWidget *vbox, *button, *radio1, *radio2, *frame, *box;
	gtk_print_operation_set_custom_tab_label(operation, "Dive details");

	vbox = gtk_vbox_new(TRUE, 5);

	frame = gtk_frame_new("Print type");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 1);

	box = gtk_hbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(frame), box);

	radio1 = gtk_radio_button_new_with_label (NULL, "Pretty print");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio1),
		print_options.type == PRETTY);
	radio2 = gtk_radio_button_new_with_label_from_widget (
		GTK_RADIO_BUTTON (radio1), "Table print");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio2),
		print_options.type == TABLE);
	gtk_box_pack_start (GTK_BOX (box), radio1, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (box), radio2, TRUE, TRUE, 0);


	frame = gtk_frame_new("Print options");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 1);

	box = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), box);

	button = gtk_check_button_new_with_label("Show profiles");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), print_options.print_profiles);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 2);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(print_profiles_toggle), NULL);

	g_signal_connect(radio1, "toggled", G_CALLBACK(set_pretty), button);
	g_signal_connect(radio2, "toggled", G_CALLBACK(set_table), button);

	gtk_widget_show_all(vbox);
	return vbox;
}

static void print_dialog_apply(GtkPrintOperation *operation, GtkWidget *widget, gpointer user_data)
{
	if (print_options.type == PRETTY) {
		if (print_options.print_profiles){
			g_signal_connect(operation, "draw_page",
				G_CALLBACK(draw_page), NULL);
		} else {
			g_signal_connect(operation, "draw_page",
				G_CALLBACK(draw_pretty_table), NULL);
		}
	} else {
		g_signal_connect(operation, "draw_page",
			G_CALLBACK(draw_table), NULL);
	}
}

static GtkPrintSettings *settings = NULL;

void do_print(void)
{
	GtkPrintOperation *print;
	GtkPrintOperationResult res;

	repaint_dive();
	print = gtk_print_operation_new();
	gtk_print_operation_set_unit(print, GTK_UNIT_POINTS);
	if (settings != NULL)
		gtk_print_operation_set_print_settings(print, settings);
	g_signal_connect(print, "create-custom-widget", G_CALLBACK(print_dialog), NULL);
	g_signal_connect(print, "custom-widget-apply", G_CALLBACK(print_dialog_apply), NULL);
	g_signal_connect(print, "begin_print", G_CALLBACK(begin_print), NULL);
	res = gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
					 GTK_WINDOW(main_window), NULL);
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (settings != NULL)
			g_object_unref(settings);
		settings = g_object_ref(gtk_print_operation_get_print_settings(print));
	}
	g_object_unref(print);
}
