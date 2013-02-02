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
#include "callbacks-gtk.h"
#include "uemis.h"
#include "device.h"
#include "webservice.h"

#include "libdivecomputer.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "subsurface-icon.h"

GtkWidget *main_window;
GtkWidget *main_vbox;
GtkWidget *error_info_bar;
GtkWidget *error_label;
GtkWidget *vpane, *hpane;
GtkWidget *notebook;

int        error_count;
const char *existing_filename;

static struct device_info *holdnicknames = NULL;
static GtkWidget *dive_profile_widget(void);
static void import_files(GtkWidget *, gpointer);

static void remember_dc(const char *model, uint32_t deviceid, const char *nickname)
{
	struct device_info *nn_entry;

	nn_entry = create_device_info(model, deviceid);
	if (!nn_entry)
		return;
	if (!nickname || !*nickname) {
		nn_entry->nickname = NULL;
		return;
	}
	nn_entry->nickname = strdup(nickname);
}

static void remove_dc(const char *model, uint32_t deviceid)
{
	free(remove_device_info(model, deviceid));
}

static GtkWidget *dive_profile;

GtkActionGroup *action_group;

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
		gtk_label_set_text(GTK_LABEL(error_label), buffer);
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
		const char *current_default = prefs.default_filename;
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

	current_default = prefs.default_filename;
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
	mark_divelist_changed(FALSE);

	/* clear the selection and the statistics */
	selected_dive = 0;
	process_selected_dives();
	clear_stats_widgets();
	clear_events();
	show_dive_stats(NULL);

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
	current_default = prefs.default_filename;
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

static void quit(GtkWidget *w, gpointer data)
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

GtkTreeViewColumn *tree_view_column_add_pixbuf(GtkWidget *tree_view, data_func_t data_func, GtkTreeViewColumn *col)
{
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, data_func, NULL, NULL);
	g_signal_connect(tree_view, "button-press-event", G_CALLBACK(icon_click_cb), col);
	return col;
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
	/* all but one column have only one renderer - so packing from the end
	 * makes no difference; for the location column we want to be able to
	 * prepend the icon in front of the text - so this works perfectly */
	gtk_tree_view_column_pack_end(col, renderer, TRUE);
	if (data_func)
		gtk_tree_view_column_set_cell_data_func(col, renderer, data_func, (void *)(long)index, NULL);
	else
		gtk_tree_view_column_add_attribute(col, renderer, "text", index);
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

/* Helper functions for gtk combo boxes */
GtkEntry *get_entry(GtkComboBox *combo_box)
{
	return GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_box)));
}

const char *get_active_text(GtkComboBox *combo_box)
{
	return gtk_entry_get_text(get_entry(combo_box));
}

void set_active_text(GtkComboBox *combo_box, const char *text)
{
	gtk_entry_set_text(get_entry(combo_box), text);
}

GtkWidget *combo_box_with_model_and_entry(GtkListStore *model)
{
	GtkWidget *widget;
	GtkEntryCompletion *completion;

	widget = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(model));
	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(widget), 0);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(model));
	gtk_entry_completion_set_inline_completion(completion, TRUE);
	gtk_entry_completion_set_inline_selection(completion, TRUE);
	gtk_entry_completion_set_popup_single_match(completion, FALSE);
	gtk_entry_set_completion(get_entry(GTK_COMBO_BOX(widget)), completion);
	g_object_unref(completion);

	return widget;
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

