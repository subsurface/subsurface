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
	GtkTreeViewColumn *nr, *date, *stars, *depth, *duration, *location;
	GtkTreeViewColumn *temperature, *cylinder, *totalweight, *nitrox, *sac, *otu;
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
	DIVE_RATING,		/* int: 0-5 stars */
	DIVE_DEPTH,		/* int: dive->maxdepth in mm */
	DIVE_DURATION,		/* int: in seconds */
	DIVE_TEMPERATURE,	/* int: in mkelvin */
	DIVE_TOTALWEIGHT,	/* int: in grams */
	DIVE_CYLINDER,
	DIVE_NITROX,		/* int: dummy */
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
		amount_selected = 1;
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
		amount_selected = g_list_length(selected_dives);
		process_selected_dives(selected_dives, model);
		repaint_dive();
		return;
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
	int nr_stars;
	char buffer[40];

	gtk_tree_model_get(model, iter, DIVE_RATING, &nr_stars, -1);
	if (nr_stars < 0 || nr_stars > 5)
		nr_stars = 0;
	snprintf(buffer, sizeof(buffer), "%s", star_strings[nr_stars]);
	g_object_set(renderer, "text", buffer, NULL);
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
		if (frac >= 5)
			integer++;
		frac = -1;
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

/*
 * Get "maximal" dive gas for a dive.
 * Rules:
 *  - Trimix trumps nitrox (highest He wins, O2 breaks ties)
 *  - Nitrox trumps air (even if hypoxic)
 * These are the same rules as the inter-dive sorting rules.
 */
static void get_dive_gas(struct dive *dive, int *o2_p, int *he_p, int *o2low_p)
{
	int i;
	int maxo2 = -1, maxhe = -1, mino2 = 1000;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		struct gasmix *mix = &cyl->gasmix;
		int o2 = mix->o2.permille;
		int he = mix->he.permille;

		if (cylinder_none(cyl))
			continue;
		if (!o2)
			o2 = AIR_PERMILLE;
		if (o2 < mino2)
			mino2 = o2;
		if (he > maxhe)
			goto newmax;
		if (he < maxhe)
			continue;
		if (o2 <= maxo2)
			continue;
newmax:
		maxhe = he;
		maxo2 = o2;
	}
	/* All air? Show/sort as "air"/zero */
	if (!maxhe && maxo2 == AIR_PERMILLE && mino2 == maxo2)
		maxo2 = mino2 = 0;
	*o2_p = maxo2;
	*he_p = maxhe;
	*o2low_p = mino2;
}

static int total_weight(struct dive *dive)
{
	int i, total_grams = 0;

	if (dive)
		for (i=0; i< MAX_WEIGHTSYSTEMS; i++)
			total_grams += dive->weightsystem[i].weight.grams;
	return total_grams;
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

#define UTF8_ELLIPSIS "\xE2\x80\xA6"

static void nitrox_data_func(GtkTreeViewColumn *col,
			     GtkCellRenderer *renderer,
			     GtkTreeModel *model,
			     GtkTreeIter *iter,
			     gpointer data)
{
	int index, o2, he, o2low;
	char buffer[80];
	struct dive *dive;

	gtk_tree_model_get(model, iter, DIVE_INDEX, &index, -1);
	dive = get_dive(index);
	get_dive_gas(dive, &o2, &he, &o2low);
	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	o2low = (o2low + 5) / 10;

	if (he)
		snprintf(buffer, sizeof(buffer), "%d/%d", o2, he);
	else if (o2)
		if (o2 == o2low)
			snprintf(buffer, sizeof(buffer), "%d", o2);
		else
			snprintf(buffer, sizeof(buffer), "%d" UTF8_ELLIPSIS "%d", o2low, o2);
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
		sac = ml_to_cuft(sac * 1000);
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
		int o2 = dive->cylinder[sample->cylinderindex].gasmix.o2.permille;
		if (!o2)
			o2 = AIR_PERMILLE;
		po2 = o2 / 1000.0 * (sample->depth.mm + 10000) / 10000.0;
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
		pressure_t start, end;
		cylinder_t *cyl = dive->cylinder + i;
		int size = cyl->type.size.mliter;
		double kilo_atm;

		if (!size)
			continue;

		start = cyl->start.mbar ? cyl->start : cyl->sample_start;
		end = cyl->end.mbar ? cyl->end : cyl->sample_end;
		kilo_atm = (to_ATM(start) - to_ATM(end)) / 1000.0;

		/* Liters of air at 1 atm == milliliters at 1k atm*/
		airuse += kilo_atm * size;
	}
	return airuse;
}

