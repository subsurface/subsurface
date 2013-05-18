/* divelist-gtk.c */
/* this creates the UI for the dive list -
 * controlled through the following interfaces:
 *
 * GdkPixbuf *get_gps_icon(void)
 * void flush_divelist(struct dive *dive)
 * void set_divelist_font(const char *font)
 * void dive_list_update_dives(void)
 * void update_dive_list_units(void)
 * void update_dive_list_col_visibility(void)
 * void dive_list_update_dives(void)
 * void add_dive_cb(GtkWidget *menuitem, gpointer data)
 * gboolean icon_click_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
 * void export_all_dives_uddf_cb()
 * GtkWidget dive_list_create(void)
 * void dive_list_destroy(void)
 * void show_and_select_dive(struct dive *dive)
 * void select_next_dive(void)
 * void select_prev_dive(void)
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glib/gi18n.h>
#include <assert.h>
#include <zip.h>
#include <libxslt/transform.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"
#include "webservice.h"

#include <gdk-pixbuf/gdk-pixdata.h>
#include "satellite.h"

struct DiveList {
	GtkWidget    *tree_view;
	GtkWidget    *container_widget;
	GtkTreeStore *model, *listmodel, *treemodel;
	GtkTreeViewColumn *nr, *date, *stars, *depth, *duration, *location;
	GtkTreeViewColumn *temperature, *cylinder, *totalweight, *suit, *nitrox, *sac, *otu, *maxcns;
	int changed;
};

static struct DiveList dive_list;
#define MODEL(_dl) GTK_TREE_MODEL((_dl).model)
#define TREEMODEL(_dl) GTK_TREE_MODEL((_dl).treemodel)
#define LISTMODEL(_dl) GTK_TREE_MODEL((_dl).listmodel)
#define STORE(_dl) GTK_TREE_STORE((_dl).model)
#define TREESTORE(_dl) GTK_TREE_STORE((_dl).treemodel)
#define LISTSTORE(_dl) GTK_TREE_STORE((_dl).listmodel)

static gboolean ignore_selection_changes = FALSE;
static gboolean in_set_cursor = FALSE;
static gboolean set_selected(GtkTreeModel *model, GtkTreePath *path,
			     GtkTreeIter *iter, gpointer data);

/*
 * The dive list has the dive data in both string format (for showing)
 * and in "raw" format (for sorting purposes)
 */
enum {
	DIVE_INDEX = 0,
	DIVE_NR,		/* int: dive->nr */
	DIVE_DATE,		/* timestamp_t: dive->when */
	DIVE_RATING,		/* int: 0-5 stars */
	DIVE_DEPTH,		/* int: dive->maxdepth in mm */
	DIVE_DURATION,		/* int: in seconds */
	DIVE_TEMPERATURE,	/* int: in mkelvin */
	DIVE_TOTALWEIGHT,	/* int: in grams */
	DIVE_SUIT,		/* "wet, 3mm" */
	DIVE_CYLINDER,
	DIVE_NITROX,		/* int: dummy */
	DIVE_SAC,		/* int: in ml/min */
	DIVE_OTU,		/* int: in OTUs */
	DIVE_MAXCNS,		/* int: in % */
	DIVE_LOCATION,		/* "2nd Cathedral, Lanai" */
	DIVE_LOC_ICON,		/* pixbuf for gps icon */
	DIVELIST_COLUMNS
};

static void merge_dive_into_trip_above_cb(GtkWidget *menuitem, GtkTreePath *path);

#ifdef DEBUG_MODEL
static gboolean dump_model_entry(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	char *location;
	int idx, nr, duration;
	struct dive *dive;
	timestamp_t when;
	struct tm tm;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_NR, &nr, DIVE_DATE, &when,
			DIVE_DURATION, &duration, DIVE_LOCATION, &location, -1);
	utc_mkdate(when, &tm);
	printf("iter %x:%x entry #%d : nr %d @ %04d-%02d-%02d %02d:%02d:%02d duration %d location %s ",
		iter->stamp, iter->user_data, idx, nr,
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		duration, location);
	dive = get_dive(idx);
	if (dive)
		printf("tripflag %d\n", dive->tripflag);
	else
		printf("without matching dive\n");

	free(location);

	return FALSE;
}

static void dump_model(GtkListStore *store)
{
	gtk_tree_model_foreach(GTK_TREE_MODEL(store), dump_model_entry, NULL);
	printf("\n---\n\n");
}
#endif

/* when subsurface starts we want to have the last dive selected. So we simply
   walk to the first leaf (and skip the summary entries - which have negative
   DIVE_INDEX) */
static void first_leaf(GtkTreeModel *model, GtkTreeIter *iter, int *diveidx)
{
	GtkTreeIter parent;
	GtkTreePath *tpath;

	while (*diveidx < 0) {
		memcpy(&parent, iter, sizeof(parent));
		tpath = gtk_tree_model_get_path(model, &parent);
		if (!gtk_tree_model_iter_children(model, iter, &parent)) {
			/* we should never have a parent without child */
			gtk_tree_path_free(tpath);
			return;
		}
		if (!gtk_tree_view_row_expanded(GTK_TREE_VIEW(dive_list.tree_view), tpath))
			gtk_tree_view_expand_row(GTK_TREE_VIEW(dive_list.tree_view), tpath, FALSE);
		gtk_tree_path_free(tpath);
		gtk_tree_model_get(model, iter, DIVE_INDEX, diveidx, -1);
	}
}

static struct dive *dive_from_path(GtkTreePath *path)
{
	GtkTreeIter iter;
	int idx;

	if (gtk_tree_model_get_iter(MODEL(dive_list), &iter, path)) {
		gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &idx, -1);
		return get_dive(idx);
	} else {
		return NULL;
	}

}

static int get_path_index(GtkTreePath *path)
{
	GtkTreeIter iter;
	int idx;

	gtk_tree_model_get_iter(MODEL(dive_list), &iter, path);
	gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &idx, -1);
	return idx;
}

/* make sure that if we expand a summary row that is selected, the children show
   up as selected, too */
static void row_expanded_cb(GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	GtkTreeIter child;
	GtkTreeModel *model = MODEL(dive_list);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
	dive_trip_t *trip;

	trip = find_trip_by_idx(get_path_index(path));
	if (!trip)
		return;

	trip->expanded = 1;
	if (!gtk_tree_model_iter_children(model, &child, iter))
		return;

	do {
		int idx;
		struct dive *dive;

		gtk_tree_model_get(model, &child, DIVE_INDEX, &idx, -1);
		dive = get_dive(idx);

		if (dive->selected)
			gtk_tree_selection_select_iter(selection, &child);
		else
			gtk_tree_selection_unselect_iter(selection, &child);
	} while (gtk_tree_model_iter_next(model, &child));
}

/* Make sure that if we collapse a summary row with any selected children, the row
   shows up as selected too */
static void row_collapsed_cb(GtkTreeView *tree_view, GtkTreeIter *iter, GtkTreePath *path, gpointer data)
{
	dive_trip_t *trip;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));

	trip = find_trip_by_idx(get_path_index(path));
	if (!trip)
		return;

	trip->expanded = 0;
	if (trip_has_selected_dives(trip)) {
		gtk_tree_selection_select_iter(selection, iter);
		trip->selected = 1;
	}
}

const char *star_strings[] = {
	ZERO_STARS,
	ONE_STARS,
	TWO_STARS,
	THREE_STARS,
	FOUR_STARS,
	FIVE_STARS
};

static void star_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int nr_stars, idx;
	char buffer[40];

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_RATING, &nr_stars, -1);
	if (idx < 0) {
		*buffer = '\0';
	} else {
		if (nr_stars < 0 || nr_stars > 5)
			nr_stars = 0;
		snprintf(buffer, sizeof(buffer), "%s", star_strings[nr_stars]);
	}
	g_object_set(renderer, "text", buffer, NULL);
}

static void date_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int idx, nr;
	timestamp_t when;
	/* this should be enought for most languages. if not increase the value. */
	char *buffer;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_DATE, &when, -1);
	nr = gtk_tree_model_iter_n_children(model, iter);

	if (idx < 0) {
		buffer = get_trip_date_string(when, nr);
	} else {
		buffer = get_dive_date_string(when);
	}
	g_object_set(renderer, "text", buffer, NULL);
	free(buffer);
}

static void depth_data_func(GtkTreeViewColumn *col,
			    GtkCellRenderer *renderer,
			    GtkTreeModel *model,
			    GtkTreeIter *iter,
			    gpointer data)
{
	int depth, integer, frac, len, idx, show_decimal;
	char buffer[40];

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_DEPTH, &depth, -1);

	if (idx < 0) {
		*buffer = '\0';
	} else {
		get_depth_values(depth, &integer, &frac, &show_decimal);
		len = snprintf(buffer, sizeof(buffer), "%d", integer);
		if (show_decimal)
			len += snprintf(buffer+len, sizeof(buffer)-len, ".%d", frac);
	}
	g_object_set(renderer, "text", buffer, NULL);
}

