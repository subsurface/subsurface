/* info.c */
/* creates the UI for the info frame - 
 * controlled through the following interfaces:
 * 
 * void flush_dive_info_changes(struct dive *dive)
 * void show_dive_info(struct dive *dive)
 *
 * called from gtk-ui:
 * GtkWidget *dive_info_frame(void)
 * GtkWidget *extended_dive_info_widget(void)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

static GtkWidget *info_frame;
static GtkWidget *depth, *duration, *temperature, *airconsumption;
static GtkEntry *location, *buddy, *divemaster;
static GtkTextBuffer *notes;
static int location_changed = 1, notes_changed = 1;
static int divemaster_changed = 1, buddy_changed = 1;

#define EMPTY_AIRCONSUMPTION " \n "
#define AIRCON_WIDTH 20

static const char *weekday(int wday)
{
	static const char wday_array[7][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	return wday_array[wday];
}

static char *get_text(GtkTextBuffer *buffer)
{
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

static void update_air_info(char *buffer)
{
	char markup[120];
	
	if (! buffer)
		buffer = EMPTY_AIRCONSUMPTION;
	snprintf(markup, sizeof(markup), "<span font=\"8\">%s</span>",buffer);
	gtk_label_set_markup(GTK_LABEL(airconsumption), markup);
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

static void update_air_info_frame(struct dive *dive)
{
	const double liters_per_cuft = 28.317;
	const char *unit, *format, *desc;
	double airuse;
	char buffer1[80];
	char buffer2[80];
	int len;

	airuse = calculate_airuse(dive);
	if (!airuse) {
		update_air_info(NULL);
		return;
	}
	switch (output_units.volume) {
	case LITER:
		unit = "l";
		format = "vol: %4.0f %s";
		break;
	case CUFT:
		unit = "cuft";
		format = "vol: %4.2f %s";
		airuse /= liters_per_cuft;
		break;
	}
	len = snprintf(buffer1, sizeof(buffer1), format, airuse, unit);
	if (dive->duration.seconds) {
		double pressure = 1 + (dive->meandepth.mm / 10000.0);
		double sac = airuse / pressure * 60 / dive->duration.seconds;
		snprintf(buffer1+len, sizeof(buffer1)-len, 
				"\nSAC: %4.2f %s/min", sac, unit);
	}
	len = 0;
	desc = dive->cylinder[0].type.description;
	if (desc || dive->cylinder[0].gasmix.o2.permille) {
		int o2 = dive->cylinder[0].gasmix.o2.permille / 10;
		if (!desc)
			desc = "";
		if (!o2)
			o2 = 21;
		len = snprintf(buffer2, sizeof(buffer2), "%s (%d%%): used ", desc, o2);
	}
	snprintf(buffer2+len, sizeof(buffer2)-len, buffer1); 
	update_air_info(buffer2);
}

void flush_dive_info_changes(struct dive *dive)
{
	if (!dive)
		return;

	if (location_changed) {
		g_free(dive->location);
		dive->location = gtk_editable_get_chars(GTK_EDITABLE(location), 0, -1);
	}

	if (divemaster_changed) {
		g_free(dive->divemaster);
		dive->divemaster = gtk_editable_get_chars(GTK_EDITABLE(divemaster), 0, -1);
	}

	if (buddy_changed) {
		g_free(dive->buddy);
		dive->buddy = gtk_editable_get_chars(GTK_EDITABLE(buddy), 0, -1);
	}

	if (notes_changed) {
		g_free(dive->notes);
		dive->notes = get_text(notes);
	}
}

void show_dive_info(struct dive *dive)
{
	struct tm *tm;
	char buffer[80];
	char *text;

	if (!dive) {
		gtk_label_set_text(GTK_LABEL(depth), "");
		gtk_label_set_text(GTK_LABEL(duration), "");
		gtk_label_set_text(GTK_LABEL(airconsumption), EMPTY_AIRCONSUMPTION);
		gtk_label_set_width_chars(GTK_LABEL(airconsumption), AIRCON_WIDTH);
		return;
	}
	/* dive number and location (or lacking that, the date) go in the window title */
	tm = gmtime(&dive->when);
	text = dive->location;
	if (!text)
		text = "";
	if (*text) {
		snprintf(buffer, sizeof(buffer), "Dive #%d - %s", dive->number, text);
	} else {
		snprintf(buffer, sizeof(buffer), "Dive #%d - %s %02d/%02d/%04d at %d:%02d",
			dive->number,
			weekday(tm->tm_wday),
			tm->tm_mon+1, tm->tm_mday,
			tm->tm_year+1900,
			tm->tm_hour, tm->tm_min);
	}
	text = buffer;
	if (!dive->number)
		text += 10;	/* Skip the "Dive #0 - " part */
	gtk_window_set_title(GTK_WINDOW(main_window), text);

	/* the date goes in the frame label */
	snprintf(buffer, sizeof(buffer), "%s %02d/%02d/%04d at %d:%02d",
		weekday(tm->tm_wday),
		tm->tm_mon+1, tm->tm_mday,
		tm->tm_year+1900,
		tm->tm_hour, tm->tm_min);
	gtk_frame_set_label(GTK_FRAME(info_frame), buffer);


	switch (output_units.length) {
	case METERS:
		snprintf(buffer, sizeof(buffer),
			"%.1f m",
			dive->maxdepth.mm / 1000.0);
		break;
	case FEET:
		snprintf(buffer, sizeof(buffer),
			"%d ft",
			to_feet(dive->maxdepth));
		break;
	}
	gtk_label_set_text(GTK_LABEL(depth), buffer);

	snprintf(buffer, sizeof(buffer),
		"%d min",
		dive->duration.seconds / 60);
	gtk_label_set_text(GTK_LABEL(duration), buffer);

	*buffer = 0;
	if (dive->watertemp.mkelvin) {
		switch (output_units.temperature) {
		case CELSIUS:
			snprintf(buffer, sizeof(buffer),
				"%d C",
				to_C(dive->watertemp));
			break;
		case FAHRENHEIT:
			snprintf(buffer, sizeof(buffer),
				"%d F",
				to_F(dive->watertemp));
			break;
		case KELVIN:
			snprintf(buffer, sizeof(buffer),
				"%d K",
				to_K(dive->watertemp));
			break;
		}
	}
	gtk_label_set_text(GTK_LABEL(temperature), buffer);

	text = dive->location ? : "";
	gtk_entry_set_text(location, text);

	text = dive->divemaster ? : "";
	gtk_entry_set_text(divemaster, text);

	text = dive->buddy ? : "";
	gtk_entry_set_text(buddy, text);

	text = dive->notes ? : "";
	gtk_text_buffer_set_text(notes, text, -1);

	update_air_info_frame(dive);
}

