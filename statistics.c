/* statistics.c */
/* creates the UI for the Info & Stats page -
 * controlled through the following interfaces:
 *
 * void show_dive_stats(struct dive *dive)
 * void flush_dive_stats_changes(struct dive *dive)
 *
 * called from gtk-ui:
 * GtkWidget *stats_widget(void)
 */
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

typedef struct {
	GtkWidget *date,
		*dive_time,
		*surf_intv,
		*max_depth,
		*avg_depth,
		*viz,
		*water_temp,
		*air_temp,
		*sac,
		*otu,
		*o2he,
		*gas_used;
} single_stat_widget_t;

static single_stat_widget_t single_w;

typedef struct {
	GtkWidget *total_time,
		*avg_time,
		*shortest_time,
		*longest_time,
		*max_overall_depth,
		*min_overall_depth,
		*avg_overall_depth,
		*min_sac,
		*avg_sac,
		*max_sac,
		*selection_size,
		*max_temp,
		*avg_temp,
		*min_temp;
} total_stats_widget_t;

static total_stats_widget_t stats_w;

typedef struct {
	int period;
	duration_t total_time;
	/* avg_time is simply total_time / nr -- let's not keep this */
	duration_t shortest_time;
	duration_t longest_time;
	depth_t max_depth;
	depth_t min_depth;
	depth_t avg_depth;
	volume_t max_sac;
	volume_t min_sac;
	volume_t avg_sac;
	int max_temp;
	int min_temp;
	double combined_temp;
	unsigned int combined_count;
	unsigned int selection_size;
	unsigned int total_sac_time;
} stats_t;

static stats_t stats;
static stats_t stats_selection;
static stats_t *stats_monthly = NULL;
static stats_t *stats_yearly = NULL;

GtkWidget *yearly_tree = NULL;

enum {
	YEAR,
	DIVES,
	TOTAL_TIME,
	AVERAGE_TIME,
	SHORTEST_TIME,
	LONGEST_TIME,
	AVG_DEPTH,
	MIN_DEPTH,
	MAX_DEPTH,
	AVG_SAC,
	MIN_SAC,
	MAX_SAC,
	AVG_TEMP,
	MIN_TEMP,
	MAX_TEMP,
	N_COLUMNS
};

static char * get_time_string(int seconds, int maxdays);

static void process_dive(struct dive *dp, stats_t *stats)
{
	int old_tt, sac_time = 0;
	const char *unit;

	old_tt = stats->total_time.seconds;
	stats->total_time.seconds += dp->duration.seconds;
	if (dp->duration.seconds > stats->longest_time.seconds)
		stats->longest_time.seconds = dp->duration.seconds;
	if (stats->shortest_time.seconds == 0 || dp->duration.seconds < stats->shortest_time.seconds)
		stats->shortest_time.seconds = dp->duration.seconds;
	if (dp->maxdepth.mm > stats->max_depth.mm)
		stats->max_depth.mm = dp->maxdepth.mm;
	if (stats->min_depth.mm == 0 || dp->maxdepth.mm < stats->min_depth.mm)
		stats->min_depth.mm = dp->maxdepth.mm;
	if (dp->watertemp.mkelvin) {
		if (stats->min_temp == 0 || dp->watertemp.mkelvin < stats->min_temp)
			stats->min_temp = dp->watertemp.mkelvin;
		if (dp->watertemp.mkelvin > stats->max_temp)
			stats->max_temp = dp->watertemp.mkelvin;
		stats->combined_temp += get_temp_units(dp->watertemp.mkelvin, &unit);
		stats->combined_count++;
	}

	/* Maybe we should drop zero-duration dives */
	if (!dp->duration.seconds)
		return;
	stats->avg_depth.mm = (1.0 * old_tt * stats->avg_depth.mm +
			dp->duration.seconds * dp->meandepth.mm) / stats->total_time.seconds;
	if (dp->sac > 2800) { /* less than .1 cuft/min (2800ml/min) is bogus */
		sac_time = stats->total_sac_time + dp->duration.seconds;
		stats->avg_sac.mliter = (1.0 * stats->total_sac_time * stats->avg_sac.mliter +
				dp->duration.seconds * dp->sac) / sac_time ;
		if (dp->sac > stats->max_sac.mliter)
			stats->max_sac.mliter = dp->sac;
		if (stats->min_sac.mliter == 0 || dp->sac < stats->min_sac.mliter)
			stats->min_sac.mliter = dp->sac;
		stats->total_sac_time = sac_time;
	}
}

