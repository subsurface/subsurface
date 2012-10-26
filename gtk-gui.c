/* gtk-gui.c */
/* gtk UI implementation */
/* creates the window and overall layout
 * divelist, dive info, equipment and printing are handled in their own source files
 */
#include <libintl.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"
#include "uemis.h"

#include "libdivecomputer.h"

GtkWidget *main_window;
GtkWidget *main_vbox;
GtkWidget *error_info_bar;
GtkWidget *error_label;
GtkWidget *vpane, *hpane;
GtkWidget *notebook;

int        error_count;
const char *existing_filename;
const char *divelist_font;
const char *default_filename;

struct units output_units;

static GtkWidget *dive_profile;

visible_cols_t visible_cols = {TRUE, FALSE};

static const char *default_dive_computer_vendor;
static const char *default_dive_computer_product;
static const char *default_dive_computer_device;
static char *uemis_max_dive_data;

static int is_default_dive_computer(const char *vendor, const char *product)
{
	return default_dive_computer_vendor && !strcmp(vendor, default_dive_computer_vendor) &&
		default_dive_computer_product && !strcmp(product, default_dive_computer_product);
}

int is_default_dive_computer_device(const char *name)
{
	return default_dive_computer_device && !strcmp(name, default_dive_computer_device);
}

static void set_default_dive_computer(const char *vendor, const char *product)
{
	if (!vendor || !*vendor)
		return;
	if (!product || !*product)
		return;
	if (is_default_dive_computer(vendor, product))
		return;
	if (default_dive_computer_vendor)
		free((void *)default_dive_computer_vendor);
	if (default_dive_computer_product)
		free((void *)default_dive_computer_product);
	default_dive_computer_vendor = vendor;
	default_dive_computer_product = product;
	subsurface_set_conf("dive_computer_vendor", PREF_STRING, vendor);
	subsurface_set_conf("dive_computer_product", PREF_STRING, product);
}

static void set_default_dive_computer_device(const char *name)
{
	if (!name || !*name)
		return;
	if (is_default_dive_computer_device(name))
		return;
	if (default_dive_computer_device)
		free((void *)default_dive_computer_device);
	default_dive_computer_device = strdup(name);
	subsurface_set_conf("dive_computer_device", PREF_STRING, name);
}

static void set_uemis_last_dive(char *data)
{
	uemis_max_dive_data = data;
	subsurface_set_conf("uemis_max_dive_data", PREF_STRING, data);
}

void repaint_dive(void)
{
	update_dive(current_dive);
	if (dive_profile)
		gtk_widget_queue_draw(dive_profile);
}

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
		snprintf(buffer, sizeof(buffer), _("Failed to open %i files."), error_count);
		gtk_label_set(GTK_LABEL(error_label), buffer);
	}
}

static GtkFileFilter *setup_filter(void)
{
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.xml");
	gtk_file_filter_add_pattern(filter, "*.XML");
	gtk_file_filter_add_pattern(filter, "*.sda");
	gtk_file_filter_add_pattern(filter, "*.SDA");
	gtk_file_filter_add_mime_type(filter, "text/xml");
	gtk_file_filter_set_name(filter, _("XML file"));

	return filter;
}

static void file_save_as(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	char *filename = NULL;
	char *current_file;
	char *current_dir;

	dialog = gtk_file_chooser_dialog_new(_("Save File As"),
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	if (existing_filename) {
		current_dir = g_path_get_dirname(existing_filename);
		current_file = g_path_get_basename(existing_filename);
	} else {
		const char *current_default = subsurface_default_filename();
		current_dir = g_path_get_dirname(current_default);
		current_file = g_path_get_basename(current_default);
	}
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_dir);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), current_file);

	free(current_dir);
	free(current_file);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	if (filename){
		save_dives(filename);
		set_filename(filename, TRUE);
		g_free(filename);
		mark_divelist_changed(FALSE);
	}
}

static void file_save(GtkWidget *w, gpointer data)
{
	const char *current_default;

	if (!existing_filename)
		return file_save_as(w, data);

	current_default = subsurface_default_filename();
	if (strcmp(existing_filename, current_default) ==  0) {
		/* if we are using the default filename the directory
		 * that we are creating the file in may not exist */
		char *current_def_dir;
		struct stat sb;

		current_def_dir = g_path_get_dirname(existing_filename);
		if (stat(current_def_dir, &sb) != 0) {
			g_mkdir(current_def_dir, S_IRWXU);
		}
		free(current_def_dir);
	}
	save_dives(existing_filename);
	mark_divelist_changed(FALSE);
}

