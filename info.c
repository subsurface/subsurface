/* info.c */
/* creates the UI for the info frame - 
 * controlled through the following interfaces:
 * 
 * void show_dive_info(struct dive *dive)
 *
 * called from gtk-ui:
 * GtkWidget *extended_dive_info_widget(void)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

static GtkEntry *location, *buddy, *divemaster;
static GtkTextView *notes;
static GtkListStore *location_list, *people_list;

static char *get_text(GtkTextView *view)
{
	GtkTextBuffer *buffer;
	GtkTextIter start;
	GtkTextIter end;

	buffer = gtk_text_view_get_buffer(view);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

/* old is NULL or a valid string, new is a valid string
 * NOTW: NULL and "" need to be treated as "unchanged" */
static int text_changed(const char *old, const char *new)
{
	return ((old && strcmp(old,new)) ||
		(!old && strcmp("",new)));
}

static char *get_combo_box_entry_text(GtkComboBoxEntry *combo_box, char **textp)
{
	char *old = *textp;
	const gchar *new;
	GtkEntry *entry;

	entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_box)));
	new = gtk_entry_get_text(entry);
	while (isspace(*new))
		new++;
	if (!text_changed(old,new))
		return NULL;
	free(old);
	*textp = strdup(new);
	return *textp;
}

#define SET_TEXT_VALUE(x) \
	gtk_entry_set_text(x, dive && dive->x ? dive->x : "")

static int divename(char *buf, size_t size, struct dive *dive)
{
	struct tm *tm = gmtime(&dive->when);
	return snprintf(buf, size, "Dive #%d - %s %02d/%02d/%04d at %d:%02d",
		dive->number,
		weekday(tm->tm_wday),
		tm->tm_mon+1, tm->tm_mday,
		tm->tm_year+1900,
		tm->tm_hour, tm->tm_min);
}

void show_dive_info(struct dive *dive)
{
	const char *text;
	char buffer[80];

	/* dive number and location (or lacking that, the date) go in the window title */
	text = dive->location;
	if (!text)
		text = "";
	if (*text) {
		snprintf(buffer, sizeof(buffer), "Dive #%d - %s", dive->number, text);
	} else {
		divename(buffer, sizeof(buffer), dive);
	}
	text = buffer;
	if (!dive->number)
		text += 10;     /* Skip the "Dive #0 - " part */
	gtk_window_set_title(GTK_WINDOW(main_window), text);

	SET_TEXT_VALUE(divemaster);
	SET_TEXT_VALUE(buddy);
	SET_TEXT_VALUE(location);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(notes),
		dive && dive->notes ? dive->notes : "", -1);
}

static void info_menu_edit_cb(GtkMenuItem *menuitem, gpointer user_data)
{
	edit_dive_info(current_dive);
}

static void populate_popup_cb(GtkTextView *entry, GtkMenu *menu, gpointer user_data)
{
	GtkWidget *item = gtk_menu_item_new_with_label("Edit");
	g_signal_connect(item, "activate", G_CALLBACK(info_menu_edit_cb), NULL);
	gtk_widget_show(item); /* Yes, really */
	gtk_menu_prepend(menu, item);
}

static GtkEntry *text_value(GtkWidget *box, const char *label)
{
	GtkWidget *widget;
	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);
	widget = gtk_entry_new();
	gtk_widget_set_can_focus(widget, FALSE);
	gtk_editable_set_editable(GTK_EDITABLE(widget), FALSE);
	gtk_container_add(GTK_CONTAINER(frame), widget);
	g_signal_connect(widget, "populate-popup", G_CALLBACK(populate_popup_cb), NULL);
	return GTK_ENTRY(widget);
}

static GtkComboBoxEntry *text_entry(GtkWidget *box, const char *label, GtkListStore *completions, const char *text)
{
	GtkEntry *entry;
	GtkWidget *combo_box;
	GtkWidget *frame = gtk_frame_new(label);
	GtkEntryCompletion *completion;

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	combo_box = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(completions), 0);
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_box)));
	if (text && *text)
		gtk_entry_set_text(entry, text);

	completion = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(completion, 0);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(completions));
	gtk_entry_completion_set_inline_completion(completion, TRUE);
	gtk_entry_completion_set_inline_selection(completion, TRUE);
	gtk_entry_completion_set_popup_single_match(completion, FALSE);
	gtk_entry_set_completion(entry, completion);

	return GTK_COMBO_BOX_ENTRY(combo_box);
}

enum writable {
	READ_ONLY,
	READ_WRITE
};

static GtkTextView *text_view(GtkWidget *box, const char *label, enum writable writable)
{
	GtkWidget *view, *vbox;
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
	if (writable == READ_ONLY) {
		gtk_widget_set_can_focus(view, FALSE);
		gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
		gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
		g_signal_connect(view, "populate-popup", G_CALLBACK(populate_popup_cb), NULL);
	}
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	gtk_container_add(GTK_CONTAINER(scrolled_window), view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	return GTK_TEXT_VIEW(view);
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

struct dive_info {
	GtkComboBoxEntry *location, *divemaster, *buddy;
	GtkTextView *notes;
};

static void save_dive_info_changes(struct dive *dive, struct dive_info *info)
{
	char *old_text, *new_text;
	int changed = 0;

	new_text = get_combo_box_entry_text(info->location, &dive->location);
	if (new_text) {
		add_location(new_text);
		changed = 1;
	}

	new_text = get_combo_box_entry_text(info->divemaster, &dive->divemaster);
	if (new_text) {
		add_people(new_text);
		changed = 1;
	}

	new_text = get_combo_box_entry_text(info->buddy, &dive->buddy);
	if (new_text) {
		add_people(new_text);
		changed = 1;
	}

	old_text = dive->notes;
	dive->notes = get_text(info->notes);
	if (text_changed(old_text,dive->notes))
		changed = 1;
	if (old_text)
		g_free(old_text);

	if (changed) {
		mark_divelist_changed(TRUE);
		flush_divelist(dive);
	}
}

static void dive_info_widget(GtkWidget *box, struct dive *dive, struct dive_info *info)
{
	GtkWidget *hbox, *label;
	char buffer[80];

	divename(buffer, sizeof(buffer), dive);
	label = gtk_label_new(buffer);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	info->location = text_entry(box, "Location", location_list, dive->location);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->divemaster = text_entry(hbox, "Dive master", people_list, dive->divemaster);
	info->buddy = text_entry(hbox, "Buddy", people_list, dive->buddy);

	info->notes = text_view(box, "Notes", READ_WRITE);
	if (dive->notes && *dive->notes)
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(info->notes), dive->notes, -1);
}

int edit_dive_info(struct dive *dive)
{
	int success;
	GtkWidget *dialog, *vbox;
	struct dive_info info;

	if (!dive)
		return 0;

	dialog = gtk_dialog_new_with_buttons("Dive Info",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	dive_info_widget(vbox, dive, &info);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success)
		save_dive_info_changes(dive, &info);

	gtk_widget_destroy(dialog);

	return success;
}

GtkWidget *extended_dive_info_widget(void)
{
	GtkWidget *vbox, *hbox;
	vbox = gtk_vbox_new(FALSE, 6);

	people_list = gtk_list_store_new(1, G_TYPE_STRING);
	location_list = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	location = text_value(vbox, "Location");

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	divemaster = text_value(hbox, "Divemaster");
	buddy = text_value(hbox, "Buddy");

	notes = text_view(vbox, "Notes", READ_ONLY);
	return vbox;
}
