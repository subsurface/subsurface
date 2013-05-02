/* planner.c
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */
#include <libintl.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <ctype.h>
#include "dive.h"
#include "divelist.h"
#include "display-gtk.h"
#include "planner.h"

GtkWidget *planner, *planner_error_bar, *error_label;

static void on_error_bar_response(GtkWidget *widget, gint response, gpointer data)
{
	if (response == GTK_RESPONSE_OK)
	{
		gtk_widget_destroy(widget);
		planner_error_bar = NULL;
		error_label = NULL;
	}
}

static void show_error(const char *fmt, ...)
{
	va_list args;
	GError *error;
	GtkWidget *box, *container;
	gboolean bar_is_visible = TRUE;

	va_start(args, fmt);
	error = g_error_new_valist(g_quark_from_string("subsurface"), DIVE_ERROR_PLAN, fmt, args);
	va_end(args);
	if (!planner_error_bar) {
		planner_error_bar = gtk_info_bar_new_with_buttons(GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
		g_signal_connect(planner_error_bar, "response", G_CALLBACK(on_error_bar_response), NULL);
		gtk_info_bar_set_message_type(GTK_INFO_BAR(planner_error_bar), GTK_MESSAGE_ERROR);
		bar_is_visible = FALSE;
	}
	container = gtk_info_bar_get_content_area(GTK_INFO_BAR(planner_error_bar));
	if (error_label)
		gtk_container_remove(GTK_CONTAINER(container), error_label);
	error_label = gtk_label_new(error->message);
	gtk_container_add(GTK_CONTAINER(container), error_label);
	box = gtk_dialog_get_content_area(GTK_DIALOG(planner));
	if (!bar_is_visible)
		gtk_box_pack_start(GTK_BOX(box), planner_error_bar, FALSE, FALSE, 0);
	gtk_widget_show_all(box);
	/* make sure this actually gets shown BEFORE the calculations run */
	while (gtk_events_pending())
		gtk_main_iteration_do(FALSE);
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

#define MAX_WAYPOINTS 12
GtkWidget *entry_depth[MAX_WAYPOINTS], *entry_duration[MAX_WAYPOINTS], *entry_gas[MAX_WAYPOINTS], *entry_po2[MAX_WAYPOINTS];
int nr_waypoints = 0;
static GtkListStore *gas_model = NULL;

static gboolean gas_focus_out_cb(GtkWidget *entry, GdkEvent *event, gpointer data)
{
	const char *gastext;
	int o2, he;
	int idx = data - NULL;
	char *error_string = NULL;

	gastext = gtk_entry_get_text(GTK_ENTRY(entry));
	o2 = he = 0;
	if (validate_gas(gastext, &o2, &he))
		add_string_list_entry(gastext, gas_model);
	add_gas_to_nth_dp(&diveplan, idx, o2, he);
	show_planned_dive(&error_string);
	if (error_string)
		show_error(error_string);
	return FALSE;
}

static void gas_changed_cb(GtkWidget *combo, gpointer data)
{
	const char *gastext;
	int o2, he;
	int idx = data - NULL;
	char *error_string = NULL;

	gastext = get_active_text(GTK_COMBO_BOX(combo));
	/* stupidly this gets called for two reasons:
	 * a) any keystroke into the entry field
	 * b) mouse selection of a dropdown
	 * we only care about b) (a) is handled much better with the focus-out event)
	 * so let's check that the text returned is actually in our model before going on
	 */
	if (match_list(gas_model, gastext) != MATCH_EXACT)
		return;
	o2 = he = 0;
	if (!validate_gas(gastext, &o2, &he)) {
		/* this should never happen as only validated texts should be
		 * in the dropdown */
		show_error(_("Invalid gas for row %d"),idx);
	}
	add_gas_to_nth_dp(&diveplan, idx, o2, he);
	show_planned_dive(&error_string);
	if (error_string)
		show_error(error_string);
}

static gboolean depth_focus_out_cb(GtkWidget *entry, GdkEvent *event, gpointer data)
{
	const char *depthtext;
	int depth = -1;
	int idx = data - NULL;
	char *error_string = NULL;

	depthtext = gtk_entry_get_text(GTK_ENTRY(entry));

	if (validate_depth(depthtext, &depth)) {
		if (depth > 150000)
			show_error(_("Warning - planning very deep dives can take excessive amounts of time"));
		add_depth_to_nth_dp(&diveplan, idx, depth);
		show_planned_dive(&error_string);
		if (error_string)
			show_error(error_string);
	} else {
		/* it might be better to instead change the color of the input field or something */
		if (depth == -1)
			show_error(_("Invalid depth - could not parse \"%s\""), depthtext);
		else
			show_error(_("Invalid depth - values deeper than 400m not supported"));
	}
	return FALSE;
}

static gboolean duration_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *durationtext;
	int duration, is_rel;
	int idx = data - NULL;
	char *error_string = NULL;

	durationtext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_time(durationtext, &duration, &is_rel))
		if (add_duration_to_nth_dp(&diveplan, idx, duration, is_rel) < 0)
			show_error(_("Warning - extremely long dives can cause long calculation time"));
	show_planned_dive(&error_string);
	if (error_string)
		show_error(error_string);
	return FALSE;
}