static gboolean ask_save_changes()
{
	GtkWidget *dialog, *label, *content;
	gboolean quit = TRUE;
	dialog = gtk_dialog_new_with_buttons(_("Save Changes?"),
		GTK_WINDOW(main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_NO, GTK_RESPONSE_NO,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	if (!existing_filename){
		label = gtk_label_new (
			_("You have unsaved changes\nWould you like to save those before closing the datafile?"));
	} else {
		char *label_text = (char*) malloc(sizeof(char) *
			(strlen(_("You have unsaved changes to file: %s \nWould you like to save those before closing the datafile?")) +
			 strlen(existing_filename)));
		sprintf(label_text,
			_("You have unsaved changes to file: %s \nWould you like to save those before closing the datafile?"),
			existing_filename);
		label = gtk_label_new (label_text);
		free(label_text);
	}
	gtk_container_add (GTK_CONTAINER (content), label);
	gtk_widget_show_all (dialog);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gint outcode = gtk_dialog_run(GTK_DIALOG(dialog));
	if (outcode == GTK_RESPONSE_ACCEPT) {
		file_save(NULL,NULL);
	} else if (outcode == GTK_RESPONSE_CANCEL) {
		quit = FALSE;
	}
	gtk_widget_destroy(dialog);
	return quit;
}

static void dive_trip_unref(gpointer data, gpointer user_data)
{
	dive_trip_t *dive_trip = (dive_trip_t *)data;
	if (dive_trip->location)
		free(dive_trip->location);
	free(data);
}

static void file_close(GtkWidget *w, gpointer data)
{
	int i;

	if (unsaved_changes())
		if (ask_save_changes() == FALSE)
			return;

	if (existing_filename)
		free((void *)existing_filename);
	existing_filename = NULL;

	/* free the dives and trips */
	for (i = 0; i < dive_table.nr; i++)
		free(get_dive(i));
	dive_table.nr = 0;
	dive_table.preexisting = 0;
	mark_divelist_changed(FALSE);

	/* inlined version of g_list_free_full(dive_trip_list, free); */
	g_list_foreach(dive_trip_list, (GFunc)dive_trip_unref, NULL);
	g_list_free(dive_trip_list);

	dive_trip_list = NULL;

	/* clear the selection and the statistics */
	amount_selected = 0;
	selected_dive = 0;
	process_selected_dives();
	clear_stats_widgets();

	/* clear the equipment page */
	clear_equipment_widgets();

	/* redraw the screen */
	dive_list_update_dives();
	show_dive_info(NULL);
}

static void file_open(GtkWidget *w, gpointer data)
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	const char *current_default;

	dialog = gtk_file_chooser_dialog_new(_("Open File"),
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	current_default = subsurface_default_filename();
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), current_default);
	/* when opening the data file we should allow only one file to be chosen */
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), FALSE);

	filter = setup_filter();
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *fn_glist;
		char *filename;

		/* first, close the existing file, if any, and forget its name */
		file_close(w, data);
		free((void *)existing_filename);
		existing_filename = NULL;

		/* we know there is only one filename */
		fn_glist = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

		GError *error = NULL;
		filename = fn_glist->data;
		parse_file(filename, &error, TRUE);
		if (error != NULL)
		{
			report_error(error);
			g_error_free(error);
			error = NULL;
		}
		g_free(filename);
		g_slist_free(fn_glist);
		report_dives(FALSE);
	}
	gtk_widget_destroy(dialog);
}

static gboolean on_delete(GtkWidget* w, gpointer data)
{
	/* Make sure to flush any modified dive data */
	update_dive(NULL);

	gboolean quit = TRUE;
	if (unsaved_changes())
		quit = ask_save_changes();

	if (quit){
		return FALSE; /* go ahead, kill the program, we're good now */
	} else {
		return TRUE; /* We are not leaving */
	}
}

static void on_destroy(GtkWidget* w, gpointer data)
{
	dive_list_destroy();
	gtk_main_quit();
}

