#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <gconf/gconf-client.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"

GtkWidget *main_window;
GtkWidget *main_vbox;
GtkWidget *error_info_bar;
GtkWidget *error_label;
int        error_count;
struct DiveList   dive_list;

GConfClient *gconf;
struct units output_units;

#define GCONF_NAME(x) "/apps/diveclog/" #x

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

static void on_info_bar_response(GtkWidget *widget, gint response,
                                 gpointer data)
{
	if (response == GTK_RESPONSE_OK)
	{
		gtk_widget_destroy(widget);
		error_info_bar = NULL;
	}
}

static void report_error(GError* error)
{
	if (error == NULL)
	{
		return;
	}
	
	if (error_info_bar == NULL)
	{
		error_count = 1;
		error_info_bar = gtk_info_bar_new_with_buttons(GTK_STOCK_OK,
		                                               GTK_RESPONSE_OK,
		                                               NULL);
		g_signal_connect(error_info_bar, "response", G_CALLBACK(on_info_bar_response), NULL);
		gtk_info_bar_set_message_type(GTK_INFO_BAR(error_info_bar),
		                              GTK_MESSAGE_ERROR);
		
		error_label = gtk_label_new(error->message);
		GtkWidget *container = gtk_info_bar_get_content_area(GTK_INFO_BAR(error_info_bar));
		gtk_container_add(GTK_CONTAINER(container), error_label);
		
		gtk_box_pack_start(GTK_BOX(main_vbox), error_info_bar, FALSE, FALSE, 0);
		gtk_widget_show_all(main_vbox);
	}
	else
	{
		error_count++;
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "Failed to open %i files.", error_count);
		gtk_label_set(GTK_LABEL(error_label), buffer);
	}
}

static void file_open(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new("Open File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *filenames;
		char *filename;
		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		
		GError *error = NULL;
		while(filenames != NULL) {
			filename = (char *)filenames->data;
			parse_xml_file(filename, &error);
			if (error != NULL)
			{
				report_error(error);
				g_error_free(error);
				error = NULL;
			}
			
			g_free(filename);
			filenames = g_slist_next(filenames);
		}
		g_slist_free(filenames);
		report_dives();
		dive_list_update_dives(dive_list);
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

static void create_radio(GtkWidget *dialog, const char *name, ...)
{
	va_list args;
	GtkRadioButton *group = NULL;
	GtkWidget *box, *label;

	box = gtk_hbox_new(TRUE, 10);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), box);

	label = gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

	va_start(args, name);
	for (;;) {
		int enabled;
		const char *name;
		GtkWidget *button;
		void *callback_fn;

		name = va_arg(args, char *);
		if (!name)
			break;
		callback_fn = va_arg(args, void *);
		enabled = va_arg(args, int);

		button = gtk_radio_button_new_with_label_from_widget(group, name);
		group = GTK_RADIO_BUTTON(button);
		gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), enabled);
		g_signal_connect(button, "toggled", G_CALLBACK(callback_fn), NULL);
	}
	va_end(args);
}

#define UNITCALLBACK(name, type, value)				\
static void name(GtkWidget *w, gpointer data) 			\
{								\
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))	\
		menu_units.type = value;			\
}

static struct units menu_units;

UNITCALLBACK(set_meter, length, METERS)
UNITCALLBACK(set_feet, length, FEET)
UNITCALLBACK(set_bar, pressure, BAR)
UNITCALLBACK(set_psi, pressure, PSI)
UNITCALLBACK(set_liter, volume, LITER)
UNITCALLBACK(set_cuft, volume, CUFT)
UNITCALLBACK(set_celsius, temperature, CELSIUS)
UNITCALLBACK(set_fahrenheit, temperature, FAHRENHEIT)