static gboolean po2_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *po2text;
	int po2;
	int idx = data - NULL;
	char *error_string = NULL;

	po2text = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_po2(po2text, &po2))
		add_po2_to_nth_dp(&diveplan, idx, po2);
	show_planned_dive(&error_string);
	if (error_string)
		show_error(error_string);
	return FALSE;
}

static gboolean starttime_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *starttimetext;
	int starttime, is_rel;
	char *error_string = NULL;

	starttimetext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_time(starttimetext, &starttime, &is_rel)) {
		/* we alway make this relative - either from the current time or from the
		 * end of the last dive, whichever is later */
		timestamp_t cur = current_time_notz();
		if (diveplan.lastdive_nr >= 0) {
			struct dive *last_dive = get_dive(diveplan.lastdive_nr);
			if (last_dive && last_dive->when + last_dive->dc.duration.seconds > cur)
				cur = last_dive->when + last_dive->dc.duration.seconds;
		}
		diveplan.when = cur + starttime;
		show_planned_dive(&error_string);
		if (error_string)
			show_error(error_string);
	} else {
		/* it might be better to instead change the color of the input field or something */
		show_error(_("Invalid starttime"));
	}
	return FALSE;
}

static gboolean surfpres_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *surfprestext;
	char *error_string = NULL;

	surfprestext = gtk_entry_get_text(GTK_ENTRY(entry));
	diveplan.surface_pressure = atoi(surfprestext);
	show_planned_dive(&error_string);
	if (error_string)
		show_error(error_string);
	return FALSE;
}

static gboolean sac_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *sactext;
	char *error_string = NULL;

	sactext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (validate_volume(sactext, data)) {
		show_planned_dive(&error_string);
		if (error_string)
			show_error(error_string);
	}
	return FALSE;
}

static gboolean gf_focus_out_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	const char *gftext;
	int gf;
	double *gfp = data;
	char *error_string = NULL;

	gftext = gtk_entry_get_text(GTK_ENTRY(entry));
	if (sscanf(gftext, "%d", &gf) == 1) {
		*gfp = gf / 100.0;
		show_planned_dive(&error_string);
		if (error_string)
			show_error(error_string);
	}
	return FALSE;
}

static gboolean last_stop_toggled_cb(GtkWidget *entry, GdkEvent * event, gpointer data)
{
	char *error_string = NULL;
	set_last_stop(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(entry)));
	show_planned_dive(&error_string);
	if (error_string)
		show_error(error_string);
	return FALSE;
}

static GtkWidget *add_gas_combobox_to_box(GtkWidget *box, const char *label, int idx)
{
	GtkWidget *frame, *combo;

	if (!gas_model) {
		gas_model = gtk_list_store_new(1, G_TYPE_STRING);
		add_string_list_entry(_("AIR"), gas_model);
		add_string_list_entry(_("EAN32"), gas_model);
		add_string_list_entry(_("EAN36"), gas_model);
	}
	combo = combo_box_with_model_and_entry(gas_model);
	gtk_widget_add_events(combo, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(gtk_bin_get_child(GTK_BIN(combo)), "focus-out-event", G_CALLBACK(gas_focus_out_cb), NULL + idx);
	g_signal_connect(combo, "changed", G_CALLBACK(gas_changed_cb), NULL + idx);
	if (label) {
		frame = gtk_frame_new(label);
		gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frame), combo);
	} else {
		gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 2);
	}

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
		entry_gas[idx] = add_gas_combobox_to_box(hbox, C_("Type of","Gas Used"), idx);
		entry_po2[idx] = add_entry_to_box(hbox, _("CC SetPoint"));
	} else {
		entry_depth[idx] = add_entry_to_box(hbox, NULL);
		entry_duration[idx] = add_entry_to_box(hbox, NULL);
		entry_gas[idx] = add_gas_combobox_to_box(hbox, NULL, idx);
		entry_po2[idx] = add_entry_to_box(hbox, NULL);
	}
	gtk_widget_add_events(entry_depth[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_depth[idx], "focus-out-event", G_CALLBACK(depth_focus_out_cb), NULL + idx);
	gtk_widget_add_events(entry_duration[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_duration[idx], "focus-out-event", G_CALLBACK(duration_focus_out_cb), NULL + idx);
	gtk_widget_add_events(entry_po2[idx], GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry_po2[idx], "focus-out-event", G_CALLBACK(po2_focus_out_cb), NULL + idx);
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
		show_error(_("Too many waypoints"));
	}
}

static void add_entry_with_callback(GtkWidget *box, int length, char *label, char *initialtext,
				gboolean (*callback)(GtkWidget *, GdkEvent *, gpointer), gpointer data)
{
	GtkWidget *entry = add_entry_to_box(box, label);
	gtk_entry_set_max_length(GTK_ENTRY(entry), length);
	gtk_entry_set_text(GTK_ENTRY(entry), initialtext);
	gtk_widget_add_events(entry, GDK_FOCUS_CHANGE_MASK);
	g_signal_connect(entry, "focus-out-event", G_CALLBACK(callback), data);
}

