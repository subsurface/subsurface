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
#include <sys/time.h>
#include <ctype.h>

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

struct preferences prefs = {
	SI_UNITS,
	{ TRUE, FALSE, },
	{ FALSE, FALSE, FALSE, 1.6, 4.0, 13.0},
	FALSE, FALSE, FALSE, 0.30, 0.75
};

struct dcnicknamelist {
	const char *nickname;
	const char *model;
	uint32_t deviceid;
	struct dcnicknamelist *next;
	gboolean saved;
};
static struct dcnicknamelist *nicknamelist;
char *nicknamestring;

static GtkWidget *dive_profile;
static const char *default_dive_computer_vendor;
static const char *default_dive_computer_product;
static const char *default_dive_computer_device;
static gboolean force_download;
static gboolean prefer_downloaded;

GtkActionGroup *action_group;

struct units *get_output_units()
{
	return &prefs.output_units;
}

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
	default_dive_computer_vendor = strdup(vendor);
	default_dive_computer_product = strdup(product);
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

static void file_close(GtkWidget *w, gpointer data)
{
	if (unsaved_changes())
		if (ask_save_changes() == FALSE)
			return;

	if (existing_filename)
		free((void *)existing_filename);
	existing_filename = NULL;

	/* free the dives and trips */
	while (dive_table.nr)
		delete_single_dive(0);
	dive_table.preexisting = 0;
	mark_divelist_changed(FALSE);

	/* clear the selection and the statistics */
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
		report_dives(FALSE, FALSE);
	}
	gtk_widget_destroy(dialog);
}

gboolean on_delete(GtkWidget* w, gpointer data)
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

	if (flags & EDITABLE) {
		g_object_set(renderer, "editable", TRUE, NULL);
		g_signal_connect(renderer, "edited", (GCallback) data_func, tree_view);
		data_func = NULL;
	}

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

static void update_screen()
{
	update_dive_list_units();
	repaint_dive();
	update_dive_list_col_visibility();
}

#define UNITCALLBACK(name, type, value)				\
static void name(GtkWidget *w, gpointer data) 			\
{								\
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))	\
		prefs.output_units.type = value;		\
	update_screen();					\
}

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

#define OPTIONCALLBACK(name, option)			\
static void name(GtkWidget *w, gpointer data)		\
{							\
	GtkWidget **entry = data;			\
	option = GTK_TOGGLE_BUTTON(w)->active;		\
	update_screen();				\
	if (entry)					\
		gtk_widget_set_sensitive(*entry, option);\
}

OPTIONCALLBACK(otu_toggle, prefs.visible_cols.otu)
OPTIONCALLBACK(maxcns_toggle, prefs.visible_cols.maxcns)
OPTIONCALLBACK(sac_toggle, prefs.visible_cols.sac)
OPTIONCALLBACK(nitrox_toggle, prefs.visible_cols.nitrox)
OPTIONCALLBACK(temperature_toggle, prefs.visible_cols.temperature)
OPTIONCALLBACK(totalweight_toggle, prefs.visible_cols.totalweight)
OPTIONCALLBACK(suit_toggle, prefs.visible_cols.suit)
OPTIONCALLBACK(cylinder_toggle, prefs.visible_cols.cylinder)
OPTIONCALLBACK(po2_toggle, prefs.pp_graphs.po2)
OPTIONCALLBACK(pn2_toggle, prefs.pp_graphs.pn2)
OPTIONCALLBACK(phe_toggle, prefs.pp_graphs.phe)
OPTIONCALLBACK(red_ceiling_toggle, prefs.profile_red_ceiling)
OPTIONCALLBACK(calc_ceiling_toggle, prefs.profile_calc_ceiling)
OPTIONCALLBACK(calc_ceiling_3m_toggle, prefs.calc_ceiling_3m_incr)
OPTIONCALLBACK(force_toggle, force_download)
OPTIONCALLBACK(prefer_dl_toggle, prefer_downloaded)

static gboolean gflow_edit(GtkWidget *w, GdkEvent *event, gpointer _data)
{
	double gflow;
	const char *buf;
	if (event->type == GDK_FOCUS_CHANGE) {
		buf = gtk_entry_get_text(GTK_ENTRY(w));
		sscanf(buf, "%lf", &gflow);
		set_gf(gflow / 100.0, -1.0);
		update_screen();
	}
	return FALSE;
}

