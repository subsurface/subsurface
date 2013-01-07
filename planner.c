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

int stoplevels[] = { 0, 3000, 6000, 9000, 12000, 15000, 21000, 30000, 42000, 60000, 90000 };

#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan)
{
	struct divedatapoint *dp;
	struct tm tm;

	if (!diveplan) {
		printf ("Diveplan NULL\n");
		return;
	}
	utc_mkdate(diveplan->when, &tm);

	printf("Diveplan @ %04d-%02d-%02d %02d:%02d:%02d (surfpres %dmbar):\n",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		diveplan->surface_pressure);
	dp = diveplan->dp;
	while (dp) {
		printf("\t%3u:%02u: %dmm gas: %d o2 %d h2\n", FRACTION(dp->time, 60), dp->depth, dp->o2, dp->he);
		dp = dp->next;
	}

}
#endif

/* returns the tissue tolerance at the end of this (partial) dive */
double tissue_at_end(struct dive *dive, char **cached_datap)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i, j, t0, t1;
	double tissue_tolerance;

	if (!dive)
		return 0.0;
	if (*cached_datap) {
		tissue_tolerance = restore_deco_state(*cached_datap);
	} else {
		tissue_tolerance = init_decompression(dive);
		cache_deco_state(tissue_tolerance, cached_datap);
	}
	dc = &dive->dc;
	if (!dc->samples)
		return tissue_tolerance;
	psample = sample = dc->sample;
	t0 = 0;
	for (i = 0; i < dc->samples; i++, sample++) {
		t1 = sample->time.seconds;
		for (j = t0; j < t1; j++) {
			int depth = psample->depth.mm + (j - t0) * (sample->depth.mm - psample->depth.mm) / (t1 - t0);
			tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
						&dive->cylinder[sample->sensor].gasmix, 1, sample->po2);
		}
		psample = sample;
		t0 = t1;
	}
	return tissue_tolerance;
}

/* how many seconds until we can ascend to the next stop? */
int time_at_last_depth(struct dive *dive, int next_stop, char **cached_data_p)
{
	int depth;
	double surface_pressure, tissue_tolerance;
	int wait = 0;
	struct sample *sample;

	if (!dive)
		return 0;
	surface_pressure = dive->surface_pressure.mbar / 1000.0;
	tissue_tolerance = tissue_at_end(dive, cached_data_p);
	sample = &dive->dc.sample[dive->dc.samples - 1];
	depth = sample->depth.mm;
	while (deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1) > next_stop) {
		wait++;
		tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
					&dive->cylinder[sample->sensor].gasmix, 1, sample->po2);
	}
	return wait;
}

int add_gas(struct dive *dive, int o2, int he)
{
	int i;
	struct gasmix *mix;
	cylinder_t *cyl;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cyl = dive->cylinder + i;
		if (cylinder_nodata(cyl))
			break;
		mix = &cyl->gasmix;
		if (o2 == mix->o2.permille && he == mix->he.permille)
			return i;
	}
	if (i == MAX_CYLINDERS) {
		printf("too many cylinders\n");
		return -1;
	}
	mix = &cyl->gasmix;
	mix->o2.permille = o2;
	mix->he.permille = he;
	return i;
}

struct dive *create_dive_from_plan(struct diveplan *diveplan)
{
	struct dive *dive;
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	int gasused = 0;
	int t = 0;
	int lastdepth = 0;

	if (!diveplan || !diveplan->dp)
		return NULL;
	dive = alloc_dive();
	dive->when = diveplan->when;
	dive->surface_pressure.mbar = diveplan->surface_pressure;
	dc = &dive->dc;
	dc->model = "Simulated Dive";
	dp = diveplan->dp;
	/* let's start with the gas given on the first segment */
	if(dp)
		add_gas(dive, dp->o2, dp->he);
	while (dp && dp->time) {
		int i, depth;

		if (dp->o2 != dive->cylinder[gasused].gasmix.o2.permille ||
		    dp->he != dive->cylinder[gasused].gasmix.he.permille) {
			int value;
			gasused = add_gas(dive, dp->o2, dp->he);
			value = dp->o2 / 10 | (dp->he / 10) << 16;
			add_event(dc, dp->time, 11, 0, value, "gaschange");
		}
		for (i = t; i < dp->time; i += 10) {
			depth = lastdepth + (i - t) * (dp->depth - lastdepth) / (dp->time - t);
			sample = prepare_sample(dc);
			sample->time.seconds = i;
			sample->depth.mm = depth;
			sample->sensor = gasused;
			dc->samples++;
		}
		sample = prepare_sample(dc);
		sample->time.seconds = dp->time;
		sample->depth.mm = dp->depth;
		sample->sensor = gasused;
		lastdepth = dp->depth;
		t = dp->time;
		dp = dp->next;
		dc->samples++;
	}
	if (dc->samples == 0) {
		/* not enough there yet to create a dive - most likely the first time is missing */
		free(dive);
		dive = NULL;
	}
	return dive;
}

