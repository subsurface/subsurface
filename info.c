#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

GtkWidget *dive_info_frame(void)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *depth;

	frame = gtk_frame_new("Dive info");
	gtk_widget_show(frame);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	depth = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(depth), "54 ft");
	gtk_editable_set_editable(GTK_EDITABLE(depth), FALSE);

	gtk_box_pack_start(GTK_BOX(hbox), depth, FALSE, FALSE, 0);

	return frame;
}