void quit(GtkWidget *w, gpointer data)
{
	/* Make sure to flush any modified dive data */
	update_dive(NULL);

	gboolean quit = TRUE;
	if (unsaved_changes())
		quit = ask_save_changes();

	if (quit){
		dive_list_destroy();
		gtk_main_quit();
	}
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

static void create_radio(GtkWidget *vbox, const char *w_name, ...)
{
	va_list args;
	GtkRadioButton *group = NULL;
	GtkWidget *box, *label;

	box = gtk_hbox_new(TRUE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

	label = gtk_label_new(w_name);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

	va_start(args, w_name);
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
UNITCALLBACK(set_kg, weight, KG)
UNITCALLBACK(set_lbs, weight, LBS)

#define OPTIONCALLBACK(name, option) \
static void name(GtkWidget *w, gpointer data) \
{ \
	option = GTK_TOGGLE_BUTTON(w)->active; \
}

OPTIONCALLBACK(otu_toggle, visible_cols.otu)
OPTIONCALLBACK(sac_toggle, visible_cols.sac)
OPTIONCALLBACK(nitrox_toggle, visible_cols.nitrox)
OPTIONCALLBACK(temperature_toggle, visible_cols.temperature)
OPTIONCALLBACK(totalweight_toggle, visible_cols.totalweight)
OPTIONCALLBACK(suit_toggle, visible_cols.suit)
OPTIONCALLBACK(cylinder_toggle, visible_cols.cylinder)
OPTIONCALLBACK(autogroup_toggle, autogroup)

static void event_toggle(GtkWidget *w, gpointer _data)
{
	gboolean *plot_ev = _data;

	*plot_ev = GTK_TOGGLE_BUTTON(w)->active;
}

static void pick_default_file(GtkWidget *w, GtkButton *button)
{
	GtkWidget *fs_dialog, *parent;
	const char *current_default;
	char *current_def_file, *current_def_dir;
	GtkFileFilter *filter;
	struct stat sb;

	fs_dialog = gtk_file_chooser_dialog_new(_("Choose Default XML File"),
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		NULL);
	parent = gtk_widget_get_ancestor(w, GTK_TYPE_DIALOG);
	gtk_widget_set_sensitive(parent, FALSE);
	gtk_window_set_transient_for(GTK_WINDOW(fs_dialog), GTK_WINDOW(parent));

	current_default = subsurface_default_filename();
	current_def_dir = g_path_get_dirname(current_default);
	current_def_file = g_path_get_basename(current_default);

	/* it's possible that the directory doesn't exist (especially for the default)
	 * For gtk's file select box to make sense we create it */
	if (stat(current_def_dir, &sb) != 0)
		g_mkdir(current_def_dir, S_IRWXU);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fs_dialog), current_def_dir);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fs_dialog), current_def_file);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(fs_dialog), FALSE);
	filter = setup_filter();
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fs_dialog), filter);
	gtk_widget_show_all(fs_dialog);
	if (gtk_dialog_run(GTK_DIALOG(fs_dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *list;

		list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(fs_dialog));
		if (g_slist_length(list) == 1)
			gtk_button_set_label(button, list->data);
		g_slist_free(list);
	}

	free(current_def_dir);
	free(current_def_file);
	free((void *)current_default);
	gtk_widget_destroy(fs_dialog);

	gtk_widget_set_sensitive(parent, TRUE);
}