static gboolean gfhigh_edit(GtkWidget *w, GdkEvent *event, gpointer _data)
{
	double gfhigh;
	const char *buf;
	if (event->type == GDK_FOCUS_CHANGE) {
		buf = gtk_entry_get_text(GTK_ENTRY(w));
		sscanf(buf, "%lf", &gfhigh);
		set_gf(-1.0, gfhigh / 100.0);
		update_screen();
	}
	return FALSE;
}

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
	GtkWidget *dialog, *notebook, *font, *frame, *box, *vbox, *button, *xmlfile_button;
	GtkWidget *entry_po2, *entry_pn2, *entry_phe, *entry_gflow, *entry_gfhigh;
	const char *current_default, *new_default;
	char threshold_text[10], utf8_buf[128];
	struct preferences oldprefs = prefs;

	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	/* create the notebook for the preferences and attach it to dialog */
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), notebook, FALSE, FALSE, 5);

	/* vbox that holds the first notebook page */
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
				gtk_label_new(_("General Settings")));
	frame = gtk_frame_new(_("Units"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	create_radio(box, _("Depth:"),
		_("Meter"), set_meter, (prefs.output_units.length == METERS),
		_("Feet"),  set_feet, (prefs.output_units.length == FEET),
		NULL);

	create_radio(box, _("Pressure:"),
		_("Bar"), set_bar, (prefs.output_units.pressure == BAR),
		_("PSI"),  set_psi, (prefs.output_units.pressure == PSI),
		NULL);

	create_radio(box, _("Volume:"),
		_("Liter"),  set_liter, (prefs.output_units.volume == LITER),
		_("CuFt"), set_cuft, (prefs.output_units.volume == CUFT),
		NULL);

	create_radio(box, _("Temperature:"),
		_("Celsius"), set_celsius, (prefs.output_units.temperature == CELSIUS),
		_("Fahrenheit"),  set_fahrenheit, (prefs.output_units.temperature == FAHRENHEIT),
		NULL);

	create_radio(box, _("Weight:"),
		_("kg"), set_kg, (prefs.output_units.weight == KG),
		_("lbs"),  set_lbs, (prefs.output_units.weight == LBS),
		NULL);

	frame = gtk_frame_new(_("Show Columns"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	button = gtk_check_button_new_with_label(_("Temp"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.temperature);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(temperature_toggle), NULL);

	button = gtk_check_button_new_with_label(_("Cyl"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.cylinder);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(cylinder_toggle), NULL);

	button = gtk_check_button_new_with_label("O" UTF8_SUBSCRIPT_2 "%");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.nitrox);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(nitrox_toggle), NULL);

	button = gtk_check_button_new_with_label(_("SAC"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.sac);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(sac_toggle), NULL);

	button = gtk_check_button_new_with_label(_("Weight"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.totalweight);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(totalweight_toggle), NULL);

	button = gtk_check_button_new_with_label(_("Suit"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.suit);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(suit_toggle), NULL);

	frame = gtk_frame_new(_("Divelist Font"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	font = gtk_font_button_new_with_font(divelist_font);
	gtk_container_add(GTK_CONTAINER(frame),font);

	frame = gtk_frame_new(_("Misc. Options"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	frame = gtk_frame_new(_("Default XML Data File"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);
	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);
	current_default = subsurface_default_filename();
	xmlfile_button = gtk_button_new_with_label(current_default);
	g_signal_connect(G_OBJECT(xmlfile_button), "clicked",
			 G_CALLBACK(pick_default_file), xmlfile_button);
	gtk_box_pack_start(GTK_BOX(box), xmlfile_button, FALSE, FALSE, 6);

	/* vbox that holds the second notebook page */
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
				gtk_label_new(_("Tec Settings")));

	frame = gtk_frame_new(_("Show Columns"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	button = gtk_check_button_new_with_label(_("OTU"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.otu);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(otu_toggle), NULL);

	button = gtk_check_button_new_with_label(_("maxCNS"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.visible_cols.maxcns);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(maxcns_toggle), NULL);

	frame = gtk_frame_new(_("Profile Settings"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	sprintf(utf8_buf, _("Show pO%s graph"), UTF8_SUBSCRIPT_2);
	button = gtk_check_button_new_with_label(utf8_buf);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.pp_graphs.po2);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(po2_toggle), &entry_po2);

	sprintf(utf8_buf, _("pO%s threshold"), UTF8_SUBSCRIPT_2);
	frame = gtk_frame_new(utf8_buf);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 6);
	entry_po2 = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_po2), 4);
	snprintf(threshold_text, sizeof(threshold_text), "%.1f", prefs.pp_graphs.po2_threshold);
	gtk_entry_set_text(GTK_ENTRY(entry_po2), threshold_text);
	gtk_widget_set_sensitive(entry_po2, prefs.pp_graphs.po2);
	gtk_container_add(GTK_CONTAINER(frame), entry_po2);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	sprintf(utf8_buf, _("Show pN%s graph"), UTF8_SUBSCRIPT_2);
	button = gtk_check_button_new_with_label(utf8_buf);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.pp_graphs.pn2);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(pn2_toggle), &entry_pn2);

	sprintf(utf8_buf, _("pN%s threshold"), UTF8_SUBSCRIPT_2);
	frame = gtk_frame_new(utf8_buf);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 6);
	entry_pn2 = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_pn2), 4);
	snprintf(threshold_text, sizeof(threshold_text), "%.1f", prefs.pp_graphs.pn2_threshold);
	gtk_entry_set_text(GTK_ENTRY(entry_pn2), threshold_text);
	gtk_widget_set_sensitive(entry_pn2, prefs.pp_graphs.pn2);
	gtk_container_add(GTK_CONTAINER(frame), entry_pn2);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	button = gtk_check_button_new_with_label(_("Show pHe graph"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.pp_graphs.phe);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(phe_toggle), &entry_phe);

	frame = gtk_frame_new(_("pHe threshold"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 6);
	entry_phe = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_phe), 4);
	snprintf(threshold_text, sizeof(threshold_text), "%.1f", prefs.pp_graphs.phe_threshold);
	gtk_entry_set_text(GTK_ENTRY(entry_phe), threshold_text);
	gtk_widget_set_sensitive(entry_phe, prefs.pp_graphs.phe);
	gtk_container_add(GTK_CONTAINER(frame), entry_phe);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	button = gtk_check_button_new_with_label(_("Show dc reported ceiling in red"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.profile_red_ceiling);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(red_ceiling_toggle), NULL);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	button = gtk_check_button_new_with_label(_("Show calculated ceiling"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.profile_calc_ceiling);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(calc_ceiling_toggle), NULL);

	button = gtk_check_button_new_with_label(_("3m increments for calculated ceiling"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.calc_ceiling_3m_incr);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(calc_ceiling_3m_toggle), NULL);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	frame = gtk_frame_new(_("GFlow"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 6);
	entry_gflow = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_gflow), 4);
	snprintf(threshold_text, sizeof(threshold_text), "%.0f", prefs.gflow * 100);
	gtk_entry_set_text(GTK_ENTRY(entry_gflow), threshold_text);
	gtk_container_add(GTK_CONTAINER(frame), entry_gflow);
	gtk_widget_add_events(entry_gflow, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(entry_gflow), "event", G_CALLBACK(gflow_edit), NULL);

	frame = gtk_frame_new(_("GFhigh"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 6);
	entry_gfhigh = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_gfhigh), 4);
	snprintf(threshold_text, sizeof(threshold_text), "%.0f", prefs.gfhigh * 100);
	gtk_entry_set_text(GTK_ENTRY(entry_gfhigh), threshold_text);
	gtk_container_add(GTK_CONTAINER(frame), entry_gfhigh);
	gtk_widget_add_events(entry_gfhigh, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(G_OBJECT(entry_gfhigh), "event", G_CALLBACK(gfhigh_edit), NULL);

	gtk_widget_show_all(dialog);
	result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		const char *po2_threshold_text, *pn2_threshold_text, *phe_threshold_text, *gflow_text, *gfhigh_text;
		/* Make sure to flush any modified old dive data with old units */
		update_dive(NULL);

		if (divelist_font)
			free((void *)divelist_font);
		divelist_font = strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(font)));
		set_divelist_font(divelist_font);
		po2_threshold_text = gtk_entry_get_text(GTK_ENTRY(entry_po2));
		sscanf(po2_threshold_text, "%lf", &prefs.pp_graphs.po2_threshold);
		pn2_threshold_text = gtk_entry_get_text(GTK_ENTRY(entry_pn2));
		sscanf(pn2_threshold_text, "%lf", &prefs.pp_graphs.pn2_threshold);
		phe_threshold_text = gtk_entry_get_text(GTK_ENTRY(entry_phe));
		sscanf(phe_threshold_text, "%lf", &prefs.pp_graphs.phe_threshold);
		gflow_text = gtk_entry_get_text(GTK_ENTRY(entry_gflow));
		sscanf(gflow_text, "%lf", &prefs.gflow);
		gfhigh_text = gtk_entry_get_text(GTK_ENTRY(entry_gfhigh));
		sscanf(gfhigh_text, "%lf", &prefs.gfhigh);
		prefs.gflow /= 100.0;
		prefs.gfhigh /= 100.0;
		set_gf(prefs.gflow, prefs.gfhigh);

		update_screen();

		subsurface_set_conf("feet", PREF_BOOL, BOOL_TO_PTR(prefs.output_units.length == FEET));
		subsurface_set_conf("psi", PREF_BOOL, BOOL_TO_PTR(prefs.output_units.pressure == PSI));
		subsurface_set_conf("cuft", PREF_BOOL, BOOL_TO_PTR(prefs.output_units.volume == CUFT));
		subsurface_set_conf("fahrenheit", PREF_BOOL, BOOL_TO_PTR(prefs.output_units.temperature == FAHRENHEIT));
		subsurface_set_conf("lbs", PREF_BOOL, BOOL_TO_PTR(prefs.output_units.weight == LBS));

		subsurface_set_conf("TEMPERATURE", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.temperature));
		subsurface_set_conf("TOTALWEIGHT", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.totalweight));
		subsurface_set_conf("SUIT", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.suit));
		subsurface_set_conf("CYLINDER", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.cylinder));
		subsurface_set_conf("NITROX", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.nitrox));
		subsurface_set_conf("SAC", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.sac));
		subsurface_set_conf("OTU", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.otu));
		subsurface_set_conf("MAXCNS", PREF_BOOL, BOOL_TO_PTR(prefs.visible_cols.maxcns));

		subsurface_set_conf("divelist_font", PREF_STRING, divelist_font);

		subsurface_set_conf("po2graph", PREF_BOOL, BOOL_TO_PTR(prefs.pp_graphs.po2));
		subsurface_set_conf("pn2graph", PREF_BOOL, BOOL_TO_PTR(prefs.pp_graphs.pn2));
		subsurface_set_conf("phegraph", PREF_BOOL, BOOL_TO_PTR(prefs.pp_graphs.phe));
		subsurface_set_conf("po2threshold", PREF_STRING, po2_threshold_text);
		subsurface_set_conf("pn2threshold", PREF_STRING, pn2_threshold_text);
		subsurface_set_conf("phethreshold", PREF_STRING, phe_threshold_text);
		subsurface_set_conf("redceiling", PREF_BOOL, BOOL_TO_PTR(prefs.profile_red_ceiling));
		subsurface_set_conf("calcceiling", PREF_BOOL, BOOL_TO_PTR(prefs.profile_calc_ceiling));
		subsurface_set_conf("calcceiling3m", PREF_BOOL, BOOL_TO_PTR(prefs.calc_ceiling_3m_incr));
		subsurface_set_conf("gflow", PREF_STRING, gflow_text);
		subsurface_set_conf("gfhigh", PREF_STRING, gfhigh_text);

		new_default = strdup(gtk_button_get_label(GTK_BUTTON(xmlfile_button)));

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
	} else if (result == GTK_RESPONSE_CANCEL) {
		prefs = oldprefs;
		set_gf(prefs.gflow, prefs.gfhigh);
		update_screen();
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

void set_autogroup(gboolean value)
{
	GtkAction *autogroup_action;

	if (value == autogroup)
		return;

	autogroup_action = gtk_action_group_get_action(action_group, "Autogroup");
	gtk_action_activate(autogroup_action);
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

static void prev_dc(GtkWidget *w, gpointer data)
{
	dc_number--;
	/* If the dc number underflows, we'll "wrap around" and use the last dc */
	repaint_dive();
}

static void next_dc(GtkWidget *w, gpointer data)
{
	dc_number++;
	/* If the dc number overflows, we'll "wrap around" and zero it */
	repaint_dive();
}

static void test_planner_cb(GtkWidget *w, gpointer data)
{
	test_planner();
}

/*
 * Get a value in tenths (so "10.2" == 102, "9" = 90)
 *
 * Return negative for errors.
 */
static int get_tenths(char *begin, char **end)
{
	int value = strtol(begin, end, 10);
	if (begin == *end)
		return -1;
	value *= 10;

	/* Fraction? We only look at the first digit */
	if (**end == '.') {
		++*end;
		if (!isdigit(**end))
			return -1;
		value += **end - '0';
		do {
			++*end;
		} while (isdigit(**end));
	}
	return value;
}

static int get_permille(char *begin, char **end)
{
	int value = get_tenths(begin, end);
	if (value >= 0) {
		/* Allow a percentage sign */
		if (**end == '%')
			++*end;
	}
	return value;
}

static int validate_gas(char *text, int *o2_p, int *he_p)
{
	int o2, he;

	if (!text)
		return 0;

	while (isspace(*text))
		text++;

	if (!*text) {
		o2 = AIR_PERMILLE; he = 0;
	} else if (!strcasecmp(text, "air")) {
		o2 = AIR_PERMILLE; he = 0; text += 3;
	} else if (!strncasecmp(text, "ean", 3)) {
		o2 = get_permille(text+3, &text); he = 0;
	} else {
		o2 = get_permille(text, &text); he = 0;
		if (*text == '/')
			he = get_permille(text+1, &text);
	}

	/* We don't want any extra crud */
	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	/* Validate the gas mix */
	if (*text || o2 < 1 || o2 > 1000 || he < 0 || o2+he > 1000)
		return 0;

	/* Let it rip */
	*o2_p = o2;
	*he_p = he;
	return 1;
}

static int validate_time(char *text, int *sec_p, int *rel_p)
{
	int min, sec, rel;
	char *end;

	if (!text)
		return 0;

	while (isspace(*text))
		text++;

	rel = 0;
	if (*text == '+') {
		rel = 1;
		text++;
		while (isspace(*text))
			text++;
	}

	min = strtol(text, &end, 10);
	if (text == end)
		return 0;

	if (min < 0 || min > 1000)
		return 0;

	/* Ok, minutes look ok */
	text = end;
	sec = 0;
	if (*text == ':') {
		text++;
		sec = strtol(text, &end, 10);
		if (end != text+2)
			return 0;
		if (sec < 0)
			return 0;
		text = end;
		if (*text == ':') {
			if (sec >= 60)
				return 0;
			min = min*60 + sec;
			text++;
			sec = strtol(text, &end, 10);
			if (end != text+2)
				return 0;
			if (sec < 0)
				return 0;
			text = end;
		}
	}

	/* Maybe we should accept 'min' at the end? */
	if (isspace(*text))
		text++;
	if (*text)
		return 0;

	*sec_p = min*60 + sec;
	*rel_p = rel;
	return 1;
}

static int validate_depth(char *text, int *mm_p)
{
	int depth, imperial;

	if (!text)
		return 0;

	depth = get_tenths(text, &text);
	if (depth < 0)
		return 0;

	while (isspace(*text))
		text++;

	imperial = get_output_units()->length == FEET;
	if (*text == 'm') {
		imperial = 0;
		text++;
	} else if (!strcasecmp(text, "ft")) {
		imperial = 1;
		text += 2;
	}
	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	if (imperial) {
		depth = feet_to_mm(depth / 10.0);
	} else {
		depth *= 100;
	}
	*mm_p = depth;
	return 1;
}

static GtkWidget *add_entry_to_box(GtkWidget *box, const char *label)
{
	GtkWidget *entry, *frame;

	entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry), 16);
	if (label) {
		frame = gtk_frame_new(label);
		gtk_container_add(GTK_CONTAINER(frame), entry);
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 2);
	}
	return entry;
}

