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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

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
		*water_temp,
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
	unsigned int combined_temp;
	unsigned int combined_count;
	unsigned int selection_size;
	unsigned int total_sac_time;
} stats_t;

static stats_t stats;
static stats_t stats_selection;


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

static void process_all_dives(struct dive *dive, struct dive **prev_dive)
{
	int idx;
	struct dive *dp;

	*prev_dive = NULL;
	memset(&stats, 0, sizeof(stats));
	if (dive_table.nr > 0) {
		stats.shortest_time.seconds = dive_table.dives[0]->duration.seconds;
		stats.min_depth.mm = dive_table.dives[0]->maxdepth.mm;
		stats.selection_size = dive_table.nr;
	}
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
	}
}

void process_selected_dives(GList *selected_dives, GtkTreeModel *model)
{
	struct dive *dp;
	unsigned int i;
	GtkTreeIter iter;
	GtkTreePath *path;

	memset(&stats_selection, 0, sizeof(stats_selection));
	stats_selection.selection_size = amount_selected;

	for (i = 0; i < amount_selected; ++i) {
		GValue value = {0, };
		path = g_list_nth_data(selected_dives, i);
		if (gtk_tree_model_get_iter(model, &iter, path)) {
			gtk_tree_model_get_value(model, &iter, 0, &value);
			dp = get_dive(g_value_get_int(&value));
		}
		process_dive(dp, &stats_selection);
	}
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
		snprintf(buf, sizeof(buf), "more than %d days", maxdays);
	else {
		int days = seconds / 3600 / 24;
		int hours = (seconds - days * 3600 * 24) / 3600;
		int minutes = (seconds - days * 3600 * 24 - hours * 3600) / 60;
		if (days > 0)
			snprintf(buf, sizeof(buf), "%dd %dh %dmin", days, hours, minutes);
		else
			snprintf(buf, sizeof(buf), "%dh %dmin", hours, minutes);
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
	struct tm *tm;

	process_all_dives(dive, &prev_dive);

	tm = gmtime(&dive->when);
	snprintf(buf, sizeof(buf),
		"%s, %s %d, %d %2d:%02d",
		weekday(tm->tm_wday),
		monthname(tm->tm_mon),
		tm->tm_mday, tm->tm_year + 1900,
		tm->tm_hour, tm->tm_min);

	set_label(single_w.date, buf);
	set_label(single_w.dive_time, "%d min", (dive->duration.seconds + 30) / 60);
	if (prev_dive)
		set_label(single_w.surf_intv, 
			get_time_string(dive->when - (prev_dive->when + prev_dive->duration.seconds), 4));
	else
		set_label(single_w.surf_intv, "unknown");
	value = get_depth_units(dive->maxdepth.mm, &decimals, &unit);
	set_label(single_w.max_depth, "%.*f %s", decimals, value, unit);
	value = get_depth_units(dive->meandepth.mm, &decimals, &unit);
	set_label(single_w.avg_depth, "%.*f %s", decimals, value, unit);
	if (dive->watertemp.mkelvin) {
		value = get_temp_units(dive->watertemp.mkelvin, &unit);
		set_label(single_w.water_temp, "%.1f %s", value, unit);
	} else
		set_label(single_w.water_temp, "");
	value = get_volume_units(dive->sac, &decimals, &unit);
	if (value > 0) {
		set_label(single_w.sac, "%.*f %s/min", decimals, value, unit);
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

	if (amount_selected < 2)
		stats_ptr = &stats;
	else
		stats_ptr = &stats_selection;

	set_label(stats_w.selection_size, "%d", stats_ptr->selection_size);
	if (stats_ptr->min_temp) {
		value = get_temp_units(stats_ptr->min_temp, &unit);
		set_label(stats_w.min_temp, "%.1f %s", value, unit);
	}
	if (stats_ptr->combined_temp && stats_ptr->combined_count)
		set_label(stats_w.avg_temp, "%.1f %s", stats_ptr->combined_temp / (stats_ptr->combined_count * 1.0), unit);
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
	set_label(stats_w.max_sac, "%.*f %s/min", decimals, value, unit);
	value = get_volume_units(stats_ptr->min_sac.mliter, &decimals, &unit);
	set_label(stats_w.min_sac, "%.*f %s/min", decimals, value, unit);
	value = get_volume_units(stats_ptr->avg_sac.mliter, &decimals, &unit);
	set_label(stats_w.avg_sac, "%.*f %s/min", decimals, value, unit);
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

	statsframe = gtk_frame_new("Statistics");
	gtk_box_pack_start(GTK_BOX(vbox), statsframe, TRUE, FALSE, 3);
	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(statsframe), framebox);

	/* first row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);
	stats_w.selection_size = new_info_label_in_frame(hbox, "Dives");
	stats_w.max_temp = new_info_label_in_frame(hbox, "Max Temp");
	stats_w.min_temp = new_info_label_in_frame(hbox, "Min Temp");
	stats_w.avg_temp = new_info_label_in_frame(hbox, "Avg Temp");

	/* second row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	stats_w.total_time = new_info_label_in_frame(hbox, "Total Time");
	stats_w.avg_time = new_info_label_in_frame(hbox, "Avg Time");
	stats_w.longest_time = new_info_label_in_frame(hbox, "Longest Dive");
	stats_w.shortest_time = new_info_label_in_frame(hbox, "Shortest Dive");

	/* third row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	stats_w.max_overall_depth = new_info_label_in_frame(hbox, "Max Depth");
	stats_w.min_overall_depth = new_info_label_in_frame(hbox, "Min Depth");
	stats_w.avg_overall_depth = new_info_label_in_frame(hbox, "Avg Depth");

	/* fourth row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	stats_w.max_sac = new_info_label_in_frame(hbox, "Max SAC");
	stats_w.min_sac = new_info_label_in_frame(hbox, "Min SAC");
	stats_w.avg_sac = new_info_label_in_frame(hbox, "Avg SAC");

	return vbox;
}

GtkWidget *single_stats_widget(void)
{
	GtkWidget *vbox, *hbox, *infoframe, *framebox;

	vbox = gtk_vbox_new(FALSE, 3);

	infoframe = gtk_frame_new("Dive Info");
	gtk_box_pack_start(GTK_BOX(vbox), infoframe, TRUE, FALSE, 3);
	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(infoframe), framebox);

	/* first row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.date = new_info_label_in_frame(hbox, "Date");
	single_w.dive_time = new_info_label_in_frame(hbox, "Dive Time");
	single_w.surf_intv = new_info_label_in_frame(hbox, "Surf Intv");

	/* second row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.max_depth = new_info_label_in_frame(hbox, "Max Depth");
	single_w.avg_depth = new_info_label_in_frame(hbox, "Avg Depth");
	single_w.water_temp = new_info_label_in_frame(hbox, "Water Temp");

	/* third row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	single_w.sac = new_info_label_in_frame(hbox, "SAC");
	single_w.otu = new_info_label_in_frame(hbox, "OTU");
	single_w.o2he = new_info_label_in_frame(hbox, "O" UTF8_SUBSCRIPT_2 " / He");
	single_w.gas_used = new_info_label_in_frame(hbox, "Gas Used");

	return vbox;
}