static GtkWidget *info_label(GtkWidget *box, const char *str, GtkJustification jtype)
{
	GtkWidget *label = gtk_label_new(str);
	gtk_label_set_justify(GTK_LABEL(label), jtype);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
	return label;
}

GtkWidget *dive_info_frame(void)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;

	frame = gtk_frame_new("Dive info");
	info_frame = frame;
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	hbox = gtk_hbox_new(FALSE, 16);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	depth = info_label(hbox, "depth", GTK_JUSTIFY_RIGHT);
	duration = info_label(hbox, "duration", GTK_JUSTIFY_RIGHT);
	temperature = info_label(hbox, "temperature", GTK_JUSTIFY_RIGHT);
	airconsumption = info_label(hbox, "air", GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(airconsumption), 1.0, 0.5);
	gtk_label_set_width_chars(GTK_LABEL(airconsumption), AIRCON_WIDTH);

	return frame;
}

static GtkEntry *text_entry(GtkWidget *box, const char *label)
{
	GtkWidget *entry;
	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame), entry);

	return GTK_ENTRY(entry);
}

static GtkTextBuffer *text_view(GtkWidget *box, const char *label)
{
	GtkWidget *view, *vbox;
	GtkTextBuffer *buffer;
	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);
	box = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(frame), box);
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(box), vbox);

	GtkWidget* scrolled_window = gtk_scrolled_window_new(0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
	gtk_widget_show(scrolled_window);

	view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	return buffer;
}

GtkWidget *extended_dive_info_widget(void)
{
	GtkWidget *vbox, *hbox;
	vbox = gtk_vbox_new(FALSE, 6);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	location = text_entry(vbox, "Location");

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	divemaster = text_entry(hbox, "Divemaster");
	buddy = text_entry(hbox, "Buddy");

	notes = text_view(vbox, "Notes");

	/* Add extended info here: name, description, yadda yadda */
	show_dive_info(current_dive);
	return vbox;
}
