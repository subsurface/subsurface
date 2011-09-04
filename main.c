#include <stdio.h>
#include <string.h>
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
	int i;

	qsort(dive_table.dives, dive_table.nr, sizeof(struct dive *), sortfn);

	for (i = 1; i < dive_table.nr; i++) {
		struct dive **pp = &dive_table.dives[i-1];
		struct dive *prev = pp[0];
		struct dive *dive = pp[1];
		struct dive *merged;

		if (prev->when + prev->duration.seconds < dive->when)
			continue;

		merged = try_to_merge(prev, dive);
		if (!merged)
			continue;

		free(prev);
		free(dive);
		*pp = merged;
		dive_table.nr--;
		memmove(pp+1, pp+2, sizeof(*pp)*(dive_table.nr - i));

		/* Redo the new 'i'th dive */
		i--;
	}
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
		save_dives(filename);
		g_free(filename);
	}
	gtk_widget_destroy(dialog);
}

static void quit(GtkWidget *w, gpointer data)
{
	gtk_main_quit();
}

static GtkActionEntry menu_items[] = {
	{ "FileMenuAction", GTK_STOCK_FILE, "File", NULL, NULL, NULL},
	{ "OpenFile",       GTK_STOCK_OPEN, NULL,   "<control>O", NULL, G_CALLBACK(file_open) },
	{ "SaveFile",       GTK_STOCK_SAVE, NULL,   "<control>S", NULL, G_CALLBACK(file_save) },
	{ "Quit",           GTK_STOCK_QUIT, NULL,   "<control>Q", NULL, G_CALLBACK(quit) },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static const gchar* ui_string = " \
	<ui> \
		<menubar name=\"MainMenu\"> \
			<menu name=\"FileMenu\" action=\"FileMenuAction\"> \
				<menuitem name=\"Open\" action=\"OpenFile\" /> \
				<menuitem name=\"Save\" action=\"SaveFile\" /> \
				<menuitem name=\"Quit\" action=\"Quit\" /> \
			</menu> \
		</menubar> \
	</ui> \
";

static GtkWidget *get_menubar_menu(GtkWidget *window)
{
	GtkActionGroup *action_group = gtk_action_group_new("Menu");
	gtk_action_group_add_actions(action_group, menu_items, nmenu_items, 0);

	GtkUIManager *ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
	GError* error = 0;
	gtk_ui_manager_add_ui_from_string(GTK_UI_MANAGER(ui_manager), ui_string, -1, &error);

	gtk_window_add_accel_group(GTK_WINDOW(window), gtk_ui_manager_get_accel_group(ui_manager));
	GtkWidget* menu = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");

	return menu;
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
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(on_destroy), NULL);
	main_window = win;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);

	menubar = get_menubar_menu(win);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	/* Table for the list of dives, cairo window, and dive info */
	table = gtk_table_new(2, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 5);
	gtk_box_pack_end(GTK_BOX(vbox), table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	/* Create the atual divelist */
	divelist = create_dive_list();
	gtk_table_attach(GTK_TABLE(table), divelist, 0, 1, 0, 2,
		0, GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0);

	/* Frame for minimal dive info */
	frame = dive_info_frame();
	gtk_table_attach(GTK_TABLE(table), frame, 1, 2, 0, 1,
		 GTK_FILL | GTK_SHRINK | GTK_EXPAND, 0, 0, 0);

	/* Notebook for dive info vs profile vs .. */
	notebook = gtk_notebook_new();
	gtk_table_attach_defaults(GTK_TABLE(table), notebook, 1, 2, 1, 2);

	/* Frame for dive profile */
	frame = dive_profile_frame();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Dive Profile"));
	dive_profile = frame;

	/* Frame for extended dive info */
	frame = extended_dive_info_frame();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, gtk_label_new("Extended Dive Info"));

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	gtk_main();
	return 0;
}
