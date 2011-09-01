#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

static GtkWidget *datetime, *depth, *duration;

void update_dive_info(struct dive *dive)
{
	struct tm *tm;
	char buffer[80];

	if (!dive) {
		gtk_entry_set_text(GTK_ENTRY(datetime), "no dive");
		gtk_entry_set_text(GTK_ENTRY(depth), "");
		gtk_entry_set_text(GTK_ENTRY(duration), "");
		return;
	}

	tm = gmtime(&dive->when);
	snprintf(buffer, sizeof(buffer),
		"%04d-%02d-%02d "
		"%02d:%02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	gtk_entry_set_text(GTK_ENTRY(datetime), buffer);

	snprintf(buffer, sizeof(buffer),
		"%d ft",
		to_feet(dive->maxdepth));
	gtk_entry_set_text(GTK_ENTRY(depth), buffer);

	snprintf(buffer, sizeof(buffer),
		"%d min",
		dive->duration.seconds / 60);
	gtk_entry_set_text(GTK_ENTRY(duration), buffer);
}

GtkWidget *dive_info_frame(void)
{
	GtkWidget *frame;
	GtkWidget *hbox;

	frame = gtk_frame_new("Dive info");
	gtk_widget_show(frame);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	datetime = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(datetime), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), datetime, FALSE, FALSE, 0);

	depth = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(depth), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), depth, FALSE, FALSE, 0);

	duration = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(duration), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), duration, FALSE, FALSE, 0);

	return frame;
}

GtkWidget *extended_dive_info_frame(void)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new("Extended dive info");
	gtk_widget_show(frame);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	/* Add extended info here: name, description, yadda yadda */
	update_dive_info(current_dive);
	return frame;
}