void update_screen()
{
	update_dive_list_units();
	repaint_dive();
	update_dive_list_col_visibility();
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
OPTIONCALLBACK(mod_toggle, prefs.mod)
OPTIONCALLBACK(ead_toggle, prefs.ead)
OPTIONCALLBACK(red_ceiling_toggle, prefs.profile_red_ceiling)
OPTIONCALLBACK(calc_ceiling_toggle, prefs.profile_calc_ceiling)
OPTIONCALLBACK(calc_ceiling_3m_toggle, prefs.calc_ceiling_3m_incr)

static gboolean gflow_edit(GtkWidget *w, GdkEvent *event, gpointer _data)
{
	double gflow;
	const char *buf;
	if (event->type == GDK_FOCUS_CHANGE) {
		buf = gtk_entry_get_text(GTK_ENTRY(w));
		sscanf(buf, "%lf", &gflow);
		prefs.gflow = gflow / 100.0;
		set_gf(prefs.gflow, -1.0);
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
		prefs.gfhigh = gfhigh / 100.0;
		set_gf(-1.0, prefs.gfhigh);
		update_screen();
	}
	return FALSE;
}

static void event_toggle(GtkWidget *w, gpointer _data)
{
	gboolean *plot_ev = _data;

	*plot_ev = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
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

	current_default = prefs.default_filename;
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
	gtk_widget_destroy(fs_dialog);

	gtk_widget_set_sensitive(parent, TRUE);
}

static void preferences_dialog(GtkWidget *w, gpointer data)
{
	int result;
	GtkWidget *dialog, *notebook, *font, *frame, *box, *vbox, *button, *xmlfile_button;
	GtkWidget *entry_po2, *entry_pn2, *entry_phe, *entry_mod, *entry_gflow, *entry_gfhigh;
	const char *current_default, *new_default;
	char threshold_text[10], mod_text[10], utf8_buf[128];
	struct preferences oldprefs = prefs;

	dialog = gtk_dialog_new_with_buttons(_("Preferences"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);

	/* create the notebook for the preferences and attach it to dialog */
	notebook = gtk_notebook_new();
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(vbox), notebook, FALSE, FALSE, 5);

	/* vbox that holds the first notebook page */
	vbox = gtk_vbox_new(FALSE, 6);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
				gtk_label_new(_("General Settings")));
	frame = gtk_frame_new(_("Units"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	create_radio(box, _("Depth:"),
		_("Meter"), set_meter, (prefs.units.length == METERS),
		_("Feet"),  set_feet, (prefs.units.length == FEET),
		NULL);

	create_radio(box, _("Pressure:"),
		_("Bar"), set_bar, (prefs.units.pressure == BAR),
		_("PSI"),  set_psi, (prefs.units.pressure == PSI),
		NULL);

	create_radio(box, _("Volume:"),
		_("Liter"),  set_liter, (prefs.units.volume == LITER),
		_("CuFt"), set_cuft, (prefs.units.volume == CUFT),
		NULL);

	create_radio(box, _("Temperature:"),
		_("Celsius"), set_celsius, (prefs.units.temperature == CELSIUS),
		_("Fahrenheit"),  set_fahrenheit, (prefs.units.temperature == FAHRENHEIT),
		NULL);

	create_radio(box, _("Weight:"),
		_("kg"), set_kg, (prefs.units.weight == KG),
		_("lbs"),  set_lbs, (prefs.units.weight == LBS),
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

	font = gtk_font_button_new_with_font(prefs.divelist_font);
	gtk_container_add(GTK_CONTAINER(frame),font);

	frame = gtk_frame_new(_("Misc. Options"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);

	frame = gtk_frame_new(_("Default XML Data File"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 5);
	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(frame), box);
	current_default = prefs.default_filename;
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

	button = gtk_check_button_new_with_label(_("Show MOD"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.mod);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(mod_toggle), &entry_mod);

	frame = gtk_frame_new(_("max ppO2"));
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 6);
	entry_mod = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY(entry_mod), 4);
	snprintf(mod_text, sizeof(mod_text), "%.1f", prefs.mod_ppO2);
	gtk_entry_set_text(GTK_ENTRY(entry_mod), mod_text);
	gtk_widget_set_sensitive(entry_mod, prefs.mod);
	gtk_container_add(GTK_CONTAINER(frame), entry_mod);

	box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(vbox), box);

	button = gtk_check_button_new_with_label(_("Show EAD, END, EADD"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), prefs.ead);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ead_toggle), NULL);

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
		const char *po2_threshold_text, *pn2_threshold_text, *phe_threshold_text, *mod_text, *gflow_text, *gfhigh_text;
		/* Make sure to flush any modified old dive data with old units */
		update_dive(NULL);

		prefs.divelist_font = strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(font)));
		set_divelist_font(prefs.divelist_font);
		po2_threshold_text = gtk_entry_get_text(GTK_ENTRY(entry_po2));
		sscanf(po2_threshold_text, "%lf", &prefs.pp_graphs.po2_threshold);
		pn2_threshold_text = gtk_entry_get_text(GTK_ENTRY(entry_pn2));
		sscanf(pn2_threshold_text, "%lf", &prefs.pp_graphs.pn2_threshold);
		phe_threshold_text = gtk_entry_get_text(GTK_ENTRY(entry_phe));
		sscanf(phe_threshold_text, "%lf", &prefs.pp_graphs.phe_threshold);
		mod_text = gtk_entry_get_text(GTK_ENTRY(entry_mod));
		sscanf(mod_text, "%lf", &prefs.mod_ppO2);
		gflow_text = gtk_entry_get_text(GTK_ENTRY(entry_gflow));
		sscanf(gflow_text, "%lf", &prefs.gflow);
		gfhigh_text = gtk_entry_get_text(GTK_ENTRY(entry_gfhigh));
		sscanf(gfhigh_text, "%lf", &prefs.gfhigh);
		prefs.gflow /= 100.0;
		prefs.gfhigh /= 100.0;
		set_gf(prefs.gflow, prefs.gfhigh);

		update_screen();

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
			prefs.default_filename = new_default;
		}

		save_preferences();
	} else if (result == GTK_RESPONSE_CANCEL) {
		prefs = oldprefs;
		set_gf(prefs.gflow, prefs.gfhigh);
		update_screen();
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

static void about_dialog_link_cb(GtkAboutDialog *dialog, const gchar *link, gpointer data)
{
	subsurface_launch_for_uri(link);
}

static void about_dialog(GtkWidget *w, gpointer data)
{
	const char *logo_property = NULL;
	GdkPixbuf *logo = NULL;
	GtkWidget * dialog;

	if (need_icon) {
		logo_property = "logo";
		logo = gdk_pixbuf_from_pixdata(&subsurface_icon_pixbuf, TRUE, NULL);
	}
	dialog = gtk_about_dialog_new();
#if !GTK_CHECK_VERSION(2,24,0) /* F*cking gtk */
	gtk_about_dialog_set_url_hook(about_dialog_link_cb, NULL, NULL);
#else
	g_signal_connect(GTK_ABOUT_DIALOG(dialog), "activate-link", G_CALLBACK(about_dialog_link_cb), NULL);
#endif
	gtk_show_about_dialog(NULL,
		"title", _("About Subsurface"),
		"program-name", "Subsurface",
		"comments", _("Multi-platform divelog software in C"),
		"website", "http://subsurface.hohndel.org",
		"license", "GNU General Public License, version 2\nhttp://www.gnu.org/licenses/old-licenses/gpl-2.0.html",
		"version", VERSION_STRING,
		"copyright", _("Linus Torvalds, Dirk Hohndel, and others, 2011, 2012, 2013"),
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


/* list columns for nickname edit treeview */
enum {
	NE_MODEL,
	NE_ID_STR,
	NE_NICKNAME,
	NE_NCOL
};

/* delete a selection of nicknames */
static void edit_dc_delete_rows(GtkTreeView *view)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	GtkTreeRowReference *ref;
	GtkTreeSelection *selection;
	GList *selected_rows, *list, *row_references = NULL;
	guint len;
	/* params for delete op */
	const char *model_str;
	const char *deviceid_string; /* convert to deviceid */
	uint32_t deviceid;

	selection = gtk_tree_view_get_selection(view);
	selected_rows = gtk_tree_selection_get_selected_rows(selection, &model);

	for (list = selected_rows; list; list = g_list_next(list)) {
		path = list->data;
		ref = gtk_tree_row_reference_new(model, path);
		row_references = g_list_append(row_references, ref);
	}
	len = g_list_length(row_references);
	if (len == 0)
		/* Warn about empty selection? */
		return;

	for (list = row_references; list; list = g_list_next(list)) {
		path = gtk_tree_row_reference_get_path((GtkTreeRowReference *)(list->data));
		gtk_tree_model_get_iter(model, &iter, path);
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
					NE_MODEL, &model_str,
					NE_ID_STR, &deviceid_string,
					-1);
		if (sscanf(deviceid_string, "0x%x8", &deviceid) == 1)
			remove_dc(model_str,  deviceid);

		gtk_list_store_remove(GTK_LIST_STORE (model), &iter);
		gtk_tree_path_free(path);
	}
	g_list_free(selected_rows);
	g_list_free(row_references);
	g_list_free(list);

	if (gtk_tree_model_get_iter_first(model, &iter))
		gtk_tree_selection_select_iter(selection, &iter);
}

/* repopulate the edited nickname cell of a DC */
static void cell_edited_cb(GtkCellRendererText *cell, gchar *path,
				gchar *new_text, gpointer store)
{
	GtkTreeIter iter;
	const char *model;
	const char *deviceid_string;
	uint32_t deviceid;
	int matched;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path);
	/* display new text */
	gtk_list_store_set(GTK_LIST_STORE(store), &iter, NE_NICKNAME, new_text, -1);
	/* and new_text */
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
					  NE_MODEL, &model,
					  NE_ID_STR, &deviceid_string,
					  -1);
	/* extract deviceid */
	matched = sscanf(deviceid_string, "0x%x8", &deviceid);

	/* remember pending commit
	 * Need to extend list rather than wipe and store only one result */
	if (matched == 1){
		if (holdnicknames == NULL){
			holdnicknames = (struct device_info *) malloc(sizeof(struct device_info));
			holdnicknames->model = strdup(model);
			holdnicknames->deviceid = deviceid;
			holdnicknames->serial_nr = NULL;
			holdnicknames->firmware = NULL;
			holdnicknames->nickname = strdup(new_text);
			holdnicknames->next = NULL;
		} else {
			struct device_info * top;
			struct device_info * last = holdnicknames;
			top = (struct device_info *) malloc(sizeof(struct device_info));
			top->model = strdup(model);
			top->deviceid = deviceid;
			top->serial_nr = NULL;
			top->firmware = NULL;
			top->nickname = strdup(new_text);
			top->next = last;
			holdnicknames = top;
		}
	}
}

