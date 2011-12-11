/* gtk-gui.c */
/* gtk UI implementation */
/* creates the window and overall layout
 * divelist, dive info, equipment and printing are handled in their own source files
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"

#include "libdivecomputer.h"

GtkWidget *main_window;
GtkWidget *main_vbox;
GtkWidget *error_info_bar;
GtkWidget *error_label;
GtkWidget *vpane, *hpane;
int        error_count;

#define DIVELIST_DEFAULT_FONT "Sans 8"
const char *divelist_font;

struct units output_units;

static GtkWidget *dive_profile;

visible_cols_t visible_cols = {TRUE, FALSE};

void repaint_dive(void)
{
	update_dive(current_dive);
	if (dive_profile)
		gtk_widget_queue_draw(dive_profile);
}

static char *existing_filename;
static gboolean need_icon = TRUE;

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
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new("Open File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.xml");
	gtk_file_filter_add_pattern(filter, "*.XML");
	gtk_file_filter_add_pattern(filter, "*.sda");
	gtk_file_filter_add_pattern(filter, "*.SDA");
	gtk_file_filter_add_mime_type(filter, "text/xml");
	gtk_file_filter_set_name(filter, "XML file");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *filenames;
		char *filename;
		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		
		GError *error = NULL;
		while(filenames != NULL) {
			filename = filenames->data;
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
		report_dives(FALSE);
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
		mark_divelist_changed(FALSE);
	}
	gtk_widget_destroy(dialog);
}

static void ask_save_changes()
{
	GtkWidget *dialog, *label, *content;
	dialog = gtk_dialog_new_with_buttons("Save Changes?",
		GTK_WINDOW(main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	label = gtk_label_new ("You have unsaved changes\nWould you like to save those before exiting the program?");
	gtk_container_add (GTK_CONTAINER (content), label);
	gtk_widget_show_all (dialog);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		file_save(NULL,NULL);
	}
	gtk_widget_destroy(dialog);
}

static gboolean on_delete(GtkWidget* w, gpointer data)
{
	/* Make sure to flush any modified dive data */
	update_dive(NULL);

	if (unsaved_changes())
		ask_save_changes();

	return FALSE; /* go ahead, kill the program, we're good now */
}

static void on_destroy(GtkWidget* w, gpointer data)
{
	gtk_main_quit();
}

static void quit(GtkWidget *w, gpointer data)
{
	/* Make sure to flush any modified dive data */
	update_dive(NULL);

	if (unsaved_changes())
		ask_save_changes();
	gtk_main_quit();
}

GtkTreeViewColumn *tree_view_column(GtkWidget *tree_view, int index, const char *title,
				data_func_t data_func, unsigned int flags)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *col;
	double xalign = 0.0; /* left as default */
	PangoAlignment align;
	gboolean visible;

	align = (flags & ALIGN_LEFT) ? PANGO_ALIGN_LEFT :
		(flags & ALIGN_RIGHT) ? PANGO_ALIGN_RIGHT :
		PANGO_ALIGN_CENTER;
	visible = !(flags & INVISIBLE);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();

	gtk_tree_view_column_set_title(col, title);
	if (!(flags & UNSORTABLE))
		gtk_tree_view_column_set_sort_column_id(col, index);
	gtk_tree_view_column_set_resizable(col, TRUE);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	if (data_func)
		gtk_tree_view_column_set_cell_data_func(col, renderer, data_func, (void *)(long)index, NULL);
	else
		gtk_tree_view_column_add_attribute(col, renderer, "text", index);
	gtk_object_set(GTK_OBJECT(renderer), "alignment", align, NULL);
	switch (align) {
	case PANGO_ALIGN_LEFT:
		xalign = 0.0;
		break;
	case PANGO_ALIGN_CENTER:
		xalign = 0.5;
		break;
	case PANGO_ALIGN_RIGHT:
		xalign = 1.0;
		break;
	}
	gtk_cell_renderer_set_alignment(GTK_CELL_RENDERER(renderer), xalign, 0.5);
	gtk_tree_view_column_set_visible(col, visible);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
	return col;
}