#define MAX_WAYPOINTS 8
GtkWidget *entry_depth[MAX_WAYPOINTS], *entry_duration[MAX_WAYPOINTS], *entry_gas[MAX_WAYPOINTS];
int nr_waypoints = 0;
static GtkListStore *gas_model = NULL;
struct diveplan diveplan = {};
char *cache_data = NULL;
struct dive *planned_dive = NULL;

/* make a copy of the diveplan so far and display the corresponding dive */
void show_planned_dive(void)
{
	struct diveplan tempplan;
	struct divedatapoint *dp, **dpp;

	memcpy(&tempplan, &diveplan, sizeof(struct diveplan));
	dpp = &tempplan.dp;
	dp = diveplan.dp;
	while(*dpp) {
		*dpp = malloc(sizeof(struct divedatapoint));
		memcpy(*dpp, dp, sizeof(struct divedatapoint));
		dp = dp->next;
		if (dp && !dp->time) {
			/* we have an incomplete entry - stop before it */
			(*dpp)->next = NULL;
			break;
		}
		dpp = &(*dpp)->next;
	}
	plan(&tempplan, &cache_data, &planned_dive);
	free_dps(tempplan.dp);
}

static gboolean gas_focus_out_cb(GtkWidget *entry, GdkEvent *event, gpointer data)
{
	char *gastext;
	int o2, he;
	int idx = data - NULL;

	gastext = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if (validate_gas(gastext, &o2, &he)) {
		add_string_list_entry(gastext, gas_model);
		add_gas_to_nth_dp(&diveplan, idx, o2, he);
		show_planned_dive();
	} else {
		/* we need to instead change the color of the input field or something */
		printf("invalid gas for row %d\n",idx);
	}
	free(gastext);
	return FALSE;
}

static gboolean depth_focus_out_cb(GtkWidget *entry, GdkEvent *event, gpointer data)
{
	char *depthtext;
	int depth;
	int idx = data - NULL;

	depthtext = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if (validate_depth(depthtext, &depth)) {
		add_depth_to_nth_dp(&diveplan, idx, depth);
		show_planned_dive();
	} else {
		/* we need to instead change the color of the input field or something */
		printf("invalid depth for row %d\n", idx);
	}
	free(depthtext);
	return FALSE;
}

static gboolean duration_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	char *durationtext;
	int duration, is_rel;
	int idx = data - NULL;

	durationtext = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if (validate_time(durationtext, &duration, &is_rel)) {
		add_duration_to_nth_dp(&diveplan, idx, duration, is_rel);
		show_planned_dive();
	} else {
		/* we need to instead change the color of the input field or something */
		printf("invalid duration for row %d\n", idx);
	}
	free(durationtext);
	return FALSE;
}

static gboolean starttime_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	char *starttimetext;
	int starttime, is_rel;

	starttimetext = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
	if (validate_time(starttimetext, &starttime, &is_rel)) {
		/* we alway make this relative for now */
		diveplan.when = time(NULL) + starttime;
	} else {
		/* we need to instead change the color of the input field or something */
		printf("invalid starttime\n");
	}
	free(starttimetext);
	return FALSE;
}

static GtkWidget *add_gas_combobox_to_box(GtkWidget *box, const char *label)
{
	GtkWidget *frame, *combo;
	GtkEntryCompletion *completion;
	GtkEntry *entry;

	if (!gas_model) {
		gas_model = gtk_list_store_new(1, G_TYPE_STRING);
		add_string_list_entry("AIR", gas_model);
		add_string_list_entry("EAN32", gas_model);
		add_string_list_entry("EAN36 @ 1.6", gas_model);
	}
	combo = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(gas_model), 0);
	gtk_widget_add_events(combo, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(combo)), "focus-out-event", G_CALLBACK(gas_focus_out_cb), NULL);

	if (label) {
		frame = gtk_frame_new(label);
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frame), combo);
	} else {
		gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 2);
	}
	entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(gas_model));
	gtk_entry_completion_set_inline_completion(completion, TRUE);
	gtk_entry_completion_set_inline_selection(completion, TRUE);
	gtk_entry_completion_set_popup_single_match(completion, FALSE);
	gtk_entry_set_completion(entry, completion);
	g_object_unref(completion);

	return combo;
}