static void duration_data_func(GtkTreeViewColumn *col,
			       GtkCellRenderer *renderer,
			       GtkTreeModel *model,
			       GtkTreeIter *iter,
			       gpointer data)
{
	unsigned int sec;
	int idx;
	char buffer[40];

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_DURATION, &sec, -1);
	if (idx < 0)
		*buffer = '\0';
	else
		snprintf(buffer, sizeof(buffer), "%d:%02d", sec / 60, sec % 60);

	g_object_set(renderer, "text", buffer, NULL);
}

static void temperature_data_func(GtkTreeViewColumn *col,
				  GtkCellRenderer *renderer,
				  GtkTreeModel *model,
				  GtkTreeIter *iter,
				  gpointer data)
{
	int value, idx;
	char buffer[80];

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_TEMPERATURE, &value, -1);

	*buffer = 0;
	if (idx >= 0 && value) {
		double deg;
		switch (prefs.units.temperature) {
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

static void gpsicon_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int idx;
	GdkPixbuf *icon;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_LOC_ICON, &icon, -1);
	g_object_set(renderer, "pixbuf", icon, NULL);
}

static void nr_data_func(GtkTreeViewColumn *col,
			   GtkCellRenderer *renderer,
			   GtkTreeModel *model,
			   GtkTreeIter *iter,
			   gpointer data)
{
	int idx, nr;
	char buffer[40];
	struct dive *dive;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_NR, &nr, -1);
	if (idx < 0) {
		*buffer = '\0';
	} else {
		/* make dives that are not in trips stand out */
		dive = get_dive(idx);
		if (!DIVE_IN_TRIP(dive))
			snprintf(buffer, sizeof(buffer), "<b>%d</b>", nr);
		else
			snprintf(buffer, sizeof(buffer), "%d", nr);
	}
	g_object_set(renderer, "markup", buffer, NULL);
}

static void weight_data_func(GtkTreeViewColumn *col,
			     GtkCellRenderer *renderer,
			     GtkTreeModel *model,
			     GtkTreeIter *iter,
			     gpointer data)
{
	int indx, decimals;
	double value;
	char buffer[80];
	struct dive *dive;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &indx, -1);
	dive = get_dive(indx);
	value = get_weight_units(total_weight(dive), &decimals, NULL);
	if (value == 0.0)
		*buffer = '\0';
	else
		snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);

	g_object_set(renderer, "text", buffer, NULL);
}

static gint nitrox_sort_func(GtkTreeModel *model,
	GtkTreeIter *iter_a,
	GtkTreeIter *iter_b,
	gpointer user_data)
{
	int index_a, index_b;
	struct dive *a, *b;
	int a_o2, b_o2;
	int a_he, b_he;
	int a_o2low, b_o2low;

	gtk_tree_model_get(model, iter_a, DIVE_INDEX, &index_a, -1);
	gtk_tree_model_get(model, iter_b, DIVE_INDEX, &index_b, -1);
	a = get_dive(index_a);
	b = get_dive(index_b);
	get_dive_gas(a, &a_o2, &a_he, &a_o2low);
	get_dive_gas(b, &b_o2, &b_he, &b_o2low);

	/* Sort by Helium first, O2 second */
	if (a_he == b_he) {
		if (a_o2 == b_o2)
			return a_o2low - b_o2low;
		return a_o2 - b_o2;
	}
	return a_he - b_he;
}

static void nitrox_data_func(GtkTreeViewColumn *col,
			     GtkCellRenderer *renderer,
			     GtkTreeModel *model,
			     GtkTreeIter *iter,
			     gpointer data)
{
	int idx;
	char *buffer;
	struct dive *dive;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	if (idx >= 0 && (dive = get_dive(idx))) {
		buffer = get_nitrox_string(dive);
		g_object_set(renderer, "text", buffer, NULL);
		free(buffer);
	} else {
		g_object_set(renderer, "text", "", NULL);
	}
}

/* Render the SAC data (integer value of "ml / min") */
static void sac_data_func(GtkTreeViewColumn *col,
			  GtkCellRenderer *renderer,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	int value, idx;
	const char *fmt;
	char buffer[16];
	double sac;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_SAC, &value, -1);

	if (idx < 0 || !value) {
		*buffer = '\0';
		goto exit;
	}

	sac = value / 1000.0;
	switch (prefs.units.volume) {
	case LITER:
		fmt = "%4.1f";
		break;
	case CUFT:
		fmt = "%4.2f";
		sac = ml_to_cuft(sac * 1000);
		break;
	}
	snprintf(buffer, sizeof(buffer), fmt, sac);
exit:
	g_object_set(renderer, "text", buffer, NULL);
}

/* Render the OTU data (integer value of "OTU") */
static void otu_data_func(GtkTreeViewColumn *col,
			  GtkCellRenderer *renderer,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	int value, idx;
	char buffer[16];

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_OTU, &value, -1);

	if (idx < 0 || !value)
		*buffer = '\0';
	else
		snprintf(buffer, sizeof(buffer), "%d", value);

	g_object_set(renderer, "text", buffer, NULL);
}

/* Render the CNS data (in full %) */
static void cns_data_func(GtkTreeViewColumn *col,
			  GtkCellRenderer *renderer,
			  GtkTreeModel *model,
			  GtkTreeIter *iter,
			  gpointer data)
{
	int value, idx;
	char buffer[16];

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, DIVE_MAXCNS, &value, -1);

	if (idx < 0 || !value)
		*buffer = '\0';
	else
		snprintf(buffer, sizeof(buffer), "%d%%", value);

	g_object_set(renderer, "text", buffer, NULL);
}

GdkPixbuf *get_gps_icon(void)
{
	return gdk_pixbuf_from_pixdata(&satellite_pixbuf, TRUE, NULL);
}

static GdkPixbuf *get_gps_icon_for_dive(struct dive *dive)
{
	if (dive_has_gps_location(dive))
		return get_gps_icon();
	else
		return NULL;
}

/*
 * Set up anything that could have changed due to editing
 * of dive information; we need to do this for both models,
 * so we simply call set_one_dive again with the non-current model
 */
/* forward declaration for recursion */
static gboolean set_one_dive(GtkTreeModel *model,
			     GtkTreePath *path,
			     GtkTreeIter *iter,
			     gpointer data);

static void fill_one_dive(struct dive *dive,
			  GtkTreeModel *model,
			  GtkTreeIter *iter)
{
	char *location, *cylinder, *suit;
	GtkTreeModel *othermodel;
	GdkPixbuf *icon;

	get_cylinder(dive, &cylinder);
	get_location(dive, &location);
	get_suit(dive, &suit);
	icon = get_gps_icon_for_dive(dive);
	gtk_tree_store_set(GTK_TREE_STORE(model), iter,
		DIVE_NR, dive->number,
		DIVE_LOCATION, location,
		DIVE_LOC_ICON, icon,
		DIVE_CYLINDER, cylinder,
		DIVE_RATING, dive->rating,
		DIVE_SAC, dive->sac,
		DIVE_OTU, dive->otu,
		DIVE_MAXCNS, dive->maxcns,
		DIVE_TOTALWEIGHT, total_weight(dive),
		DIVE_SUIT, suit,
		-1);

	if (icon)
		g_object_unref(icon);
	free(location);
	free(cylinder);
	free(suit);

	if (model == TREEMODEL(dive_list))
		othermodel = LISTMODEL(dive_list);
	else
		othermodel = TREEMODEL(dive_list);
	if (othermodel != MODEL(dive_list))
		/* recursive call */
		gtk_tree_model_foreach(othermodel, set_one_dive, dive);
}

static gboolean set_one_dive(GtkTreeModel *model,
			     GtkTreePath *path,
			     GtkTreeIter *iter,
			     gpointer data)
{
	int idx;
	struct dive *dive;

	/* Get the dive number */
	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	if (idx < 0)
		return FALSE;
	dive = get_dive(idx);
	if (!dive)
		return TRUE;
	if (data && dive != data)
		return FALSE;

	fill_one_dive(dive, model, iter);
	return dive == data;
}

void flush_divelist(struct dive *dive)
{
	GtkTreeModel *model = MODEL(dive_list);

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
	GtkTreeModel *model = MODEL(dive_list);

	(void) get_depth_units(0, NULL, &unit);
	gtk_tree_view_column_set_title(dive_list.depth, unit);

	(void) get_temp_units(0, &unit);
	gtk_tree_view_column_set_title(dive_list.temperature, unit);

	(void) get_weight_units(0, NULL, &unit);
	gtk_tree_view_column_set_title(dive_list.totalweight, unit);

	gtk_tree_model_foreach(model, set_one_dive, NULL);
}

void update_dive_list_col_visibility(void)
{
	gtk_tree_view_column_set_visible(dive_list.cylinder, prefs.visible_cols.cylinder);
	gtk_tree_view_column_set_visible(dive_list.temperature, prefs.visible_cols.temperature);
	gtk_tree_view_column_set_visible(dive_list.totalweight, prefs.visible_cols.totalweight);
	gtk_tree_view_column_set_visible(dive_list.suit, prefs.visible_cols.suit);
	gtk_tree_view_column_set_visible(dive_list.nitrox, prefs.visible_cols.nitrox);
	gtk_tree_view_column_set_visible(dive_list.sac, prefs.visible_cols.sac);
	gtk_tree_view_column_set_visible(dive_list.otu, prefs.visible_cols.otu);
	gtk_tree_view_column_set_visible(dive_list.maxcns, prefs.visible_cols.maxcns);
	return;
}