static void init_tree()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeStore *store;
	int i;
	PangoFontDescription *font_desc = pango_font_description_from_string(divelist_font);

	gtk_widget_modify_font(yearly_tree, font_desc);
	pango_font_description_free(font_desc);

	renderer = gtk_cell_renderer_text_new ();
	/* don't use empty strings "" - they confuse gettext */
	char *columnstop[] = { N_("Year"), N_("#"), N_("Duration"), " ", " ", " ", N_("Depth"), " ", " ", N_("SAC"), " ", " ", N_("Temperature"), " ", " " };
	const char *columnsbot[15];
	columnsbot[0] = C_("Stats", " > Month");
	columnsbot[1] = " ";
	columnsbot[2] = C_("Duration","Total");
	columnsbot[3] = C_("Duration","Average");
	columnsbot[4] = C_("Duration","Shortest");
	columnsbot[5] = C_("Duration","Longest");
	columnsbot[6] = C_("Depth", "Average");
	columnsbot[7] = C_("Depth","Minimum");
	columnsbot[8] = C_("Depth","Maximum");
	columnsbot[9] = C_("SAC","Average");
	columnsbot[10]= C_("SAC","Minimum");
	columnsbot[11]= C_("SAC","Maximum");
	columnsbot[12]= C_("Temp","Average");
	columnsbot[13]= C_("Temp","Minimum");
	columnsbot[14]= C_("Temp","Maximum");

	/* Add all the columns to the tree view */
	for (i = 0; i < N_COLUMNS; ++i) {
		char buf[80];
		column = gtk_tree_view_column_new();
		snprintf(buf, sizeof(buf), "%s\n%s", _(columnstop[i]), columnsbot[i]);
		gtk_tree_view_column_set_title(column, buf);
		gtk_tree_view_append_column(GTK_TREE_VIEW(yearly_tree), column);
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_add_attribute(column, renderer, "text", i);
		gtk_tree_view_column_set_resizable(column, TRUE);
	}

	/* Field types */
	store = gtk_tree_store_new (
			N_COLUMNS,	// Columns in structure
			G_TYPE_STRING,	// Period (year or month)
			G_TYPE_STRING,	// Number of dives
			G_TYPE_STRING,	// Total duration
			G_TYPE_STRING,	// Average dive duation
			G_TYPE_STRING,	// Shortest dive
			G_TYPE_STRING,	// Longest dive
			G_TYPE_STRING,	// Average depth
			G_TYPE_STRING,	// Shallowest dive
			G_TYPE_STRING,	// Deepest dive
			G_TYPE_STRING,	// Average air consumption (SAC)
			G_TYPE_STRING,	// Minimum SAC
			G_TYPE_STRING,	// Maximum SAC
			G_TYPE_STRING,	// Average temperature
			G_TYPE_STRING,	// Minimum temperature
			G_TYPE_STRING	// Maximum temperature
			);

	gtk_tree_view_set_model (GTK_TREE_VIEW (yearly_tree), GTK_TREE_MODEL (store));
	g_object_unref (store);
}

static void add_row_to_tree(GtkTreeStore *store, char *value, int index, GtkTreeIter *row_iter, GtkTreeIter *parent)
{
	gtk_tree_store_append(store, row_iter, parent);
	gtk_tree_store_set(store, row_iter, index, value, -1);
}

static void add_cell_to_tree(GtkTreeStore *store, char *value, int index, GtkTreeIter *parent)
{
	gtk_tree_store_set(store, parent, index, value, -1);
}
static char * get_minutes(int seconds)
{
	static char buf[80];
	snprintf(buf, sizeof(buf), "%d:%.2d", FRACTION(seconds, 60));
	return buf;
}