static void add_waypoint_widgets(GtkWidget *box, int idx)
{
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
	if (idx == 0) {
		entry_depth[idx] = add_entry_to_box(hbox, _("Ending Depth"));
		entry_duration[idx] = add_entry_to_box(hbox, _("Segment Time"));
		entry_gas[idx] = add_gas_combobox_to_box(hbox, _("Gas Used"));
	} else {
		entry_depth[idx] = add_entry_to_box(hbox, NULL);
		entry_duration[idx] = add_entry_to_box(hbox, NULL);
		entry_gas[idx] = add_gas_combobox_to_box(hbox, NULL);
	}
	gtk_widget_add_events(entry_depth[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_depth[idx], "focus-out-event", G_CALLBACK(depth_focus_out_cb), NULL + idx);
	gtk_widget_add_events(entry_duration[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_duration[idx], "focus-out-event", G_CALLBACK(duration_focus_out_cb), NULL + idx);
}

static void add_waypoint_cb(GtkButton *button, gpointer _data)
{
	GtkWidget *vbox = _data;
	if (nr_waypoints < MAX_WAYPOINTS) {
		GtkWidget *ovbox, *dialog;
		add_waypoint_widgets(vbox, nr_waypoints);
		nr_waypoints++;
		ovbox = gtk_widget_get_parent(GTK_WIDGET(button));
		dialog = gtk_widget_get_parent(ovbox);
		gtk_widget_show_all(dialog);
	} else {
		// some error
	}
}

void input_plan()
{
	GtkWidget *planner, *content, *vbox, *outervbox, *add_row, *deltat;
	int lasttime = 0;
	char starttimebuf[64] = "+60:00";

	if (diveplan.dp)
		free_dps(diveplan.dp);
	memset(&diveplan, 0, sizeof(diveplan));
	planned_dive = NULL;
	planner = gtk_dialog_new_with_buttons(_("Dive Plan - THIS IS JUST A SIMULATION; DO NOT USE FOR DIVING"),
					GTK_WINDOW(main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);

	content = gtk_dialog_get_content_area (GTK_DIALOG (planner));
	outervbox = gtk_vbox_new(FALSE, 2);
	gtk_container_add (GTK_CONTAINER (content), outervbox);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(outervbox), vbox, TRUE, TRUE, 0);
	deltat = add_entry_to_box(vbox, _("Dive starts in how many minutes?"));
	gtk_entry_set_max_length(GTK_ENTRY(deltat), 12);
	gtk_entry_set_text(GTK_ENTRY(deltat), starttimebuf);
	gtk_widget_add_events(deltat, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(deltat, "focus-out-event", G_CALLBACK(starttime_focus_out_cb), NULL);
	diveplan.when = time(NULL) + 3600;
	nr_waypoints = 4;
	add_waypoint_widgets(vbox, 0);
	add_waypoint_widgets(vbox, 1);
	add_waypoint_widgets(vbox, 2);
	add_waypoint_widgets(vbox, 3);
	add_row = gtk_button_new_with_label(_("Add waypoint"));
	g_signal_connect(G_OBJECT(add_row), "clicked", G_CALLBACK(add_waypoint_cb), vbox);
	gtk_box_pack_start(GTK_BOX(outervbox), add_row, FALSE, FALSE, 0);
	gtk_widget_show_all(planner);
	if (gtk_dialog_run(GTK_DIALOG(planner)) == GTK_RESPONSE_ACCEPT) {
		int i;
		const char *deltattext;

		deltattext = gtk_entry_get_text(GTK_ENTRY(deltat));
		diveplan.when = time(NULL) + 60 * atoi(deltattext);
		free_dps(diveplan.dp);
		diveplan.dp = 0;
		for (i = 0; i < nr_waypoints; i++) {
			char *depthtext, *durationtext, *gastext;
			int depth, duration, o2, he, is_rel;
			GtkWidget *entry;

			depthtext = strdup(gtk_entry_get_text(GTK_ENTRY(entry_depth[i])));
			if (!validate_depth(depthtext, &depth)) {
				// mark error and redo?
				free(depthtext);
				continue;
			}
			durationtext = strdup(gtk_entry_get_text(GTK_ENTRY(entry_duration[i])));
			if (!validate_time(durationtext, &duration, &is_rel)) {
				// mark error and redo?
				free(durationtext);
				continue;
			}
			if (!is_rel)
				duration -= lasttime;
			entry = gtk_bin_get_child(GTK_BIN(entry_gas[i]));
			gastext = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
			if (!validate_gas(gastext, &o2, &he)) {
				// mark error and redo?
				free(gastext);
				continue;
			}
			/* just in case this didn't get added by the callback */
			add_string_list_entry(gastext, gas_model);

			// still need to handle desired pO2 and a setpoint (for CC)

			if (duration == 0)
				break;
			plan_add_segment(&diveplan, duration, depth, o2, he);
			lasttime += duration;
			free(depthtext);
			free(durationtext);
			free(gastext);
		}
	}
	show_planned_dive();
	gtk_widget_destroy(planner);
}

static GtkActionEntry menu_items[] = {
	{ "FileMenuAction", NULL, N_("File"), NULL, NULL, NULL},
	{ "LogMenuAction",  NULL, N_("Log"), NULL, NULL, NULL},
	{ "ViewMenuAction",  NULL, N_("View"), NULL, NULL, NULL},
	{ "FilterMenuAction",  NULL, N_("Filter"), NULL, NULL, NULL},
	{ "PlannerMenuAction",  NULL, N_("Planner"), NULL, NULL, NULL},
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
	{ "PrevDC",         NULL, N_("Prev DC"), NULL, NULL, G_CALLBACK(prev_dc) },
	{ "NextDC",         NULL, N_("Next DC"), NULL, NULL, G_CALLBACK(next_dc) },
	{ "TestPlan",       NULL, N_("Test Planner"), NULL, NULL, G_CALLBACK(test_planner_cb) },
	{ "InputPlan",      NULL, N_("Input Plan"), NULL, NULL, G_CALLBACK(input_plan) },
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
					<menuitem name=\"PrevDC\" action=\"PrevDC\" /> \
					<menuitem name=\"NextDC\" action=\"NextDC\" /> \
				</menu> \
			</menu> \
			<menu name=\"FilterMenu\" action=\"FilterMenuAction\"> \
				<menuitem name=\"SelectEvents\" action=\"SelectEvents\" /> \
			</menu> \
			<menu name=\"PlannerMenu\" action=\"PlannerMenuAction\"> \
				<menuitem name=\"TestPlan\" action=\"TestPlan\" /> \
				<menuitem name=\"InputPlan\" action=\"InputPlan\" /> \
			</menu> \
			<menu name=\"Help\" action=\"HelpMenuAction\"> \
				<menuitem name=\"About\" action=\"About\" /> \
			</menu> \
		</menubar> \
	</ui> \
";

static GtkWidget *get_menubar_menu(GtkWidget *window, GtkUIManager *ui_manager)
{
	action_group = gtk_action_group_new("Menu");
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

static gboolean on_key_press(GtkWidget *w, GdkEventKey *event, GtkWidget *divelist)
{
	if (event->type != GDK_KEY_PRESS || event->state != 0)
		return FALSE;
	switch (event->keyval) {
	case GDK_Up:
		select_prev_dive();
		return TRUE;
	case GDK_Down:
		select_next_dive();
		return TRUE;
	case GDK_Left:
		prev_dc(NULL, NULL);
		return TRUE;
	case GDK_Right:
		next_dc(NULL, NULL);
		return TRUE;
	}
	return FALSE;
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
		prefs.output_units.length = FEET;
	if (subsurface_get_conf("psi", PREF_BOOL))
		prefs.output_units.pressure = PSI;
	if (subsurface_get_conf("cuft", PREF_BOOL))
		prefs.output_units.volume = CUFT;
	if (subsurface_get_conf("fahrenheit", PREF_BOOL))
		prefs.output_units.temperature = FAHRENHEIT;
	if (subsurface_get_conf("lbs", PREF_BOOL))
		prefs.output_units.weight = LBS;
	/* an unset key is FALSE - all these are hidden by default */
	prefs.visible_cols.cylinder = PTR_TO_BOOL(subsurface_get_conf("CYLINDER", PREF_BOOL));
	prefs.visible_cols.temperature = PTR_TO_BOOL(subsurface_get_conf("TEMPERATURE", PREF_BOOL));
	prefs.visible_cols.totalweight = PTR_TO_BOOL(subsurface_get_conf("TOTALWEIGHT", PREF_BOOL));
	prefs.visible_cols.suit = PTR_TO_BOOL(subsurface_get_conf("SUIT", PREF_BOOL));
	prefs.visible_cols.nitrox = PTR_TO_BOOL(subsurface_get_conf("NITROX", PREF_BOOL));
	prefs.visible_cols.otu = PTR_TO_BOOL(subsurface_get_conf("OTU", PREF_BOOL));
	prefs.visible_cols.maxcns = PTR_TO_BOOL(subsurface_get_conf("MAXCNS", PREF_BOOL));
	prefs.visible_cols.sac = PTR_TO_BOOL(subsurface_get_conf("SAC", PREF_BOOL));
	prefs.pp_graphs.po2 = PTR_TO_BOOL(subsurface_get_conf("po2graph", PREF_BOOL));
	prefs.pp_graphs.pn2 = PTR_TO_BOOL(subsurface_get_conf("pn2graph", PREF_BOOL));
	prefs.pp_graphs.phe = PTR_TO_BOOL(subsurface_get_conf("phegraph", PREF_BOOL));
	conf_value = subsurface_get_conf("po2threshold", PREF_STRING);
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.pp_graphs.po2_threshold);
		free((void *)conf_value);
	}
	conf_value = subsurface_get_conf("pn2threshold", PREF_STRING);
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.pp_graphs.pn2_threshold);
		free((void *)conf_value);
	}
	conf_value = subsurface_get_conf("phethreshold", PREF_STRING);
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.pp_graphs.phe_threshold);
		free((void *)conf_value);
	}
	prefs.profile_red_ceiling = PTR_TO_BOOL(subsurface_get_conf("redceiling", PREF_BOOL));
	prefs.profile_calc_ceiling = PTR_TO_BOOL(subsurface_get_conf("calcceiling", PREF_BOOL));
	prefs.calc_ceiling_3m_incr = PTR_TO_BOOL(subsurface_get_conf("calcceiling3m", PREF_BOOL));
	conf_value = subsurface_get_conf("gflow", PREF_STRING);
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.gflow);
		prefs.gflow /= 100.0;
		set_gf(prefs.gflow, -1.0);
		free((void *)conf_value);
	}
	conf_value = subsurface_get_conf("gfhigh", PREF_STRING);
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.gfhigh);
		prefs.gfhigh /= 100.0;
		set_gf(-1.0, prefs.gfhigh);
		free((void *)conf_value);
	}
	divelist_font = subsurface_get_conf("divelist_font", PREF_STRING);

	default_filename = subsurface_get_conf("default_filename", PREF_STRING);

	default_dive_computer_vendor = subsurface_get_conf("dive_computer_vendor", PREF_STRING);
	default_dive_computer_product = subsurface_get_conf("dive_computer_product", PREF_STRING);
	default_dive_computer_device = subsurface_get_conf("dive_computer_device", PREF_STRING);
	conf_value = subsurface_get_conf("dc_nicknames", PREF_STRING);
	nicknamestring = strdup("");
	if (conf_value) {
		char *next_token, *nickname, *model, *conf_copy;
		uint32_t deviceid;
		int len;
		next_token = conf_copy = strdup(conf_value);
		len = strlen(next_token);
		while ((next_token = g_utf8_strchr(next_token, len, '{')) != NULL) {
			/* replace the '{' so we keep looking in case any test fails */
			*next_token = '(';
			/* the next 8 chars are the deviceid, after that we have the model
			 * and the utf8 nickname (if set) */
			if (sscanf(next_token, "(%8x,", &deviceid) > 0) {
				char *namestart, *nameend, *tupleend;
				namestart = g_utf8_strchr(next_token, len, ',');
				/* we know that namestart isn't NULL based on the sscanf succeeding */
				len = strlen(namestart + 1);
				nameend = g_utf8_strchr(namestart + 1, len, ',');
				tupleend = g_utf8_strchr(namestart + 1, len, '}');
				if (!tupleend)
					/* the config entry is messed up - bail */
					break;
				if (!nameend || tupleend < nameend)
					nameend = tupleend;
				*nameend = '\0';
				model = strdup(namestart + 1);
				if (tupleend == nameend) {
					/* no nickname */
					nickname = strdup("");
				} else {
					namestart = nameend + 1;
					len = strlen(namestart);
					nameend = g_utf8_strchr(namestart, len, '}');
					if (!nameend)
						break;
					*nameend = '\0';
					nickname = strdup(namestart);
				}
				remember_dc(model, deviceid, nickname, FALSE);
				next_token = nameend + 1;
			};
		}
		free((void *)conf_value);
		free(conf_copy);
	}
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

	/* handle some keys globally (to deal with gtk focus issues) */
	g_signal_connect (G_OBJECT (win), "key_press_event", G_CALLBACK (on_key_press), dive_list);

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
	tooltip_rects[tooltips].text = strdup(text);
	tooltips++;
}