static void preferences_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *font, *frame, *box, *vbox, *button;
	const char *current_default, *new_default;

	menu_units = output_units;

	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	frame = gtk_frame_new(_("Units"));
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	create_radio(box, _("Depth:"),
		_("Meter"), set_meter, (output_units.length == METERS),
		_("Feet"),  set_feet, (output_units.length == FEET),
		NULL);

	create_radio(box, _("Pressure:"),
		_("Bar"), set_bar, (output_units.pressure == BAR),
		_("PSI"),  set_psi, (output_units.pressure == PSI),
		NULL);

	create_radio(box, _("Volume:"),
		_("Liter"),  set_liter, (output_units.volume == LITER),
		_("CuFt"), set_cuft, (output_units.volume == CUFT),
		NULL);

	create_radio(box, _("Temperature:"),
		_("Celsius"), set_celsius, (output_units.temperature == CELSIUS),
		_("Fahrenheit"),  set_fahrenheit, (output_units.temperature == FAHRENHEIT),
		NULL);

	create_radio(box, _("Weight:"),
		_("kg"), set_kg, (output_units.weight == KG),
		_("lbs"),  set_lbs, (output_units.weight == LBS),
		NULL);

	frame = gtk_frame_new(_("Show Columns"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	button = gtk_check_button_new_with_label(_("Temp"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.temperature);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(temperature_toggle), NULL);

	button = gtk_check_button_new_with_label(_("Cyl"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.cylinder);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(cylinder_toggle), NULL);

	button = gtk_check_button_new_with_label("O" UTF8_SUBSCRIPT_2 "%");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.nitrox);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(nitrox_toggle), NULL);

	button = gtk_check_button_new_with_label(_("SAC"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.sac);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(sac_toggle), NULL);

	button = gtk_check_button_new_with_label(_("OTU"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.otu);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(otu_toggle), NULL);

	button = gtk_check_button_new_with_label(_("Weight"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.totalweight);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(totalweight_toggle), NULL);

	button = gtk_check_button_new_with_label(_("Suit"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), visible_cols.suit);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(suit_toggle), NULL);

	frame = gtk_frame_new(_("Divelist Font"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, FALSE, FALSE, 5);

	font = gtk_font_button_new_with_font(divelist_font);
	gtk_container_add(GTK_CONTAINER(frame),font);

	frame = gtk_frame_new(_("Misc. Options"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	button = gtk_check_button_new_with_label(_("Automatically group dives in trips"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), autogroup);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(autogroup_toggle), NULL);

	frame = gtk_frame_new(_("Default XML Data File"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);
	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);
	current_default = subsurface_default_filename();
	button = gtk_button_new_with_label(current_default);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pick_default_file), button);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		/* Make sure to flush any modified old dive data with old units */
		update_dive(NULL);

		if (divelist_font)
			free((void *)divelist_font);
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
		subsurface_set_conf("lbs", PREF_BOOL, BOOL_TO_PTR(output_units.weight == LBS));
		subsurface_set_conf("TEMPERATURE", PREF_BOOL, BOOL_TO_PTR(visible_cols.temperature));
		subsurface_set_conf("TOTALWEIGHT", PREF_BOOL, BOOL_TO_PTR(visible_cols.totalweight));
		subsurface_set_conf("SUIT", PREF_BOOL, BOOL_TO_PTR(visible_cols.suit));
		subsurface_set_conf("CYLINDER", PREF_BOOL, BOOL_TO_PTR(visible_cols.cylinder));
		subsurface_set_conf("NITROX", PREF_BOOL, BOOL_TO_PTR(visible_cols.nitrox));
		subsurface_set_conf("SAC", PREF_BOOL, BOOL_TO_PTR(visible_cols.sac));
		subsurface_set_conf("OTU", PREF_BOOL, BOOL_TO_PTR(visible_cols.otu));
		subsurface_set_conf("divelist_font", PREF_STRING, divelist_font);
		subsurface_set_conf("autogroup", PREF_BOOL, BOOL_TO_PTR(autogroup));
		new_default = strdup(gtk_button_get_label(GTK_BUTTON(button)));

		/* if we opened the default file and are changing its name,
		 * update existing_filename */
		if (existing_filename) {
			if (strcmp(current_default, existing_filename) == 0) {
				free((void *)existing_filename);
				existing_filename = strdup(new_default);
			}
		}

		if (strcmp(current_default, new_default)) {
			subsurface_set_conf("default_filename", PREF_STRING, new_default);
			free((void *)default_filename);
			default_filename = new_default;
		}

		/* Flush the changes out to the system */
		subsurface_flush_conf();
	}
	free((void *)current_default);
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

	dialog = gtk_dialog_new_with_buttons(_("Select Events"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);
	/* initialize the function that fills the table */
	create_toggle(NULL, NULL, NULL);

	frame = gtk_frame_new(_("Enable / Disable Events"));
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

static void autogroup_cb(GtkWidget *w, gpointer data)
{
	autogroup = !autogroup;
	if (! autogroup)
		remove_autogen_trips();
	dive_list_update_dives();
}

static void renumber_dialog(GtkWidget *w, gpointer data)
{
	int result;
	struct dive *dive;
	GtkWidget *dialog, *frame, *button, *vbox;

	dialog = gtk_dialog_new_with_buttons(_("Renumber"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	frame = gtk_frame_new(_("New starting number"));
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
		GtkWidget *image = gtk_image_new_from_file(subsurface_icon_name());

		if (gtk_image_get_storage_type(GTK_IMAGE(image)) == GTK_IMAGE_PIXBUF) {
			logo = gtk_image_get_pixbuf(GTK_IMAGE(image));
			logo_property = "logo";
		}
	}
	gtk_show_about_dialog(NULL,
		"title", _("About Subsurface"),
		"program-name", "Subsurface",
		"comments", _("Multi-platform divelog software in C"),
		"website", "http://subsurface.hohndel.org",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION_STRING,
		"copyright", _("Linus Torvalds, Dirk Hohndel, and others, 2011, 2012"),
		/*++GETTEXT the term translator-credits is magic - list the names of the tranlators here */
		"translator_credits", _("translator-credits"),
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

static void view_three(GtkWidget *w, gpointer data)
{
	GtkAllocation alloc;
	GtkRequisition requisition;

	gtk_widget_get_allocation(hpane, &alloc);
	gtk_paned_set_position(GTK_PANED(hpane), alloc.width/2);
	gtk_widget_get_allocation(vpane, &alloc);
	gtk_widget_size_request(notebook, &requisition);
	/* pick the requested size for the notebook plus 6 pixels for frame */
	gtk_paned_set_position(GTK_PANED(vpane), requisition.height + 6);
}

static void toggle_zoom(GtkWidget *w, gpointer data)
{
	zoomed_plot = (zoomed_plot)?0 : 1;
	/*Update dive*/
	repaint_dive();
}

static GtkActionEntry menu_items[] = {
	{ "FileMenuAction", NULL, N_("File"), NULL, NULL, NULL},
	{ "LogMenuAction",  NULL, N_("Log"), NULL, NULL, NULL},
	{ "ViewMenuAction",  NULL, N_("View"), NULL, NULL, NULL},
	{ "FilterMenuAction",  NULL, N_("Filter"), NULL, NULL, NULL},
	{ "HelpMenuAction", NULL, N_("Help"), NULL, NULL, NULL},
	{ "NewFile",        GTK_STOCK_NEW, N_("New"),   CTRLCHAR "N", NULL, G_CALLBACK(file_close) },
	{ "OpenFile",       GTK_STOCK_OPEN, N_("Open..."),   CTRLCHAR "O", NULL, G_CALLBACK(file_open) },
	{ "SaveFile",       GTK_STOCK_SAVE, N_("Save..."),   CTRLCHAR "S", NULL, G_CALLBACK(file_save) },
	{ "SaveAsFile",     GTK_STOCK_SAVE_AS, N_("Save As..."),   SHIFTCHAR CTRLCHAR "S", NULL, G_CALLBACK(file_save_as) },
	{ "CloseFile",      GTK_STOCK_CLOSE, N_("Close"), NULL, NULL, G_CALLBACK(file_close) },
	{ "Print",          GTK_STOCK_PRINT, N_("Print..."),  CTRLCHAR "P", NULL, G_CALLBACK(do_print) },
	{ "ImportFile",     GTK_STOCK_GO_BACK, N_("Import XML File(s)..."), CTRLCHAR "I", NULL, G_CALLBACK(import_files) },
	{ "DownloadLog",    GTK_STOCK_GO_DOWN, N_("Download From Dive Computer..."), CTRLCHAR "D", NULL, G_CALLBACK(download_dialog) },
	{ "AddDive",        GTK_STOCK_ADD, N_("Add Dive..."), NULL, NULL, G_CALLBACK(add_dive_cb) },
	{ "Preferences",    GTK_STOCK_PREFERENCES, N_("Preferences..."), PREFERENCE_ACCEL, NULL, G_CALLBACK(preferences_dialog) },
	{ "Renumber",       NULL, N_("Renumber..."), NULL, NULL, G_CALLBACK(renumber_dialog) },
	{ "YearlyStats",    NULL, N_("Yearly Statistics"), NULL, NULL, G_CALLBACK(show_yearly_stats) },
	{ "SelectEvents",   NULL, N_("Select Events..."), NULL, NULL, G_CALLBACK(selectevents_dialog) },
	{ "Quit",           GTK_STOCK_QUIT, N_("Quit"),   CTRLCHAR "Q", NULL, G_CALLBACK(quit) },
	{ "About",          GTK_STOCK_ABOUT, N_("About Subsurface"),  NULL, NULL, G_CALLBACK(about_dialog) },
	{ "ViewList",       NULL, N_("List"),  CTRLCHAR "1", NULL, G_CALLBACK(view_list) },
	{ "ViewProfile",    NULL, N_("Profile"), CTRLCHAR "2", NULL, G_CALLBACK(view_profile) },
	{ "ViewInfo",       NULL, N_("Info"), CTRLCHAR "3", NULL, G_CALLBACK(view_info) },
	{ "ViewThree",      NULL, N_("Three"), CTRLCHAR "4", NULL, G_CALLBACK(view_three) },
};
static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static GtkToggleActionEntry toggle_items[] = {
	{ "Autogroup",      NULL, N_("Autogroup"), NULL, NULL, G_CALLBACK(autogroup_cb), FALSE },
	{ "ToggleZoom",     NULL, N_("Toggle Zoom"), CTRLCHAR "0", NULL, G_CALLBACK(toggle_zoom), FALSE },
};
static gint ntoggle_items = sizeof (toggle_items) / sizeof (toggle_items[0]);

static const gchar* ui_string = " \
	<ui> \
		<menubar name=\"MainMenu\"> \
			<menu name=\"FileMenu\" action=\"FileMenuAction\"> \
				<menuitem name=\"New\" action=\"NewFile\" /> \
				<menuitem name=\"Open\" action=\"OpenFile\" /> \
				<menuitem name=\"Save\" action=\"SaveFile\" /> \
				<menuitem name=\"Save As\" action=\"SaveAsFile\" /> \
				<menuitem name=\"Close\" action=\"CloseFile\" /> \
				<separator name=\"Separator1\"/> \
				<menuitem name=\"Import XML File\" action=\"ImportFile\" /> \
				<separator name=\"Separator2\"/> \
				<menuitem name=\"Print\" action=\"Print\" /> \
				<separator name=\"Separator3\"/> \
				<menuitem name=\"Preferences\" action=\"Preferences\" /> \
				<separator name=\"Separator4\"/> \
				<menuitem name=\"Quit\" action=\"Quit\" /> \
			</menu> \
			<menu name=\"LogMenu\" action=\"LogMenuAction\"> \
				<menuitem name=\"Download From Dive Computer\" action=\"DownloadLog\" /> \
				<separator name=\"Separator1\"/> \
				<menuitem name=\"Add Dive\" action=\"AddDive\" /> \
				<separator name=\"Separator2\"/> \
				<menuitem name=\"Renumber\" action=\"Renumber\" /> \
				<menuitem name=\"Autogroup\" action=\"Autogroup\" /> \
				<menuitem name=\"Toggle Zoom\" action=\"ToggleZoom\" /> \
				<menuitem name=\"YearlyStats\" action=\"YearlyStats\" /> \
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

static GtkWidget *get_menubar_menu(GtkWidget *window, GtkUIManager *ui_manager)
{
	GtkActionGroup *action_group = gtk_action_group_new("Menu");
	gtk_action_group_set_translation_domain(action_group, "subsurface");
	gtk_action_group_add_actions(action_group, menu_items, nmenu_items, 0);
	toggle_items[0].is_active = autogroup;
	gtk_action_group_add_toggle_actions(action_group, toggle_items, ntoggle_items, 0);

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
	GtkWidget *nb_page;
	GtkWidget *dive_list;
	GtkWidget *menubar;
	GtkWidget *vbox;
	GtkWidget *scrolled;
	GdkScreen *screen;
	GtkIconTheme *icon_theme=NULL;
	GtkSettings *settings;
	GtkUIManager *ui_manager;
	const char *conf_value;

	gtk_init(argcp, argvp);
	settings = gtk_settings_get_default();
	gtk_settings_set_long_property(settings, "gtk-tooltip-timeout", 10, "subsurface setting");
	gtk_settings_set_long_property(settings, "gtk-menu-images", 1, "subsurface setting");
	gtk_settings_set_long_property(settings, "gtk-button-images", 1, "subsurface setting");

	/* check if utf8 stars are available as a default OS feature */
	if (!subsurface_os_feature_available(UTF8_FONT_WITH_STARS)) {
		star_strings[0] = "     ";
		star_strings[1] = "*    ";
		star_strings[2] = "**   ";
		star_strings[3] = "***  ";
		star_strings[4] = "**** ";
		star_strings[5] = "*****";
	}
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
	if (subsurface_get_conf("lbs", PREF_BOOL))
		output_units.weight = LBS;
	/* an unset key is FALSE - all these are hidden by default */
	visible_cols.cylinder = PTR_TO_BOOL(subsurface_get_conf("CYLINDER", PREF_BOOL));
	visible_cols.temperature = PTR_TO_BOOL(subsurface_get_conf("TEMPERATURE", PREF_BOOL));
	visible_cols.totalweight = PTR_TO_BOOL(subsurface_get_conf("TOTALWEIGHT", PREF_BOOL));
	visible_cols.suit = PTR_TO_BOOL(subsurface_get_conf("SUIT", PREF_BOOL));
	visible_cols.nitrox = PTR_TO_BOOL(subsurface_get_conf("NITROX", PREF_BOOL));
	visible_cols.otu = PTR_TO_BOOL(subsurface_get_conf("OTU", PREF_BOOL));
	visible_cols.sac = PTR_TO_BOOL(subsurface_get_conf("SAC", PREF_BOOL));

	divelist_font = subsurface_get_conf("divelist_font", PREF_STRING);
	autogroup = PTR_TO_BOOL(subsurface_get_conf("autogroup", PREF_BOOL));
	default_filename = subsurface_get_conf("default_filename", PREF_STRING);

	default_dive_computer_vendor = subsurface_get_conf("dive_computer_vendor", PREF_STRING);
	default_dive_computer_product = subsurface_get_conf("dive_computer_product", PREF_STRING);
	default_dive_computer_device = subsurface_get_conf("dive_computer_device", PREF_STRING);
	conf_value = subsurface_get_conf("uemis_max_dive_data", PREF_STRING);
	if (!conf_value)
		uemis_max_dive_data = strdup("");
	else
		uemis_max_dive_data = strdup(conf_value);
	free((char *)conf_value);
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
	if (need_icon) {
		const char *icon_name = subsurface_icon_name();
		if (!access(icon_name, R_OK))
			gtk_window_set_icon_from_file(GTK_WINDOW(win), icon_name, NULL);
	}
	g_signal_connect(G_OBJECT(win), "delete-event", G_CALLBACK(on_delete), NULL);
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(on_destroy), NULL);
	main_window = win;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	main_vbox = vbox;

	ui_manager = gtk_ui_manager_new();
	menubar = get_menubar_menu(win, ui_manager);

	subsurface_ui_setup(settings, menubar, vbox, ui_manager);

	vpane = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(vbox), vpane, TRUE, TRUE, 3);
	hpane = gtk_hpaned_new();
	gtk_paned_add1(GTK_PANED(vpane), hpane);
	g_signal_connect_after(G_OBJECT(vbox), "realize", G_CALLBACK(view_three), NULL);

	/* Notebook for dive info vs profile vs .. */
	notebook = gtk_notebook_new();
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_paned_add1(GTK_PANED(hpane), scrolled);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), notebook);
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
	nb_page = extended_dive_info_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nb_page, gtk_label_new(_("Dive Notes")));

	/* Frame for dive equipment */
	nb_page = equipment_widget(W_IDX_PRIMARY);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nb_page, gtk_label_new(_("Equipment")));

	/* Frame for single dive statistics */
	nb_page = single_stats_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nb_page, gtk_label_new(_("Dive Info")));

	/* Frame for total dive statistics */
	nb_page = total_stats_widget();
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nb_page, gtk_label_new(_("Stats")));

	gtk_widget_set_app_paintable(win, TRUE);
	gtk_widget_show_all(win);

	return;
}