static void add_cell(GtkTreeStore *store, GtkTreeIter *parent, unsigned int val, int cell, gboolean depth_not_volume)
{
	double value;
	int decimals;
	const char *unit;
	char value_str[40];

	if (depth_not_volume) {
		value = get_depth_units(val, &decimals, &unit);
		snprintf(value_str, sizeof(value_str), "%.*f %s", decimals, value, unit);
	} else {
		value = get_volume_units(val, &decimals, &unit);
		snprintf(value_str, sizeof(value_str), "%.*f %s/min", decimals, value, unit);
	}
	add_cell_to_tree(store, value_str, cell, parent);
}

static void process_interval_stats(stats_t stats_interval, GtkTreeIter *parent, GtkTreeIter *row)
{
	double value;
	const char *unit;
	char value_str[40];
	GtkTreeStore *store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(yearly_tree)));

	/* Year or month */
	snprintf(value_str, sizeof(value_str), "%d", stats_interval.period);
	add_row_to_tree(store, value_str, 0, row, parent);
	/* Dives */
	snprintf(value_str, sizeof(value_str), "%d", stats_interval.selection_size);
	add_cell_to_tree(store, value_str, 1,  row);
	/* Total duration */
	add_cell_to_tree(store, get_time_string(stats_interval.total_time.seconds, 0), 2, row);
	/* Average dive duration */
	add_cell_to_tree(store, get_minutes(stats_interval.total_time.seconds / stats_interval.selection_size), 3, row);
	/* Shortest duration */
	add_cell_to_tree(store, get_minutes(stats_interval.shortest_time.seconds), 4, row);
	/* Longest duration */
	add_cell_to_tree(store, get_minutes(stats_interval.longest_time.seconds), 5, row);
	/* Average depth */
	add_cell(store, row, stats_interval.avg_depth.mm, 6, TRUE);
	/* Smallest maximum depth */
	add_cell(store, row, stats_interval.min_depth.mm, 7, TRUE);
	/* Deepest maximum depth */
	add_cell(store, row, stats_interval.max_depth.mm, 8, TRUE);
	/* Average air consumption */
	add_cell(store, row, stats_interval.avg_sac.mliter, 9, FALSE);
	/* Smallest average air consumption */
	add_cell(store, row, stats_interval.min_sac.mliter, 10, FALSE);
	/* Biggest air consumption */
	add_cell(store, row, stats_interval.max_sac.mliter, 11, FALSE);
	/* Average water temperature */
	value = get_temp_units(stats_interval.min_temp, &unit);
	if (stats_interval.combined_temp && stats_interval.combined_count) {
		snprintf(value_str, sizeof(value_str), "%.1f %s", stats_interval.combined_temp / stats_interval.combined_count, unit);
		add_cell_to_tree(store, value_str, 12, row);
	} else {
		add_cell_to_tree(store, "", 12, row);
	}
	/* Coldest water temperature */
	if (value > -100.0) {
		snprintf(value_str, sizeof(value_str), "%.1f %s\t", value, unit);
		add_cell_to_tree(store, value_str, 13, row);
	} else {
		add_cell_to_tree(store, "", 13, row);
	}
	/* Warmest water temperature */
	value = get_temp_units(stats_interval.max_temp, &unit);
	if (value > -100.0) {
		snprintf(value_str, sizeof(value_str), "%.1f %s", value, unit);
		add_cell_to_tree(store, value_str, 14, row);
	} else {
		add_cell_to_tree(store, "", 14, row);
	}
}

void clear_statistics()
{
	GtkTreeStore *store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(yearly_tree)));
	gtk_tree_store_clear(store);
	yearly_tree = NULL;
}

static gboolean stat_on_delete(GtkWidget *window, GdkEvent *event, gpointer data)
{
	clear_statistics();
	gtk_widget_destroy(window);
	return TRUE;
}

static void key_press_event(GtkWidget *window, GdkEventKey *event, gpointer data)
{
	if ((event->string != NULL && event->keyval == GDK_Escape) ||
			(event->string != NULL && event->keyval == GDK_w && event->state & GDK_CONTROL_MASK)) {
		clear_statistics();
		gtk_widget_destroy(window);
	}
}

