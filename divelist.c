/* divelist.c */
/* this creates the UI for the dive list -
 * controlled through the following interfaces:
 *
 * void flush_divelist(struct dive *dive)
 * GtkWidget dive_list_create(void)
 * void dive_list_update_dives(void)
 * void update_dive_list_units(void)
 * void set_divelist_font(const char *font)
 * void mark_divelist_changed(int changed)
 * int unsaved_changes()
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "divelist.h"
#include "dive.h"
#include "display.h"
#include "display-gtk.h"

struct DiveList {
	GtkWidget    *tree_view;
	GtkWidget    *container_widget;
	GtkListStore *model;
	GtkTreeViewColumn *nr, *date, *depth, *duration, *location;
	GtkTreeViewColumn *temperature, *cylinder, *nitrox, *sac, *otu;
	int changed;
};

static struct DiveList dive_list;

/*
 * The dive list has the dive data in both string format (for showing)
 * and in "raw" format (for sorting purposes)
 */
enum {
	DIVE_INDEX = 0,
	DIVE_NR,		/* int: dive->nr */
	DIVE_DATE,		/* time_t: dive->when */
	DIVE_DEPTH,		/* int: dive->maxdepth in mm */
	DIVE_DURATION,		/* int: in seconds */
	DIVE_TEMPERATURE,	/* int: in mkelvin */
	DIVE_CYLINDER,
	DIVE_NITROX,		/* int: in permille */
	DIVE_SAC,		/* int: in ml/min */
	DIVE_OTU,		/* int: in OTUs */
	DIVE_LOCATION,		/* "2nd Cathedral, Lanai" */
	DIVELIST_COLUMNS
};

static GList *selected_dives;

static void selection_cb(GtkTreeSelection *selection, GtkTreeModel *model)
{
	GtkTreeIter iter;
	GValue value = {0, };
	GtkTreePath *path;

	int nr_selected = gtk_tree_selection_count_selected_rows(selection);

	if (selected_dives) {
		g_list_foreach (selected_dives, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (selected_dives);
	}
	selected_dives = gtk_tree_selection_get_selected_rows(selection, NULL);

	switch (nr_selected) {
	case 0: /* keep showing the last selected dive */
		return;
	case 1:	
		/* just pick that dive as selected */
		path = g_list_nth_data(selected_dives, 0);
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_model_get_value(model, &iter, DIVE_INDEX, &value);
			selected_dive = g_value_get_int(&value);
			repaint_dive();
		}
		return;
	default: /* multiple selections - what now? At this point I
		  * don't want to change the selected dive unless
		  * there is exactly one dive selected; not sure this
		  * is the most intuitive solution.
		  * I do however want to keep around which dives have
		  * been selected */
		return;
	}
}

static void date_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int val;
	struct tm *tm;
	time_t when;
	char buffer[40];

	gtk_tree_model_get(model, iter, DIVE_DATE, &val, -1);

	/* 2038 problem */
	when = val;

	tm = gmtime(&when);
	snprintf(buffer, sizeof(buffer),
		"%s, %s %d, %d %02d:%02d",
		weekday(tm->tm_wday),
		monthname(tm->tm_mon),
		tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min);
	g_object_set(renderer, "text", buffer, NULL);
}

static void depth_data_func(GtkTreeViewColumn *col,
			    GtkCellRenderer *renderer,
			    GtkTreeModel *model,
			    GtkTreeIter *iter,
			    gpointer data)
{
	int depth, integer, frac, len;
	char buffer[40];

	gtk_tree_model_get(model, iter, DIVE_DEPTH, &depth, -1);

	switch (output_units.length) {
	case METERS:
		/* To tenths of meters */
		depth = (depth + 49) / 100;
		integer = depth / 10;
		frac = depth % 10;
		if (integer < 20)
			break;
		frac = -1;
		/* Rounding? */
		break;
	case FEET:
		integer = mm_to_feet(depth) + 0.5;
		frac = -1;
		break;
	default:
		return;
	}
	len = snprintf(buffer, sizeof(buffer), "%d", integer);
	if (frac >= 0)
		len += snprintf(buffer+len, sizeof(buffer)-len, ".%d", frac);

	g_object_set(renderer, "text", buffer, NULL);
}

