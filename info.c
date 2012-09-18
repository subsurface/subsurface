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
#include <sys/time.h>

#include "dive.h"
#include "display.h"
#include "display-gtk.h"
#include "divelist.h"

static GtkEntry *location, *buddy, *divemaster, *rating, *suit;
static GtkTextView *notes;
static GtkListStore *location_list, *people_list, *star_list, *suit_list;

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
	return (old && strcmp(old,new)) ||
		(!old && strcmp("",new));
}

static const char *skip_space(const char *str)
{
	if (str) {
		while (isspace(*str))
			str++;
		if (!*str)
			str = NULL;
	}
	return str;
}

/*
 * Get the string from a combo box.
 *
 * The "master" string is the string of the current dive - we only consider it
 * changed if the old string is either empty, or matches that master string.
 */
static char *get_combo_box_entry_text(GtkComboBoxEntry *combo_box, char **textp, const char *master)
{
	char *old = *textp;
	const char *old_text;
	const gchar *new;
	GtkEntry *entry;

	old_text = skip_space(old);
	master = skip_space(master);

	/*
	 * If we had a master string, and it doesn't match our old
	 * string, we will always pick the old value (it means that
	 * we're editing another dive's info that already had a
	 * valid value).
	 */
	if (master && old_text)
		if (strcmp(master, old_text))
			return NULL;

	entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_box)));
	new = gtk_entry_get_text(entry);
	while (isspace(*new))
		new++;
	/* If the master string didn't change, don't change other dives either! */
	if (!text_changed(master,new))
		return NULL;
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
	char title[80];
	char *basename;

	if (!dive) {
		if (existing_filename) {
			basename = g_path_get_basename(existing_filename);
			snprintf(title, 80, "Subsurface: %s", basename);
			free(basename);
			gtk_window_set_title(GTK_WINDOW(main_window), title);
		} else {
			gtk_window_set_title(GTK_WINDOW(main_window), "Subsurface");
		}
		SET_TEXT_VALUE(divemaster);
		SET_TEXT_VALUE(buddy);
		SET_TEXT_VALUE(location);
		SET_TEXT_VALUE(suit);
		gtk_entry_set_text(rating, star_strings[0]);
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(notes), "", -1);
		return;
	}

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

	/* put it all together */
	if (existing_filename) {
		basename = g_path_get_basename(existing_filename);
		snprintf(title, 80, "%s: %s", basename, text);
		free(basename);
		gtk_window_set_title(GTK_WINDOW(main_window), title);
	} else {
		gtk_window_set_title(GTK_WINDOW(main_window), text);
	}
	SET_TEXT_VALUE(divemaster);
	SET_TEXT_VALUE(buddy);
	SET_TEXT_VALUE(location);
	SET_TEXT_VALUE(suit);
	gtk_entry_set_text(rating, star_strings[dive->rating]);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(notes),
		dive && dive->notes ? dive->notes : "", -1);
}

static int delete_dive_info(struct dive *dive)
{
	int success;
	GtkWidget *dialog;

	if (!dive)
		return 0;

	dialog = gtk_dialog_new_with_buttons("Delete Dive",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		delete_dive(dive);
		mark_divelist_changed(TRUE);
		dive_list_update_dives();
	}

	gtk_widget_destroy(dialog);

	return success;
}

static void info_menu_edit_cb(GtkMenuItem *menuitem, gpointer user_data)
{
	edit_multi_dive_info(NULL);
}

static void info_menu_delete_cb(GtkMenuItem *menuitem, gpointer user_data)
{
	/* this needs to delete all the selected dives as well, I guess? */
	delete_dive_info(current_dive);
}

static void add_menu_item(GtkMenu *menu, const char *label, const char *icon, void (*cb)(GtkMenuItem *, gpointer))
{
	GtkWidget *item;
	if (icon) {
		GtkWidget *image;
		item = gtk_image_menu_item_new_with_label(label);
		image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
	} else {
		item = gtk_menu_item_new_with_label(label);
	}
	g_signal_connect(item, "activate", G_CALLBACK(cb), NULL);
	gtk_widget_show(item); /* Yes, really */
	gtk_menu_prepend(menu, item);
}

