#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"
#include "divelist.h"

static GtkWidget *divedate, *divetime, *depth, *duration, *temperature, *locationnote;
static GtkEntry *location;
static GtkTextBuffer *notes;
static int location_changed = 1, notes_changed = 1;

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

void flush_dive_info_changes(struct dive *dive)
{
	if (!dive)
		return;

	if (location_changed) {
		g_free(dive->location);
		dive->location = gtk_editable_get_chars(GTK_EDITABLE(location), 0, -1);
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
	int len;

	if (!dive) {
		gtk_label_set_text(GTK_LABEL(divedate), "no dive");
		gtk_label_set_text(GTK_LABEL(divetime), "");
		gtk_label_set_text(GTK_LABEL(depth), "");
		gtk_label_set_text(GTK_LABEL(duration), "");
		return;
	}

	tm = gmtime(&dive->when);
	snprintf(buffer, sizeof(buffer),
		"%s %02d/%02d/%04d",
		weekday(tm->tm_wday),
		tm->tm_mon+1, tm->tm_mday, tm->tm_year+1900);
	gtk_label_set_text(GTK_LABEL(divedate), buffer);

	snprintf(buffer, sizeof(buffer),
		"%02d:%02d:%02d",
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	gtk_label_set_text(GTK_LABEL(divetime), buffer);

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

	len = 0;
	if (dive->nr)
		len = snprintf(buffer, sizeof(buffer), "%d. ", dive->nr);
	snprintf(buffer+len, sizeof(buffer)-len, "%s", text);
	gtk_label_set_text(GTK_LABEL(locationnote), buffer);

	text = dive->notes ? : "";
	gtk_text_buffer_set_text(notes, text, -1);
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
	GtkWidget *hbox, *hbox2;
	GtkWidget *vbox;

	frame = gtk_frame_new("Dive info");
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(TRUE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 3);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	hbox = gtk_hbox_new(TRUE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
	gtk_container_add(GTK_CONTAINER(vbox), hbox);

	hbox2 = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox2), 3);
	gtk_container_add(GTK_CONTAINER(vbox), hbox2);

	divedate = info_label(hbox, "date", GTK_JUSTIFY_RIGHT);
	divetime = info_label(hbox, "time", GTK_JUSTIFY_RIGHT);
	depth = info_label(hbox, "depth", GTK_JUSTIFY_RIGHT);
	duration = info_label(hbox, "duration", GTK_JUSTIFY_RIGHT);
	temperature = info_label(hbox, "temperature", GTK_JUSTIFY_RIGHT);

	locationnote = info_label(hbox2, "location", GTK_JUSTIFY_LEFT);
	gtk_label_set_width_chars(GTK_LABEL(locationnote), 80);

	return frame;
}

static GtkEntry *text_entry(GtkWidget *box, const char *label)
{
	GtkWidget *entry;

	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	entry = gtk_entry_new ();
	gtk_container_add(GTK_CONTAINER(frame), entry);

	return GTK_ENTRY(entry);
}

static GtkTextBuffer *text_view(GtkWidget *box, const char *label, gboolean expand)
{
	GtkWidget *view;
	GtkTextBuffer *buffer;

	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, expand, expand, 0);

	GtkWidget* scrolled_window = gtk_scrolled_window_new (0, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
	gtk_widget_show(scrolled_window);

	view = gtk_text_view_new ();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
	return buffer;
}

GtkWidget *extended_dive_info_widget(void)
{
	GtkWidget *vbox;

	vbox = gtk_vbox_new(FALSE, 6);

	location = text_entry(vbox, "Location");
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	notes = text_view(vbox, "Notes", TRUE);

	/* Add extended info here: name, description, yadda yadda */
	show_dive_info(current_dive);
	return vbox;
}