#define INSIDE_RECT(_r,_x,_y)	((_r.x <= _x) && (_r.x + _r.width >= _x) && \
				(_r.y <= _y) && (_r.y + _r.height >= _y))

static gboolean profile_tooltip (GtkWidget *widget, gint x, gint y,
			gboolean keyboard_mode, GtkTooltip *tooltip, struct graphics_context *gc)
{
	int i;
	cairo_rectangle_t *drawing_area = &gc->drawing_area;
	gint tx = x - drawing_area->x; /* get transformed coordinates */
	gint ty = y - drawing_area->y;
	gint width, height, time = -1;
	char buffer[256], plot[256];
	const char *event = "";

	if (tx < 0 || ty < 0)
		return FALSE;

	/* don't draw a tooltip if nothing is there */
	if (gc->pi.nr == 0)
		return FALSE;

	width = drawing_area->width - 2*drawing_area->x;
	height = drawing_area->height - 2*drawing_area->y;
	if (width <= 0 || height <= 0)
		return FALSE;

	if (tx > width || ty > height)
		return FALSE;

	time = (tx * gc->maxtime) / width;

	/* are we over an event marker ? */
	for (i = 0; i < tooltips; i++) {
		if (INSIDE_RECT(tooltip_rects[i].rect, tx, ty)) {
			event = tooltip_rects[i].text;
			break;
		}
	}
	get_plot_details(gc, time, plot, sizeof(plot));

	snprintf(buffer, sizeof(buffer), "@ %d:%02d%c%s%c%s", time / 60, time % 60,
		*plot ? '\n' : ' ', plot,
		*event ? '\n' : ' ', event);
	gtk_tooltip_set_text(tooltip, buffer);
	return TRUE;

}

static double zoom_factor = 1.0;
static int zoom_x = -1, zoom_y = -1;

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	int i = 0;
	struct dive *dive = current_dive;
	static struct graphics_context gc = { .printer = 0 };

	/* the drawing area gives TOTAL width * height - x,y is used as the topx/topy offset
	 * so effective drawing area is width-2x * height-2y */
	gc.drawing_area.width = widget->allocation.width;
	gc.drawing_area.height = widget->allocation.height;
	gc.drawing_area.x = MIN(50,gc.drawing_area.width / 20.0);
	gc.drawing_area.y = MIN(50,gc.drawing_area.height / 20.0);

	gc.cr = gdk_cairo_create(widget->window);
	g_object_set(widget, "has-tooltip", TRUE, NULL);
	g_signal_connect(widget, "query-tooltip", G_CALLBACK(profile_tooltip), &gc);
	init_profile_background(&gc);
	cairo_paint(gc.cr);

	if (zoom_factor > 1.0) {
		double n = -(zoom_factor-1);
		cairo_translate(gc.cr, n*zoom_x, n*zoom_y);
		cairo_scale(gc.cr, zoom_factor, zoom_factor);
	}

	if (dive) {
		if (tooltip_rects) {
			while (i < tooltips) {
				if (tooltip_rects[i].text)
					free((void *)tooltip_rects[i].text);
				i++;
			}
			free(tooltip_rects);
			tooltip_rects = NULL;
		}
		tooltips = 0;
		plot(&gc, dive, SC_SCREEN);
	}
	cairo_destroy(gc.cr);

	return FALSE;
}