static void populate_popup_cb(GtkTextView *entry, GtkMenu *menu, gpointer user_data)
{
	add_menu_item(menu, "Delete", GTK_STOCK_DELETE, info_menu_delete_cb);
	add_menu_item(menu, "Edit", GTK_STOCK_EDIT, info_menu_edit_cb);
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
	if (entry)
		free(entry);

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

void add_suit(const char *string)
{
	add_string_list_entry(string, suit_list);
}

static int get_rating(const char *string)
{
	int rating_val = 0;
	int i;

	for (i = 0; i <= 5; i++)
		if (!strcmp(star_strings[i],string))
			rating_val = i;
	return rating_val;
}

struct dive_info {
	GtkComboBoxEntry *location, *divemaster, *buddy, *rating, *suit;
	GtkTextView *notes;
};

static void save_dive_info_changes(struct dive *dive, struct dive *master, struct dive_info *info)
{
	char *old_text, *new_text;
	char *rating_string;
	int changed = 0;

	new_text = get_combo_box_entry_text(info->location, &dive->location, master->location);
	if (new_text) {
		add_location(new_text);
		changed = 1;
	}

	new_text = get_combo_box_entry_text(info->divemaster, &dive->divemaster, master->divemaster);
	if (new_text) {
		add_people(new_text);
		changed = 1;
	}

	new_text = get_combo_box_entry_text(info->buddy, &dive->buddy, master->buddy);
	if (new_text) {
		add_people(new_text);
		changed = 1;
	}

	new_text = get_combo_box_entry_text(info->suit, &dive->suit, master->suit);
	if (new_text) {
		add_suit(new_text);
		changed = 1;
	}

	rating_string = strdup(star_strings[dive->rating]);
	new_text = get_combo_box_entry_text(info->rating, &rating_string, star_strings[master->rating]);
	if (new_text) {
		dive->rating = get_rating(rating_string);
		free(rating_string);
		changed =1;
	}

	if (info->notes) {
		old_text = dive->notes;
		dive->notes = get_text(info->notes);
		if (text_changed(old_text,dive->notes))
			changed = 1;
		if (old_text)
			g_free(old_text);
	}
	if (changed) {
		mark_divelist_changed(TRUE);
		update_dive(dive);
	}
}

static void dive_trip_widget(GtkWidget *box, struct dive *trip, struct dive_info *info)
{
	GtkWidget *hbox, *label;
	char buffer[80] = "Edit trip summary";

	label = gtk_label_new(buffer);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	info->location = text_entry(box, "Location", location_list, trip->location);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->notes = text_view(box, "Notes", READ_WRITE);
	if (trip->notes && *trip->notes)
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(info->notes), trip->notes, -1);
}

static void dive_info_widget(GtkWidget *box, struct dive *dive, struct dive_info *info, gboolean multi)
{
	GtkWidget *hbox, *label, *frame, *equipment;
	char buffer[80] = "Edit multiple dives";

	if (!multi)
		divename(buffer, sizeof(buffer), dive);
	label = gtk_label_new(buffer);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	info->location = text_entry(box, "Location", location_list, dive->location);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->divemaster = text_entry(hbox, "Dive master", people_list, dive->divemaster);
	info->buddy = text_entry(hbox, "Buddy", people_list, dive->buddy);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->rating = text_entry(hbox, "Rating", star_list, star_strings[dive->rating]);
	info->suit = text_entry(hbox, "Suit", suit_list, dive->suit);

	/* only show notes if editing a single dive */
	if (multi) {
		info->notes = NULL;
	} else {
		info->notes = text_view(box, "Notes", READ_WRITE);
		if (dive->notes && *dive->notes)
			gtk_text_buffer_set_text(gtk_text_view_get_buffer(info->notes), dive->notes, -1);
	}
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	/* create a secondary Equipment widget */
	frame = gtk_frame_new("Equipment");
	equipment = equipment_widget(W_IDX_SECONDARY);
	gtk_container_add(GTK_CONTAINER(frame), equipment);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
}

/* we use these to find out if we edited the cylinder or weightsystem entries */
static cylinder_t remember_cyl[MAX_CYLINDERS];
static weightsystem_t remember_ws[MAX_WEIGHTSYSTEMS];
#define CYL_BYTES sizeof(cylinder_t) * MAX_CYLINDERS
#define WS_BYTES sizeof(weightsystem_t) * MAX_WEIGHTSYSTEMS

static void save_equipment_data(struct dive *dive)
{
	if (dive) {
		memcpy(remember_cyl, dive->cylinder, CYL_BYTES);
		memcpy(remember_ws, dive->weightsystem, WS_BYTES);
	}
}

/* the editing happens on the master dive; we copy the equipment
   data if it has changed in the master dive and the other dive
   either has no entries for the equipment or the same entries
   as the master dive had before it was edited */