#define SUB_RESPONSE_DELETE 1	/* no delete response in gtk+2 */
#define SUB_DONE 2		/* enable escape when done */

/* show the dialog to edit dc nicknames */
static void edit_dc_nicknames(GtkWidget *w, gpointer data)
{
	const gchar *C_INACTIVE = "#e8e8ee", *C_ACTIVE = "#ffffff";	/* cell colours */
	GtkWidget *dialog, *confirm, *view, *scroll, *vbox;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;
	gint res = -1;
	char id_string[11] = {0};
	struct device_info * nnl;

	dialog = gtk_dialog_new_with_buttons(_("Edit Dive Computer Nicknames"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_DELETE,
		SUB_RESPONSE_DELETE,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_CANCEL,
		GTK_STOCK_APPLY,
		GTK_RESPONSE_APPLY,
		NULL);
	gtk_widget_set_size_request(dialog, 700, 400);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	view = gtk_tree_view_new();
	store = gtk_list_store_new(NE_NCOL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);

	/* columns */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "background", C_INACTIVE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, _("Model"),
							renderer, "text", NE_MODEL, NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "background", C_INACTIVE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, _("Device Id"),
							renderer, "text", NE_ID_STR, NULL);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "background", C_INACTIVE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(view), -1, _("Nickname"),
							renderer, "text", NE_NICKNAME, NULL);
	g_object_set(renderer, "editable", TRUE, NULL);
	g_object_set(renderer, "background", C_ACTIVE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(cell_edited_cb), store);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model);

	/* populate list store from device_info_list */
	nnl = head_of_device_info_list();
	while (nnl) {
		sprintf(&id_string[0], "%#08x", nnl->deviceid);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
				NE_MODEL, nnl->model,
				NE_ID_STR, id_string,
				NE_NICKNAME, nnl->nickname,
				-1);
		nnl = nnl->next;
	}

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_SINGLE);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(scroll), view);
	gtk_container_add(GTK_CONTAINER(vbox),
			gtk_label_new(_("Edit a dive computer nickname by double-clicking the in the relevant nickname field")));
	gtk_container_add(GTK_CONTAINER(vbox), scroll);
	gtk_widget_set_size_request(scroll, 500, 300);
	gtk_widget_show_all(dialog);

	do {
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		if (res == SUB_RESPONSE_DELETE) {
			confirm = gtk_dialog_new_with_buttons(_("Delete a dive computer information entry"),
							GTK_WINDOW(dialog),
							GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_STOCK_YES,
							GTK_RESPONSE_YES,
							GTK_STOCK_NO,
							GTK_RESPONSE_NO,
							NULL);
			gtk_widget_set_size_request(confirm, 350, 90);
			vbox = gtk_dialog_get_content_area(GTK_DIALOG(confirm));
			gtk_container_add(GTK_CONTAINER(vbox),
					gtk_label_new(_("Ok to delete the selected entry?")));
			gtk_widget_show_all(confirm);
			if (gtk_dialog_run(GTK_DIALOG(confirm)) == GTK_RESPONSE_YES) {
				edit_dc_delete_rows(GTK_TREE_VIEW(view));
				res = SUB_DONE; /* want to close ** both ** dialogs now */
			}
			mark_divelist_changed(TRUE);
			gtk_widget_destroy(confirm);
		}
		if (res == GTK_RESPONSE_APPLY && holdnicknames && holdnicknames->model != NULL) {
			struct device_info * walk = holdnicknames;
			struct device_info * release = holdnicknames;
			struct device_info * track = holdnicknames->next;
			while (walk) {
				remember_dc(walk->model, walk->deviceid, walk->nickname);
				walk = walk->next;
			}
			/* clear down list */
			while (release){
				free(release);
				release = track;
				if (track)
					track = track->next;
			}
			holdnicknames = NULL;
			mark_divelist_changed(TRUE);
		}
	} while (res != SUB_DONE && res != GTK_RESPONSE_CANCEL && res != GTK_RESPONSE_DELETE_EVENT && res != GTK_RESPONSE_APPLY);
	gtk_widget_destroy(dialog);
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
	{ "DownloadWeb",    GTK_STOCK_CONNECT, N_("Download From Web Service..."), NULL, NULL, G_CALLBACK(webservice_download_dialog) },
	{ "AddDive",        GTK_STOCK_ADD, N_("Add Dive..."), NULL, NULL, G_CALLBACK(add_dive_cb) },
	{ "Preferences",    GTK_STOCK_PREFERENCES, N_("Preferences..."), PREFERENCE_ACCEL, NULL, G_CALLBACK(preferences_dialog) },
	{ "Renumber",       NULL, N_("Renumber..."), NULL, NULL, G_CALLBACK(renumber_dialog) },
	{ "YearlyStats",    NULL, N_("Yearly Statistics"), NULL, NULL, G_CALLBACK(show_yearly_stats) },