static void zoom_event(int x, int y, double inc)
{
	zoom_x = x;
	zoom_y = y;
	inc += zoom_factor;
	if (inc < 1.0)
		inc = 1.0;
	else if (inc > 10)
		inc = 10;
	zoom_factor = inc;
}

gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
	switch (event->direction) {
	case GDK_SCROLL_UP:
		zoom_event(event->x, event->y, 0.1);
		break;
	case GDK_SCROLL_DOWN:
		zoom_event(event->x, event->y, -0.1);
		break;
	default:
		return TRUE;
	}
	gtk_widget_queue_draw(widget);
	return TRUE;
}

gboolean clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	switch (event->button) {
	case 1:
		zoom_x = event->x;
		zoom_y = event->y;
		zoom_factor = 2.5;
		break;
	default:
		return TRUE;
	}
	gtk_widget_queue_draw(widget);
	return TRUE;
}

gboolean released(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	switch (event->button) {
	case 1:
		zoom_x = zoom_y = -1;
		zoom_factor = 1.0;
		break;
	default:
		return TRUE;
	}
	gtk_widget_queue_draw(widget);
	return TRUE;
}

gboolean motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	if (zoom_x < 0)
		return TRUE;

	zoom_x = event->x;
	zoom_y = event->y;
	gtk_widget_queue_draw(widget);
	return TRUE;
}

GtkWidget *dive_profile_widget(void)
{
	GtkWidget *da;

	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 350, 250);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);
	g_signal_connect(da, "button-press-event", G_CALLBACK(clicked), NULL);
	g_signal_connect(da, "scroll-event", G_CALLBACK(scroll_event), NULL);
	g_signal_connect(da, "button-release-event", G_CALLBACK(released), NULL);
	g_signal_connect(da, "motion-notify-event", G_CALLBACK(motion), NULL);
	gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);

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

struct vendor {
	const char *vendor;
	struct product *productlist;
	struct vendor *next;
};

struct product {
	const char *product;
	dc_descriptor_t *descriptor;
	struct product *next;
};

struct vendor *dc_list;

struct mydescriptor {
	const char *vendor;
	const char *product;
	dc_family_t type;
	unsigned int model;
};

/* create a list of lists and keep the elements sorted */
static void add_dc(const char *vendor, const char *product, dc_descriptor_t *descriptor)
{
	struct vendor *dcl = dc_list;
	struct vendor **dclp = &dc_list;
	struct product *pl, **plp;

	if (!vendor || !product)
		return;
	while (dcl && strcmp(dcl->vendor, vendor) < 0) {
		dclp = &dcl->next;
		dcl = dcl->next;
	}
	if (!dcl || strcmp(dcl->vendor, vendor)) {
		dcl = calloc(sizeof(struct vendor), 1);
		dcl->next = *dclp;
		*dclp = dcl;
		dcl->vendor = strdup(vendor);
	}
	/* we now have a pointer to the requested vendor */
	plp = &dcl->productlist;
	pl = *plp;
	while (pl && strcmp(pl->product, product) < 0) {
		plp = &pl->next;
		pl = pl->next;
	}
	if (!pl || strcmp(pl->product, product)) {
		pl = calloc(sizeof(struct product), 1);
		pl->next = *plp;
		*plp = pl;
		pl->product = strdup(product);
	}
	/* one would assume that the vendor / product combinations are unique,
	 * but that is not the case. At the time of this writing, there are two
	 * flavors of the Oceanic OC1 - but looking at the code in libdivecomputer
	 * they are handled exactly the same, so we ignore this issue for now
	 *
	    if (pl->descriptor && memcmp(pl->descriptor, descriptor, sizeof(struct mydescriptor)))
		printf("duplicate entry with different descriptor for %s - %s\n", vendor, product);
	    else
	 */
	pl->descriptor = descriptor;
}

/* fill the vendors and create and fill the respective product stores; return the longest product name
 * and also the indices of the default vendor / product */
static int fill_computer_list(GtkListStore *vendorstore, GtkListStore ***productstore, int *vendor_index, int *product_index)
{
	int i, j, numvendor, width = 10;
	GtkTreeIter iter;
	dc_iterator_t *iterator = NULL;
	dc_descriptor_t *descriptor = NULL;
	struct mydescriptor *mydescriptor;
	struct vendor *dcl;
	struct product *pl;
	GtkListStore **pstores;

	dc_descriptor_iterator(&iterator);
	while (dc_iterator_next (iterator, &descriptor) == DC_STATUS_SUCCESS) {
		const char *vendor = dc_descriptor_get_vendor(descriptor);
		const char *product = dc_descriptor_get_product(descriptor);
		add_dc(vendor, product, descriptor);
		if (product && strlen(product) > width)
			width = strlen(product);
	}
	dc_iterator_free(iterator);
	/* and add the Uemis Zurich which we are handling internally
	   THIS IS A HACK as we magically have a data structure here that
	   happens to match a data structure that is internal to libdivecomputer;
	   this WILL BREAK if libdivecomputer changes the dc_descriptor struct...
	   eventually the UEMIS code needs to move into libdivecomputer, I guess */
	mydescriptor = malloc(sizeof(struct mydescriptor));
	mydescriptor->vendor = "Uemis";
	mydescriptor->product = "Zurich";
	mydescriptor->type = DC_FAMILY_NULL;
	mydescriptor->model = 0;
	add_dc("Uemis", "Zurich", (dc_descriptor_t *)mydescriptor);
	dcl = dc_list;
	numvendor = 0;
	while (dcl) {
		numvendor++;
		dcl = dcl->next;
	}
	/* we need an extra vendor for the empty one */
	numvendor += 1;
	dcl = dc_list;
	i = 0;
	*vendor_index = *product_index = -1;
	if (*productstore)
		free(*productstore);
	pstores = *productstore = malloc(numvendor * sizeof(GtkListStore *));
	while (dcl) {
		gtk_list_store_append(vendorstore, &iter);
		gtk_list_store_set(vendorstore, &iter,
			0, dcl->vendor,
			-1);
		pl = dcl->productlist;
		pstores[i + 1] = gtk_list_store_new(1, G_TYPE_POINTER);
		j = 0;
		while (pl) {
			gtk_list_store_append(pstores[i + 1], &iter);
			gtk_list_store_set(pstores[i + 1], &iter,
				0, pl->descriptor,
				-1);
			if (is_default_dive_computer(dcl->vendor, pl->product)) {
				*vendor_index = i;
				*product_index = j;
			}
			j++;
			pl = pl->next;
		}
		i++;
		dcl = dcl->next;
	}
	/* now add the empty product list in case no vendor is selected */
	pstores[0] = gtk_list_store_new(1, G_TYPE_POINTER);

	return width;
}

void render_dc_vendor(GtkCellLayout *cell,
		GtkCellRenderer *renderer,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
	const char *vendor;

	gtk_tree_model_get(model, iter, 0, &vendor, -1);
	g_object_set(renderer, "text", vendor, NULL);
}

void render_dc_product(GtkCellLayout *cell,
		GtkCellRenderer *renderer,
		GtkTreeModel *model,
		GtkTreeIter *iter,
		gpointer data)
{
	dc_descriptor_t *descriptor = NULL;
	const char *product;

	gtk_tree_model_get(model, iter, 0, &descriptor, -1);
	product = dc_descriptor_get_product(descriptor);
	g_object_set(renderer, "text", product, NULL);
}