/* Select the iter asked for, and set the keyboard focus on it */
static void go_to_iter(GtkTreeSelection *selection, GtkTreeIter *iter);
static void fill_dive_list(void)
{
	int i, trip_index = 0;
	GtkTreeIter iter, parent_iter, lookup, *parent_ptr = NULL;
	GtkTreeStore *liststore, *treestore;
	GdkPixbuf *icon;

	/* Do we need to create any dive groups automatically? */
	if (autogroup)
		autogroup_dives();

	treestore = TREESTORE(dive_list);
	liststore = LISTSTORE(dive_list);

	clear_trip_indexes();

	i = dive_table.nr;
	while (--i >= 0) {
		struct dive *dive = get_dive(i);
		dive_trip_t *trip;
		if (((dive->dive_tags & DTAG_INVALID) && !prefs.display_invalid_dives) ||
		    (dive->dive_tags & dive_mask) != dive_mask)
			continue;
		trip = dive->divetrip;

		if (!trip) {
			parent_ptr = NULL;
		} else if (!trip->index) {
			trip->index = ++trip_index;

			/* Create new trip entry */
			gtk_tree_store_append(treestore, &parent_iter, NULL);
			parent_ptr = &parent_iter;

			/* a duration of 0 (and negative index) identifies a group */
			gtk_tree_store_set(treestore, parent_ptr,
					DIVE_INDEX, -trip_index,
					DIVE_DATE, trip->when,
					DIVE_LOCATION, trip->location,
					DIVE_DURATION, 0,
					-1);
		} else {
			int idx, ok;
			GtkTreeModel *model = TREEMODEL(dive_list);

			parent_ptr = NULL;
			ok = gtk_tree_model_get_iter_first(model, &lookup);
			while (ok) {
				gtk_tree_model_get(model, &lookup, DIVE_INDEX, &idx, -1);
				if (idx == -trip->index) {
					parent_ptr = &lookup;
					break;
				}
				ok = gtk_tree_model_iter_next(model, &lookup);
			}
		}

		/* store dive */
		update_cylinder_related_info(dive);
		gtk_tree_store_append(treestore, &iter, parent_ptr);
		icon = get_gps_icon_for_dive(dive);
		gtk_tree_store_set(treestore, &iter,
			DIVE_INDEX, i,
			DIVE_NR, dive->number,
			DIVE_DATE, dive->when,
			DIVE_DEPTH, dive->maxdepth,
			DIVE_DURATION, dive->duration.seconds,
			DIVE_LOCATION, dive->location,
			DIVE_LOC_ICON, icon,
			DIVE_RATING, dive->rating,
			DIVE_TEMPERATURE, dive->watertemp.mkelvin,
			DIVE_SAC, 0,
			-1);
		if (icon)
			g_object_unref(icon);
		gtk_tree_store_append(liststore, &iter, NULL);
		gtk_tree_store_set(liststore, &iter,
			DIVE_INDEX, i,
			DIVE_NR, dive->number,
			DIVE_DATE, dive->when,
			DIVE_DEPTH, dive->maxdepth,
			DIVE_DURATION, dive->duration.seconds,
			DIVE_LOCATION, dive->location,
			DIVE_LOC_ICON, icon,
			DIVE_RATING, dive->rating,
			DIVE_TEMPERATURE, dive->watertemp.mkelvin,
			DIVE_TOTALWEIGHT, 0,
			DIVE_SUIT, dive->suit,
			DIVE_SAC, 0,
			-1);
	}

	update_dive_list_units();
	if (amount_selected == 0 && gtk_tree_model_get_iter_first(MODEL(dive_list), &iter)) {
		GtkTreeSelection *selection;

		/* select the last dive (and make sure it's an actual dive that is selected) */
		gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &selected_dive, -1);
		first_leaf(MODEL(dive_list), &iter, &selected_dive);
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
		go_to_iter(selection, &iter);
	}
}

static void restore_tree_state(void);

void dive_list_update_dives(void)
{
	dive_table.preexisting = dive_table.nr;
	ignore_selection_changes = TRUE;
	gtk_tree_store_clear(TREESTORE(dive_list));
	gtk_tree_store_clear(LISTSTORE(dive_list));
	ignore_selection_changes = FALSE;
	fill_dive_list();
	restore_tree_state();
	repaint_dive();
}

static gint gtk_dive_nr_sort(GtkTreeModel *model,
	GtkTreeIter *iter_a,
	GtkTreeIter *iter_b,
	gpointer user_data)
{
	int idx_a, idx_b;
	timestamp_t when_a, when_b;

	gtk_tree_model_get(model, iter_a, DIVE_INDEX, &idx_a, DIVE_DATE, &when_a, -1);
	gtk_tree_model_get(model, iter_b, DIVE_INDEX, &idx_b, DIVE_DATE, &when_b, -1);

	return dive_nr_sort(idx_a, idx_b, when_a, when_b);
}

static struct divelist_column {
	const char *header;
	data_func_t data;
	sort_func_t sort;
	unsigned int flags;
	int *visible;
} dl_column[] = {
	[DIVE_NR] = { "#", nr_data_func, gtk_dive_nr_sort, ALIGN_RIGHT },
	[DIVE_DATE] = { N_("Date"), date_data_func, NULL, ALIGN_LEFT },
	[DIVE_RATING] = { UTF8_BLACKSTAR, star_data_func, NULL, ALIGN_LEFT },
	[DIVE_DEPTH] = { N_("ft"), depth_data_func, NULL, ALIGN_RIGHT },
	[DIVE_DURATION] = { N_("min"), duration_data_func, NULL, ALIGN_RIGHT },
	[DIVE_TEMPERATURE] = { UTF8_DEGREE "F", temperature_data_func, NULL, ALIGN_RIGHT, &prefs.visible_cols.temperature },
	[DIVE_TOTALWEIGHT] = { N_("lbs"), weight_data_func, NULL, ALIGN_RIGHT, &prefs.visible_cols.totalweight },
	[DIVE_SUIT] = { N_("Suit"), NULL, NULL, ALIGN_LEFT, &prefs.visible_cols.suit },
	[DIVE_CYLINDER] = { N_("Cyl"), NULL, NULL, 0, &prefs.visible_cols.cylinder },
	[DIVE_NITROX] = { "O" UTF8_SUBSCRIPT_2 "%", nitrox_data_func, nitrox_sort_func, 0, &prefs.visible_cols.nitrox },
	[DIVE_SAC] = { N_("SAC"), sac_data_func, NULL, 0, &prefs.visible_cols.sac },
	[DIVE_OTU] = { N_("OTU"), otu_data_func, NULL, 0, &prefs.visible_cols.otu },
	[DIVE_MAXCNS] = { N_("maxCNS"), cns_data_func, NULL, 0, &prefs.visible_cols.maxcns },
	[DIVE_LOCATION] = { N_("Location"), NULL, NULL, ALIGN_LEFT },
};


static GtkTreeViewColumn *divelist_column(struct DiveList *dl, struct divelist_column *col)
{
	int index = col - &dl_column[0];
	const char *title = _(col->header);
	data_func_t data_func = col->data;
	sort_func_t sort_func = col->sort;
	unsigned int flags = col->flags;
	int *visible = col->visible;
	GtkWidget *tree_view = dl->tree_view;
	GtkTreeStore *treemodel = dl->treemodel;
	GtkTreeStore *listmodel = dl->listmodel;
	GtkTreeViewColumn *ret;

	if (visible && !*visible)
		flags |= INVISIBLE;
	ret = tree_view_column(tree_view, index, title, data_func, flags);
	if (sort_func) {
		/* the sort functions are needed in the corresponding models */
		if (index == DIVE_NR)
			gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(treemodel), index, sort_func, NULL, NULL);
		else
			gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(listmodel), index, sort_func, NULL, NULL);
	}
	return ret;
}

/*
 * This is some crazy crap. The only way to get default focus seems
 * to be to grab focus as the widget is being shown the first time.
 */
static void realize_cb(GtkWidget *tree_view, gpointer userdata)
{
	gtk_widget_grab_focus(tree_view);
}

/*
 * Double-clicking on a group entry will expand a collapsed group
 * and vice versa.
 */
static void collapse_expand(GtkTreeView *tree_view, GtkTreePath *path)
{
	if (!gtk_tree_view_row_expanded(tree_view, path))
		gtk_tree_view_expand_row(tree_view, path, FALSE);
	else
		gtk_tree_view_collapse_row(tree_view, path);

}

/* Double-click on a dive list */
static void row_activated_cb(GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			gpointer userdata)
{
	int index;
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(MODEL(dive_list), &iter, path))
		return;

	gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &index, -1);
	/* a negative index is special for the "group by date" entries */
	if (index < 0) {
		collapse_expand(tree_view, path);
		return;
	}
	edit_dive_info(get_dive(index), FALSE);
}