/* set up the dialog where the user can input their dive plan */
void input_plan()
{
	GtkWidget *content, *vbox, *hbox, *outervbox, *add_row, *label;
	char *bottom_sac, *deco_sac, gflowstring[4], gfhighstring[4];
	char *explanationtext = _("<small>Add segments below.\nEach line describes part of the planned dive.\n"
			"An entry with depth, time and gas describes a segment that ends "
			"at the given depth, takes the given time (if relative, e.g. '+3:30') "
			"or ends at the given time (if absolute e.g '@5:00', 'runtime'), and uses the given gas.\n"
			"An empty gas means 'use previous gas' (or AIR if no gas was specified).\n"
			"An entry that has a depth and a gas given but no time is special; it "
			"informs the planner that the gas specified is available for the ascent "
			"once the depth given has been reached.\n"
			"CC SetPoint specifies CC (rebreather) dives, leave empty for OC.</small>\n");
	char *labeltext;
	char *error_string = NULL;
	int len;

	disclaimer = _("DISCLAIMER / WARNING: THIS IS A NEW IMPLEMENTATION OF THE BUHLMANN "
			"ALGORITHM AND A DIVE PLANNER IMPLEMENTION BASED ON THAT WHICH HAS "
			"RECEIVED ONLY A LIMITED AMOUNT OF TESTING. WE STRONGLY RECOMMEND NOT TO "
			"PLAN DIVES SIMPLY BASED ON THE RESULTS GIVEN HERE.");
	if (diveplan.dp)
		free_dps(diveplan.dp);
	memset(&diveplan, 0, sizeof(diveplan));
	diveplan.lastdive_nr = dive_table.nr - 1;
	free(cache_data);
	cache_data = NULL;
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

	len = strlen(explanationtext) + strlen(disclaimer) + sizeof("<span foreground='red'></span>");
	labeltext = malloc(len);
	snprintf(labeltext, len, "%s<span foreground='red'>%s</span>", explanationtext, disclaimer);
	label = gtk_label_new(labeltext);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_width_chars(GTK_LABEL(label), 60);
	gtk_box_pack_start(GTK_BOX(outervbox), label, TRUE, TRUE, 0);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(outervbox), vbox, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	add_entry_with_callback(hbox, 12, _("Dive starts when?"), "+60:00", starttime_focus_out_cb, NULL);
	add_entry_with_callback(hbox, 12, _("Surface Pressure (mbar)"), SURFACE_PRESSURE_STRING, surfpres_focus_out_cb, NULL);

	if (get_units()->length == METERS)
		labeltext = _("Last stop at 6 Meters");
	else
		labeltext = _("Last stop at 20 Feet");

	set_last_stop(FALSE);
	content = gtk_check_button_new_with_label(labeltext);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(content), 0);
	gtk_box_pack_start(GTK_BOX(hbox), content, FALSE, FALSE, 6);
	g_signal_connect(G_OBJECT(content), "toggled", G_CALLBACK(last_stop_toggled_cb), NULL);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	if (get_units()->volume == CUFT) {
		bottom_sac = _("0.7 cuft/min");
		deco_sac = _("0.6 cuft/min");
		diveplan.bottomsac = 1000 * cuft_to_l(0.7);
		diveplan.decosac = 1000 * cuft_to_l(0.6);
	} else {
		bottom_sac = _("20 l/min");
		deco_sac = _("17 l/min");
		diveplan.bottomsac = 20000;
		diveplan.decosac = 17000;
	}
	add_entry_with_callback(hbox, 12, _("SAC during dive"), bottom_sac, sac_focus_out_cb, &diveplan.bottomsac);
	add_entry_with_callback(hbox, 12, _("SAC during decostop"), deco_sac, sac_focus_out_cb, &diveplan.decosac);
	plangflow = prefs.gflow;
	plangfhigh = prefs.gfhigh;
	snprintf(gflowstring, sizeof(gflowstring), "%3.0f", 100 * plangflow);
	snprintf(gfhighstring, sizeof(gflowstring), "%3.0f", 100 * plangfhigh);
	add_entry_with_callback(hbox, 5, _("GFlow for plan"), gflowstring, gf_focus_out_cb, &plangflow);
	add_entry_with_callback(hbox, 5, _("GFhigh for plan"), gfhighstring, gf_focus_out_cb, &plangfhigh);
	diveplan.when = current_time_notz() + 3600;
	diveplan.surface_pressure = SURFACE_PRESSURE;
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
		plan(&diveplan, &cache_data, &planned_dive, &error_string);
		if (error_string)
			show_error(error_string);
		mark_divelist_changed(TRUE);
	} else {
		if (planned_dive) {
			/* we have added a dive during the dynamic construction
			 * in the dialog; get rid of it */
			delete_single_dive(dive_table.nr - 1);
			report_dives(FALSE, FALSE);
			planned_dive = NULL;
		}
	}
	gtk_widget_destroy(planner);
	planner_error_bar = NULL;
	error_label = NULL;
	set_gf(prefs.gflow, prefs.gfhigh);
}