void run_ui(void)
{
	gtk_main();
}

void exit_ui(void)
{
	subsurface_close_conf();
	if (default_filename)
		free((char *)default_filename);
	if (existing_filename)
		free((void *)existing_filename);
	if (default_dive_computer_device)
		free((void *)default_dive_computer_device);
}

typedef struct {
	cairo_rectangle_t rect;
	const char *text;
} tooltip_record_t;

static tooltip_record_t *tooltip_rects;
static int tooltips;

void attach_tooltip(int x, int y, int w, int h, const char *text)
{
	cairo_rectangle_t *rect;
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
	cairo_rectangle_t *drawing_area = user_data;
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
	static cairo_rectangle_t drawing_area;

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
		plot(&gc, &drawing_area, dive, SC_SCREEN);
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
	return ret;
}

struct mydescriptor {
	const char *vendor;
	const char *product;
	dc_family_t type;
	unsigned int model;
};

static int fill_computer_list(GtkListStore *store)
{
	int index = -1, i;
	GtkTreeIter iter;
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;

	i = 0;
	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next (iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, descriptor,
			-1);
		if (is_default_dive_computer(vendor, product))
			index = i;
		i++;
	}
	dc_iterator_free(iterator);
	/* and add the Uemis Zurich which we are handling internally
	   THIS IS A HACK as we use the internal of libdivecomputer
	   data structures... eventually the UEMIS code needs to move
	   into libdivecomputer, I guess */
	mydescriptor = malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
			0, mydescriptor, -1);
	if (is_default_dive_computer("Uemis", "Zurich"))
		index = i;

	return index;
}

