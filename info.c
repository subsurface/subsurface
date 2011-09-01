#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

static GtkWidget *divedate, *divetime, *depth, *duration;
static GtkWidget *location, *notes;

static const char *weekday(int wday)
{
	static const char wday_array[7][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	return wday_array[wday];
}

void update_dive_info(struct dive *dive)
{
	struct tm *tm;
	char buffer[80];

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

	snprintf(buffer, sizeof(buffer),
		"%d ft",
		to_feet(dive->maxdepth));
	gtk_label_set_text(GTK_LABEL(depth), buffer);

	snprintf(buffer, sizeof(buffer),
		"%d min",
		dive->duration.seconds / 60);
	gtk_label_set_text(GTK_LABEL(duration), buffer);
}

static GtkWidget *info_label(GtkWidget *box, const char *str)
{
	GtkWidget *label = gtk_label_new(str);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
	gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
	return label;
}

GtkWidget *dive_info_frame(void)
{
	GtkWidget *frame;
	GtkWidget *hbox;

	frame = gtk_frame_new("Dive info");
	gtk_widget_show(frame);

	hbox = gtk_hbox_new(TRUE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	divedate = info_label(hbox, "date");
	divetime = info_label(hbox, "time");
	depth = info_label(hbox, "depth");
	duration = info_label(hbox, "duration");

	return frame;
}

static GtkWidget *text_entry(GtkWidget *box, const char *label)
{
	GtkWidget *entry;
	GtkWidget *frame = gtk_frame_new(label);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	entry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame), entry);
	return entry;
}

GtkWidget *extended_dive_info_frame(void)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new("Extended dive info");
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	location = text_entry(vbox, "Location");
	notes = text_entry(vbox, "Notes");
	location = gtk_entry_new();

	/* Add extended info here: name, description, yadda yadda */
	update_dive_info(current_dive);
	return frame;
}