static void create_radio(GtkWidget *vbox, const char *name, ...)
{
	va_list args;
	GtkRadioButton *group = NULL;
	GtkWidget *box, *label;

	box = gtk_hbox_new(TRUE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

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

#define OPTIONCALLBACK(name, option) \
static void name(GtkWidget *w, gpointer data) \
{ \
	option = GTK_TOGGLE_BUTTON(w)->active; \
}

OPTIONCALLBACK(otu_toggle, visible_cols.otu)
OPTIONCALLBACK(sac_toggle, visible_cols.sac)
OPTIONCALLBACK(nitrox_toggle, visible_cols.nitrox)
OPTIONCALLBACK(temperature_toggle, visible_cols.temperature)
OPTIONCALLBACK(cylinder_toggle, visible_cols.cylinder)

static void event_toggle(GtkWidget *w, gpointer _data)
{
	gboolean *plot_ev = _data;

	*plot_ev = GTK_TOGGLE_BUTTON(w)->active;
}

static void preferences_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *font, *frame, *box, *vbox, *button;

	menu_units = output_units;

	dialog = gtk_dialog_new_with_buttons("Preferences",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	frame = gtk_frame_new("Units");
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	create_radio(box, "Depth:",
		"Meter", set_meter, (output_units.length == METERS),
		"Feet",  set_feet, (output_units.length == FEET),
		NULL);

	create_radio(box, "Pressure:",
		"Bar", set_bar, (output_units.pressure == BAR),
		"PSI",  set_psi, (output_units.pressure == PSI),
		NULL);

	create_radio(box, "Volume:",
		"Liter",  set_liter, (output_units.volume == LITER),
		"CuFt", set_cuft, (output_units.volume == CUFT),
		NULL);

	create_radio(box, "Temperature:",
		"Celsius", set_celsius, (output_units.temperature == CELSIUS),
		"Fahrenheit",  set_fahrenheit, (output_units.temperature == FAHRENHEIT),
		NULL);

	frame = gtk_frame_new("Columns");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	button = gtk_check_button_new_with_label("Show Temp");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.temperature);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(temperature_toggle), NULL);

	button = gtk_check_button_new_with_label("Show Cyl");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.cylinder);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(cylinder_toggle), NULL);

	button = gtk_check_button_new_with_label("Show O" UTF8_SUBSCRIPT_2 "%");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.nitrox);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(nitrox_toggle), NULL);

	button = gtk_check_button_new_with_label("Show SAC");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.sac);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(sac_toggle), NULL);

	button = gtk_check_button_new_with_label("Show OTU");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.otu);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(otu_toggle), NULL);

	font = gtk_font_button_new_with_font(divelist_font);
	gtk_box_pack_start(GTK_BOX(vbox), font, FALSE, FALSE, 5);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		/* Make sure to flush any modified old dive data with old units */
		update_dive(NULL);

		divelist_font = strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(font)));
		set_divelist_font(divelist_font);

		output_units = menu_units;
		update_dive_list_units();
		repaint_dive();
		update_dive_list_col_visibility();

		subsurface_set_conf("feet", PREF_BOOL, BOOL_TO_PTR(output_units.length == FEET));
		subsurface_set_conf("psi", PREF_BOOL, BOOL_TO_PTR(output_units.pressure == PSI));
		subsurface_set_conf("cuft", PREF_BOOL, BOOL_TO_PTR(output_units.volume == CUFT));
		subsurface_set_conf("fahrenheit", PREF_BOOL, BOOL_TO_PTR(output_units.temperature == FAHRENHEIT));
		subsurface_set_conf("TEMPERATURE", PREF_BOOL, BOOL_TO_PTR(visible_cols.temperature));
		subsurface_set_conf("CYLINDER", PREF_BOOL, BOOL_TO_PTR(visible_cols.cylinder));
		subsurface_set_conf("NITROX", PREF_BOOL, BOOL_TO_PTR(visible_cols.nitrox));
		subsurface_set_conf("SAC", PREF_BOOL, BOOL_TO_PTR(visible_cols.sac));
		subsurface_set_conf("OTU", PREF_BOOL, BOOL_TO_PTR(visible_cols.otu));
		subsurface_set_conf("divelist_font", PREF_STRING, divelist_font);
		subsurface_close_conf();
	}
	gtk_widget_destroy(dialog);
}

