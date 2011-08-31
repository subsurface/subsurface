#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

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

static GtkWidget *dive_profile;

void repaint_dive(void)
{
	gtk_widget_queue_draw(dive_profile);
}

int main(int argc, char **argv)
{
	int i;
	GtkWidget *win;
	GtkWidget *divelist;
	GtkWidget *table;
	GtkWidget *frame;

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

	/* Table for the list of dives, cairo window, and dive info */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	gtk_container_add(GTK_CONTAINER(win), table);
	gtk_widget_show(table);

	/* Create the atual divelist */
	divelist = create_dive_list();
	gtk_table_attach_defaults(GTK_TABLE(table), divelist, 0, 1, 0, 2);

	/* Frame for dive profile */
	frame = dive_profile_frame();
	gtk_table_attach_defaults(GTK_TABLE(table), frame, 1, 2, 1, 2);
	dive_profile = frame;

	/* Frame for dive info */
	frame = dive_info_frame();
	gtk_table_attach_defaults(GTK_TABLE(table), frame, 1, 2, 0, 1);

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