void render_dive_computer(GtkCellLayout *cell,
		GtkCellRenderer *renderer,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
	char buffer[40];
	dc_descriptor_t *descriptor = NULL;
	const char *vendor, *product;

	gtk_tree_model_get(model, iter, 0, &descriptor, -1);
	vendor = dc_descriptor_get_vendor(descriptor);
	product = dc_descriptor_get_product(descriptor);
	snprintf(buffer, sizeof(buffer), "%s %s", vendor, product);
	g_object_set(renderer, "text", buffer, NULL);
}

static void dive_computer_selector_changed(GtkWidget *combo, gpointer data)
{
	GtkWidget *import, *button;

	import = gtk_widget_get_ancestor(combo, GTK_TYPE_DIALOG);
	button = gtk_dialog_get_widget_for_response(GTK_DIALOG(import), GTK_RESPONSE_ACCEPT);
	gtk_widget_set_sensitive(button, TRUE);
}

static GtkComboBox *dive_computer_selector(GtkWidget *vbox)
{
	GtkWidget *hbox, *combo_box, *frame;
	GtkListStore *model;
	GtkCellRenderer *renderer;
	int default_index;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	model = gtk_list_store_new(1, G_TYPE_POINTER);
	default_index = fill_computer_list(model);

	frame = gtk_frame_new(_("Dive computer"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(model));
	g_signal_connect(G_OBJECT(combo_box), "changed", G_CALLBACK(dive_computer_selector_changed), NULL);
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combo_box), renderer, render_dive_computer, NULL, NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), default_index);

	return GTK_COMBO_BOX(combo_box);
}

