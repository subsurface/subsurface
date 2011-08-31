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

int main(int argc, char **argv)
{
	int i;
	GtkWidget *win;
	GtkWidget *divelist;
	GtkWidget *vbox;
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

	/* HBOX for the list of dives and cairo window */
	vbox=gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_widget_show(vbox);

	/* Create the atual divelist */
	divelist = create_dive_list();
	gtk_container_add(GTK_CONTAINER(vbox), divelist);

	/* Frame for dive profile */
	frame = dive_profile_frame();
	gtk_container_add(GTK_CONTAINER(vbox), frame);

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