static int calculate_sac(struct dive *dive)
{
	double airuse, pressure, sac;
	int duration, i;

	airuse = calculate_airuse(dive);
	if (!airuse)
		return 0;
	if (!dive->duration.seconds)
		return 0;

	/* find and eliminate long surface intervals */
	duration = dive->duration.seconds;
	for (i = 0; i < dive->samples; i++) {
		if (dive->sample[i].depth.mm < 100) { /* less than 10cm */
			int end = i + 1;
			while (end < dive->samples && dive->sample[end].depth.mm < 100)
				end++;
			/* we only want the actual surface time during a dive */
			if (end < dive->samples) {
				end--;
				duration -= dive->sample[end].time.seconds -
						dive->sample[i].time.seconds;
				i = end + 1;
			}
		}
	}
	/* Mean pressure in atm: 1 atm per 10m */
	pressure = 1 + (dive->meandepth.mm / 10000.0);
	sac = airuse / pressure * 60 / duration;

	/* milliliters per minute.. */
	return sac * 1000;
}

void update_cylinder_related_info(struct dive *dive)
{
	if (dive != NULL) {
		dive->sac = calculate_sac(dive);
		dive->otu = calculate_otu(dive);
	}
}

static void get_string(char **str, const char *s)
{
	int len;
	char *n;

	if (!s)
		s = "";
	len = strlen(s);
	if (len > 60)
		len = 60;
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

/*
 * Set up anything that could have changed due to editing
 * of dive information
 */
static void fill_one_dive(struct dive *dive,
			  GtkTreeModel *model,
			  GtkTreeIter *iter)
{
	char *location, *cylinder;

	get_cylinder(dive, &cylinder);
	get_location(dive, &location);

	gtk_list_store_set(GTK_LIST_STORE(model), iter,
		DIVE_NR, dive->number,
		DIVE_LOCATION, location,
		DIVE_CYLINDER, cylinder,
		DIVE_RATING, dive->rating,
		DIVE_SAC, dive->sac,
		DIVE_OTU, dive->otu,
		DIVE_TOTALWEIGHT, total_weight(dive),
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
	gtk_tree_view_column_set_visible(dive_list.cylinder, visible_cols.cylinder);
	gtk_tree_view_column_set_visible(dive_list.temperature, visible_cols.temperature);
	gtk_tree_view_column_set_visible(dive_list.totalweight, visible_cols.totalweight);
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

	i = dive_table.nr;
	while (--i >= 0) {
		struct dive *dive = dive_table.dives[i];

		update_cylinder_related_info(dive);
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			DIVE_INDEX, i,
			DIVE_NR, dive->number,
			DIVE_DATE, dive->when,
			DIVE_DEPTH, dive->maxdepth,
			DIVE_DURATION, dive->duration.seconds,
			DIVE_LOCATION, "location",
			DIVE_TEMPERATURE, dive->watertemp.mkelvin,
			DIVE_TOTALWEIGHT, 0,
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

static struct divelist_column {
	const char *header;
	data_func_t data;
	sort_func_t sort;
	unsigned int flags;
	int *visible;
} dl_column[] = {
	[DIVE_NR] = { "#", NULL, NULL, ALIGN_RIGHT | UNSORTABLE },
	[DIVE_DATE] = { "Date", date_data_func, NULL, ALIGN_LEFT },
	[DIVE_RATING] = { UTF8_BLACKSTAR, star_data_func, NULL, ALIGN_LEFT },
	[DIVE_DEPTH] = { "ft", depth_data_func, NULL, ALIGN_RIGHT },
	[DIVE_DURATION] = { "min", duration_data_func, NULL, ALIGN_RIGHT },
	[DIVE_TEMPERATURE] = { UTF8_DEGREE "F", temperature_data_func, NULL, ALIGN_RIGHT, &visible_cols.temperature },
	[DIVE_TOTALWEIGHT] = { "lbs", weight_data_func, NULL, ALIGN_RIGHT, &visible_cols.totalweight },
	[DIVE_CYLINDER] = { "Cyl", NULL, NULL, 0, &visible_cols.cylinder },
	[DIVE_NITROX] = { "O" UTF8_SUBSCRIPT_2 "%", nitrox_data_func, nitrox_sort_func, 0, &visible_cols.nitrox },
	[DIVE_SAC] = { "SAC", sac_data_func, NULL, 0, &visible_cols.sac },
	[DIVE_OTU] = { "OTU", otu_data_func, NULL, 0, &visible_cols.otu },
	[DIVE_LOCATION] = { "Location", NULL, NULL, ALIGN_LEFT },
};


static GtkTreeViewColumn *divelist_column(struct DiveList *dl, struct divelist_column *col)
{
	int index = col - &dl_column[0];
	const char *title = col->header;
	data_func_t data_func = col->data;
	sort_func_t sort_func = col->sort;
	unsigned int flags = col->flags;
	int *visible = col->visible;
	GtkWidget *tree_view = dl->tree_view;
	GtkListStore *model = dl->model;
	GtkTreeViewColumn *ret;

	if (visible && !*visible)
		flags |= INVISIBLE;
	ret = tree_view_column(tree_view, index, title, data_func, flags);
	if (sort_func)
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), index, sort_func, NULL, NULL);
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

static void row_activated_cb(GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			GtkTreeModel *model)
{
	int index;
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;
	gtk_tree_model_get(model, &iter, DIVE_INDEX, &index, -1);
	edit_dive_info(get_dive(index));
}

void add_dive_cb(GtkWidget *menuitem, gpointer data)
{
	struct dive *dive;

	dive = alloc_dive();
	if (add_new_dive(dive)) {
		record_dive(dive);
		report_dives(TRUE);
		return;
	}
	free(dive);
}

static void popup_divelist_menu(GtkTreeView *tree_view, GtkTreeModel *model, int button)
{
	GtkWidget *menu, *menuitem;

	menu = gtk_menu_new();
	menuitem = gtk_menu_item_new_with_label("Add dive");
	g_signal_connect(menuitem, "activate", G_CALLBACK(add_dive_cb), model);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show_all(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		button, gtk_get_current_event_time());
}

static void popup_menu_cb(GtkTreeView *tree_view,
			GtkTreeModel *model)
{
	popup_divelist_menu(tree_view, model, 0);
}

static gboolean button_press_cb(GtkWidget *treeview, GdkEventButton *event, GtkTreeModel *model)
{
	/* Right-click? Bring up the menu */
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
		popup_divelist_menu(GTK_TREE_VIEW(treeview), model, 3);
		return TRUE;
	}
	return FALSE;
}

GtkWidget *dive_list_create(void)
{
	GtkTreeSelection  *selection;

	dive_list.model = gtk_list_store_new(DIVELIST_COLUMNS,
				G_TYPE_INT,			/* index */
				G_TYPE_INT,			/* nr */
				G_TYPE_INT,			/* Date */
				G_TYPE_INT,			/* Star rating */
				G_TYPE_INT, 			/* Depth */
				G_TYPE_INT,			/* Duration */
				G_TYPE_INT,			/* Temperature */
				G_TYPE_INT,			/* Total weight */
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

	dive_list.nr = divelist_column(&dive_list, dl_column + DIVE_NR);
	dive_list.date = divelist_column(&dive_list, dl_column + DIVE_DATE);
	dive_list.stars = divelist_column(&dive_list, dl_column + DIVE_RATING);
	dive_list.depth = divelist_column(&dive_list, dl_column + DIVE_DEPTH);
	dive_list.duration = divelist_column(&dive_list, dl_column + DIVE_DURATION);
	dive_list.temperature = divelist_column(&dive_list, dl_column + DIVE_TEMPERATURE);
	dive_list.totalweight = divelist_column(&dive_list, dl_column + DIVE_TOTALWEIGHT);
	dive_list.cylinder = divelist_column(&dive_list, dl_column + DIVE_CYLINDER);
	dive_list.nitrox = divelist_column(&dive_list, dl_column + DIVE_NITROX);
	dive_list.sac = divelist_column(&dive_list, dl_column + DIVE_SAC);
	dive_list.otu = divelist_column(&dive_list, dl_column + DIVE_OTU);
	dive_list.location = divelist_column(&dive_list, dl_column + DIVE_LOCATION);

	fill_dive_list();

	g_object_set(G_OBJECT(dive_list.tree_view), "headers-visible", TRUE,
					  "search-column", DIVE_LOCATION,
					  "rules-hint", TRUE,
					  NULL);

	g_signal_connect_after(dive_list.tree_view, "realize", G_CALLBACK(realize_cb), NULL);
	g_signal_connect(dive_list.tree_view, "row-activated", G_CALLBACK(row_activated_cb), dive_list.model);
	g_signal_connect(dive_list.tree_view, "button-press-event", G_CALLBACK(button_press_cb), dive_list.model);
	g_signal_connect(dive_list.tree_view, "popup-menu", G_CALLBACK(popup_menu_cb), dive_list.model);
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