static void create_toggle(const char* label, int *on, void *_data)
{
	GtkWidget *button, *table = _data;
	int rows, cols, x, y;
	static int count;

	if (table == NULL) {
		/* magic way to reset the number of toggle buttons
		 * that we have already added - call this before you
		 * create the dialog */
		count = 0;
		return;
	}
	g_object_get(G_OBJECT(table), "n-columns", &cols, "n-rows", &rows, NULL);
	if (count > rows * cols) {
		gtk_table_resize(GTK_TABLE(table),rows+1,cols);
		rows++;
	}
	x = count % cols;
	y = count / cols;
	button = gtk_check_button_new_with_label(label);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), *on);
	gtk_table_attach_defaults(GTK_TABLE(table), button, x, x+1, y, y+1);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(event_toggle), on);
	count++;
}

static void selectevents_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *frame, *vbox, *table;

	dialog = gtk_dialog_new_with_buttons("SelectEvents",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);
	/* initialize the function that fills the table */
	create_toggle(NULL, NULL, NULL);

	frame = gtk_frame_new("Enable / Disable Events");
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	table = gtk_table_new(1, 4, TRUE);
	gtk_container_add(GTK_CONTAINER(frame), table);

	evn_foreach(&create_toggle, table);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		repaint_dive();
	}
	gtk_widget_destroy(dialog);
}

static void renumber_dialog(GtkWidget *w, gpointer data)
{
	int result;
	struct dive *dive;
	GtkWidget *dialog, *frame, *button, *vbox;

	dialog = gtk_dialog_new_with_buttons("Renumber",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	frame = gtk_frame_new("New starting number");
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	button = gtk_spin_button_new_with_range(1, 50000, 1);
	gtk_container_add(GTK_CONTAINER(frame), button);

	/*
	 * Do we have a number for the first dive already? Use that
	 * as the default.
	 */
	dive = get_dive(0);
	if (dive && dive->number)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), dive->number);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		int nr = gtk_spin_button_get_value(GTK_SPIN_BUTTON(button));
		renumber_dives(nr);
		repaint_dive();
	}
	gtk_widget_destroy(dialog);
}

static void about_dialog(GtkWidget *w, gpointer data)
{
	const char *logo_property = NULL;
	GdkPixbuf *logo = NULL;

	if (need_icon) {
#if defined __linux__ || defined __APPLE__
		GtkWidget *image = gtk_image_new_from_file("subsurface.svg");
#elif defined WIN32
		GtkWidget *image = gtk_image_new_from_file("subsurface.ico");
#endif

		if (image) {
			logo = gtk_image_get_pixbuf(GTK_IMAGE(image));
			logo_property = "logo";
		}
	}

	gtk_show_about_dialog(NULL,
		"program-name", "SubSurface",
		"comments", "Half-arsed divelog software in C",
		"license", "GPLv2",
		"version", VERSION_STRING,
		"copyright", "Linus Torvalds 2011",
		"logo-icon-name", "subsurface",
		/* Must be last: */
		logo_property, logo,
		NULL);
}

static void view_list(GtkWidget *w, gpointer data)
{
	gtk_paned_set_position(GTK_PANED(vpane), 0);
}

static void view_profile(GtkWidget *w, gpointer data)
{
	gtk_paned_set_position(GTK_PANED(hpane), 0);
	gtk_paned_set_position(GTK_PANED(vpane), 65535);
}

static void view_info(GtkWidget *w, gpointer data)
{
	gtk_paned_set_position(GTK_PANED(vpane), 65535);
	gtk_paned_set_position(GTK_PANED(hpane), 65535);
}

/* Ooh. I don't know how to get the half-way size. So I'm just using random numbers */
static void view_three(GtkWidget *w, gpointer data)
{
	gtk_paned_set_position(GTK_PANED(hpane), 400);
	gtk_paned_set_position(GTK_PANED(vpane), 200);
}

