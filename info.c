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

static GtkComboBoxEntry *location, *buddy, *divemaster;
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

/* old is NULL or a valid string, new is a valid string
 * NOTW: NULL and "" need to be treated as "unchanged" */
static int text_changed(char *old, char *new)
{
	return ((old && strcmp(old,new)) ||
		(!old && strcmp("",new)));
}

void flush_dive_info_changes(struct dive *dive)
{
	char *old_text;
	int changed = 0;

	if (!dive)
		return;

	if (location_changed) {
		char *new_text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(location));
		old_text = dive->location;
		dive->location = new_text;
		add_location(new_text);
		if (text_changed(old_text,dive->location))
			changed = 1;
		if (old_text)
			g_free(old_text);
	}

	if (divemaster_changed) {
		char *new_text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(divemaster));
		old_text = dive->divemaster;
		dive->divemaster = new_text;
		add_people(new_text);
		if (text_changed(old_text,dive->divemaster))
			changed = 1;
		if (old_text)
			g_free(old_text);
	}

	if (buddy_changed) {
		char *new_text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(buddy));
		old_text = dive->buddy;
		dive->buddy = new_text;
		add_people(new_text);
		if (text_changed(old_text,dive->buddy))
			changed = 1;
		if (old_text)
			g_free(old_text);
	}

	if (notes_changed) {
		old_text = dive->notes;
		dive->notes = get_text(notes);
		if (text_changed(old_text,dive->notes))
			changed = 1;
		if (old_text)
			g_free(old_text);
	}
	if (changed)
		mark_divelist_changed(TRUE);
}

static void set_combo_box_entry_text(GtkComboBoxEntry *combo_box, const char *text)
{
	GtkEntry *entry = GTK_ENTRY(GTK_BIN(combo_box)->child);
	gtk_entry_set_text(entry, text);
}

#define SET_TEXT_ENTRY(x) \
	set_combo_box_entry_text(x, dive && dive->x ? dive->x : "")

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

static GtkComboBoxEntry *text_entry(GtkWidget *box, const char *label, GtkListStore *completions)
{
	GtkEntry *entry;
	GtkWidget *combo_box;
	GtkWidget *frame = gtk_frame_new(label);
	GtkEntryCompletion *completion;

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	combo_box = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(completions), 0);
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	entry = GTK_ENTRY(GTK_BIN(combo_box)->child);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(completions));
	gtk_entry_completion_set_inline_completion(completion, TRUE);
	gtk_entry_completion_set_inline_selection(completion, TRUE);
	gtk_entry_completion_set_popup_single_match(completion, FALSE);
	gtk_entry_set_completion(entry, completion);

	return GTK_COMBO_BOX_ENTRY(combo_box);
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

	view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	return buffer;
}

static enum {
	MATCH_EXACT,
	MATCH_PREPEND,
	MATCH_AFTER
} found_string_entry;
static GtkTreeIter string_entry_location;

static gboolean match_string_entry(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	const char *string = data;
	char *entry;
	int cmp;

	gtk_tree_model_get(model, iter, 0, &entry, -1);
	cmp = strcmp(entry, string);

	/* Stop. The entry is bigger than the new one */
	if (cmp > 0)
		return TRUE;

	/* Exact match */
	if (!cmp) {
		found_string_entry = MATCH_EXACT;
		return TRUE;
	}

	string_entry_location = *iter;
	found_string_entry = MATCH_AFTER;
	return FALSE;
}

static int match_list(GtkListStore *list, const char *string)
{
	found_string_entry = MATCH_PREPEND;
	gtk_tree_model_foreach(GTK_TREE_MODEL(list), match_string_entry, (void *)string);
	return found_string_entry;
}

static GtkListStore *location_list, *people_list;

static void add_string_list_entry(const char *string, GtkListStore *list)
{
	GtkTreeIter *iter, loc;

	if (!string || !*string)
		return;

	switch (match_list(list, string)) {
	case MATCH_EXACT:
		return;
	case MATCH_PREPEND:
		iter = NULL;
		break;
	case MATCH_AFTER:
		iter = &string_entry_location;
		break;
	}
	gtk_list_store_insert_after(list, &loc, iter);
	gtk_list_store_set(list, &loc, 0, string, -1);
}

void add_people(const char *string)
{
	add_string_list_entry(string, people_list);
}

void add_location(const char *string)
{
	add_string_list_entry(string, location_list);
}

GtkWidget *extended_dive_info_widget(void)
{
	GtkWidget *vbox, *hbox;
	vbox = gtk_vbox_new(FALSE, 6);

	people_list = gtk_list_store_new(1, G_TYPE_STRING);
	location_list = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	location = text_entry(vbox, gettext("Location"), location_list);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	divemaster = text_entry(hbox, gettext("Divemaster"), people_list);
	buddy = text_entry(hbox, gettext("Buddy"), people_list);

	notes = text_view(vbox, gettext("Notes"));
	return vbox;
}
