/* info.c */
/* creates the UI for the info frame - 
 * controlled through the following interfaces:
 * 
 * void flush_dive_info_changes(struct dive *dive)
 * void show_dive_info(struct dive *dive)
 *
 * called from gtk-ui:
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

static GtkEntry *location, *buddy, *divemaster;
static GtkTextBuffer *notes;
static int location_changed = 1, notes_changed = 1;
static int divemaster_changed = 1, buddy_changed = 1;

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

#define SET_TEXT_ENTRY(x) \
	gtk_entry_set_text(x, dive && dive->x ? dive->x : "")

void show_dive_info(struct dive *dive)
{
	struct tm *tm;
	const char *text;
	char buffer[80];

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
		text += 10;     /* Skip the "Dive #0 - " part */
	gtk_window_set_title(GTK_WINDOW(main_window), text);

	SET_TEXT_ENTRY(divemaster);
	SET_TEXT_ENTRY(buddy);
	SET_TEXT_ENTRY(location);
	gtk_text_buffer_set_text(notes, dive && dive->notes ? dive->notes : "", -1);
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
	return vbox;
}