void report_dives(bool is_imported, bool prefer_imported)
{
	process_dives(is_imported, prefer_imported);
	dive_list_update_dives();
}

void add_dive_cb(GtkWidget *menuitem, gpointer data)
{
	struct dive *dive;

	dive = alloc_dive();
	if (add_new_dive(dive)) {
		record_dive(dive);
		report_dives(TRUE, FALSE);
		return;
	}
	free(dive);
}

static void edit_trip_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	int idx;
	GtkTreeIter iter;
	dive_trip_t *dive_trip;

	gtk_tree_model_get_iter(MODEL(dive_list), &iter, path);
	gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &idx, -1);
	dive_trip = find_trip_by_idx(idx);
	if (edit_trip(dive_trip))
		gtk_tree_store_set(STORE(dive_list), &iter, DIVE_LOCATION, dive_trip->location, -1);
}

static void edit_selected_dives_cb(GtkWidget *menuitem, gpointer data)
{
	edit_multi_dive_info(NULL);
}

static void edit_dive_from_path_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	struct dive *dive = dive_from_path(path);

	edit_multi_dive_info(dive);
}

static void edit_dive_when_cb(GtkWidget *menuitem, struct dive *dive)
{
	GtkWidget *dialog, *cal, *h, *m, *timehbox;
	timestamp_t when;

	guint yval, mval, dval;
	int success;
	struct tm tm;

	if (!dive)
		return;

	when = dive->when;
	utc_mkdate(when, &tm);
	dialog = create_date_time_widget(&tm, &cal, &h, &m, &timehbox);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (!success) {
		gtk_widget_destroy(dialog);
		return;
	}
	memset(&tm, 0, sizeof(tm));
	gtk_calendar_get_date(GTK_CALENDAR(cal), &yval, &mval, &dval);
	tm.tm_year = yval;
	tm.tm_mon = mval;
	tm.tm_mday = dval;
	tm.tm_hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(h));
	tm.tm_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(m));

	gtk_widget_destroy(dialog);
	when = utc_mktime(&tm);
	if (dive->when != when) {
		/* if this is the only dive in the trip, just change the trip time */
		if (dive->divetrip && dive->divetrip->nrdives == 1)
			dive->divetrip->when = when;
		/* if this is suddenly before the start of the trip, remove it from the trip */
		else if (dive->divetrip && dive->divetrip->when > when)
			remove_dive_from_trip(dive);
		else if (find_matching_trip(when) != dive->divetrip)
			remove_dive_from_trip(dive);
		dive->when = when;
		mark_divelist_changed(TRUE);
		report_dives(FALSE, FALSE);
		dive_list_update_dives();
	}
}

static void show_gps_location_cb(GtkWidget *menuitem, struct dive *dive)
{
	show_gps_location(dive, NULL);
}

gboolean icon_click_cb(GtkWidget *w, GdkEventButton *event, gpointer data)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	GtkTreeViewColumn *col;
	int idx;
	struct dive *dive;

	/* left click ? */
	if (event->button == 1 &&
	    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(dive_list.tree_view), event->x, event->y, &path, &col, NULL, NULL)) {
		/* is it the icon column ? (we passed the correct column in when registering the callback) */
		if (col == data) {
			gtk_tree_model_get_iter(MODEL(dive_list), &iter, path);
			gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &idx, -1);
			dive = get_dive(idx);
			if (dive && dive_has_gps_location(dive))
				show_gps_location(dive, NULL);
		}
		if (path)
			gtk_tree_path_free(path);
	}
	/* keep processing the click */
	return FALSE;
}

static void save_as_cb(GtkWidget *menuitem, struct dive *dive)
{
	GtkWidget *dialog;
	char *filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Save File As"),
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	if (filename){
		save_dives_logic(filename, TRUE);
		g_free(filename);
	}
}

static void invalid_dives_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	int i;
	int changed = 0;
	struct dive *dive;

	if (!amount_selected)
		return;
	/* walk the dive list in chronological order */
	for_each_dive(i, dive) {
		if (!dive->selected)
			continue;
		/* now swap the invalid tag if just 1 dive was selected
		 * otherwise set all to invalid */
		if(amount_selected == 1) {
                        if (dive->dive_tags & DTAG_INVALID)
                                dive->dive_tags &= ~DTAG_INVALID;
                        else
                                dive->dive_tags |= DTAG_INVALID;
			changed = 1;
                } else {
			if (! dive->dive_tags & DTAG_INVALID) {
				dive->dive_tags |= DTAG_INVALID;
				changed = 1;
			}
                }
		/* if invalid dives aren't shown they need to be
		 * de-selected here to avoid confusion */
		if (dive->selected && !prefs.display_invalid_dives) {
			dive->selected = 0;
			amount_selected--;
		}
	}
	if (amount_selected == 0)
		selected_dive = -1;
	if (changed) {
		dive_list_update_dives();
		mark_divelist_changed(TRUE);
	}
}

static void expand_all_cb(GtkWidget *menuitem, GtkTreeView *tree_view)
{
	gtk_tree_view_expand_all(tree_view);
}

static void collapse_all_cb(GtkWidget *menuitem, GtkTreeView *tree_view)
{
	gtk_tree_view_collapse_all(tree_view);
}

/* Move a top-level dive into the trip above it */
static void merge_dive_into_trip_above_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	int idx;
	struct dive *dive;
	dive_trip_t *trip;

	idx = get_path_index(path);
	dive = get_dive(idx);

	/* Needs to be a dive, and at the top level */
	if (!dive || dive->divetrip)
		return;

	/* Find the "trip above". */
	for (;;) {
		if (!gtk_tree_path_prev(path))
			return;
		idx = get_path_index(path);
		trip = find_trip_by_idx(idx);
		if (trip)
			break;
	}

	add_dive_to_trip(dive, trip);
	if (dive->selected) {
		for_each_dive(idx, dive) {
			if (!dive->selected)
				continue;
			add_dive_to_trip(dive, trip);
		}
	}

	trip->expanded = 1;
	dive_list_update_dives();
	mark_divelist_changed(TRUE);
}

static void insert_trip_before_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	int idx;
	struct dive *dive;
	dive_trip_t *trip;

	idx = get_path_index(path);
	dive = get_dive(idx);
	if (!dive)
		return;
	trip = create_and_hookup_trip_from_dive(dive);
	if (dive->selected) {
		for_each_dive(idx, dive) {
			if (!dive->selected)
				continue;
			add_dive_to_trip(dive, trip);
		}
	}
	trip->expanded = 1;
	dive_list_update_dives();
	mark_divelist_changed(TRUE);
}

static void remove_from_trip_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	struct dive *dive;
	int idx;

	idx = get_path_index(path);
	if (idx < 0)
		return;
	dive = get_dive(idx);

	if (dive->selected) {
		/* remove all the selected dives */
		for_each_dive(idx, dive) {
			if (!dive->selected)
				continue;
			remove_dive_from_trip(dive);
		}
	} else {
		/* just remove the dive the mouse pointer is on */
		remove_dive_from_trip(dive);
	}
	dive_list_update_dives();
	mark_divelist_changed(TRUE);
}