static void dive_computer_selector_changed(GtkWidget *combo, gpointer data)
{
	GtkWidget *import, *button;

	import = gtk_widget_get_ancestor(combo, GTK_TYPE_DIALOG);
	button = gtk_dialog_get_widget_for_response(GTK_DIALOG(import), GTK_RESPONSE_ACCEPT);
	gtk_widget_set_sensitive(button, TRUE);
}

static GtkListStore **product_model;
static void dive_computer_vendor_changed(GtkComboBox *vendorcombo, GtkComboBox *productcombo)
{
	int vendor = gtk_combo_box_get_active(vendorcombo);
	gtk_combo_box_set_model(productcombo, GTK_TREE_MODEL(product_model[vendor + 1]));
	gtk_combo_box_set_active(productcombo, -1);
}

static GtkComboBox *dive_computer_selector(GtkWidget *vbox)
{
	GtkWidget *hbox, *vendor_combo_box, *product_combo_box, *frame;
	GtkListStore *vendor_model;
	GtkCellRenderer *vendor_renderer, *product_renderer;
	int vendor_default_index, product_default_index, width;

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

	vendor_model = gtk_list_store_new(1, G_TYPE_POINTER);

	width = fill_computer_list(vendor_model, &product_model, &vendor_default_index, &product_default_index);

	frame = gtk_frame_new(_("Dive computer vendor and product"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 3);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vendor_combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(vendor_model));
	product_combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(product_model[vendor_default_index + 1]));

	g_signal_connect(G_OBJECT(vendor_combo_box), "changed", G_CALLBACK(dive_computer_vendor_changed), product_combo_box);
	g_signal_connect(G_OBJECT(product_combo_box), "changed", G_CALLBACK(dive_computer_selector_changed), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), vendor_combo_box, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox), product_combo_box, FALSE, FALSE, 3);

	vendor_renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(vendor_combo_box), vendor_renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(vendor_combo_box), vendor_renderer, render_dc_vendor, NULL, NULL);

	product_renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_fixed_size(product_renderer, 10 * width, -1);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(product_combo_box), product_renderer, TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(product_combo_box), product_renderer, render_dc_product, NULL, NULL);

	gtk_combo_box_set_active(GTK_COMBO_BOX(vendor_combo_box), vendor_default_index);
	gtk_combo_box_set_active(GTK_COMBO_BOX(product_combo_box), product_default_index);

	return GTK_COMBO_BOX(product_combo_box);
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

	if (default_index != -1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), default_index);
	else
		if (default_dive_computer_device)
			gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_box))),
		                   default_dive_computer_device);

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

	remember_tree_state();
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
			report_dives(TRUE, FALSE);
			g_slist_free(filenames);
		}
	}

	free(current_def_dir);
	gtk_widget_destroy(fs_dialog);
	restore_tree_state();
}

static GtkWidget *import_dive_computer(device_data_t *data, GtkDialog *dialog)
{
	GError *error;
	GtkWidget *vbox, *info, *container, *label, *button;

	/* HACK to simply include the Uemis Zurich in the list */
	if (! strcmp(data->vendor, "Uemis") && ! strcmp(data->product, "Zurich")) {
		error = uemis_download(data->devname, &data->progress, data->dialog, data->force_download);
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

/* this prevents clicking the [x] button, while the import thread is still running */
static void download_dialog_delete(GtkWidget *w, gpointer data)
{
	/* a no-op */
}

void download_dialog(GtkWidget *w, gpointer data)
{
	int result;
	char *devname, *ns, *ne;
	GtkWidget *dialog, *button, *hbox, *vbox, *label, *info = NULL;
	GtkComboBox *computer, *device;
	GtkTreeIter iter;
	device_data_t devicedata = {
		.devname = NULL,
	};

	remember_tree_state();
	dialog = gtk_dialog_new_with_buttons(_("Download From Dive Computer"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	g_signal_connect(dialog, "delete-event", G_CALLBACK(download_dialog_delete), NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	label = gtk_label_new(_(" Please select dive computer and device. "));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 3);
	computer = dive_computer_selector(vbox);
	device = dc_device_selector(vbox);
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);
	devicedata.progress.bar = gtk_progress_bar_new();
	gtk_container_add(GTK_CONTAINER(hbox), devicedata.progress.bar);

	force_download = FALSE;
	button = gtk_check_button_new_with_label(_("Force download of all dives"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(force_toggle), NULL);

	prefer_downloaded = FALSE;
	button = gtk_check_button_new_with_label(_("Always prefer downloaded dive"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(prefer_dl_toggle), NULL);

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
		/* once the accept event is triggered the dialog becomes non-modal.
		 * lets re-set that */
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
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
		set_default_dive_computer(vendor, product);

		/* get the device name from the combo box entry and set as default */
		devname = strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(device)))));
		set_default_dive_computer_device(devname);
		/* clear leading and trailing white space from the device name and also
		 * everything after (and including) the first '(' char. */
		ns = devname;
		while (*ns == ' ' || *ns == '\t')
			ns++;
		ne = ns;
		while (*ne && *ne != '(')
			ne++;
		*ne = '\0';
		if (ne > ns)
			while (*(--ne) == ' ' || *ne == '\t')
				*ne = '\0';
		devicedata.devname = ns;
		devicedata.dialog = GTK_DIALOG(dialog);
		devicedata.force_download = force_download;
		force_download = FALSE; /* when retrying we don't want to restart */
		info = import_dive_computer(&devicedata, GTK_DIALOG(dialog));
		free((void *)devname);
		if (info)
			goto repeat;
		report_dives(TRUE, prefer_downloaded);
		break;
	default:
		/* it's possible that some dives were downloaded */
		report_dives(TRUE, prefer_downloaded);
		break;
	}
	gtk_widget_destroy(dialog);
	restore_tree_state();
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
	else
		existing_filename = NULL;
}

/* just find the entry for this divecomputer */
static struct dcnicknamelist *get_dc_nicknameentry(const char *model, int deviceid)
{
	struct dcnicknamelist *known = nicknamelist;

	/* a 0 deviceid doesn't get a nickname - those come from development
	 * versions of Subsurface that didn't store the deviceid in the divecomputer entries */
	if (!deviceid)
		return NULL;
	if (!model)
		model = "";
	while (known) {
		if (!strcmp(known->model, model) && known->deviceid == deviceid)
			return known;
		known = known->next;
	}
	return NULL;
}

const char *get_dc_nickname(const char *model, uint32_t deviceid)
{
	struct dcnicknamelist *known = get_dc_nicknameentry(model, deviceid);
	if (known) {
		if (known->nickname && *known->nickname)
			return known->nickname;
		else
			return known->model;
	}
	return NULL;
}

gboolean dc_was_saved(struct divecomputer *dc)
{
	struct dcnicknamelist *nn_entry = get_dc_nicknameentry(dc->model, dc->deviceid);
	return nn_entry && nn_entry->saved;
}

void mark_dc_saved(struct divecomputer *dc)
{
	struct dcnicknamelist *nn_entry = get_dc_nicknameentry(dc->model, dc->deviceid);
	if (nn_entry)
		nn_entry->saved = TRUE;
}

void clear_dc_saved_status()
{
	struct dcnicknamelist *nn_entry = nicknamelist;

	while (nn_entry) {
		nn_entry->saved = FALSE;
		nn_entry = nn_entry->next;
	}
}

/* do we have a DIFFERENT divecomputer of the same model? */
static struct dcnicknamelist *get_different_dc_nicknameentry(const char *model, int deviceid)
{
	struct dcnicknamelist *known = nicknamelist;

	/* a 0 deviceid matches any DC of the same model - those come from development
	 * versions of Subsurface that didn't store the deviceid in the divecomputer entries */
	if (!deviceid)
		return NULL;
	if (!model)
		model = "";
	while (known) {
		if (known->model && !strcmp(known->model, model) &&
		    known->deviceid != deviceid)
			return known;
		known = known->next;
	}
	return NULL;
}