static GtkActionEntry menu_items[] = {
	{ "FileMenuAction", GTK_STOCK_FILE, "File", NULL, NULL, NULL},
	{ "LogMenuAction",  GTK_STOCK_FILE, "Log", NULL, NULL, NULL},
	{ "ViewMenuAction",  GTK_STOCK_FILE, "View", NULL, NULL, NULL},
	{ "FilterMenuAction",  GTK_STOCK_FILE, "Filter", NULL, NULL, NULL},
	{ "HelpMenuAction", GTK_STOCK_HELP, "Help", NULL, NULL, NULL},
	{ "OpenFile",       GTK_STOCK_OPEN, NULL,   "<control>O", NULL, G_CALLBACK(file_open) },
	{ "SaveFile",       GTK_STOCK_SAVE, NULL,   "<control>S", NULL, G_CALLBACK(file_save) },
	{ "Print",          GTK_STOCK_PRINT, NULL,  "<control>P", NULL, G_CALLBACK(do_print) },
	{ "Import",         NULL, "Import", NULL, NULL, G_CALLBACK(import_dialog) },
	{ "Preferences",    NULL, "Preferences", NULL, NULL, G_CALLBACK(preferences_dialog) },
	{ "Renumber",       NULL, "Renumber", NULL, NULL, G_CALLBACK(renumber_dialog) },
	{ "SelectEvents",   NULL, "SelectEvents", NULL, NULL, G_CALLBACK(selectevents_dialog) },
	{ "Quit",           GTK_STOCK_QUIT, NULL,   "<control>Q", NULL, G_CALLBACK(quit) },
	{ "About",          GTK_STOCK_ABOUT, NULL,  NULL, NULL, G_CALLBACK(about_dialog) },
	{ "ViewList",       NULL, "List",  "<control>1", NULL, G_CALLBACK(view_list) },
	{ "ViewProfile",    NULL, "Profile", "<control>2", NULL, G_CALLBACK(view_profile) },
	{ "ViewInfo",       NULL, "Info", "<control>3", NULL, G_CALLBACK(view_info) },
	{ "ViewThree",       NULL, "Three", "<control>4", NULL, G_CALLBACK(view_three) },
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
				<menuitem name=\"Preferences\" action=\"Preferences\" /> \
				<separator name=\"Separator3\"/> \
				<menuitem name=\"Quit\" action=\"Quit\" /> \
			</menu> \
			<menu name=\"LogMenu\" action=\"LogMenuAction\"> \
				<menuitem name=\"Renumber\" action=\"Renumber\" /> \
				<menu name=\"View\" action=\"ViewMenuAction\"> \
					<menuitem name=\"List\" action=\"ViewList\" /> \
					<menuitem name=\"Profile\" action=\"ViewProfile\" /> \
					<menuitem name=\"Info\" action=\"ViewInfo\" /> \
					<menuitem name=\"Paned\" action=\"ViewThree\" /> \
				</menu> \
			</menu> \
			<menu name=\"FilterMenu\" action=\"FilterMenuAction\"> \
				<menuitem name=\"SelectEvents\" action=\"SelectEvents\" /> \
			</menu> \
			<menu name=\"Help\" action=\"HelpMenuAction\"> \
				<menuitem name=\"About\" action=\"About\" /> \
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

void init_ui(int *argcp, char ***argvp)
{
	GtkWidget *win;
	GtkWidget *notebook;
	GtkWidget *dive_info;
	GtkWidget *dive_list;
	GtkWidget *equipment;
	GtkWidget *stats;
	GtkWidget *menubar;
	GtkWidget *vbox;
	GdkScreen *screen;
	GtkIconTheme *icon_theme=NULL;
	GtkSettings *settings;

	gtk_init(argcp, argvp);
	settings = gtk_settings_get_default();
	gtk_settings_set_long_property(settings, "gtk_tooltip_timeout", 10, "subsurface setting");

	g_type_init();

	subsurface_open_conf();
	if (subsurface_get_conf("feet", PREF_BOOL))
		output_units.length = FEET;
	if (subsurface_get_conf("psi", PREF_BOOL))
		output_units.pressure = PSI;
	if (subsurface_get_conf("cuft", PREF_BOOL))
		output_units.volume = CUFT;
	if (subsurface_get_conf("fahrenheit", PREF_BOOL))
		output_units.temperature = FAHRENHEIT;
	/* an unset key is FALSE - all these are hidden by default */
	visible_cols.cylinder = PTR_TO_BOOL(subsurface_get_conf("CYLINDER", PREF_BOOL));
	visible_cols.temperature = PTR_TO_BOOL(subsurface_get_conf("TEMPERATURE", PREF_BOOL));
	visible_cols.nitrox = PTR_TO_BOOL(subsurface_get_conf("NITROX", PREF_BOOL));
	visible_cols.otu = PTR_TO_BOOL(subsurface_get_conf("OTU", PREF_BOOL));
	visible_cols.sac = PTR_TO_BOOL(subsurface_get_conf("SAC", PREF_BOOL));

	divelist_font = subsurface_get_conf("divelist_font", PREF_STRING);

	if (!divelist_font)
		divelist_font = DIVELIST_DEFAULT_FONT;

	error_info_bar = NULL;
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_set_application_name ("subsurface");
	/* Let's check if the subsurface icon has been installed or if
	 * we need to try to load it from the current directory */
	screen = gdk_screen_get_default();
	if (screen)
		icon_theme = gtk_icon_theme_get_for_screen(screen);
	if (icon_theme) {
		if (gtk_icon_theme_has_icon(icon_theme, "subsurface")) {
			need_icon = FALSE;
			gtk_window_set_default_icon_name ("subsurface");
		}
	}
	if (need_icon)
#if defined __linux__ || defined __APPLE__
		gtk_window_set_icon_from_file(GTK_WINDOW(win), "subsurface.svg", NULL);
#elif defined WIN32
		gtk_window_set_icon_from_file(GTK_WINDOW(win), "subsurface.ico", NULL);
#endif
	g_signal_connect(G_OBJECT(win), "delete-event", G_CALLBACK(on_delete), NULL);
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(on_destroy), NULL);
	main_window = win;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	main_vbox = vbox;

	menubar = get_menubar_menu(win);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	vpane = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), vpane, TRUE, TRUE, 3);

	hpane = gtk_hpaned_new();
	gtk_paned_add1(GTK_PANED(vpane), hpane);

	/* Notebook for dive info vs profile vs .. */
	notebook = gtk_notebook_new();
	gtk_paned_add1(GTK_PANED(hpane), notebook);
	g_signal_connect(notebook, "switch-page", G_CALLBACK(switch_page), NULL);

	/* Create the actual divelist */
	dive_list = dive_list_create();
	gtk_widget_set_name(dive_list, "Dive List");
	gtk_paned_add2(GTK_PANED(vpane), dive_list);

	/* Frame for dive profile */
	dive_profile = dive_profile_widget();
	gtk_widget_set_name(dive_profile, "Dive Profile");
	gtk_paned_add2(GTK_PANED(hpane), dive_profile);

	/* Frame for extended dive info */
	dive_info = extended_dive_info_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), dive_info, gtk_label_new("Dive Notes"));

	/* Frame for dive equipment */
	equipment = equipment_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), equipment, gtk_label_new("Equipment"));

	/* Frame for dive statistics */
	stats = stats_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), stats, gtk_label_new("Info & Stats"));

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	return;
}