void update_yearly_stats()
{
	int i, j, combined_months, month = 0;
	GtkTreeIter year_iter, month_iter;
	GtkTreeStore *store;

	store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(yearly_tree)));
	gtk_tree_store_clear(store);

	for (i = 0; stats_yearly != NULL && stats_yearly[i].period; ++i) {
		process_interval_stats(stats_yearly[i], NULL, &year_iter);
		combined_months = 0;

		for (j = 0; combined_months < stats_yearly[i].selection_size; ++j) {
			combined_months += stats_monthly[month].selection_size;
			process_interval_stats(stats_monthly[month++], &year_iter, &month_iter);
		}
	}
}

void show_yearly_stats()
{
	GtkWidget *window;
	GtkWidget *sw;

	if (yearly_tree)
		return;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sw = gtk_scrolled_window_new (NULL, NULL);
	yearly_tree = gtk_tree_view_new ();

	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	gtk_window_set_title(GTK_WINDOW(window), _("Yearly Statistics"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	GTK_WINDOW(window)->allow_shrink = TRUE;
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);

	gtk_container_add (GTK_CONTAINER (sw), yearly_tree);
	gtk_container_add (GTK_CONTAINER (window), sw);

	/* Display the yearly statistics on top level
	 * Monthly statistics are available by expanding a year */
	init_tree();
	update_yearly_stats();

	g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (key_press_event), NULL);
	g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (stat_on_delete), NULL);
	gtk_widget_show_all(window);
}

static void process_all_dives(struct dive *dive, struct dive **prev_dive)
{
	int idx;
	struct dive *dp;
	struct tm tm;
	int current_year = 0;
	int current_month = 0;
	int year_iter = 0;
	int month_iter = 0;
	int prev_month = 0, prev_year = 0;
	unsigned int size;

	*prev_dive = NULL;
	memset(&stats, 0, sizeof(stats));
	if (dive_table.nr > 0) {
		stats.shortest_time.seconds = dive_table.dives[0]->duration.seconds;
		stats.min_depth.mm = dive_table.dives[0]->maxdepth.mm;
		stats.selection_size = dive_table.nr;
	}

	/* allocate sufficient space to hold the worst
	 * case (one dive per year or all dives during
	 * one month) for yearly and monthly statistics*/

	if (stats_yearly != NULL) {
		free(stats_yearly);
		free(stats_monthly);
	}
	size = sizeof(stats_t) * (dive_table.nr + 1);
	stats_yearly = malloc(size);
	stats_monthly = malloc(size);
	if (!stats_yearly || !stats_monthly)
		return;
	memset(stats_yearly, 0, size);
	memset(stats_monthly, 0, size);

	/* this relies on the fact that the dives in the dive_table
	 * are in chronological order */
	for (idx = 0; idx < dive_table.nr; idx++) {
		dp = dive_table.dives[idx];
		if (dp->when == dive->when) {
			/* that's the one we are showing */
			if (idx > 0)
				*prev_dive = dive_table.dives[idx-1];
		}
		process_dive(dp, &stats);

		/* yearly statistics */
		utc_mkdate(dp->when, &tm);
		if (current_year == 0)
			current_year = tm.tm_year + 1900;

		if (current_year != tm.tm_year + 1900) {
			current_year = tm.tm_year + 1900;
			process_dive(dp, &(stats_yearly[++year_iter]));
		} else
			process_dive(dp, &(stats_yearly[year_iter]));

		stats_yearly[year_iter].selection_size++;
		stats_yearly[year_iter].period = current_year;

		/* monthly statistics */
		if (current_month == 0) {
			current_month = tm.tm_mon + 1;
		} else {
			if (current_month != tm.tm_mon + 1)
				current_month = tm.tm_mon + 1;
			if (prev_month != current_month || prev_year != current_year)
				month_iter++;
		}

		process_dive(dp, &(stats_monthly[month_iter]));
		stats_monthly[month_iter].selection_size++;
		stats_monthly[month_iter].period = current_month;
		prev_month = current_month;
		prev_year = current_year;
	}
	if (yearly_tree)
		update_yearly_stats();
}