static void remove_trip(GtkTreePath *trippath)
{
	int idx, i;
	dive_trip_t *trip;
	struct dive *dive;

	idx = get_path_index(trippath);
	trip = find_trip_by_idx(idx);
	if (!trip)
		return;

	for_each_dive(i, dive) {
		if (dive->divetrip != trip)
			continue;
		remove_dive_from_trip(dive);
	}

	dive_list_update_dives();

#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

static void remove_trip_cb(GtkWidget *menuitem, GtkTreePath *trippath)
{
	int success;
	GtkWidget *dialog;

	dialog = gtk_dialog_new_with_buttons(_("Remove Trip"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	gtk_widget_destroy(dialog);
	if (!success)
		return;

	remove_trip(trippath);
	mark_divelist_changed(TRUE);
}

static void merge_trips_cb(GtkWidget *menuitem, GtkTreePath *trippath)
{
	GtkTreePath *prevpath;
	GtkTreeIter thistripiter, prevtripiter;
	GtkTreeModel *tm = MODEL(dive_list);
	dive_trip_t *thistrip, *prevtrip;
	timestamp_t when;

	/* this only gets called when we are on a trip and there is another trip right before */
	prevpath = gtk_tree_path_copy(trippath);
	gtk_tree_path_prev(prevpath);
	gtk_tree_model_get_iter(tm, &thistripiter, trippath);
	gtk_tree_model_get(tm, &thistripiter, DIVE_DATE, &when, -1);
	thistrip = find_matching_trip(when);
	gtk_tree_model_get_iter(tm, &prevtripiter, prevpath);
	gtk_tree_model_get(tm, &prevtripiter, DIVE_DATE, &when, -1);
	prevtrip = find_matching_trip(when);
	/* move dives from trip */
	assert(thistrip != prevtrip);
	while (thistrip->dives)
		add_dive_to_trip(thistrip->dives, prevtrip);
	dive_list_update_dives();
	mark_divelist_changed(TRUE);
}

static gboolean restore_node_state(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	int idx;
	struct dive *dive;
	dive_trip_t *trip;
	GtkTreeView *tree_view = GTK_TREE_VIEW(dive_list.tree_view);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view);

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	if (idx < 0) {
		trip = find_trip_by_idx(idx);
		if (trip && trip->expanded)
			gtk_tree_view_expand_row(tree_view, path, FALSE);
		if (trip && trip->selected)
			gtk_tree_selection_select_iter(selection, iter);
	} else {
		dive = get_dive(idx);
		if (dive && dive->selected)
			gtk_tree_selection_select_iter(selection, iter);
	}
	/* continue foreach */
	return FALSE;
}

/* restore expanded and selected state */
static void restore_tree_state(void)
{
	gtk_tree_model_foreach(MODEL(dive_list), restore_node_state, NULL);
}

/* called when multiple dives are selected and one of these is right-clicked for delete */
static void delete_selected_dives_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	int i;
	struct dive *dive;
	int success;
	GtkWidget *dialog;
	char *dialog_title;

	if (!amount_selected)
		return;
	if (amount_selected == 1)
		dialog_title = _("Delete dive");
	else
		dialog_title = _("Delete dives");

	dialog = gtk_dialog_new_with_buttons(dialog_title,
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	gtk_widget_destroy(dialog);
	if (!success)
		return;

	/* walk the dive list in chronological order */
	for (i = 0; i < dive_table.nr; i++) {
		dive = get_dive(i);
		if (!dive)
			continue;
		if (!dive->selected)
			continue;
		/* now remove the dive from the table and free it. also move the iterator back,
		 * so that we don't skip a dive */
		delete_single_dive(i);
		i--;
	}
	dive_list_update_dives();

	/* if no dives are selected at this point clear the display widgets */
	if (!amount_selected) {
		selected_dive = -1;
		process_selected_dives();
		clear_stats_widgets();
		clear_equipment_widgets();
		show_dive_info(NULL);
	}
	mark_divelist_changed(TRUE);
}

/* this gets called with path pointing to a dive, either in the top level
 * or as part of a trip */
static void delete_dive_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	int idx;
	GtkTreeIter iter;
	int success;
	GtkWidget *dialog;

	dialog = gtk_dialog_new_with_buttons(_("Delete dive"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	gtk_widget_destroy(dialog);
	if (!success)
		return;

	if (!gtk_tree_model_get_iter(MODEL(dive_list), &iter, path))
		return;
	gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &idx, -1);
	delete_single_dive(idx);
	dive_list_update_dives();
	mark_divelist_changed(TRUE);
}

void divelogs_status_dialog(char *error, GtkMessageType type)
{
	GtkWidget *dialog, *vbox, *label;

	dialog = gtk_message_dialog_new(
			GTK_WINDOW(main_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			type,
			GTK_BUTTONS_OK,
			_("%s: Response from divelogs.de"), type == GTK_MESSAGE_INFO ? _("Info") : _("Error")
			);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	label = create_label("%s", error);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);

}

static void upload_dives_divelogs(const gboolean selected)
{
	int i;
	struct dive *dive;
	FILE *f;
	char filename[PATH_MAX], *tempfile;
	size_t streamsize;
	char *membuf;
	xmlDoc *doc;
	xsltStylesheetPtr xslt = NULL;
	xmlDoc *transformed;
	struct zip_source *s[dive_table.nr];
	struct zip *zip;
	const gchar *tmpdir = g_get_tmp_dir();
	GtkMessageType type;
	char *error = NULL;
	char *parsed = NULL, *endat = NULL;

	/*
	 * Creating a temporary .DLD file to be eventually uploaded to
	 * divelogs.de. I wonder if this could be done in-memory.
	 */
	tempfile = g_build_filename(tmpdir, "export.DLD-XXXXXX", NULL);
	int fd = g_mkstemp(tempfile);
	if (fd != -1)
		close(fd);
	zip = zip_open(tempfile, ZIP_CREATE, NULL);

	if (!zip)
		return;

	if (!amount_selected)
		return;

	/* walk the dive list in chronological order */
	for (i = 0; i < dive_table.nr; i++) {

		dive = get_dive(i);
		if (!dive)
			continue;
		if (selected && !dive->selected)
			continue;

		f = tmpfile();
		if (!f)
			return;
		save_dive(f, dive);
		fseek(f, 0, SEEK_END);
		streamsize = ftell(f);
		rewind(f);
		membuf = malloc(streamsize + 1);
		if (!membuf || !fread(membuf, streamsize, 1, f))
			return;
		membuf[streamsize] = 0;
		fclose(f);

		/*
		 * Parse the memory buffer into XML document and
		 * transform it to divelogs.de format, finally dumping
		 * the XML into a character buffer.
		 */
		doc = xmlReadMemory(membuf, strlen(membuf), "divelog", NULL, 0);
		if (!doc)
			continue;

		free((void *)membuf);
		xslt = get_stylesheet("divelogs-export.xslt");
		if (!xslt)
			return;
		transformed = xsltApplyStylesheet(xslt, doc, NULL);
		xsltFreeStylesheet(xslt);
		xmlDocDumpMemory(transformed, (xmlChar **) &membuf, (int *)&streamsize);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);

		/*
		 * Save the XML document into a zip file.
		 */
		snprintf(filename, PATH_MAX, "%d.xml", i + 1);
		s[i] = zip_source_buffer(zip, membuf, streamsize, 1);
		if (s[i]) {
			int64_t ret = zip_add(zip, filename, s[i]);
			if (ret == -1)
				fprintf(stderr, "failed to include dive %d\n", i);
		}
	}
	zip_close(zip);
	if (!divelogde_upload(tempfile, &error)) {
		type = GTK_MESSAGE_ERROR;
	} else {
		/* The upload status XML message should be parsed
		 * properly and displayed in a sensible manner. But just
		 * displaying the information part of the raw message is
		 * better than nothing.
		 * And at least the dialog is customized to indicate
		 * error or success.
		 */
		if (error) {
			parsed = strstr(error, "<Login>");
			endat = strstr(error, "</divelogsDataImport>");
			if (parsed && endat)
				*endat = '\0';
		}
		if (error && strstr(error, "failed"))
			type = GTK_MESSAGE_ERROR;
		else
			type = GTK_MESSAGE_INFO;
	}
	if (parsed)
		divelogs_status_dialog(parsed, type);
	else if (error)
		divelogs_status_dialog(error, type);
	free(error);

	g_unlink(tempfile);
	g_free(tempfile);
}

void upload_selected_dives_divelogs_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	upload_dives_divelogs(TRUE);
}

void upload_all_dives_divelogs_cb()
{
	upload_dives_divelogs(FALSE);
}

static void export_dives_uddf(const gboolean selected)
{
	FILE *f;
	char *filename = NULL;
	size_t streamsize;
	char *membuf;
	xmlDoc *doc;
	xsltStylesheetPtr xslt = NULL;
	xmlDoc *transformed;
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Export As UDDF File"),
			GTK_WINDOW(main_window),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);

	if (!filename)
		return;

	/* Save XML to file and convert it into a memory buffer */
	save_dives_logic(filename, selected);
	f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	streamsize = ftell(f);
	rewind(f);

	membuf = malloc(streamsize + 1);
	if (!membuf || !fread(membuf, streamsize, 1, f)) {
		fprintf(stderr, "Failed to read memory buffer\n");
		return;
	}
	membuf[streamsize] = 0;
	fclose(f);
	g_unlink(filename);

	/*
	 * Parse the memory buffer into XML document and
	 * transform it to UDDF format, finally dumping
	 * the XML into a character buffer.
	 */
	doc = xmlReadMemory(membuf, strlen(membuf), "divelog", NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to read XML memory\n");
		return;
	}
	free((void *)membuf);

	/* Convert to UDDF format */
	xslt = get_stylesheet("uddf-export.xslt");
	if (!xslt) {
		fprintf(stderr, "Failed to open UDDF conversion stylesheet\n");
		return;
	}
	transformed = xsltApplyStylesheet(xslt, doc, NULL);
	xsltFreeStylesheet(xslt);
	xmlFreeDoc(doc);

	/* Write the transformed XML to file */
	f = g_fopen(filename, "w");
	xmlDocFormatDump(f, transformed, 1);
	xmlFreeDoc(transformed);

	fclose(f);
	g_free(filename);
}

static void export_selected_dives_uddf_cb(GtkWidget *menuitem, GtkTreePath *path)
{
	export_dives_uddf(TRUE);
}

void export_all_dives_uddf_cb()
{
	export_dives_uddf(FALSE);
}

static void merge_dives_cb(GtkWidget *menuitem, void *unused)
{
	int i;
	struct dive *dive;

	for_each_dive(i, dive) {
		if (dive->selected) {
			merge_dive_index(i, dive);
			return;
		}
	}
}