#if HAVE_OSM_GPS_MAP
	{ "DivesLocations", NULL, N_("Dives Locations"), CTRLCHAR "M", NULL, G_CALLBACK(show_gps_locations) },
#endif
	{ "SelectEvents",   NULL, N_("Select Events..."), NULL, NULL, G_CALLBACK(selectevents_dialog) },
	{ "Quit",           GTK_STOCK_QUIT, N_("Quit"),   CTRLCHAR "Q", NULL, G_CALLBACK(quit) },
	{ "About",          GTK_STOCK_ABOUT, N_("About Subsurface"),  NULL, NULL, G_CALLBACK(about_dialog) },
	{ "ViewList",       NULL, N_("List"),  CTRLCHAR "1", NULL, G_CALLBACK(view_list) },
	{ "ViewProfile",    NULL, N_("Profile"), CTRLCHAR "2", NULL, G_CALLBACK(view_profile) },
	{ "ViewInfo",       NULL, N_("Info"), CTRLCHAR "3", NULL, G_CALLBACK(view_info) },
	{ "ViewThree",      NULL, N_("Three"), CTRLCHAR "4", NULL, G_CALLBACK(view_three) },
	{ "EditNames",      NULL, N_("Edit Device Names"), CTRLCHAR "E", NULL, G_CALLBACK(edit_dc_nicknames) },
	{ "PrevDC",         NULL, N_("Prev DC"), NULL, NULL, G_CALLBACK(prev_dc) },
	{ "NextDC",         NULL, N_("Next DC"), NULL, NULL, G_CALLBACK(next_dc) },
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
				<menuitem name=\"Download From Web Service\" action=\"DownloadWeb\" /> \
				<menuitem name=\"Edit Dive Computer Names\" action=\"EditNames\" /> \
				<separator name=\"Separator1\"/> \
				<menuitem name=\"Add Dive\" action=\"AddDive\" /> \
				<separator name=\"Separator2\"/> \
				<menuitem name=\"Renumber\" action=\"Renumber\" /> \
				<menuitem name=\"Autogroup\" action=\"Autogroup\" /> \
				<menuitem name=\"Toggle Zoom\" action=\"ToggleZoom\" /> \
				<menuitem name=\"YearlyStats\" action=\"YearlyStats\" /> "
