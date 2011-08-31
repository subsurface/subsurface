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

static gboolean on_expose(GtkWidget* w, GdkEventExpose* e, gpointer data)
{
	cairo_t* cr;
	cr = gdk_cairo_create(w->window);
	cairo_destroy(cr);
	return FALSE;
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
	g_signal_connect(G_OBJECT(win), "expose-event", G_CALLBACK(on_expose), NULL);

	/* VBOX for the list of dives */
	vbox=gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_widget_show(vbox);

	/* Scrolled window for the list goes into the vbox.. */
	scrolled_window=gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolled_window, 250, 350);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vbox), scrolled_window);
	gtk_widget_show(scrolled_window);

	/* Create the atual divelist */
	divelist = create_dive_list();

	/* .. and connect it to the scrolled window */
	gtk_container_add(GTK_CONTAINER(scrolled_window), divelist);

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
