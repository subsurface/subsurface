#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

GtkWidget *main_window;

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
	update_dive_info(current_dive);
	gtk_widget_queue_draw(dive_profile);
}

static char *existing_filename;

static void file_open(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Open File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		printf("Open: '%s'\n", filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static void file_save(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Save File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	if (!existing_filename) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "Untitled document");
	} else
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), existing_filename);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		printf("Save: '%s'\n", filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static GtkItemFactoryEntry menu_items[] = {
	{ "/_File",		NULL,		NULL,		0, "<Branch>" },
	{ "/File/_Open",	"<control>O",	file_open,	0, "<StockItem>", GTK_STOCK_OPEN },
	{ "/File/_Save",	"<control>S",	file_save,	0, "<StockItem>", GTK_STOCK_SAVE },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

/* This is just directly from the gtk menubar tutorial. */
static GtkWidget *get_menubar_menu(GtkWidget *window)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);

	gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	return gtk_item_factory_get_widget(item_factory, "<main>");
}

int main(int argc, char **argv)
{
	int i;
	GtkWidget *win;
	GtkWidget *divelist;
	GtkWidget *table;
	GtkWidget *notebook;
	GtkWidget *frame;
	GtkWidget *menubar;
	GtkWidget *vbox;

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
	main_window = win;

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_add(GTK_CONTAINER(win), vbox);

	menubar = get_menubar_menu(win);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

	/* Table for the list of dives, cairo window, and dive info */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	gtk_box_pack_end(GTK_BOX(vbox), table, FALSE, TRUE, 0);
	gtk_widget_show(table);

	/* Create the atual divelist */
	divelist = create_dive_list();
	gtk_table_attach_defaults(GTK_TABLE(table), divelist, 0, 1, 0, 2);

	/* Notebook for dive info vs profile vs .. */
	notebook = gtk_notebook_new();
	gtk_table_attach_defaults(GTK_TABLE(table), notebook, 1, 2, 1, 2);

	/* Frame for dive info */
	frame = dive_info_frame();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Dive Info"));

	/* Frame for dive profile */
	frame = dive_profile_frame();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Dive Profile"));
	dive_profile = frame;

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