void update_equipment_data(struct dive *dive, struct dive *master)
{
	if (dive == master)
		return;
	if ( ! cylinders_equal(remember_cyl, master->cylinder) &&
		(no_cylinders(dive->cylinder) ||
			cylinders_equal(dive->cylinder, remember_cyl)))
		copy_cylinders(master->cylinder, dive->cylinder);
	if (! weightsystems_equal(remember_ws, master->weightsystem) &&
		(no_weightsystems(dive->weightsystem) ||
			weightsystems_equal(dive->weightsystem, remember_ws)))
		memcpy(dive->weightsystem, master->weightsystem, WS_BYTES);
}

gboolean edit_trip(struct dive *trip)
{
	GtkWidget *dialog, *vbox;
	int success;
	gboolean changed = FALSE;
	char *old_text, *new_text;
	struct dive_info info;

	memset(&info, 0, sizeof(struct dive_info));
	dialog = gtk_dialog_new_with_buttons("Edit Trip Info",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	dive_trip_widget(vbox, trip, &info);
	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		/* we need to store the edited location and notes */
		new_text = get_combo_box_entry_text(info.location, &trip->location, trip->location);
		if (new_text) {
			add_location(new_text);
			changed = TRUE;
		}
		if (info.notes) {
			old_text = trip->notes;
			trip->notes = get_text(info.notes);
			if (text_changed(old_text, trip->notes))
				changed = TRUE;
			if (old_text)
				g_free(old_text);
		}
		if (changed) {
			mark_divelist_changed(TRUE);
			flush_divelist(trip);
		}
	}
	gtk_widget_destroy(dialog);
	return changed;
}