#if HAVE_OSM_GPS_MAP
				"<menuitem name=\"DivesLocations\" action=\"DivesLocations\" /> "
#endif
				"<menu name=\"View\" action=\"ViewMenuAction\"> \
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

static gboolean notebook_tooltip (GtkWidget *widget, gint x, gint y,
			gboolean keyboard_mode, GtkTooltip *tooltip, gpointer data)
{
	if (amount_selected > 0 && gtk_notebook_get_current_page(GTK_NOTEBOOK(widget)) == 0) {
		gtk_tooltip_set_text(tooltip, _("To edit dive information\ndouble click on it in the dive list"));
		return TRUE;
	} else {
		return FALSE;
	}
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

	load_preferences();

	default_dive_computer_vendor = subsurface_get_conf("dive_computer_vendor");
	default_dive_computer_product = subsurface_get_conf("dive_computer_product");
	default_dive_computer_device = subsurface_get_conf("dive_computer_device");
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

	/* add tooltip that tells people how to edit things */
	g_object_set(notebook, "has-tooltip", TRUE, NULL);
	g_signal_connect(notebook, "query-tooltip", G_CALLBACK(notebook_tooltip), NULL);

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
	if (amount_selected == 0 || gc->pi.nr == 0)
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

static void common_drawing_function(GtkWidget *widget, struct graphics_context *gc)
{
	int i = 0;
	struct dive *dive = current_dive;

	gc->drawing_area.x = MIN(50,gc->drawing_area.width / 20.0);
	gc->drawing_area.y = MIN(50,gc->drawing_area.height / 20.0);

	g_object_set(widget, "has-tooltip", TRUE, NULL);
	g_signal_connect(widget, "query-tooltip", G_CALLBACK(profile_tooltip), gc);
	init_profile_background(gc);
	cairo_paint(gc->cr);

	if (zoom_factor > 1.0) {
		double n = -(zoom_factor-1);
		cairo_translate(gc->cr, n*zoom_x, n*zoom_y);
		cairo_scale(gc->cr, zoom_factor, zoom_factor);
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
		plot(gc, dive, SC_SCREEN);
	}
}

#if GTK_CHECK_VERSION(3,0,0)

static gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	guint width, height;
	static struct graphics_context gc = { .printer = 0 };

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	gc.drawing_area.width = width;
	gc.drawing_area.height = height;
	gc.cr = cr;

	common_drawing_function(widget, &gc);
	return FALSE;
}