static GtkComboBox *dc_device_selector(GtkWidget *vbox)
{
	GtkWidget *hbox, *combo_box, *frame;
	GtkListStore *model;
	GtkCellRenderer *renderer;
	int default_index;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	model = gtk_list_store_new(1, G_TYPE_STRING);
	default_index = subsurface_fill_device_list(model);

	frame = gtk_frame_new(_("Device or mount point"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	combo_box = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(model), 0);
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, TRUE);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), default_index);

	return GTK_COMBO_BOX(combo_box);
}

static void do_import_file(gpointer data, gpointer user_data)
{
	GError *error = NULL;
	parse_file(data, &error, FALSE);

	if (error != NULL)
	{
		report_error(error);
		g_error_free(error);
		error = NULL;
	}
}

void import_files(GtkWidget *w, gpointer data)
{
	GtkWidget *fs_dialog;
	const char *current_default;
	char *current_def_dir;
	GtkFileFilter *filter;
	struct stat sb;
	GSList *filenames = NULL;

	fs_dialog = gtk_file_chooser_dialog_new(_("Choose XML Files To Import Into Current Data File"),
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	/* I'm not sure what the best default path should be... */
	if (existing_filename) {
		current_def_dir = g_path_get_dirname(existing_filename);
	} else {
		current_default = subsurface_default_filename();
		current_def_dir = g_path_get_dirname(current_default);
		free((void *)current_default);
	}

	/* it's possible that the directory doesn't exist (especially for the default)
	 * For gtk's file select box to make sense we create it */
	if (stat(current_def_dir, &sb) != 0)
		g_mkdir(current_def_dir, S_IRWXU);

	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fs_dialog), current_def_dir);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(fs_dialog), TRUE);
	filter = setup_filter();
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(fs_dialog), filter);
	gtk_widget_show_all(fs_dialog);
	if (gtk_dialog_run(GTK_DIALOG(fs_dialog)) == GTK_RESPONSE_ACCEPT) {
		/* grab the selected file list, import each file and update the list */
		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(fs_dialog));
		if (filenames) {
			g_slist_foreach(filenames, do_import_file, NULL);
			report_dives(TRUE);
			g_slist_free(filenames);
		}
	}

	free(current_def_dir);
	gtk_widget_destroy(fs_dialog);
}