/* make sure we skip the selected summary entries */
void process_selected_dives(void)
{
	struct dive *dive;
	unsigned int i, nr;

	memset(&stats_selection, 0, sizeof(stats_selection));

	nr = 0;
	for_each_dive(i, dive) {
		if (dive->selected) {
			process_dive(dive, &stats_selection);
			nr++;
		}
	}
	stats_selection.selection_size = nr;
}

static void set_label(GtkWidget *w, const char *fmt, ...)
{
	char buf[80];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	gtk_label_set_text(GTK_LABEL(w), buf);
}

static char * get_time_string(int seconds, int maxdays)
{
	static char buf[80];
	if (maxdays && seconds > 3600 * 24 * maxdays)
		snprintf(buf, sizeof(buf), _("more than %d days"), maxdays);
	else {
		int days = seconds / 3600 / 24;
		int hours = (seconds - days * 3600 * 24) / 3600;
		int minutes = (seconds - days * 3600 * 24 - hours * 3600) / 60;
		if (days > 0)
			snprintf(buf, sizeof(buf), _("%dd %dh %dmin"), days, hours, minutes);
		else
			snprintf(buf, sizeof(buf), _("%dh %dmin"), hours, minutes);
	}
	return buf;
}

static void show_single_dive_stats(struct dive *dive)
{
	char buf[80];
	double value;
	int decimals;
	const char *unit;
	int idx, offset, gas_used;
	struct dive *prev_dive;
	struct tm tm;

	process_all_dives(dive, &prev_dive);

	utc_mkdate(dive->when, &tm);
	snprintf(buf, sizeof(buf),
		/*++GETTEXT 80 chars: weekday, monthname, day, year, hour, min */
		_("%1$s, %2$s %3$d, %4$d %5$2d:%6$02d"),
		weekday(tm.tm_wday),
		monthname(tm.tm_mon),
		tm.tm_mday, tm.tm_year + 1900,
		tm.tm_hour, tm.tm_min);

	set_label(single_w.date, buf);
	set_label(single_w.dive_time, _("%d min"), (dive->duration.seconds + 30) / 60);
	if (prev_dive)
		set_label(single_w.surf_intv,
			get_time_string(dive->when - (prev_dive->when + prev_dive->duration.seconds), 4));
	else
		set_label(single_w.surf_intv, _("unknown"));
	value = get_depth_units(dive->maxdepth.mm, &decimals, &unit);
	set_label(single_w.max_depth, "%.*f %s", decimals, value, unit);
	value = get_depth_units(dive->meandepth.mm, &decimals, &unit);
	set_label(single_w.avg_depth, "%.*f %s", decimals, value, unit);
	set_label(single_w.viz, star_strings[dive->visibility]);
	if (dive->watertemp.mkelvin) {
		value = get_temp_units(dive->watertemp.mkelvin, &unit);
		set_label(single_w.water_temp, "%.1f %s", value, unit);
	} else
		set_label(single_w.water_temp, "");
	if (dive->airtemp.mkelvin) {
		value = get_temp_units(dive->airtemp.mkelvin, &unit);
		set_label(single_w.air_temp, "%.1f %s", value, unit);
	} else
		set_label(single_w.air_temp, "");
	value = get_volume_units(dive->sac, &decimals, &unit);
	if (value > 0) {
		set_label(single_w.sac, _("%.*f %s/min"), decimals, value, unit);
	} else
		set_label(single_w.sac, "");
	set_label(single_w.otu, "%d", dive->otu);
	offset = 0;
	gas_used = 0;
	buf[0] = '\0';
	/* for the O2/He readings just create a list of them */
	for (idx = 0; idx < MAX_CYLINDERS; idx++) {
		cylinder_t *cyl = &dive->cylinder[idx];
		unsigned int start, end;

		start = cyl->start.mbar ? : cyl->sample_start.mbar;
		end = cyl->end.mbar ? : cyl->sample_end.mbar;
		if (!cylinder_none(cyl)) {
			/* 0% O2 strangely means air, so 21% - I don't like that at all */
			int o2 = cyl->gasmix.o2.permille ? : AIR_PERMILLE;
			if (offset > 0) {
				snprintf(buf+offset, 80-offset, ", ");
				offset += 2;
			}
			snprintf(buf+offset, 80-offset, "%d/%d", (o2 + 5) / 10,
				(cyl->gasmix.he.permille + 5) / 10);
			offset = strlen(buf);
		}
		/* and if we have size, start and end pressure, we can
		 * calculate the total gas used */
		if (cyl->type.size.mliter && start && end)
			gas_used += cyl->type.size.mliter / 1000.0 * (start - end);
	}
	set_label(single_w.o2he, buf);
	if (gas_used) {
		value = get_volume_units(gas_used, &decimals, &unit);
		set_label(single_w.gas_used, "%.*f %s", decimals, value, unit);
	} else
		set_label(single_w.gas_used, "");
}