/* Called if there are exactly two selected dives and the dive at idx is one of them */
static void add_dive_merge_label(int idx, GtkMenuShell *menu)
{
	struct dive *a, *b;
	GtkWidget *menuitem;

	/* The other selected dive must be next to it.. */
	a = get_dive(idx);
	b = get_dive(idx+1);
	if (!b || !b->selected) {
		b = a;
		a = get_dive(idx-1);
		if (!a || !a->selected)
			return;
	}

	/* .. and they had better be in the same dive trip */
	if (a->divetrip != b->divetrip)
		return;

	/* .. and if the surface interval is excessive, you must be kidding us */
	if (b->when > a->when + a->duration.seconds + 30*60)
		return;

	/* If so, we can add a "merge dive" menu entry */
	menuitem = gtk_menu_item_new_with_label(_("Merge dives"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(merge_dives_cb), NULL);
	gtk_menu_shell_append(menu, menuitem);
}

static void popup_divelist_menu(GtkTreeView *tree_view, GtkTreeModel *model, int button, GdkEventButton *event)
{
	GtkWidget *menu, *menuitem, *image;
	char editplurallabel[] = N_("Edit dives");
	char editsinglelabel[] = N_("Edit dive");
	char *editlabel;
	char deleteplurallabel[] = N_("Delete dives");
	char deletesinglelabel[] = N_("Delete dive");
	char *deletelabel;
	char exportuddflabel[] = N_("Export dive(s) to UDDF");
	char uploaddivelogslabel[] = N_("Upload dive(s) to divelogs.de");
	GtkTreePath *path, *prevpath, *nextpath;
	GtkTreeIter iter, previter, nextiter;
	int idx, previdx, nextidx;
	struct dive *dive;

	if (!event || !gtk_tree_view_get_path_at_pos(tree_view, event->x, event->y, &path, NULL, NULL, NULL))
		return;
	gtk_tree_model_get_iter(MODEL(dive_list), &iter, path);
	gtk_tree_model_get(MODEL(dive_list), &iter, DIVE_INDEX, &idx, -1);

	menu = gtk_menu_new();
	menuitem = gtk_image_menu_item_new_with_label(_("Add dive"));
	image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect(menuitem, "activate", G_CALLBACK(add_dive_cb), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (idx < 0) {
		/* mouse pointer is on a trip summary entry */
		menuitem = gtk_menu_item_new_with_label(_("Edit Trip Summary"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(edit_trip_cb), path);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		prevpath = gtk_tree_path_copy(path);
		if (gtk_tree_path_prev(prevpath) &&
		    gtk_tree_model_get_iter(MODEL(dive_list), &previter, prevpath)) {
			gtk_tree_model_get(MODEL(dive_list), &previter, DIVE_INDEX, &previdx, -1);
			if (previdx < 0) {
				menuitem = gtk_menu_item_new_with_label(_("Merge trip with trip above"));
				g_signal_connect(menuitem, "activate", G_CALLBACK(merge_trips_cb), path);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			}
		}
		nextpath = gtk_tree_path_copy(path);
		gtk_tree_path_next(nextpath);
		if (gtk_tree_model_get_iter(MODEL(dive_list), &nextiter, nextpath)) {
			gtk_tree_model_get(MODEL(dive_list), &nextiter, DIVE_INDEX, &nextidx, -1);
			if (nextidx < 0) {
				menuitem = gtk_menu_item_new_with_label(_("Merge trip with trip below"));
				g_signal_connect(menuitem, "activate", G_CALLBACK(merge_trips_cb), nextpath);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			}
		}
		menuitem = gtk_menu_item_new_with_label(_("Remove Trip"));
		g_signal_connect(menuitem, "activate", G_CALLBACK(remove_trip_cb), path);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	} else {
		dive = get_dive(idx);
		/* if we right click on selected dive(s), edit, delete or tag them as invalid */
		if (dive->selected) {
			if (amount_selected == 1) {
				deletelabel = _(deletesinglelabel);
				editlabel = _(editsinglelabel);
				menuitem = gtk_menu_item_new_with_label(_("Edit dive date/time"));
				g_signal_connect(menuitem, "activate", G_CALLBACK(edit_dive_when_cb), dive);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			} else {
				deletelabel = _(deleteplurallabel);
				editlabel = _(editplurallabel);
			}
			menuitem = gtk_menu_item_new_with_label(_("Save as"));
			g_signal_connect(menuitem, "activate", G_CALLBACK(save_as_cb), dive);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			if (amount_selected == 1) {
                                if (dive->dive_tags & DTAG_INVALID)
					menuitem = gtk_menu_item_new_with_label(_("Mark valid"));
                                else
					menuitem = gtk_menu_item_new_with_label(_("Mark invalid"));
                        } else {
                                menuitem = gtk_menu_item_new_with_label(_("Mark invalid"));
                        }
			g_signal_connect(menuitem, "activate", G_CALLBACK(invalid_dives_cb), path);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			menuitem = gtk_menu_item_new_with_label(deletelabel);
			g_signal_connect(menuitem, "activate", G_CALLBACK(delete_selected_dives_cb), path);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			menuitem = gtk_menu_item_new_with_label(_(uploaddivelogslabel));
			g_signal_connect(menuitem, "activate", G_CALLBACK(upload_selected_dives_divelogs_cb), path);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			menuitem = gtk_menu_item_new_with_label(_(exportuddflabel));
			g_signal_connect(menuitem, "activate", G_CALLBACK(export_selected_dives_uddf_cb), path);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			menuitem = gtk_menu_item_new_with_label(editlabel);
			g_signal_connect(menuitem, "activate", G_CALLBACK(edit_selected_dives_cb), NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			/* Two contiguous selected dives? */
			if (amount_selected == 2)
				add_dive_merge_label(idx, GTK_MENU_SHELL(menu));
		} else {
			menuitem = gtk_menu_item_new_with_label(_("Edit dive date/time"));
			g_signal_connect(menuitem, "activate", G_CALLBACK(edit_dive_when_cb), dive);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			deletelabel = _(deletesinglelabel);
			menuitem = gtk_menu_item_new_with_label(deletelabel);
			g_signal_connect(menuitem, "activate", G_CALLBACK(delete_dive_cb), path);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

			editlabel = _(editsinglelabel);
			menuitem = gtk_menu_item_new_with_label(editlabel);
			g_signal_connect(menuitem, "activate", G_CALLBACK(edit_dive_from_path_cb), path);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}
		/* Only offer to show on map if it has a location. */
		if (dive_has_gps_location(dive)) {
			menuitem = gtk_menu_item_new_with_label(_("Show in map"));
			g_signal_connect(menuitem, "activate", G_CALLBACK(show_gps_location_cb), dive);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}
		/* only offer trip editing options when we are displaying the tree model */
		if (dive_list.model == dive_list.treemodel) {
			int depth = gtk_tree_path_get_depth(path);
			int *indices = gtk_tree_path_get_indices(path);
			/* top level dive or child dive that is not the first child */
			if (depth == 1 || indices[1] > 0) {
				menuitem = gtk_menu_item_new_with_label(_("Create new trip above"));
				g_signal_connect(menuitem, "activate", G_CALLBACK(insert_trip_before_cb), path);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			}
			prevpath = gtk_tree_path_copy(path);
			/* top level dive with a trip right before it */
			if (depth == 1 &&
			    gtk_tree_path_prev(prevpath) &&
			    gtk_tree_model_get_iter(MODEL(dive_list), &previter, prevpath) &&
			    gtk_tree_model_iter_n_children(model, &previter)) {
				menuitem = gtk_menu_item_new_with_label(_("Add to trip above"));
				g_signal_connect(menuitem, "activate", G_CALLBACK(merge_dive_into_trip_above_cb), path);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			}
			if (DIVE_IN_TRIP(dive)) {
				if (dive->selected && amount_selected > 1)
					menuitem = gtk_menu_item_new_with_label(_("Remove selected dives from trip"));
				else
					menuitem = gtk_menu_item_new_with_label(_("Remove dive from trip"));
				g_signal_connect(menuitem, "activate", G_CALLBACK(remove_from_trip_cb), path);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			}
		}
	}
	menuitem = gtk_menu_item_new_with_label(_("Expand all"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(expand_all_cb), tree_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	menuitem = gtk_menu_item_new_with_label(_("Collapse all"));
	g_signal_connect(menuitem, "activate", G_CALLBACK(collapse_all_cb), tree_view);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		button, gtk_get_current_event_time());
}

static void popup_menu_cb(GtkTreeView *tree_view, gpointer userdata)
{
	popup_divelist_menu(tree_view, MODEL(dive_list), 0, NULL);
}

static gboolean button_press_cb(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	/* Right-click? Bring up the menu */
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
		popup_divelist_menu(GTK_TREE_VIEW(treeview), MODEL(dive_list), 3, event);
		return TRUE;
	}
	return FALSE;
}

/* make sure 'path' is shown in the divelist widget; since set_cursor changes the
 * selection to be only 'path' we need to let our selection handling callbacks know
 * that we didn't really mean this */
static void scroll_to_path(GtkTreePath *path)
{
	GtkTreeSelection *selection;

	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(dive_list.tree_view), path);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(dive_list.tree_view), path, NULL, FALSE, 0, 0);
	in_set_cursor = TRUE;
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(dive_list.tree_view), path, NULL, FALSE);
	in_set_cursor = FALSE;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
	gtk_tree_model_foreach(MODEL(dive_list), set_selected, selection);

}

/* we need to have a temporary copy of the selected dives while
   switching model as the selection_cb function keeps getting called
   when gtk_tree_selection_select_path is called.  We also need to
   keep copies of the sort order so we can restore that as well after
   switching models. */
static gboolean second_call = FALSE;
static GtkSortType sortorder[] = { [0 ... DIVELIST_COLUMNS - 1] = GTK_SORT_DESCENDING, };
static int lastcol = DIVE_NR;

/* Check if this dive was selected previously and select it again in the new model;
 * This is used after we switch models to maintain consistent selections.
 * We always return FALSE to iterate through all dives */
static gboolean set_selected(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	GtkTreeSelection *selection = GTK_TREE_SELECTION(data);
	int idx, selected;
	struct dive *dive;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	if (idx < 0) {
		/* this is a trip - restore its state */
		dive_trip_t *trip = find_trip_by_idx(idx);
		if (trip && trip->expanded)
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(dive_list.tree_view), path);
		if (trip && trip->selected)
			gtk_tree_selection_select_path(selection, path);
	} else {
		dive = get_dive(idx);
		selected = dive && dive->selected;
		if (selected) {
			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(dive_list.tree_view), path);
			gtk_tree_selection_select_path(selection, path);
		}
	}
	return FALSE;
}