static GError *setup_uemis_import(device_data_t *data)
{
	GError *error = NULL;
	char *buf = NULL;

	error = uemis_download(data->devname, &uemis_max_dive_data, &buf, &data->progress);
	if (buf && strlen(buf) > 1) {
#ifdef DEBUGFILE
		fprintf(debugfile, "xml buffer \"%s\"\n\n", buf);
#endif
		parse_xml_buffer("Uemis Download", buf, strlen(buf), &error, FALSE);
		set_uemis_last_dive(uemis_max_dive_data);
#if UEMIS_DEBUG
		fprintf(debugfile, "uemis_max_dive_data: %s\n", uemis_max_dive_data);
#endif
	}
	return error;
}

static GtkWidget *import_dive_computer(device_data_t *data, GtkDialog *dialog)
{
	GError *error;
	GtkWidget *vbox, *info, *container, *label, *button;

	/* HACK to simply include the Uemis Zurich in the list */
	if (! strcmp(data->vendor, "Uemis") && ! strcmp(data->product, "Zurich")) {
		error = setup_uemis_import(data);
	} else {
		error = do_import(data);
	}
	if (!error)
		return NULL;

	button = gtk_dialog_get_widget_for_response(dialog, GTK_RESPONSE_ACCEPT);
	gtk_button_set_use_stock(GTK_BUTTON(button), 0);
	gtk_button_set_label(GTK_BUTTON(button), _("Retry"));

	vbox = gtk_dialog_get_content_area(dialog);

	info = gtk_info_bar_new();
	container = gtk_info_bar_get_content_area(GTK_INFO_BAR(info));
	label = gtk_label_new(error->message);
	gtk_container_add(GTK_CONTAINER(container), label);
	gtk_box_pack_start(GTK_BOX(vbox), info, FALSE, FALSE, 0);
	return info;
}

void download_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *button, *hbox, *vbox, *label, *info = NULL;
	GtkComboBox *computer, *device;
	GtkTreeIter iter;
	device_data_t devicedata = {
		.devname = NULL,
	};

	dialog = gtk_dialog_new_with_buttons(_("Download From Dive Computer"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	label = gtk_label_new(_(" Please select dive computer and device. "));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 3);
	computer = dive_computer_selector(vbox);
	device = dc_device_selector(vbox);
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);
	devicedata.progress.bar = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(hbox), devicedata.progress.bar);

	button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	if (!gtk_combo_box_get_active_iter(computer, &iter))
		gtk_widget_set_sensitive(button, FALSE);

repeat:
	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	switch (result) {
		dc_descriptor_t *descriptor;
		GtkTreeModel *model;

	case GTK_RESPONSE_ACCEPT:
		if (info)
			gtk_widget_destroy(info);
		const char *vendor, *product;

		if (!gtk_combo_box_get_active_iter(computer, &iter))
			break;

		model = gtk_combo_box_get_model(computer);
		gtk_tree_model_get(model, &iter,
				0, &descriptor,
				-1);

		vendor = dc_descriptor_get_vendor(descriptor);
		product = dc_descriptor_get_product(descriptor);

		devicedata.descriptor = descriptor;
		devicedata.vendor = vendor;
		devicedata.product = product;

		if (!gtk_combo_box_get_active_iter(device, &iter))
			break;

		model = gtk_combo_box_get_model(device);
		gtk_tree_model_get(model, &iter,
				0, &devicedata.devname,
				-1);
		set_default_dive_computer(vendor, product);
		set_default_dive_computer_device(devicedata.devname);
		info = import_dive_computer(&devicedata, GTK_DIALOG(dialog));
		if (info)
			goto repeat;
		report_dives(TRUE);
		break;
	default:
		/* it's possible that some dives were downloaded */
		report_dives(TRUE);
		break;
	}
	gtk_widget_destroy(dialog);
}

void update_progressbar(progressbar_t *progress, double value)
{
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress->bar), value);
}

void update_progressbar_text(progressbar_t *progress, const char *text)
{
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress->bar), text);
}

void set_filename(const char *filename, gboolean force)
{
	if (!force && existing_filename)
		return;
	free((void *)existing_filename);
	if (filename)
		existing_filename = strdup(filename);
}