/* no curly braces or commas, please */
static char *cleanedup_nickname(const char *nickname, int len)
{
	char *clean;
	if (nickname) {
		char *brace;

		if (!g_utf8_validate(nickname, -1, NULL))
			return strdup("");
		brace = clean = strdup(nickname);
		while (*brace) {
			if (*brace == '{')
				*brace = '(';
			else if (*brace == '}')
				*brace = ')';
			else if (*brace == ',')
				*brace = '.';
			brace = g_utf8_next_char(brace);
			if (*brace && g_utf8_next_char(brace) - clean >= len)
				*brace = '\0';
		}
	} else {
		clean = strdup("");
	}
	return clean;
}

void replace_nickname_nicknamestring(const char *model, int deviceid, const char *nickname)
{
	char pattern[160];
	char *entry, *brace, *new_nn;
	int len;

	if (!nickname)
		nickname = "";
	snprintf(pattern, sizeof(pattern), "{%08x,%s", deviceid, model);
	entry = strstr(nicknamestring, pattern);
	if (!entry)
		/* this cannot happen as we know we have an entry for this deviceid */
		goto bail;

	len = strlen(entry);
	brace = g_utf8_strchr(entry, len, '}');
	if (!brace)
		goto bail;
	*entry =  *brace = '\0';
	len = strlen(nicknamestring) + strlen(brace + 1) + strlen(pattern) + strlen(nickname) + 3;
	new_nn = malloc(len);
	if (strlen(nickname))
		snprintf(new_nn, len, "%s%s,%s}%s", nicknamestring, pattern, nickname, brace + 1);
	else
		snprintf(new_nn, len, "%s%s}%s", nicknamestring, pattern, brace + 1);
	free(nicknamestring);
	nicknamestring = new_nn;
	return;

bail:
	printf("invalid nicknamestring %s (while looking at deviceid %08x\n", nicknamestring, deviceid);
	return;

}

void remove_dc(const char *model, uint32_t deviceid, gboolean change_conf)
{
	struct dcnicknamelist *nn_entry, **prevp = &nicknamelist;
	char pattern[160];
	char *nnstring, *brace;

	if (!deviceid || !model || !*model)
		return;
	nn_entry = get_dc_nicknameentry(model, deviceid);
	if (!nn_entry)
		return;
	while (prevp && *prevp != nn_entry)
		prevp = &(*prevp)->next;
	if (!prevp)
		/* that should be impossible */
		goto bail;
	(*prevp) = nn_entry->next;

	/* now remove it from the config string */
	snprintf(pattern, sizeof(pattern), "{%08x,%s", deviceid, model);
	nnstring = strstr(nicknamestring, pattern);
	if (!nnstring)
		goto bail;
	brace = strchr(nnstring, '}');
	if (!brace)
		goto bail;
	brace++;
	memmove(nnstring, brace, strlen(brace) + 1);

	if (change_conf)
		subsurface_set_conf("dc_nicknames", PREF_STRING, nicknamestring);

#if defined(NICKNAME_DEBUG)
	struct dcnicknamelist *nn_entry = nicknamelist;
	fprintf(debugfile, "nicknames:\n");
	while (nn_entry) {
		fprintf(debugfile, "id %8x model %s nickname %s\n", nn_entry->deviceid, nn_entry->model,
			nn_entry->nickname && *nn_entry->nickname ? nn_entry->nickname : "(none)");
		nn_entry = nn_entry->next;
	}
	fprintf(debugfile, "----------\n");
#endif

bail:
	free(nn_entry);
}

void remember_dc(const char *model, uint32_t deviceid, const char *nickname, gboolean change_conf)
{
	/* we don't want to record entries with a deviceid of 0; those are from earlier
	 * development versions of Subsurface before we stored the hash in the divecomputer
	 * entries... we don't know which actual divecomputer those entries are from */
	if (!deviceid)
		return;
	if (!nickname)
		nickname = "";
	if (!get_dc_nickname(model, deviceid)) {
		char buffer[160];
		struct dcnicknamelist *nn_entry = malloc(sizeof(struct dcnicknamelist));
		nn_entry->deviceid = deviceid;
		nn_entry->model = strdup(model);
		/* make sure there are no curly braces or commas in the string and that
		 * it will fit in the buffer */
		nn_entry->nickname = cleanedup_nickname(nickname, sizeof(buffer) - 13 - strlen(model));
		nn_entry->next = nicknamelist;
		nicknamelist = nn_entry;
		if (*nickname != '\0')
			snprintf(buffer, sizeof(buffer), "{%08x,%s,%s}", deviceid, model, nn_entry->nickname);
		else
			snprintf(buffer, sizeof(buffer), "{%08x,%s}", deviceid, model);
		nicknamestring = realloc(nicknamestring, strlen(nicknamestring) + strlen(buffer) + 1);
		strcat(nicknamestring, buffer);
	} else {
		/* modify existing entry */
		struct dcnicknamelist *nn_entry = get_dc_nicknameentry(model, deviceid);
		if (!nn_entry->model || !*nn_entry->model)
			nn_entry->model = model;
		nn_entry->nickname = cleanedup_nickname(nickname, 80);
		replace_nickname_nicknamestring(model, deviceid, nickname);
	}
	if (change_conf)
		subsurface_set_conf("dc_nicknames", PREF_STRING, nicknamestring);

#if defined(NICKNAME_DEBUG)
	struct dcnicknamelist *nn_entry = nicknamelist;
	fprintf(debugfile, "nicknames:\n");
	while (nn_entry) {
		fprintf(debugfile, "id %8x model %s nickname %s\n", nn_entry->deviceid, nn_entry->model,
			nn_entry->nickname && *nn_entry->nickname ? nn_entry->nickname : "(none)");
		nn_entry = nn_entry->next;
	}
	fprintf(debugfile, "----------\n");
#endif
}

void set_dc_nickname(struct dive *dive)
{
	GtkWidget *dialog, *vbox, *entry, *frame, *label;
	char nickname[160] = "";
	char dialogtext[1024];
	const char *name = nickname;
	struct divecomputer *dc = &dive->dc;

	if (!dive)
		return;
	while (dc) {
#if NICKNAME_DEBUG & 16
		fprintf(debugfile, "set_dc_nickname for model %s deviceid %8x\n", dc->model ? : "", dc->deviceid);
#endif
		if (get_dc_nickname(dc->model, dc->deviceid) == NULL) {
			struct dcnicknamelist *nn_entry = get_different_dc_nicknameentry(dc->model, dc->deviceid);
			if (!nn_entry) {
				/* just remember the dive computer without setting a nickname */
				if (dc->model)
					remember_dc(dc->model, dc->deviceid, "", TRUE);
			} else {
				dialog = gtk_dialog_new_with_buttons(
					_("Dive Computer Nickname"),
					GTK_WINDOW(main_window),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);
				vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
				snprintf(dialogtext, sizeof(dialogtext),
					 _("You already have a dive computer of this model\n"
					   "named %s\n"
					   "Subsurface can maintain a nickname for this device to "
					   "distinguish it from the existing one. "
					   "The default is the model and device ID as shown below.\n"
					   "If you don't want to name this dive computer click "
					   "'Cancel' and Subsurface will simply display its model "
					   "as its name (which may mean that you cannot tell the two "
					   "dive computers apart in the logs)."),
					   nn_entry->nickname && *nn_entry->nickname ? nn_entry->nickname :
					   (nn_entry->model && *nn_entry->model ? nn_entry->model : _("(nothing)")));
				label = gtk_label_new(dialogtext);
				gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
				gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 3);
				frame = gtk_frame_new(_("Nickname"));
				gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 3);
				entry = gtk_entry_new();
				gtk_container_add(GTK_CONTAINER(frame), entry);
				gtk_entry_set_max_length(GTK_ENTRY(entry), 68);
				snprintf(nickname, sizeof(nickname), "%s (%08x)", dc->model, dc->deviceid);
				gtk_entry_set_text(GTK_ENTRY(entry), nickname);
				gtk_widget_show_all(dialog);
				if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
					if (strcmp(dc->model, gtk_entry_get_text(GTK_ENTRY(entry))))
						name = cleanedup_nickname(gtk_entry_get_text(GTK_ENTRY(entry)),	sizeof(nickname));
				}
				gtk_widget_destroy(dialog);
				remember_dc(dc->model, dc->deviceid, name, TRUE);
			}
		}
		dc = dc->next;
	}
}