static void duration_data_func(GtkTreeViewColumn *col,
			       GtkCellRenderer *renderer,
			       GtkTreeModel *model,
			       GtkTreeIter *iter,
			       gpointer data)
{
	unsigned int sec;
	char buffer[16];

	gtk_tree_model_get(model, iter, DIVE_DURATION, &sec, -1);
	snprintf(buffer, sizeof(buffer), "%d:%02d", sec / 60, sec % 60);

	g_object_set(renderer, "text", buffer, NULL);
}

static void temperature_data_func(GtkTreeViewColumn *col,
				  GtkCellRenderer *renderer,
				  GtkTreeModel *model,
				  GtkTreeIter *iter,
				  gpointer data)
{
	int value;
	char buffer[80];

	gtk_tree_model_get(model, iter, DIVE_TEMPERATURE, &value, -1);

	*buffer = 0;
	if (value) {
		double deg;
		switch (output_units.temperature) {
		case CELSIUS:
			deg = mkelvin_to_C(value);
			break;
		case FAHRENHEIT:
			deg = mkelvin_to_F(value);
			break;
		default:
			return;
		}
		snprintf(buffer, sizeof(buffer), "%.1f", deg);
	}

	g_object_set(renderer, "text", buffer, NULL);
}

static void nitrox_data_func(GtkTreeViewColumn *col,
			     GtkCellRenderer *renderer,
			     GtkTreeModel *model,
			     GtkTreeIter *iter,
			     gpointer data)
{
	int value;
	char buffer[80];

	gtk_tree_model_get(model, iter, DIVE_NITROX, &value, -1);

	if (value)
		snprintf(buffer, sizeof(buffer), "%.1f", value/10.0);
	else
		strcpy(buffer, "air");

	g_object_set(renderer, "text", buffer, NULL);
}

/* Render the SAC data (integer value of "ml / min") */
static void sac_data_func(GtkTreeViewColumn *col,
			  GtkCellRenderer *renderer,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	int value;
	const double liters_per_cuft = 28.317;
	const char *fmt;
	char buffer[16];
	double sac;

	gtk_tree_model_get(model, iter, DIVE_SAC, &value, -1);

	if (!value) {
		g_object_set(renderer, "text", "", NULL);
		return;
	}

	sac = value / 1000.0;
	switch (output_units.volume) {
	case LITER:
		fmt = "%4.1f";
		break;
	case CUFT:
		fmt = "%4.2f";
		sac /= liters_per_cuft;
		break;
	}
	snprintf(buffer, sizeof(buffer), fmt, sac);

	g_object_set(renderer, "text", buffer, NULL);
}

/* Render the OTU data (integer value of "OTU") */
static void otu_data_func(GtkTreeViewColumn *col,
			  GtkCellRenderer *renderer,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	int value;
	char buffer[16];

	gtk_tree_model_get(model, iter, DIVE_OTU, &value, -1);

	if (!value) {
		g_object_set(renderer, "text", "", NULL);
		return;
	}

	snprintf(buffer, sizeof(buffer), "%d", value);

	g_object_set(renderer, "text", buffer, NULL);
}

/* calculate OTU for a dive */
static int calculate_otu(struct dive *dive)
{
	int i;
	double otu = 0.0;

	for (i = 1; i < dive->samples; i++) {
		int t;
		double po2;
		struct sample *sample = dive->sample + i;
		struct sample *psample = sample - 1;
		t = sample->time.seconds - psample->time.seconds;
		po2 = dive->cylinder[sample->cylinderindex].gasmix.o2.permille / 1000.0 *
			(sample->depth.mm + 10000) / 10000.0;
		if (po2 >= 0.5)
			otu += pow(po2 - 0.5, 0.83) * t / 30.0;
	}
	return otu + 0.5;
}
/*
 * Return air usage (in liters).
 */
static double calculate_airuse(struct dive *dive)
{
	double airuse = 0;
	int i;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		int size = cyl->type.size.mliter;
		double kilo_atm;

		if (!size)
			continue;

		kilo_atm = (cyl->start.mbar - cyl->end.mbar) / 1013250.0;

		/* Liters of air at 1 atm == milliliters at 1k atm*/
		airuse += kilo_atm * size;
	}
	return airuse;
}

static void get_sac(struct dive *dive, int *val)
{
	double airuse, pressure, sac;

	*val = 0;
	airuse = calculate_airuse(dive);
	if (!airuse)
		return;
	if (!dive->duration.seconds)
		return;

	/* Mean pressure in atm: 1 atm per 10m */
	pressure = 1 + (dive->meandepth.mm / 10000.0);
	sac = airuse / pressure * 60 / dive->duration.seconds;

	/* milliliters per minute.. */
	*val = sac * 1000;
}

