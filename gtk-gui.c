#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <gconf/gconf-client.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"

#include "libdivecomputer.h"

GtkWidget *main_window;
GtkWidget *main_vbox;
GtkWidget *error_info_bar;
GtkWidget *error_label;
int        error_count;
struct DiveList   dive_list;

GConfClient *gconf;
struct units output_units;

#define GCONF_NAME(x) "/apps/subsurface/" #x

void on_destroy(GtkWidget* w, gpointer data)
{
	gtk_main_quit();
}

static GtkWidget *dive_profile;

void repaint_dive(void)
{
	update_dive(current_dive);
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

void report_error(GError* error)
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
		dive_list_update_dives();
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
		/* Make sure to flush any modified old dive data with old units */
		update_dive(NULL);
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

static void renumber_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *frame, *button;

	dialog = gtk_dialog_new_with_buttons("Renumber",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	frame = gtk_frame_new("New starting number");
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), frame);

	button = gtk_spin_button_new_with_range(1, 50000, 1);
	gtk_container_add(GTK_CONTAINER(frame), button);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		int nr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
		renumber_dives(nr);
		repaint_dive();
	}
	gtk_widget_destroy(dialog);
}

static GtkActionEntry menu_items[] = {
	{ "FileMenuAction", GTK_STOCK_FILE, "Log", NULL, NULL, NULL},
	{ "OpenFile",       GTK_STOCK_OPEN, NULL,   "<control>O", NULL, G_CALLBACK(file_open) },
	{ "SaveFile",       GTK_STOCK_SAVE, NULL,   "<control>S", NULL, G_CALLBACK(file_save) },
	{ "Print",          GTK_STOCK_PRINT, NULL,  "<control>P", NULL, G_CALLBACK(do_print) },
	{ "Import",         NULL, "Import", NULL, NULL, G_CALLBACK(import_dialog) },
	{ "Units",          NULL, "Units",    NULL, NULL, G_CALLBACK(unit_dialog) },
	{ "Renumber",       NULL, "Renumber", NULL, NULL, G_CALLBACK(renumber_dialog) },
	{ "Quit",           GTK_STOCK_QUIT, NULL,   "<control>Q", NULL, G_CALLBACK(quit) },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static const gchar* ui_string = " \
	<ui> \
		<menubar name=\"MainMenu\"> \
			<menu name=\"FileMenu\" action=\"FileMenuAction\"> \
				<menuitem name=\"Open\" action=\"OpenFile\" /> \
				<menuitem name=\"Save\" action=\"SaveFile\" /> \
				<menuitem name=\"Print\" action=\"Print\" /> \
				<separator name=\"Separator1\"/> \
				<menuitem name=\"Import\" action=\"Import\" /> \
				<separator name=\"Separator2\"/> \
				<menuitem name=\"Units\" action=\"Units\" /> \
				<menuitem name=\"Renumber\" action=\"Renumber\" /> \
				<separator name=\"Separator3\"/> \
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

static void switch_page(GtkNotebook *notebook, gint arg1, gpointer user_data)
{
	repaint_dive();
}

void init_ui(int argc, char **argv)
{
	GtkWidget *win;
	GtkWidget *paned;
	GtkWidget *info_box;
	GtkWidget *notebook;
	GtkWidget *frame;
	GtkWidget *dive_info;
	GtkWidget *equipment;
	GtkWidget *menubar;
	GtkWidget *vbox;

	gtk_init(&argc, &argv);

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
	g_signal_connect(notebook, "switch-page", G_CALLBACK(switch_page), NULL);
	gtk_box_pack_start(GTK_BOX(info_box), notebook, TRUE, TRUE, 6);

	/* Frame for dive profile */
	dive_profile = dive_profile_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dive_profile, gtk_label_new("Dive Profile"));

	/* Frame for extended dive info */
	dive_info = extended_dive_info_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dive_info, gtk_label_new("Dive Notes"));

	/* Frame for dive equipment */
	equipment = equipment_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), equipment, gtk_label_new("Equipment"));

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	return;
}

void run_ui(void)
{
	gtk_main();
}

/* get the filenames the user selects and call the parsing function
 * on them
 * return 0 if the user cancelled the dialog
 */
int open_import_file_dialog(char *filterpattern, char *filtertext, 
			void(* parse_function)(char *))
{
	int ret=0;

	GtkWidget *dialog;
	GtkFileFilter *filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, filterpattern);
	gtk_file_filter_set_name(filter, filtertext);
	dialog = gtk_file_chooser_dialog_new("Open File",
					GTK_WINDOW(main_window),
					GTK_FILE_CHOOSER_ACTION_OPEN,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *filenames;
		char *filename;
		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		while(filenames != NULL) {
			filename = (char *)filenames->data;
			parse_function(filename);
			g_free(filename);
			filenames = g_slist_next(filenames);
		}
		g_slist_free(filenames);
		ret = 1;
	}
	gtk_widget_destroy(dialog);

	return ret;
}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = current_dive;
	struct graphics_context gc = { .printer = 0 };
	int w,h;

	w = widget->allocation.width;
	h = widget->allocation.height;

	gc.cr = gdk_cairo_create(widget->window);
	set_source_rgb(&gc, 0, 0, 0);
	cairo_paint(gc.cr);

	if (dive)
		plot(&gc, w, h, dive);

	cairo_destroy(gc.cr);

	return FALSE;
}

GtkWidget *dive_profile_widget(void)
{
	GtkWidget *da;

	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 350, 250);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);

	return da;
}

int process_ui_events(void)
{
	int ret=0;

	while (gtk_events_pending()) {
		if (gtk_main_iteration_do(0)) {
			ret = 1;
			break;
		}
	}
	return(ret);
}


static void fill_computer_list(GtkListStore *store)
{
	GtkTreeIter iter;
	struct device_list *list = device_list;

	for (list = device_list ; list->name ; list++) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, list->name,
			1, list->type,
			-1);
	}
}

static GtkComboBox *dive_computer_selector(GtkWidget *dialog)
{
	GtkWidget *hbox, *combo_box;
	GtkListStore *model;
	GtkCellRenderer *renderer;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, FALSE, 3);

	model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	fill_computer_list(model);

	combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
	gtk_box_pack_start(GTK_BOX(hbox), combo_box, FALSE, TRUE, 3);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 0, NULL);

	return GTK_COMBO_BOX(combo_box);
}

void import_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *hbox;
	GtkComboBox *computer;
	device_data_t devicedata = {
		.devname = "/dev/ttyUSB0",
	};

	dialog = gtk_dialog_new_with_buttons("Import from dive computer",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	computer = dive_computer_selector(dialog);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, FALSE, TRUE, 3);
	devicedata.progress->bar = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(hbox), devicedata.progress->bar);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		int type;
		GtkTreeIter iter;
		GtkTreeModel *model;
		const char *comp;
	case GTK_RESPONSE_ACCEPT:
		if (!gtk_combo_box_get_active_iter(computer, &iter))
			break;
		model = gtk_combo_box_get_model(computer);
		gtk_tree_model_get(model, &iter,
			0, &comp,
			1, &type,
			-1);
		devicedata.type = type;
		devicedata.name = comp;
		do_import(&devicedata);
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);

	report_dives();
	dive_list_update_dives();
}

void update_progressbar(progressbar_t *progress, double value)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->bar), value);
}