void run_ui(void)
{
	gtk_main();
}

typedef struct {
	cairo_rectangle_int_t rect;
	const char *text;
} tooltip_record_t;

static tooltip_record_t *tooltip_rects;
static int tooltips;

void attach_tooltip(int x, int y, int w, int h, const char *text)
{
	cairo_rectangle_int_t *rect;
	tooltip_rects = realloc(tooltip_rects, (tooltips + 1) * sizeof(tooltip_record_t));
	rect = &tooltip_rects[tooltips].rect;
	rect->x = x;
	rect->y = y;
	rect->width = w;
	rect->height = h;
	tooltip_rects[tooltips].text = text;
	tooltips++;
}

#define INSIDE_RECT(_r,_x,_y)	((_r.x <= _x) && (_r.x + _r.width >= _x) && \
				(_r.y <= _y) && (_r.y + _r.height >= _y))

static gboolean profile_tooltip (GtkWidget *widget, gint x, gint y,
			gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
	int i;
	cairo_rectangle_int_t *drawing_area = user_data;
	gint tx = x - drawing_area->x; /* get transformed coordinates */
	gint ty = y - drawing_area->y;

	/* are we over an event marker ? */
	for (i = 0; i < tooltips; i++) {
		if (INSIDE_RECT(tooltip_rects[i].rect, tx, ty)) {
			gtk_tooltip_set_text(tooltip,tooltip_rects[i].text);
			return TRUE; /* show tooltip */
		}
	}
	return FALSE; /* don't show tooltip */
}

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = current_dive;
	struct graphics_context gc = { .printer = 0 };
	static cairo_rectangle_int_t drawing_area;

	/* the drawing area gives TOTAL width * height - x,y is used as the topx/topy offset
	 * so effective drawing area is width-2x * height-2y */
	drawing_area.width = widget->allocation.width;
	drawing_area.height = widget->allocation.height;
	drawing_area.x = drawing_area.width / 20.0;
	drawing_area.y = drawing_area.height / 20.0;

	gc.cr = gdk_cairo_create(widget->window);
	g_object_set(widget, "has-tooltip", TRUE, NULL);
	g_signal_connect(widget, "query-tooltip", G_CALLBACK(profile_tooltip), &drawing_area);
	init_profile_background(&gc);
	cairo_paint(gc.cr);

	if (dive) {
		if (tooltip_rects) {
			free(tooltip_rects);
			tooltip_rects = NULL;
		}
		tooltips = 0;
		plot(&gc, &drawing_area, dive);
	}
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