static void get_string(char **str, const char *s)
{
	int len;
	char *n;

	if (!s)
		s = "";
	len = strlen(s);
	if (len > 40)
		len = 40;
	n = malloc(len+1);
	memcpy(n, s, len);
	n[len] = 0;
	*str = n;
}

static void get_location(struct dive *dive, char **str)
{
	get_string(str, dive->location);
}

static void get_cylinder(struct dive *dive, char **str)
{
	get_string(str, dive->cylinder[0].type.description);
}

static void fill_one_dive(struct dive *dive,
			  GtkTreeModel *model,
			  GtkTreeIter *iter)
{
	int sac;
	char *location, *cylinder;

	get_cylinder(dive, &cylinder);
	get_location(dive, &location);
	get_sac(dive, &sac);

	/*
	 * We only set the fields that changed: the strings.
	 * The core data itself is unaffected by units
	 */
	gtk_list_store_set(GTK_LIST_STORE(model), iter,
		DIVE_NR, dive->number,
		DIVE_LOCATION, location,
		DIVE_CYLINDER, cylinder,
		DIVE_SAC, sac,
		DIVE_OTU, dive->otu,
		-1);
}

static gboolean set_one_dive(GtkTreeModel *model,
			     GtkTreePath *path,
			     GtkTreeIter *iter,
			     gpointer data)
{
	GValue value = {0, };
	struct dive *dive;

	/* Get the dive number */
	gtk_tree_model_get_value(model, iter, DIVE_INDEX, &value);
	dive = get_dive(g_value_get_int(&value));
	if (!dive)
		return TRUE;
	if (data && dive != data)
		return FALSE;

	fill_one_dive(dive, model, iter);
	return dive == data;
}

void flush_divelist(struct dive *dive)
{
	GtkTreeModel *model = GTK_TREE_MODEL(dive_list.model);

	gtk_tree_model_foreach(model, set_one_dive, dive);
}

void set_divelist_font(const char *font)
{
	PangoFontDescription *font_desc = pango_font_description_from_string(font);
	gtk_widget_modify_font(dive_list.tree_view, font_desc);
	pango_font_description_free(font_desc);
}

void update_dive_list_units(void)
{
	const char *unit;
	GtkTreeModel *model = GTK_TREE_MODEL(dive_list.model);

	switch (output_units.length) {
	case METERS:
		unit = gettext("m");
		break;
	case FEET:
		unit = gettext("ft");
		break;
	}
	gtk_tree_view_column_set_title(dive_list.depth, unit);

	switch (output_units.temperature) {
	case CELSIUS:
		unit = UTF8_DEGREE "C";
		break;
	case FAHRENHEIT:
		unit = UTF8_DEGREE "F";
		break;
	case KELVIN:
		unit = "Kelvin";
		break;
	}
	gtk_tree_view_column_set_title(dive_list.temperature, unit);

	gtk_tree_model_foreach(model, set_one_dive, NULL);
}

void update_dive_list_col_visibility(void)
{
	gtk_tree_view_column_set_visible(dive_list.cylinder, visible_cols.cylinder);
	gtk_tree_view_column_set_visible(dive_list.temperature, visible_cols.temperature);
	gtk_tree_view_column_set_visible(dive_list.nitrox, visible_cols.nitrox);
	gtk_tree_view_column_set_visible(dive_list.sac, visible_cols.sac);
	gtk_tree_view_column_set_visible(dive_list.otu, visible_cols.otu);
	return;
}

static void fill_dive_list(void)
{
	int i;
	GtkTreeIter iter;
	GtkListStore *store;

	store = GTK_LIST_STORE(dive_list.model);

	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = dive_table.dives[i];

		dive->otu = calculate_otu(dive);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			DIVE_INDEX, i,
			DIVE_NR, dive->number,
			DIVE_DATE, dive->when,
			DIVE_DEPTH, dive->maxdepth,
			DIVE_DURATION, dive->duration.seconds,
			DIVE_LOCATION, gettext("location"),
			DIVE_TEMPERATURE, dive->watertemp.mkelvin,
			DIVE_NITROX, dive->cylinder[0].gasmix.o2,
			DIVE_SAC, 0,
			-1);
	}

	update_dive_list_units();
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(dive_list.model), &iter)) {
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
		gtk_tree_selection_select_iter(selection, &iter);
	}
}