void free_dps(struct divedatapoint *dp)
{
	while (dp) {
		struct divedatapoint *ndp = dp->next;
		free(dp);
		dp = ndp;
	}
}

struct divedatapoint *create_dp(int time_incr, int depth, int o2, int he)
{
	struct divedatapoint *dp;

	dp = malloc(sizeof(struct divedatapoint));
	dp->time = time_incr;
	dp->depth = depth;
	dp->o2 = o2;
	dp->he = he;
	dp->next = NULL;
	return dp;
}

struct divedatapoint *get_nth_dp(struct diveplan *diveplan, int idx)
{
	struct divedatapoint **ldpp, *dp = diveplan->dp;
	int i = 0;
	ldpp = &diveplan->dp;

	while (dp && i++ < idx) {
		ldpp = &dp->next;
		dp = dp->next;
	}
	while (i++ <= idx) {
		*ldpp = dp = create_dp(0, 0, 0, 0);
		ldpp = &((*ldpp)->next);
	}
	return dp;
}

void add_duration_to_nth_dp(struct diveplan *diveplan, int idx, int duration, gboolean is_rel)
{
	struct divedatapoint *pdp, *dp = get_nth_dp(diveplan, idx);
	if (idx > 0 && is_rel) {
		pdp = get_nth_dp(diveplan, idx - 1);
		duration += pdp->time;
	}
	dp->time = duration;
}

void add_depth_to_nth_dp(struct diveplan *diveplan, int idx, int depth)
{
	struct divedatapoint *dp = get_nth_dp(diveplan, idx);
	dp->depth = depth;
}

void add_gas_to_nth_dp(struct diveplan *diveplan, int idx, int o2, int he)
{
	struct divedatapoint *dp = get_nth_dp(diveplan, idx);
	if (o2 > 206 && o2 < 213 && he == 0) {
		/* we treat air in a special case */
		dp->o2 = dp->he = 0;
	} else {
		dp->o2 = o2;
		dp->he = he;
	}
}
void add_to_end_of_diveplan(struct diveplan *diveplan, struct divedatapoint *dp)
{
	struct divedatapoint **lastdp = &diveplan->dp;
	struct divedatapoint *ldp = *lastdp;
	while(*lastdp) {
		ldp = *lastdp;
		lastdp = &(*lastdp)->next;
	}
	*lastdp = dp;
	if (ldp)
		dp->time += ldp->time;
}

void plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he)
{
	struct divedatapoint *dp = create_dp(duration, depth, o2, he);
	add_to_end_of_diveplan(diveplan, dp);
}

void plan(struct diveplan *diveplan, char **cached_datap, struct dive **divep)
{
	struct dive *dive;
	struct sample *sample;
	int wait_time, o2, he;
	int ceiling, depth, transitiontime;
	int stopidx;
	double tissue_tolerance;

	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = 1013;
	if (*divep)
		delete_single_dive(dive_table.nr - 1);
	*divep = dive = create_dive_from_plan(diveplan);
	if (!dive)
		return;
	record_dive(dive);

	sample = &dive->dc.sample[dive->dc.samples - 1];
	o2 = dive->cylinder[sample->sensor].gasmix.o2.permille;
	he = dive->cylinder[sample->sensor].gasmix.he.permille;

	tissue_tolerance = tissue_at_end(dive, cached_datap);
	ceiling = deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1);

	for (stopidx = 1; stopidx < sizeof(stoplevels) / sizeof(int); stopidx++)
		if (stoplevels[stopidx] >= ceiling)
			break;

	while (stopidx > 0) {
		depth = dive->dc.sample[dive->dc.samples - 1].depth.mm;
		if (depth > stoplevels[stopidx]) {
			transitiontime = (depth - stoplevels[stopidx]) / 150;
			plan_add_segment(diveplan, transitiontime, stoplevels[stopidx], o2, he);
			/* re-create the dive */
			delete_single_dive(dive_table.nr - 1);
			*divep = dive = create_dive_from_plan(diveplan);
			record_dive(dive);
		}
		wait_time = time_at_last_depth(dive, stoplevels[stopidx - 1], cached_datap);
		if (wait_time)
			plan_add_segment(diveplan, wait_time, stoplevels[stopidx], o2, he);
		transitiontime = (stoplevels[stopidx] - stoplevels[stopidx - 1]) / 150;
		plan_add_segment(diveplan, transitiontime, stoplevels[stopidx - 1], o2, he);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		*divep = dive = create_dive_from_plan(diveplan);
		record_dive(dive);
		stopidx--;
	}
	/* now make the dive visible as last dive of the dive list */
	report_dives(FALSE, FALSE);
	select_last_dive();
}


/* and now the UI for all this */
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
#if DEBUG_PLAN & 1
	dump_plan(&tempplan);
#endif
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