#else /* gtk2 */

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	GtkAllocation allocation;
	static struct graphics_context gc = { .printer = 0 };

	/* the drawing area gives TOTAL width * height - x,y is used as the topx/topy offset
	 * so effective drawing area is width-2x * height-2y */
	gtk_widget_get_allocation(widget, &allocation);
	gc.drawing_area.width = allocation.width;
	gc.drawing_area.height = allocation.height;
	gc.cr = gdk_cairo_create(gtk_widget_get_window(widget));

	common_drawing_function(widget, &gc);
	cairo_destroy(gc.cr);
	return FALSE;
}

#endif

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

static gboolean scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
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

static gboolean clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
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

static gboolean released(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
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

static gboolean motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
	if (zoom_x < 0)
		return TRUE;

	zoom_x = event->x;
	zoom_y = event->y;
	gtk_widget_queue_draw(widget);
	return TRUE;
}

static GtkWidget *dive_profile_widget(void)
{
	GtkWidget *da;

	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 350, 250);
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(da, "draw", G_CALLBACK (draw_callback), NULL);
#else
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);
#endif
	g_signal_connect(da, "button-press-event", G_CALLBACK(clicked), NULL);
	g_signal_connect(da, "scroll-event", G_CALLBACK(scroll_event), NULL);
	g_signal_connect(da, "button-release-event", G_CALLBACK(released), NULL);
	g_signal_connect(da, "motion-notify-event", G_CALLBACK(motion), NULL);
	gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK);

	return da;
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

static void import_files(GtkWidget *w, gpointer data)
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
		current_default = prefs.default_filename;
		current_def_dir = g_path_get_dirname(current_default);
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

const char *get_dc_nickname(const char *model, uint32_t deviceid)
{
	struct device_info *known = get_device_info(model, deviceid);
	if (known) {
		if (known->nickname && *known->nickname)
			return known->nickname;
		else
			return known->model;
	}
	return NULL;
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
			struct device_info *nn_entry = get_different_device_info(dc->model, dc->deviceid);
			if (nn_entry) {
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
					if (strcmp(dc->model, gtk_entry_get_text(GTK_ENTRY(entry)))) {
						name = gtk_entry_get_text(GTK_ENTRY(entry));
						remember_dc(dc->model, dc->deviceid, name);
						mark_divelist_changed(TRUE);
					}
				} else {
					/* Remember that we declined the nickname */
					remember_dc(dc->model, dc->deviceid, NULL);
				}
				gtk_widget_destroy(dialog);
			}
		}
		dc = dc->next;
	}
}
