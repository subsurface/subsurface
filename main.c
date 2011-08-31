#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "dive.h"

static void show_dive(int nr, struct dive *dive)
{
	int i;
	struct tm *tm;

	tm = gmtime(&dive->when);

	printf("At %02d:%02d:%02d %04d-%02d-%02d  (%d ft max, %d minutes)\n",
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		to_feet(dive->maxdepth), dive->duration.seconds / 60);

	if (!verbose)
		return;

	for (i = 0; i < dive->samples; i++) {
		struct sample *s = dive->sample + i;

		printf("%4d:%02d: %3d ft, %2d C, %4d PSI\n",
			s->time.seconds / 60,
			s->time.seconds % 60,
			to_feet(s->depth),
			to_C(s->temperature),
			to_PSI(s->tankpressure));
	}
}

static int sortfn(const void *_a, const void *_b)
{
	const struct dive *a = *(void **)_a;
	const struct dive *b = *(void **)_b;

	if (a->when < b->when)
		return -1;
	if (a->when > b->when)
		return 1;
	return 0;
}

/*
 * This doesn't really report anything at all. We just sort the
 * dives, the GUI does the reporting
 */
static void report_dives(void)
{
	qsort(dive_table.dives, dive_table.nr, sizeof(struct dive *), sortfn);
}

static void parse_argument(const char *arg)
{
	const char *p = arg+1;

	do {
		switch (*p) {
		case 'v':
			verbose++;
			continue;
		default:
			fprintf(stderr, "Bad argument '%s'\n", arg);
			exit(1);
		}
	} while (*++p);
}

static void on_destroy(GtkWidget* w, gpointer data)
{
	gtk_main_quit();
}

static GtkTreeModel *fill_dive_list(void)
{
	int i;
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new(1, G_TYPE_STRING);

	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = dive_table.dives[i];

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, dive->name,
			-1);
	}

	return GTK_TREE_MODEL(store);
}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = dive_table.dives[0];
	cairo_t *cr;
	int i;

	cr = gdk_cairo_create(widget->window);
	cairo_set_source_rgb(cr, 0, 0, 0);
	gdk_cairo_rectangle(cr, &event->area);
	cairo_fill(cr);

	cairo_set_line_width(cr, 3);
	cairo_set_source_rgb(cr, 1, 1, 1);

	if (dive->samples) {
		struct sample *sample = dive->sample;
		cairo_move_to(cr, sample->time.seconds / 5, to_feet(sample->depth) * 3);
		for (i = 1; i < dive->samples; i++) {
			sample++;
			cairo_line_to(cr, sample->time.seconds / 5, to_feet(sample->depth) * 3);
		}
		cairo_stroke(cr);
	}

	cairo_destroy(cr);

	return FALSE;
}

static GtkWidget *create_dive_list(void)
{
	GtkWidget *list;
	GtkCellRenderer *renderer;
	GtkTreeModel *model;

	list = gtk_tree_view_new();

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(list),
		-1, "Dive", renderer, "text", 0, NULL);

	model = fill_dive_list();
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), model);
	g_object_unref(model);
	return list;
}

int main(int argc, char **argv)
{
	int i;
	GtkWidget *win;
	GtkWidget *divelist;
	GtkWidget *vbox;
	GtkWidget *scrolled_window;
	GtkWidget *frame;
	GtkWidget *da;

	parse_xml_init();

	gtk_init(&argc, &argv);

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		parse_xml_file(a);
	}

	report_dives();

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(win), "destroy",      G_CALLBACK(on_destroy), NULL);

	/* HBOX for the list of dives and cairo window */
	vbox=gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_widget_show(vbox);

	/* Scrolled window for the list goes into the vbox.. */
	scrolled_window=gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolled_window, 150, 350);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);
	gtk_widget_show(scrolled_window);

	/* Frame for dive profile */
	frame = gtk_frame_new("Dive profile");
	gtk_container_add(GTK_CONTAINER(vbox), frame);
	gtk_widget_show(frame);
	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 450, 350);
	gtk_container_add(GTK_CONTAINER(frame), da);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);

	/* Create the atual divelist */
	divelist = create_dive_list();

	/* .. and connect it to the scrolled window */
	gtk_container_add(GTK_CONTAINER(scrolled_window), divelist);

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