static void show_total_dive_stats(struct dive *dive)
{
	double value;
	int decimals, seconds;
	const char *unit;
	stats_t *stats_ptr;

	stats_ptr = &stats_selection;

	set_label(stats_w.selection_size, "%d", stats_ptr->selection_size);
	if (stats_ptr->min_temp) {
		value = get_temp_units(stats_ptr->min_temp, &unit);
		set_label(stats_w.min_temp, "%.1f %s", value, unit);
	}
	if (stats_ptr->combined_temp && stats_ptr->combined_count)
		set_label(stats_w.avg_temp, "%.1f %s", stats_ptr->combined_temp / stats_ptr->combined_count, unit);
	if (stats_ptr->max_temp) {
		value = get_temp_units(stats_ptr->max_temp, &unit);
		set_label(stats_w.max_temp, "%.1f %s", value, unit);
	}
	set_label(stats_w.total_time, get_time_string(stats_ptr->total_time.seconds, 0));
	seconds = stats_ptr->total_time.seconds;
	if (stats_ptr->selection_size)
		seconds /= stats_ptr->selection_size;
	set_label(stats_w.avg_time, get_time_string(seconds, 0));
	set_label(stats_w.longest_time, get_time_string(stats_ptr->longest_time.seconds, 0));
	set_label(stats_w.shortest_time, get_time_string(stats_ptr->shortest_time.seconds, 0));
	value = get_depth_units(stats_ptr->max_depth.mm, &decimals, &unit);
	set_label(stats_w.max_overall_depth, "%.*f %s", decimals, value, unit);
	value = get_depth_units(stats_ptr->min_depth.mm, &decimals, &unit);
	set_label(stats_w.min_overall_depth, "%.*f %s", decimals, value, unit);
	value = get_depth_units(stats_ptr->avg_depth.mm, &decimals, &unit);
	set_label(stats_w.avg_overall_depth, "%.*f %s", decimals, value, unit);
	value = get_volume_units(stats_ptr->max_sac.mliter, &decimals, &unit);
	set_label(stats_w.max_sac, _("%.*f %s/min"), decimals, value, unit);
	value = get_volume_units(stats_ptr->min_sac.mliter, &decimals, &unit);
	set_label(stats_w.min_sac, _("%.*f %s/min"), decimals, value, unit);
	value = get_volume_units(stats_ptr->avg_sac.mliter, &decimals, &unit);
	set_label(stats_w.avg_sac, _("%.*f %s/min"), decimals, value, unit);
}

void show_dive_stats(struct dive *dive)
{
	/* they have to be called in this order, as 'total' depends on
	 * calculations done in 'single' */
	show_single_dive_stats(dive);
	show_total_dive_stats(dive);
}

void flush_dive_stats_changes(struct dive *dive)
{
	/* We do nothing: we require the "Ok" button press */
}

static GtkWidget *new_info_label_in_frame(GtkWidget *box, const char *label)
{
	GtkWidget *label_widget;
	GtkWidget *frame;

	frame = gtk_frame_new(label);
	label_widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 3);
	gtk_container_add(GTK_CONTAINER(frame), label_widget);

	return label_widget;
}

