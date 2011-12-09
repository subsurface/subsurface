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
		*gas_used,
		*total_time,
		*avg_time,
		*max_overall_depth,
		*avg_overall_depth,
		*min_sac,
		*avg_sac,
		*max_sac;
} info_stat_widget_t;

static info_stat_widget_t info_stat_w;

typedef struct {
	duration_t total_time;
	/* avg_time is simply total_time / nr -- let's not keep this */
	depth_t max_depth;
	depth_t avg_depth;
	volume_t max_sac;
	volume_t min_sac;
	volume_t avg_sac;
} info_stat_t;

static info_stat_t info_stat;

static void process_all_dives(struct dive *dive, struct dive **prev_dive)
{
	int idx;
	struct dive *dp;
	int old_tt, sac_time = 0;

	*prev_dive = NULL;
	memset(&info_stat, 0, sizeof(info_stat));
	/* this relies on the fact that the dives in the dive_table
	 * are in chronological order */
	for (idx = 0; idx < dive_table.nr; idx++) {
		dp = dive_table.dives[idx];
		if (dp->when == dive->when) {
			/* that's the one we are showing */
			if (idx > 0)
				*prev_dive = dive_table.dives[idx-1];
		}
		old_tt = info_stat.total_time.seconds;
		info_stat.total_time.seconds += dp->duration.seconds;
		if (dp->maxdepth.mm > info_stat.max_depth.mm)
			info_stat.max_depth.mm = dp->maxdepth.mm;
		info_stat.avg_depth.mm = (1.0 * old_tt * info_stat.avg_depth.mm +
				dp->duration.seconds * dp->meandepth.mm) / info_stat.total_time.seconds;
		if (dp->sac > 2800) { /* less than .1 cuft/min (2800ml/min) is bogus */
			int old_sac_time = sac_time;
			sac_time += dp->duration.seconds;
			info_stat.avg_sac.mliter = (1.0 * old_sac_time * info_stat.avg_sac.mliter +
						dp->duration.seconds * dp->sac) / sac_time ;
			if (dp->sac > info_stat.max_sac.mliter)
				info_stat.max_sac.mliter = dp->sac;
			if (info_stat.min_sac.mliter == 0 || dp->sac < info_stat.min_sac.mliter)
				info_stat.min_sac.mliter = dp->sac;
		}
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

void show_dive_stats(struct dive *dive)
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

	set_label(info_stat_w.date, buf);
	set_label(info_stat_w.dive_time, "%d min", (dive->duration.seconds + 30) / 60);
	if (prev_dive)
		set_label(info_stat_w.surf_intv, 
			get_time_string(dive->when - (prev_dive->when + prev_dive->duration.seconds), 4));
	else
		set_label(info_stat_w.surf_intv, "unknown");
	value = get_depth_units(dive->maxdepth.mm, &decimals, &unit);
	set_label(info_stat_w.max_depth, "%.*f %s", decimals, value, unit);
	value = get_depth_units(dive->meandepth.mm, &decimals, &unit);
	set_label(info_stat_w.avg_depth, "%.*f %s", decimals, value, unit);
	if (dive->watertemp.mkelvin) {
		value = get_temp_units(dive->watertemp.mkelvin, &unit);
		set_label(info_stat_w.water_temp, "%.1f %s", value, unit);
	} else
		set_label(info_stat_w.water_temp, "");
	value = get_volume_units(dive->sac, &decimals, &unit);
	if (value > 0) {
		set_label(info_stat_w.sac, "%.*f %s/min", decimals, value, unit);
	} else
		set_label(info_stat_w.sac, "");
	set_label(info_stat_w.otu, "%d", dive->otu);
	offset = 0;
	gas_used = 0;
	buf[0] = '\0';
	/* for the O2/He readings just create a list of them */
	for (idx = 0; idx < MAX_CYLINDERS; idx++) {
		cylinder_t *cyl = &dive->cylinder[idx];
		unsigned int start, end;

		start = cyl->start.mbar ? : cyl->sample_start.mbar;
		end = cyl->end.mbar ? : cyl->sample_end.mbar;
		/* we assume that every valid cylinder has either a working pressure
		 * or a size; but for good measure let's also accept cylinders with
		 * a starting or ending pressure*/
		if (cyl->type.workingpressure.mbar || cyl->type.size.mliter || cyl->gasmix.o2.permille || start || end) {
			/* 0% O2 strangely means air, so 21% - I don't like that at all */
			int o2 = cyl->gasmix.o2.permille ? : 209;
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
	set_label(info_stat_w.o2he, buf);
	if (gas_used) {
		value = get_volume_units(gas_used, &decimals, &unit);
		set_label(info_stat_w.gas_used, "%.*f %s", decimals, value, unit);
	} else
		set_label(info_stat_w.gas_used, "");
	/* and now do the statistics */
	set_label(info_stat_w.total_time, get_time_string(info_stat.total_time.seconds, 0));
	set_label(info_stat_w.avg_time, get_time_string(info_stat.total_time.seconds / dive_table.nr, 0));
	value = get_depth_units(info_stat.max_depth.mm, &decimals, &unit);
	set_label(info_stat_w.max_overall_depth, "%.*f %s", decimals, value, unit);
	value = get_depth_units(info_stat.avg_depth.mm, &decimals, &unit);
	set_label(info_stat_w.avg_overall_depth, "%.*f %s", decimals, value, unit);
	value = get_volume_units(info_stat.max_sac.mliter, &decimals, &unit);
	set_label(info_stat_w.max_sac, "%.*f %s/min", decimals, value, unit);
	value = get_volume_units(info_stat.min_sac.mliter, &decimals, &unit);
	set_label(info_stat_w.min_sac, "%.*f %s/min", decimals, value, unit);
	value = get_volume_units(info_stat.avg_sac.mliter, &decimals, &unit);
	set_label(info_stat_w.avg_sac, "%.*f %s/min", decimals, value, unit);
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

GtkWidget *stats_widget(void)
{

	GtkWidget *vbox, *hbox, *infoframe, *statsframe, *framebox;

	vbox = gtk_vbox_new(FALSE, 3);

	infoframe = gtk_frame_new("Dive Info");
	gtk_box_pack_start(GTK_BOX(vbox), infoframe, TRUE, FALSE, 3);
	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(infoframe), framebox);

	/* first row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	info_stat_w.date = new_info_label_in_frame(hbox, "Date");
	info_stat_w.dive_time = new_info_label_in_frame(hbox, "Dive Time");
	info_stat_w.surf_intv = new_info_label_in_frame(hbox, "Surf Intv");

	/* second row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	info_stat_w.max_depth = new_info_label_in_frame(hbox, "Max Depth");
	info_stat_w.avg_depth = new_info_label_in_frame(hbox, "Avg Depth");
	info_stat_w.water_temp = new_info_label_in_frame(hbox, "Water Temp");

	/* third row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	info_stat_w.sac = new_info_label_in_frame(hbox, "SAC");
	info_stat_w.otu = new_info_label_in_frame(hbox, "OTU");
	info_stat_w.o2he = new_info_label_in_frame(hbox, "O" UTF8_SUBSCRIPT_2 " / He");
	info_stat_w.gas_used = new_info_label_in_frame(hbox, "Gas Used");

	statsframe = gtk_frame_new("Statistics");
	gtk_box_pack_start(GTK_BOX(vbox), statsframe, TRUE, FALSE, 3);
	framebox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(statsframe), framebox);

	/* first row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	info_stat_w.total_time = new_info_label_in_frame(hbox, "Total Time");
	info_stat_w.avg_time = new_info_label_in_frame(hbox, "Avg Time");
	info_stat_w.max_overall_depth = new_info_label_in_frame(hbox, "Max Depth");
	info_stat_w.avg_overall_depth = new_info_label_in_frame(hbox, "Avg Depth");

	/* second row */
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(framebox), hbox, TRUE, FALSE, 3);

	info_stat_w.max_sac = new_info_label_in_frame(hbox, "Max SAC");
	info_stat_w.min_sac = new_info_label_in_frame(hbox, "Min SAC");
	info_stat_w.avg_sac = new_info_label_in_frame(hbox, "Avg SAC");

	return vbox;
}