void dive_list_update_dives(void)
{
	gtk_list_store_clear(GTK_LIST_STORE(dive_list.model));
	fill_dive_list();
	repaint_dive();
}

static GtkTreeViewColumn *divelist_column(struct DiveList *dl, int index, const char *title,
				data_func_t data_func, PangoAlignment align, gboolean visible)
{
	return tree_view_column(dl->tree_view, index, title, data_func, align, visible);
}

/*
 * This is some crazy crap. The only way to get default focus seems
 * to be to grab focus as the widget is being shown the first time.
 */
static void realize_cb(GtkWidget *tree_view, gpointer userdata)
{
	gtk_widget_grab_focus(tree_view);
}

GtkWidget *dive_list_create(void)
{
	GtkTreeSelection  *selection;

	dive_list.model = gtk_list_store_new(DIVELIST_COLUMNS,
				G_TYPE_INT,			/* index */
				G_TYPE_INT,			/* nr */
				G_TYPE_INT,			/* Date */
				G_TYPE_INT, 			/* Depth */
				G_TYPE_INT,			/* Duration */
				G_TYPE_INT,			/* Temperature */
				G_TYPE_STRING,			/* Cylinder */
				G_TYPE_INT,			/* Nitrox */
				G_TYPE_INT,			/* SAC */
				G_TYPE_INT,			/* OTU */
				G_TYPE_STRING			/* Location */
				);
	dive_list.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(dive_list.model));
	set_divelist_font(divelist_font);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));

	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_MULTIPLE);
	gtk_widget_set_size_request(dive_list.tree_view, 200, 200);

	dive_list.nr = divelist_column(&dive_list, DIVE_NR, "#", NULL, PANGO_ALIGN_RIGHT, TRUE);
	gtk_tree_view_column_set_sort_column_id(dive_list.nr, -1);
	dive_list.date = divelist_column(&dive_list, DIVE_DATE, gettext("Date"), date_data_func, PANGO_ALIGN_LEFT, TRUE);
	dive_list.depth = divelist_column(&dive_list, DIVE_DEPTH, gettext("ft"), depth_data_func, PANGO_ALIGN_RIGHT, TRUE);
	dive_list.duration = divelist_column(&dive_list, DIVE_DURATION, gettext("min"), duration_data_func, PANGO_ALIGN_RIGHT, TRUE);
	dive_list.temperature = divelist_column(&dive_list, DIVE_TEMPERATURE, UTF8_DEGREE "F", temperature_data_func, PANGO_ALIGN_RIGHT, visible_cols.temperature);
	dive_list.cylinder = divelist_column(&dive_list, DIVE_CYLINDER, gettext("Cyl"), NULL, PANGO_ALIGN_CENTER, visible_cols.cylinder);
	dive_list.nitrox = divelist_column(&dive_list, DIVE_NITROX, "O" UTF8_SUBSCRIPT_2 "%", nitrox_data_func, PANGO_ALIGN_CENTER, visible_cols.nitrox);
	dive_list.sac = divelist_column(&dive_list, DIVE_SAC, gettext("SAC"), sac_data_func, PANGO_ALIGN_CENTER, visible_cols.sac);
	dive_list.otu = divelist_column(&dive_list, DIVE_OTU, gettext("OTU"), otu_data_func, PANGO_ALIGN_CENTER, visible_cols.otu);
	dive_list.location = divelist_column(&dive_list, DIVE_LOCATION, gettext("Location"), NULL, PANGO_ALIGN_LEFT, TRUE);

	fill_dive_list();

	g_object_set(G_OBJECT(dive_list.tree_view), "headers-visible", TRUE,
					  "search-column", DIVE_LOCATION,
					  "rules-hint", TRUE,
					  NULL);

	g_signal_connect_after(dive_list.tree_view, "realize", G_CALLBACK(realize_cb), NULL);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), dive_list.model);

	dive_list.container_widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dive_list.container_widget),
			       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(dive_list.container_widget), dive_list.tree_view);

	dive_list.changed = 0;

	return dive_list.container_widget;
}

void mark_divelist_changed(int changed)
{
	dive_list.changed = changed;
}

int unsaved_changes()
{
	return dive_list.changed;
}