GtkWidget *total_stats_widget(void)
{
	GtkWidget *vbox, *hbox, *statsframe, *framebox;

	vbox = gtk_vbox_new(FALSE, 3);

	statsframe = gtk_frame_new(_("Statistics"));
	gtk_box_pack_start(GTK_BOX(vbox), statsframe, TRUE, FALSE, 3);
	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(statsframe), framebox);

	/* first row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);
	stats_w.selection_size = new_info_label_in_frame(hbox, _("Dives"));
	stats_w.max_temp = new_info_label_in_frame(hbox, _("Max Temp"));
	stats_w.min_temp = new_info_label_in_frame(hbox, _("Min Temp"));
	stats_w.avg_temp = new_info_label_in_frame(hbox, _("Avg Temp"));

	/* second row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	stats_w.total_time = new_info_label_in_frame(hbox, _("Total Time"));
	stats_w.avg_time = new_info_label_in_frame(hbox, _("Avg Time"));
	stats_w.longest_time = new_info_label_in_frame(hbox, _("Longest Dive"));
	stats_w.shortest_time = new_info_label_in_frame(hbox, _("Shortest Dive"));

	/* third row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	stats_w.max_overall_depth = new_info_label_in_frame(hbox, _("Max Depth"));
	stats_w.min_overall_depth = new_info_label_in_frame(hbox, _("Min Depth"));
	stats_w.avg_overall_depth = new_info_label_in_frame(hbox, _("Avg Depth"));

	/* fourth row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	stats_w.max_sac = new_info_label_in_frame(hbox, _("Max SAC"));
	stats_w.min_sac = new_info_label_in_frame(hbox, _("Min SAC"));
	stats_w.avg_sac = new_info_label_in_frame(hbox, _("Avg SAC"));

	return vbox;
}

GtkWidget *single_stats_widget(void)
{
	GtkWidget *vbox, *hbox, *infoframe, *framebox;

	vbox = gtk_vbox_new(FALSE, 3);

	infoframe = gtk_frame_new(_("Dive Info"));
	gtk_box_pack_start(GTK_BOX(vbox), infoframe, TRUE, FALSE, 3);
	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(infoframe), framebox);

	/* first row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.date = new_info_label_in_frame(hbox, _("Date"));
	single_w.dive_time = new_info_label_in_frame(hbox, _("Dive Time"));
	single_w.surf_intv = new_info_label_in_frame(hbox, _("Surf Intv"));

	/* second row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.max_depth = new_info_label_in_frame(hbox, _("Max Depth"));
	single_w.avg_depth = new_info_label_in_frame(hbox, _("Avg Depth"));
	single_w.viz = new_info_label_in_frame(hbox, _("Visibility"));

	/* third row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.water_temp = new_info_label_in_frame(hbox, _("Water Temp"));
	single_w.air_temp = new_info_label_in_frame(hbox, _("Air Temp"));

	/* fourth row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.sac = new_info_label_in_frame(hbox, _("SAC"));
	single_w.otu = new_info_label_in_frame(hbox, _("OTU"));
	single_w.o2he = new_info_label_in_frame(hbox, "O" UTF8_SUBSCRIPT_2 " / He");
	single_w.gas_used = new_info_label_in_frame(hbox, _("Gas Used"));

	return vbox;
}

void clear_stats_widgets(void)
{
	set_label(single_w.date, "");
	set_label(single_w.dive_time, "");
	set_label(single_w.surf_intv, "");
	set_label(single_w.max_depth, "");
	set_label(single_w.avg_depth, "");
	set_label(single_w.viz, "");
	set_label(single_w.water_temp, "");
	set_label(single_w.air_temp, "");
	set_label(single_w.sac, "");
	set_label(single_w.sac, "");
	set_label(single_w.otu, "");
	set_label(single_w.o2he, "");
	set_label(single_w.gas_used, "");
	set_label(stats_w.total_time,"");
	set_label(stats_w.avg_time,"");
	set_label(stats_w.shortest_time,"");
	set_label(stats_w.longest_time,"");
	set_label(stats_w.max_overall_depth,"");
	set_label(stats_w.min_overall_depth,"");
	set_label(stats_w.avg_overall_depth,"");
	set_label(stats_w.min_sac,"");
	set_label(stats_w.avg_sac,"");
	set_label(stats_w.max_sac,"");
	set_label(stats_w.selection_size,"");
	set_label(stats_w.max_temp,"");
	set_label(stats_w.avg_temp,"");
	set_label(stats_w.min_temp,"");
}