static GtkComboBox *dive_computer_selector(GtkWidget *vbox)
{
	GtkWidget *hbox, *combo_box, *frame;
	GtkListStore *model;
	GtkCellRenderer *renderer;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	fill_computer_list(model);

	frame = gtk_frame_new("Dive computer");
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer, "text", 0, NULL);

	return GTK_COMBO_BOX(combo_box);
}

static GtkEntry *dive_computer_device(GtkWidget *vbox)
{
	GtkWidget *hbox, *entry, *frame;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	frame = gtk_frame_new("Device name");
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame), entry);
	gtk_entry_set_text(GTK_ENTRY(entry), "/dev/ttyUSB0");

	return GTK_ENTRY(entry);
}

/* once a file is selected in the FileChooserButton we want to exit the import dialog */
static void on_file_set(GtkFileChooserButton *widget, gpointer _data)
{
	GtkDialog *main_dialog = _data;

	gtk_dialog_response(main_dialog, GTK_RESPONSE_ACCEPT);
}

static GtkWidget *xml_file_selector(GtkWidget *vbox, GtkWidget *main_dialog)
{
	GtkWidget *hbox, *frame, *chooser, *dialog;
	GtkFileFilter *filter;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	frame = gtk_frame_new("XML file name");
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);
	dialog = gtk_file_chooser_dialog_new("Open XML File",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.xml");
	gtk_file_filter_add_pattern(filter, "*.XML");
	gtk_file_filter_add_pattern(filter, "*.sda");
	gtk_file_filter_add_pattern(filter, "*.SDA");
	gtk_file_filter_add_mime_type(filter, "text/xml");
	gtk_file_filter_set_name(filter, "XML file");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	chooser = gtk_file_chooser_button_new_with_dialog(dialog);
	g_signal_connect(G_OBJECT(chooser), "file-set", G_CALLBACK(on_file_set), main_dialog);

	gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(chooser), 30);
	gtk_container_add(GTK_CONTAINER(frame), chooser);

	return chooser;
}

static void do_import_file(gpointer data, gpointer user_data)
{
	GError *error = NULL;
	parse_xml_file(data, &error);

	if (error != NULL)
	{
		report_error(error);
		g_error_free(error);
		error = NULL;
	}
}

void import_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *hbox, *vbox, *label;
	GtkComboBox *computer;
	GtkEntry *device;
	GtkWidget *XMLchooser;
	device_data_t devicedata = {
		.devname = NULL,
	};

	dialog = gtk_dialog_new_with_buttons("Import from dive computer",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	label = gtk_label_new("Import: \nLoad XML file or import directly from dive computer");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 3);
	XMLchooser = xml_file_selector(vbox, dialog);
	computer = dive_computer_selector(vbox);
	device = dive_computer_device(vbox);
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);
	devicedata.progress.bar = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(hbox), devicedata.progress.bar);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		int type;
		GtkTreeIter iter;
		GtkTreeModel *model;
		const char *comp;
		GSList *list;
	case GTK_RESPONSE_ACCEPT:
		/* what happened - did the user pick a file? In that case
		 * we ignore whether a dive computer model was picked */
		list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(XMLchooser));
		if (g_slist_length(list) == 0) {
			if (!gtk_combo_box_get_active_iter(computer, &iter))
				break;
			model = gtk_combo_box_get_model(computer);
			gtk_tree_model_get(model, &iter,
					0, &comp,
					1, &type,
					-1);
			devicedata.type = type;
			devicedata.name = comp;
			devicedata.devname = gtk_entry_get_text(device);
			do_import(&devicedata);
		} else {
			g_slist_foreach(list,do_import_file,NULL);
			g_slist_free(list);
		}
		break;
	default:
		break;
	}
	gtk_widget_destroy(dialog);

	report_dives(TRUE);
}

void update_progressbar(progressbar_t *progress, double value)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->bar), value);
}


void set_filename(const char *filename)
{
	if (!existing_filename && filename)
		existing_filename = strdup(filename);
	return;
}