static gboolean scroll_to_this(GtkTreeModel *model, GtkTreePath *path,
				GtkTreeIter *iter, gpointer data)
{
	int idx;
	struct dive *dive;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	dive = get_dive(idx);
	if (dive == current_dive) {
		scroll_to_path(path);
		return TRUE;
	}
	return FALSE;
}

static void scroll_to_current(GtkTreeModel *model)
{
	if (current_dive)
		gtk_tree_model_foreach(model, scroll_to_this, current_dive);
}

static void update_column_and_order(int colid)
{
	/* Careful: the index into treecolumns is off by one as we don't have a
	   tree_view column for DIVE_INDEX */
	GtkTreeViewColumn **treecolumns = &dive_list.nr;

	/* this will trigger a second call into sort_column_change_cb,
	   so make sure we don't start an infinite recursion... */
	second_call = TRUE;
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(dive_list.model), colid, sortorder[colid]);
	gtk_tree_view_column_set_sort_order(treecolumns[colid - 1], sortorder[colid]);
	second_call = FALSE;
	scroll_to_current(GTK_TREE_MODEL(dive_list.model));
}

/* If the sort column is nr (default), show the tree model.
   For every other sort column only show the list model.
   If the model changed, inform the new model of the chosen sort column and make
   sure the same dives are still selected.

   The challenge with this function is that once we change the model
   we also need to change the sort column again (as it was changed in
   the other model) and that causes this function to be called
   recursively - so we need to catch that.
*/
static void sort_column_change_cb(GtkTreeSortable *treeview, gpointer data)
{
	int colid;
	GtkSortType order;
	GtkTreeStore *currentmodel = dive_list.model;

	gtk_widget_grab_focus(dive_list.tree_view);
	if (second_call)
		return;

	gtk_tree_sortable_get_sort_column_id(treeview, &colid, &order);
	if (colid == lastcol) {
		/* we just changed sort order */
		sortorder[colid] = order;
		return;
	} else {
		lastcol = colid;
	}
	if (colid == DIVE_NR)
		dive_list.model = dive_list.treemodel;
	else
		dive_list.model = dive_list.listmodel;
	if (dive_list.model != currentmodel) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));

		gtk_tree_view_set_model(GTK_TREE_VIEW(dive_list.tree_view), MODEL(dive_list));
		update_column_and_order(colid);
		gtk_tree_model_foreach(MODEL(dive_list), set_selected, selection);
	} else {
		if (order != sortorder[colid]) {
			update_column_and_order(colid);
		}
	}
}

static gboolean modify_selection_cb(GtkTreeSelection *selection, GtkTreeModel *model,
				GtkTreePath *path, gboolean was_selected, gpointer userdata)
{
	int idx;
	GtkTreeIter iter;

	if (!was_selected || in_set_cursor)
		return TRUE;
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, DIVE_INDEX, &idx, -1);
	if (idx < 0) {
		int i;
		struct dive *dive;
		dive_trip_t *trip = find_trip_by_idx(idx);
		if (!trip)
			return TRUE;

		trip->selected = 0;
		/* If this is expanded, let the gtk selection happen for each dive under it */
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(dive_list.tree_view), path))
			return TRUE;
		/* Otherwise, consider each dive under it deselected */
		for_each_dive(i, dive) {
			if (dive->divetrip == trip)
				deselect_dive(i);
		}
	} else {
		deselect_dive(idx);
	}
	return TRUE;
}

/* This gets called for each selected entry after a selection has changed */
static void entry_selected(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	int idx;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	if (idx < 0) {
		int i;
		struct dive *dive;
		dive_trip_t *trip = find_trip_by_idx(idx);

		if (!trip)
			return;
		trip->selected = 1;

		/* If this is expanded, let the gtk selection happen for each dive under it */
		if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(dive_list.tree_view), path)) {
			trip->fixup = 1;
			return;
		}

		/* Otherwise, consider each dive under it selected */
		for_each_dive(i, dive) {
			if (dive->divetrip == trip)
				select_dive(i);
		}
		trip->fixup = 0;
	} else {
		select_dive(idx);
	}
}

static void update_gtk_selection(GtkTreeSelection *selection, GtkTreeModel *model)
{
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first(model, &iter))
		return;
	do {
		GtkTreeIter child;

		if (!gtk_tree_model_iter_children(model, &child, &iter))
			continue;

		do {
			int idx;
			struct dive *dive;
			dive_trip_t *trip;

			gtk_tree_model_get(model, &child, DIVE_INDEX, &idx, -1);
			dive = get_dive(idx);
			if (!dive || !dive->selected)
				break;
			trip = dive->divetrip;
			if (!trip)
				break;
			gtk_tree_selection_select_iter(selection, &child);
		} while (gtk_tree_model_iter_next(model, &child));
	} while (gtk_tree_model_iter_next(model, &iter));
}

/* this is called when gtk thinks that the selection has changed */
static void selection_cb(GtkTreeSelection *selection, GtkTreeModel *model)
{
	int i, fixup;
	struct dive *dive;

	if (ignore_selection_changes)
		return;

	gtk_tree_selection_selected_foreach(selection, entry_selected, model);

	/*
	 * Go through all the dives, if there is a trip that is selected but no
	 * dives under it are selected, force-select all the dives
	 */

	/* First, clear "fixup" for any trip that has selected dives */
	for_each_dive(i, dive) {
		dive_trip_t *trip = dive->divetrip;
		if (!trip || !trip->fixup)
			continue;
		if (dive->selected || !trip->selected)
			trip->fixup = 0;
	}

	/*
	 * Ok, not fixup is only set for trips that are selected
	 * but have no selected dives in them. Select all dives
	 * for such trips.
	 */
	fixup = 0;
	for_each_dive(i, dive) {
		dive_trip_t *trip = dive->divetrip;
		if (!trip || !trip->fixup)
			continue;
		fixup = 1;
		select_dive(i);
	}

	/*
	 * Ok, we did a forced selection of dives, now we need to update the gtk
	 * view of what is selected too..
	 */
	if (fixup)
		update_gtk_selection(selection, model);

#if DEBUG_SELECTION_TRACKING
	dump_selection();
#endif

	process_selected_dives();
	repaint_dive();
}