int edit_multi_dive_info(struct dive *single_dive)
{
	int success;
	GtkWidget *dialog, *vbox;
	struct dive_info info;
	struct dive *master;
	gboolean multi;

	dialog = gtk_dialog_new_with_buttons("Dive Info",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	master = single_dive;
	if (!master)
		master = current_dive;

	/* See if we should use multi dive mode */
	multi = FALSE;
	if (!single_dive) {
		int i;
		struct dive *dive;

		for_each_dive(i, dive) {
			if (dive != master && dive->selected) {
				multi = TRUE;
				break;
			}
		}
	}
	/* edit a temporary copy of the master dive;
	 * edit_dive is a global dive structure that is modified by the
	 * cylinder / weightsystem dialogs if we open W_IDX_SECONDARY
	 * edit widgets as we do here */
	memcpy(&edit_dive, master, sizeof(struct dive));

	dive_info_widget(vbox, &edit_dive, &info, multi);
	show_dive_equipment(&edit_dive, W_IDX_SECONDARY);
	save_equipment_data(&edit_dive);
	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (success) {
		mark_divelist_changed(TRUE);
		/* Update the non-current selected dives first */
		if (!single_dive) {
			int i;
			struct dive *dive;

			for_each_dive(i, dive) {
				if (dive == master || !dive->selected)
					continue;
				/* copy all "info" fields */
				save_dive_info_changes(dive, &edit_dive, &info);
				/* copy the cylinders / weightsystems */
				update_equipment_data(dive, &edit_dive);
				/* this is extremely inefficient... it loops through all
				   dives to find the right one - but we KNOW the index already */
				update_cylinder_related_info(dive);
				flush_divelist(dive);
			}
		}

		/* Update the master dive last! */
		save_dive_info_changes(master, &edit_dive, &info);
		update_equipment_data(master, &edit_dive);
		update_cylinder_related_info(master);
		flush_divelist(master);
		process_selected_dives();
		update_dive(master);
	}
	gtk_widget_destroy(dialog);

	return success;
}

int edit_dive_info(struct dive *dive)
{
	if (!dive)
		return 0;
	return edit_multi_dive_info(dive);
}

static GtkWidget *frame_box(GtkWidget *vbox, const char *fmt, ...)
{
	va_list ap;
	char buffer[64];
	GtkWidget *frame, *hbox;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	frame = gtk_frame_new(buffer);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	hbox = gtk_hbox_new(0, 3);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	return hbox;
}

/* Fixme - should do at least depths too - a dive without a depth is kind of pointless */
static time_t dive_time_widget(struct dive *dive)
{
	GtkWidget *dialog;
	GtkWidget *cal, *hbox, *vbox, *box;
	GtkWidget *h, *m;
	GtkWidget *duration, *depth;
	GtkWidget *label;
	guint yval, mval, dval;
	struct tm tm, *time;
	int success;
	double depthinterval, val;

	dialog = gtk_dialog_new_with_buttons("Date and Time",
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	/* Calendar hbox */
	hbox = frame_box(vbox, "Date:");
	cal = gtk_calendar_new();
	gtk_box_pack_start(GTK_BOX(hbox), cal, FALSE, TRUE, 0);

	/* Time hbox */
	hbox = frame_box(vbox, "Time");

	h = gtk_spin_button_new_with_range (0.0, 23.0, 1.0);
	m = gtk_spin_button_new_with_range (0.0, 59.0, 1.0);

	/*
	 * If we have a dive selected, 'add dive' will default
	 * to one hour after the end of that dive. Otherwise,
	 * we'll just take the current time.
	 */
	if (amount_selected == 1) {
		time_t when = current_dive->when;
		when += current_dive->duration.seconds;
		when += 60*60;
		time = gmtime(&when);
	} else {
		time_t now;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		now = tv.tv_sec;
		time = localtime(&now);
	}
	gtk_calendar_select_month(GTK_CALENDAR(cal), time->tm_mon, time->tm_year + 1900);
	gtk_calendar_select_day(GTK_CALENDAR(cal), time->tm_mday);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(h), time->tm_hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(m), (time->tm_min / 5)*5);

	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(h), TRUE);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(m), TRUE);

	gtk_box_pack_end(GTK_BOX(hbox), m, FALSE, FALSE, 0);
	label = gtk_label_new(":");
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), h, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Duration hbox */
	box = frame_box(hbox, "Duration (min)");
	duration = gtk_spin_button_new_with_range (0.0, 1000.0, 1.0);
	gtk_box_pack_end(GTK_BOX(box), duration, FALSE, FALSE, 0);

	/* Depth box */
	box = frame_box(hbox, "Depth (%s):", output_units.length == FEET ? "ft" : "m");
	if (output_units.length == FEET) {
		depthinterval = 1.0;
	} else {
		depthinterval = 0.1;
	}
	depth = gtk_spin_button_new_with_range (0.0, 1000.0, depthinterval);
	gtk_box_pack_end(GTK_BOX(box), depth, FALSE, FALSE, 0);

	/* All done, show it and wait for editing */
	gtk_widget_show_all(dialog);
	success = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT;
	if (!success) {
		gtk_widget_destroy(dialog);
		return 0;
	}

	memset(&tm, 0, sizeof(tm));
	gtk_calendar_get_date(GTK_CALENDAR(cal), &yval, &mval, &dval);
	tm.tm_year = yval;
	tm.tm_mon = mval;
	tm.tm_mday = dval;

	tm.tm_hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(h));
	tm.tm_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(m));

	val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(depth));
	if (output_units.length == FEET) {
		dive->maxdepth.mm = feet_to_mm(val);
	} else {
		dive->maxdepth.mm = val * 1000 + 0.5;
	}

	dive->duration.seconds = gtk_spin_button_get_value(GTK_SPIN_BUTTON(duration))*60;

	gtk_widget_destroy(dialog);
	dive->when = utc_mktime(&tm);

	return 1;
}

int add_new_dive(struct dive *dive)
{
	if (!dive)
		return 0;

	if (!dive_time_widget(dive))
		return 0;

	return edit_dive_info(dive);
}

GtkWidget *extended_dive_info_widget(void)
{
	GtkWidget *vbox, *hbox;
	vbox = gtk_vbox_new(FALSE, 6);

	people_list = gtk_list_store_new(1, G_TYPE_STRING);
	location_list = gtk_list_store_new(1, G_TYPE_STRING);
	star_list = gtk_list_store_new(1, G_TYPE_STRING);
	add_string_list_entry(ZERO_STARS, star_list);
	add_string_list_entry(ONE_STARS, star_list);
	add_string_list_entry(TWO_STARS, star_list);
	add_string_list_entry(THREE_STARS, star_list);
	add_string_list_entry(FOUR_STARS, star_list);
	add_string_list_entry(FIVE_STARS, star_list);
	suit_list = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	location = text_value(vbox, "Location");

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	divemaster = text_value(hbox, "Divemaster");
	buddy = text_value(hbox, "Buddy");

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	rating = text_value(hbox, "Rating");
	suit = text_value(hbox, "Suit");

	notes = text_view(vbox, "Notes", READ_ONLY);
	return vbox;
}