static void unit_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog;

	menu_units = output_units;

	dialog = gtk_dialog_new_with_buttons("Units",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	create_radio(dialog, "Depth:",
		"Meter", set_meter, (output_units.length == METERS),
		"Feet",  set_feet, (output_units.length == FEET),
		NULL);

	create_radio(dialog, "Pressure:",
		"Bar", set_bar, (output_units.pressure == BAR),
		"PSI",  set_psi, (output_units.pressure == PSI),
		NULL);

	create_radio(dialog, "Volume:",
		"Liter",  set_liter, (output_units.volume == LITER),
		"CuFt", set_cuft, (output_units.volume == CUFT),
		NULL);

	create_radio(dialog, "Temperature:",
		"Celsius", set_celsius, (output_units.temperature == CELSIUS),
		"Fahrenheit",  set_fahrenheit, (output_units.temperature == FAHRENHEIT),
		NULL);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		output_units = menu_units;
		update_dive_list_units(&dive_list);
		repaint_dive();
		gconf_client_set_bool(gconf, GCONF_NAME(feet), output_units.length == FEET, NULL);
		gconf_client_set_bool(gconf, GCONF_NAME(psi), output_units.pressure == PSI, NULL);
		gconf_client_set_bool(gconf, GCONF_NAME(cuft), output_units.volume == CUFT, NULL);
		gconf_client_set_bool(gconf, GCONF_NAME(fahrenheit), output_units.temperature == FAHRENHEIT, NULL);
	}
	gtk_widget_destroy(dialog);
}

static GtkActionEntry menu_items[] = {
	{ "FileMenuAction", GTK_STOCK_FILE, "Log", NULL, NULL, NULL},
	{ "OpenFile",       GTK_STOCK_OPEN, NULL,   "<control>O", NULL, G_CALLBACK(file_open) },
	{ "SaveFile",       GTK_STOCK_SAVE, NULL,   "<control>S", NULL, G_CALLBACK(file_save) },
	{ "Quit",           GTK_STOCK_QUIT, NULL,   "<control>Q", NULL, G_CALLBACK(quit) },
	{ "Units",          NULL, "Units", NULL, NULL, G_CALLBACK(unit_dialog) },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static const gchar* ui_string = " \
	<ui> \
		<menubar name=\"MainMenu\"> \
			<menu name=\"FileMenu\" action=\"FileMenuAction\"> \
				<menuitem name=\"Open\" action=\"OpenFile\" /> \
				<menuitem name=\"Save\" action=\"SaveFile\" /> \
				<separator name=\"Separator1\"/> \
				<menuitem name=\"Units\" action=\"Units\" /> \
				<separator name=\"Separator2\"/> \
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
	GtkWidget *paned;
	GtkWidget *info_box;
	GtkWidget *notebook;
	GtkWidget *frame;
	GtkWidget *dive_info;
	GtkWidget *cylinder_management;
	GtkWidget *menubar;
	GtkWidget *vbox;

	output_units = SI_units;
	parse_xml_init();

	gtk_init(&argc, &argv);

	g_type_init();
	gconf = gconf_client_get_default();

	if (gconf_client_get_bool(gconf, GCONF_NAME(feet), NULL))
		output_units.length = FEET;
	if (gconf_client_get_bool(gconf, GCONF_NAME(psi), NULL))
		output_units.pressure = PSI;
	if (gconf_client_get_bool(gconf, GCONF_NAME(cuft), NULL))
		output_units.volume = CUFT;
	if (gconf_client_get_bool(gconf, GCONF_NAME(fahrenheit), NULL))
		output_units.temperature = FAHRENHEIT;

	error_info_bar = NULL;
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(on_destroy), NULL);
	main_window = win;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	main_vbox = vbox;

	menubar = get_menubar_menu(win);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	/* HPane for left the dive list, and right the dive info */
	paned = gtk_hpaned_new();
	gtk_box_pack_end(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

	/* Create the actual divelist */
	dive_list = dive_list_create();
	gtk_paned_add1(GTK_PANED(paned), dive_list.container_widget);

	/* VBox for dive info, and tabs */
	info_box = gtk_vbox_new(FALSE, 6);
	gtk_paned_add2(GTK_PANED(paned), info_box);

	/* Frame for minimal dive info */
	frame = dive_info_frame();
	gtk_box_pack_start(GTK_BOX(info_box), frame, FALSE, TRUE, 6);

	/* Notebook for dive info vs profile vs .. */
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(info_box), notebook, TRUE, TRUE, 6);

	/* Frame for dive profile */
	dive_profile = dive_profile_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dive_profile, gtk_label_new("Dive Profile"));

	/* Frame for extended dive info */
	dive_info = extended_dive_info_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dive_info, gtk_label_new("Dive Notes"));

	/* Frame for extended dive info */
	cylinder_management = cylinder_management_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), cylinder_management, gtk_label_new("Cylinders"));

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);
	
	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		GError *error = NULL;
		parse_xml_file(a, &error);
		
		if (error != NULL)
		{
			report_error(error);
			g_error_free(error);
			error = NULL;
		}
	}

	report_dives();
	dive_list_update_dives(dive_list);

	gtk_main();
	return 0;
}