GtkWidget *dive_list_create(void)
{
	GtkTreeSelection  *selection;

	dive_list.listmodel = gtk_tree_store_new(DIVELIST_COLUMNS,
				G_TYPE_INT,			/* index */
				G_TYPE_INT,			/* nr */
				G_TYPE_INT64,			/* Date */
				G_TYPE_INT,			/* Star rating */
				G_TYPE_INT, 			/* Depth */
				G_TYPE_INT,			/* Duration */
				G_TYPE_INT,			/* Temperature */
				G_TYPE_INT,			/* Total weight */
				G_TYPE_STRING,			/* Suit */
				G_TYPE_STRING,			/* Cylinder */
				G_TYPE_INT,			/* Nitrox */
				G_TYPE_INT,			/* SAC */
				G_TYPE_INT,			/* OTU */
				G_TYPE_INT,			/* MAXCNS */
				G_TYPE_STRING,			/* Location */
				GDK_TYPE_PIXBUF			/* GPS icon */
				);
	dive_list.treemodel = gtk_tree_store_new(DIVELIST_COLUMNS,
				G_TYPE_INT,			/* index */
				G_TYPE_INT,			/* nr */
				G_TYPE_INT64,			/* Date */
				G_TYPE_INT,			/* Star rating */
				G_TYPE_INT, 			/* Depth */
				G_TYPE_INT,			/* Duration */
				G_TYPE_INT,			/* Temperature */
				G_TYPE_INT,			/* Total weight */
				G_TYPE_STRING,			/* Suit */
				G_TYPE_STRING,			/* Cylinder */
				G_TYPE_INT,			/* Nitrox */
				G_TYPE_INT,			/* SAC */
				G_TYPE_INT,			/* OTU */
				G_TYPE_INT,			/* MAXCNS */
				G_TYPE_STRING,			/* Location */
				GDK_TYPE_PIXBUF			/* GPS icon */
				);
	dive_list.model = dive_list.treemodel;
	dive_list.tree_view = gtk_tree_view_new_with_model(TREEMODEL(dive_list));
	set_divelist_font(prefs.divelist_font);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));

	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_MULTIPLE);
	gtk_widget_set_size_request(dive_list.tree_view, 200, 200);

	/* check if utf8 stars are available as a default OS feature */
	if (!subsurface_os_feature_available(UTF8_FONT_WITH_STARS))
		dl_column[3].header = "*";

	dive_list.nr = divelist_column(&dive_list, dl_column + DIVE_NR);
	dive_list.date = divelist_column(&dive_list, dl_column + DIVE_DATE);
	dive_list.stars = divelist_column(&dive_list, dl_column + DIVE_RATING);
	dive_list.depth = divelist_column(&dive_list, dl_column + DIVE_DEPTH);
	dive_list.duration = divelist_column(&dive_list, dl_column + DIVE_DURATION);
	dive_list.temperature = divelist_column(&dive_list, dl_column + DIVE_TEMPERATURE);
	dive_list.totalweight = divelist_column(&dive_list, dl_column + DIVE_TOTALWEIGHT);
	dive_list.suit = divelist_column(&dive_list, dl_column + DIVE_SUIT);
	dive_list.cylinder = divelist_column(&dive_list, dl_column + DIVE_CYLINDER);
	dive_list.nitrox = divelist_column(&dive_list, dl_column + DIVE_NITROX);
	dive_list.sac = divelist_column(&dive_list, dl_column + DIVE_SAC);
	dive_list.otu = divelist_column(&dive_list, dl_column + DIVE_OTU);
	dive_list.maxcns = divelist_column(&dive_list, dl_column + DIVE_MAXCNS);
	dive_list.location = divelist_column(&dive_list, dl_column + DIVE_LOCATION);
	gtk_tree_view_column_set_sort_indicator(dive_list.nr, TRUE);
	gtk_tree_view_column_set_sort_order(dive_list.nr, GTK_SORT_DESCENDING);
	/* now add the GPS icon to the location column */
	tree_view_column_add_pixbuf(dive_list.tree_view, gpsicon_data_func, dive_list.location);

	fill_dive_list();

	g_object_set(G_OBJECT(dive_list.tree_view), "headers-visible", TRUE,
					  "search-column", DIVE_LOCATION,
					  "rules-hint", TRUE,
					  NULL);

	g_signal_connect_after(dive_list.tree_view, "realize", G_CALLBACK(realize_cb), NULL);
	g_signal_connect(dive_list.tree_view, "row-activated", G_CALLBACK(row_activated_cb), NULL);
	g_signal_connect(dive_list.tree_view, "row-expanded", G_CALLBACK(row_expanded_cb), NULL);
	g_signal_connect(dive_list.tree_view, "row-collapsed", G_CALLBACK(row_collapsed_cb), NULL);
	g_signal_connect(dive_list.tree_view, "button-press-event", G_CALLBACK(button_press_cb), NULL);
	g_signal_connect(dive_list.tree_view, "popup-menu", G_CALLBACK(popup_menu_cb), NULL);
	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), dive_list.model);
	g_signal_connect(dive_list.listmodel, "sort-column-changed", G_CALLBACK(sort_column_change_cb), NULL);
	g_signal_connect(dive_list.treemodel, "sort-column-changed", G_CALLBACK(sort_column_change_cb), NULL);

	gtk_tree_selection_set_select_function(selection, modify_selection_cb, NULL, NULL);

	dive_list.container_widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dive_list.container_widget),
			       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(dive_list.container_widget), dive_list.tree_view);

	dive_list.changed = 0;

	return dive_list.container_widget;
}

void dive_list_destroy(void)
{
	gtk_widget_destroy(dive_list.tree_view);
	g_object_unref(dive_list.treemodel);
	g_object_unref(dive_list.listmodel);
}

struct iteridx {
	int idx;
	GtkTreeIter *iter;
};

static gboolean iter_has_idx(GtkTreeModel *model, GtkTreePath *path,
			     GtkTreeIter *iter, gpointer _data)
{
	struct iteridx *iteridx = _data;
	int idx;
	/* Get the dive number */
	gtk_tree_model_get(model, iter, DIVE_INDEX, &idx, -1);
	if (idx == iteridx->idx) {
		iteridx->iter = gtk_tree_iter_copy(iter);
		return TRUE; /* end foreach */
	}
	return FALSE;
}

static GtkTreeIter *get_iter_from_idx(int idx)
{
	struct iteridx iteridx = {idx, };
	gtk_tree_model_foreach(MODEL(dive_list), iter_has_idx, &iteridx);
	return iteridx.iter;
}

static void scroll_to_selected(GtkTreeIter *iter)
{
	GtkTreePath *treepath;
	treepath = gtk_tree_model_get_path(MODEL(dive_list), iter);
	scroll_to_path(treepath);
	gtk_tree_path_free(treepath);
}

static void go_to_iter(GtkTreeSelection *selection, GtkTreeIter *iter)
{
	gtk_tree_selection_unselect_all(selection);
	gtk_tree_selection_select_iter(selection, iter);
	scroll_to_selected(iter);
}

void show_and_select_dive(struct dive *dive)
{
	GtkTreeSelection *selection;
	GtkTreeIter *iter;
	struct dive *odive;
	int i, divenr;

	divenr = get_divenr(dive);
	if (divenr < 0)
		/* we failed to find the dive */
		return;
	iter = get_iter_from_idx(divenr);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
	for_each_dive(i, odive)
		odive->selected = FALSE;
	amount_selected = 1;
	selected_dive = divenr;
	dive->selected = TRUE;
	go_to_iter(selection, iter);
	gtk_tree_iter_free(iter);
}

void select_next_dive(void)
{
	GtkTreeIter *nextiter, *parent = NULL;
	GtkTreeIter *iter = get_iter_from_idx(selected_dive);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
	int idx;

	if (!iter)
		return;
	nextiter = gtk_tree_iter_copy(iter);
	if (!gtk_tree_model_iter_next(MODEL(dive_list), nextiter)) {
		if (!gtk_tree_model_iter_parent(MODEL(dive_list), nextiter, iter)) {
			/* we're at the last top level node */
			goto free_iter;
		}
		if (!gtk_tree_model_iter_next(MODEL(dive_list), nextiter)) {
			/* last trip */
			goto free_iter;
		}
	}
	gtk_tree_model_get(MODEL(dive_list), nextiter, DIVE_INDEX, &idx, -1);
	if (idx < 0) {
		/* need the first child */
		parent = gtk_tree_iter_copy(nextiter);
		if (! gtk_tree_model_iter_children(MODEL(dive_list), nextiter, parent))
			goto free_iter;
	}
	go_to_iter(selection, nextiter);
free_iter:
	if (nextiter)
		gtk_tree_iter_free(nextiter);
	if (parent)
		gtk_tree_iter_free(parent);
	gtk_tree_iter_free(iter);
}

void select_prev_dive(void)
{
	GtkTreeIter previter, *parent = NULL;
	GtkTreeIter *iter = get_iter_from_idx(selected_dive);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(dive_list.tree_view));
	GtkTreePath *treepath;
	int idx;

	if (!iter)
		return;
	treepath = gtk_tree_model_get_path(MODEL(dive_list), iter);
	if (!gtk_tree_path_prev(treepath)) {
		if (!gtk_tree_model_iter_parent(MODEL(dive_list), &previter, iter))
			/* we're at the last top level node */
			goto free_iter;
		gtk_tree_path_free(treepath);
		treepath = gtk_tree_model_get_path(MODEL(dive_list), &previter);
		if (!gtk_tree_path_prev(treepath))
			/* first trip */
			goto free_iter;
		if (!gtk_tree_model_get_iter(MODEL(dive_list), &previter, treepath))
			goto free_iter;
	}
	if (!gtk_tree_model_get_iter(MODEL(dive_list), &previter, treepath))
		goto free_iter;
	gtk_tree_model_get(MODEL(dive_list), &previter, DIVE_INDEX, &idx, -1);
	if (idx < 0) {
		/* need the last child */
		parent = gtk_tree_iter_copy(&previter);
		if (! gtk_tree_model_iter_nth_child(MODEL(dive_list), &previter, parent,
				gtk_tree_model_iter_n_children(MODEL(dive_list), parent) - 1))
			goto free_iter;
	}
	go_to_iter(selection, &previter);
free_iter:
	gtk_tree_path_free(treepath);
	if (parent)
		gtk_tree_iter_free(parent);
	gtk_tree_iter_free(iter);
}
