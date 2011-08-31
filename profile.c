#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

int selected_dive = 0;

static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
	struct dive *dive = dive_table.dives[selected_dive];
	cairo_t *cr;
	int i;

	cr = gdk_cairo_create(widget->window);
	cairo_set_source_rgb(cr, 0, 0, 0);
	gdk_cairo_rectangle(cr, &event->area);
	cairo_fill(cr);

	cairo_set_line_width(cr, 3);
	cairo_set_source_rgb(cr, 1, 1, 1);

	if (dive->samples) {
		struct sample *sample = dive->sample;
		cairo_move_to(cr, sample->time.seconds / 5, to_feet(sample->depth) * 3);
		for (i = 1; i < dive->samples; i++) {
			sample++;
			cairo_line_to(cr, sample->time.seconds / 5, to_feet(sample->depth) * 3);
		}
		cairo_stroke(cr);
	}

	cairo_destroy(cr);

	return FALSE;
}

GtkWidget *dive_profile_frame(void)
{
	GtkWidget *frame;
	GtkWidget *da;

	frame = gtk_frame_new("Dive profile");
	gtk_widget_show(frame);
	da = gtk_drawing_area_new();
	gtk_widget_set_size_request(da, 450, 350);
	g_signal_connect(da, "expose_event", G_CALLBACK(expose_event), NULL);
	gtk_container_add(GTK_CONTAINER(frame), da);

	return frame;
}
