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
#include <glib/gi18n.h>

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
static char *get_combo_box_entry_text(GtkComboBox *combo_box, char **textp, const char *master)
{
	char *old = *textp;
	const char *old_text;
	const gchar *new;

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

	new = get_active_text(combo_box);
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
	struct tm tm;

	utc_mkdate(dive->when, &tm);
	/*++GETTEXT 80 char buffer: dive nr, weekday, month, day, year, hour, min */
	return snprintf(buf, size, _("Dive #%1$d - %2$s %3$02d/%4$02d/%5$04d at %6$d:%7$02d"),
		dive->number,
		weekday(tm.tm_wday),
		tm.tm_mon+1, tm.tm_mday,
		tm.tm_year+1900,
		tm.tm_hour, tm.tm_min);
}

void show_dive_info(struct dive *dive)
{
	const char *subs = "Subsurface: ";
	const char *text;
	const int maxlen = 128;
	char *basename;
	char *title;
	char *buffer = NULL;
	int len1, len2, sz;

	if (!dive) {
		if (existing_filename) {
			basename = g_path_get_basename(existing_filename);
			len1 = strlen(subs);
			len2 = g_utf8_strlen(basename, -1);
			sz = (len1 + len2 + 1) * sizeof(gunichar);
			title = malloc(sz);
			strncpy(title, subs, len1);
			g_utf8_strncpy(title + len1, basename, len2);
			gtk_window_set_title(GTK_WINDOW(main_window), title);
			free(basename);
			free(title);
		} else {
			gtk_window_set_title(GTK_WINDOW(main_window), "Subsurface");
		}
		SET_TEXT_VALUE(divemaster);
		SET_TEXT_VALUE(buddy);
		SET_TEXT_VALUE(location);
		SET_TEXT_VALUE(suit);
		gtk_entry_set_text(rating, star_strings[0]);
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(notes), "", -1);
		show_dive_equipment(NULL, W_IDX_PRIMARY);
		return;
	}

	/* dive number and location (or lacking that, the date) go in the window title */
	text = dive->location;
	if (!text)
		text = "";
	if (*text) {
		if (dive->number) {
			len1 = g_utf8_strlen(text, -1);
			sz = (len1 + 32) * sizeof(gunichar);
			buffer = malloc(sz);
			snprintf(buffer, sz, _("Dive #%d - "), dive->number);
			g_utf8_strncpy(buffer + strlen(buffer), text, len1);
			text = buffer;
		}
	} else {
		sz = (maxlen + 32) * sizeof(gunichar);
		buffer = malloc(sz);
		divename(buffer, sz, dive);
		text = buffer;
	}

	/* put it all together */
	if (existing_filename) {
		basename = g_path_get_basename(existing_filename);
		len1 = g_utf8_strlen(basename, -1);
		len2 = g_utf8_strlen(text, -1);
		if (len2 > maxlen)
			len2 = maxlen;
		sz = (len1 + len2 + 3) * sizeof(gunichar); /* reserver space for ": " */
		title = malloc(sz);
		g_utf8_strncpy(title, basename, len1);
		strncpy(title + strlen(basename), (const char *)": ", 2);
		g_utf8_strncpy(title + strlen(basename) + 2, text, len2);
		gtk_window_set_title(GTK_WINDOW(main_window), title);
		free(basename);
		free(title);
	} else {
		gtk_window_set_title(GTK_WINDOW(main_window), text);
	}
	if (buffer)
		free(buffer);
	SET_TEXT_VALUE(divemaster);
	SET_TEXT_VALUE(buddy);
	SET_TEXT_VALUE(location);
	SET_TEXT_VALUE(suit);
	gtk_entry_set_text(rating, star_strings[dive->rating]);
	gtk_text_buffer_set_text(gtk_text_view_get_buffer(notes),
		dive && dive->notes ? dive->notes : "", -1);
}

static void info_menu_edit_cb(GtkMenuItem *menuitem, gpointer user_data)
{
	if (amount_selected)
		edit_multi_dive_info(NULL);
}

static void add_menu_item(GtkMenuShell *menu, const char *label, const char *icon, void (*cb)(GtkMenuItem *, gpointer))
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
	gtk_menu_shell_prepend(menu, item);
}

static void populate_popup_cb(GtkTextView *entry, GtkMenuShell *menu, gpointer user_data)
{
	if (amount_selected)
		add_menu_item(menu, _("Edit"), GTK_STOCK_EDIT, info_menu_edit_cb);
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

static GtkEntry *single_text_entry(GtkWidget *box, const char *label, const char *text)
{
	GtkEntry *entry;
	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);
	entry = GTK_ENTRY(gtk_entry_new());
	gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(entry));
	if (text && *text)
		gtk_entry_set_text(entry, text);
	return entry;
}

static GtkComboBox *text_entry(GtkWidget *box, const char *label, GtkListStore *completions, const char *text)
{
	GtkWidget *combo_box;
	GtkWidget *frame = gtk_frame_new(label);

	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	combo_box = combo_box_with_model_and_entry(completions);
	gtk_container_add(GTK_CONTAINER(frame), combo_box);

	if (text && *text)
		set_active_text(GTK_COMBO_BOX(combo_box), text);

	return GTK_COMBO_BOX(combo_box);
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

int match_list(GtkListStore *list, const char *string)
{
	found_string_entry = MATCH_PREPEND;
	gtk_tree_model_foreach(GTK_TREE_MODEL(list), match_string_entry, (void *)string);
	return found_string_entry;
}

void add_string_list_entry(const char *string, GtkListStore *list)
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

/* this has to be done with UTF8 as people might want to enter the degree symbol */
static gboolean parse_gps_text(const char *gps_text, double *latitude, double *longitude)
{
	const char *text = gps_text;
	char *endptr;
	gboolean south = FALSE;
	gboolean west = FALSE;
	double parselat, parselong;
	gunichar degrees = UCS4_DEGREE;
	gunichar c;

	while (g_unichar_isspace(g_utf8_get_char(text)))
		text = g_utf8_next_char(text);

	/* an empty string is interpreted as 0.0,0.0 and therefore "no gps location" */
	if (!*text) {
		*latitude = 0.0;
		*longitude = 0.0;
		return TRUE;
	}
	/* ok, let's parse by hand - first degrees of latitude */
	if (g_unichar_toupper(g_utf8_get_char(text)) == 'N' ||
	    !strncmp(text, _("N"), strlen(_("N"))))
		text++;
	if (g_unichar_toupper(g_utf8_get_char(text)) == 'S' ||
	    !strncmp(text, _("S"), strlen(_("S")))) {
		text++;
		south = TRUE;
	}
	parselat = strtod(text, &endptr);
	if (text == endptr)
		return FALSE;
	text = endptr;
	if (parselat < 0.0) {
		south = TRUE;
		parselat *= -1;
	}

	/* next optional minutes as decimal, skipping degree symbol */
	while (g_unichar_isspace(c = g_utf8_get_char(text)) || c == degrees)
		text = g_utf8_next_char(text);
	if (g_unichar_toupper(c) != 'E' && g_unichar_toupper(c) != 'W' && c != ';' && c != ',') {
		parselat += strtod(text, &endptr) / 60.0;
		if (text == endptr)
			return FALSE;
		text = endptr;
		/* skip trailing minute symbol */
		if (g_utf8_get_char(text) == '\'')
			text = g_utf8_next_char(text);
	}
	/* skip seperator between latitude and longitude */
	while (g_unichar_isspace(c = g_utf8_get_char(text)) || c == ';' || c == ',')
		text = g_utf8_next_char(text);

	/* next degrees of longitude */
	if (g_unichar_toupper(g_utf8_get_char(text)) == 'E' ||
	    !strncmp(text, _("E"), strlen(_("E"))))
		text++;
	if (g_unichar_toupper(g_utf8_get_char(text)) == 'W' ||
	    !strncmp(text, _("W"), strlen(_("W")))) {
		text++;
		west = TRUE;
	}
	parselong = strtod(text, &endptr);
	if (text == endptr)
		return FALSE;
	text = endptr;
	if (parselong < 0.0) {
		west = TRUE;
		parselong *= -1;
	}

	/* next optional minutes as decimal, skipping degree symbol */
	while (g_unichar_isspace(c = g_utf8_get_char(text)) || c == degrees)
		text = g_utf8_next_char(text);
	if (*text) {
		parselong += strtod(text, &endptr) / 60.0;
		if (text == endptr)
			return FALSE;
		text = endptr;
		/* skip trailing minute symbol */
		if (g_utf8_get_char(text) == '\'')
			text = g_utf8_next_char(text);
		/* make sure there's nothing else left on the input */
		while (g_unichar_isspace(g_utf8_get_char(text)))
			text = g_utf8_next_char(text);
		if (*text)
			return FALSE;
	}
	if (west && parselong > 0.0)
		parselong *= -1;
	if (south && parselat > 0.0)
		parselat *= -1;
	*latitude = parselat;
	*longitude = parselong;
	return TRUE;
}

static gboolean gps_changed(struct dive *dive, struct dive *master, const char *gps_text)
{
	double latitude, longitude;
	int latudeg, longudeg;

	/* if we have a master and the dive's gps address is different from it,
	 * don't change the dive */
	if (master && (master->latitude.udeg != dive->latitude.udeg ||
		       master->longitude.udeg != dive->longitude.udeg))
		return FALSE;

	if (!parse_gps_text(gps_text, &latitude, &longitude))
		return FALSE;

	latudeg = 1000000 * latitude + 0.5;
	longudeg = 1000000 * longitude + 0.5;

	/* if master gps didn't change, don't change dive */
	if (master && master->latitude.udeg == latudeg && master->longitude.udeg == longudeg)
		return FALSE;
	/* if dive gps didn't change, nothing changed */
	if (dive->latitude.udeg == latudeg && dive->longitude.udeg == longudeg)
		return FALSE;
	/* ok, update the dive and mark things changed */
	dive->latitude.udeg = latudeg;
	dive->longitude.udeg = longudeg;
	return TRUE;
}

struct dive_info {
	GtkComboBox *location, *divemaster, *buddy, *rating, *suit, *viz;
	GtkEntry *airtemp, *gps;
	GtkWidget *gps_icon;
	GtkTextView *notes;
};

static void save_dive_info_changes(struct dive *dive, struct dive *master, struct dive_info *info)
{
	char *old_text, *new_text;
	const char *gps_text;
	char *rating_string;
	double newtemp;
	int changed = 0;

	new_text = get_combo_box_entry_text(info->location, &dive->location, master->location);
	if (new_text) {
		add_location(new_text);
		changed = 1;
	}

	gps_text = gtk_entry_get_text(info->gps);
	if (gps_changed(dive, master, gps_text))
		changed = 1;

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
		changed = 1;
	}
	free(rating_string);

	rating_string = strdup(star_strings[dive->visibility]);
	new_text = get_combo_box_entry_text(info->viz, &rating_string, star_strings[master->visibility]);
	if (new_text) {
		dive->visibility = get_rating(rating_string);
		changed = 1;
	}
	free(rating_string);

	new_text = (char *)gtk_entry_get_text(info->airtemp);
	if (sscanf(new_text, "%lf", &newtemp) == 1) {
		unsigned long mkelvin;
		switch (prefs.units.temperature) {
		case CELSIUS:
			mkelvin = C_to_mkelvin(newtemp);
			break;
		case FAHRENHEIT:
			mkelvin = F_to_mkelvin(newtemp);
			break;
		default:
			mkelvin = 0;
		}
		if (mkelvin != dive->dc.airtemp.mkelvin) {
			dive->dc.airtemp.mkelvin = mkelvin;
			changed = 1;
		}
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

static void dive_trip_widget(GtkWidget *box, dive_trip_t *trip, struct dive_info *info)
{
	GtkWidget *hbox, *label;
	char buffer[128] = N_("Edit trip summary");

	label = gtk_label_new(_(buffer));
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	info->location = text_entry(box, _("Location"), location_list, trip->location);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->notes = text_view(box, _("Notes"), READ_WRITE);
	if (trip->notes && *trip->notes)
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(info->notes), trip->notes, -1);
}

struct location_update {
	GtkEntry *entry;
	struct dive *dive;
	void (*callback)(float, float);
} location_update;

static void print_gps_coordinates(char *buffer, int len, float lat, float lon)
{
	unsigned int latdeg, londeg;
	float latmin, lonmin;
	char *lath, *lonh;

	lath = lat >= 0.0 ? _("N") : _("S");
	lonh = lon >= 0.0 ? _("E") : _("W");
	lat = fabs(lat);
	lon = fabs(lon);
	latdeg = lat;
	londeg = lon;
	latmin = (lat - latdeg) * 60.0;
	lonmin = (lon - londeg) * 60.0;
	snprintf(buffer, len, "%s%u%s %6.3f\' , %s%u%s %6.3f\'",
		lath, latdeg, UTF8_DEGREE, latmin,
		lonh, londeg, UTF8_DEGREE, lonmin);
}

static void update_gps_entry(float lat, float lon)
{
	char gps_text[45];

	if (location_update.entry) {
		print_gps_coordinates(gps_text, 45, lat, lon);
		gtk_entry_set_text(location_update.entry, gps_text);
	}
}

#if HAVE_OSM_GPS_MAP
static gboolean gps_map_callback(GtkWidget *w, gpointer data)
{
	struct dive *dive = location_update.dive;
	show_gps_location(dive, update_gps_entry);
	return TRUE;
}
#endif

static void dive_info_widget(GtkWidget *box, struct dive *dive, struct dive_info *info, gboolean multi)
{
	GtkWidget *hbox, *label, *frame, *equipment, *image;
	char buffer[128];
	char airtemp[6];
	char gps_text[45] = "";
	const char *unit;
	double value;

	snprintf(buffer, sizeof(buffer), "%s", _("Edit multiple dives"));

	if (!multi)
		divename(buffer, sizeof(buffer), dive);
	label = gtk_label_new(buffer);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, TRUE, 0);

	info->location = text_entry(box, _("Location"), location_list, dive->location);

	if (dive_has_gps_location(dive))
		print_gps_coordinates(gps_text, sizeof(gps_text), dive->latitude.udeg / 1000000.0,
								  dive->longitude.udeg / 1000000.0);
	hbox = gtk_hbox_new(FALSE, 2);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);
	info->gps = single_text_entry(hbox, _("GPS (WGS84 or GPS format)"), gps_text);
	gtk_entry_set_width_chars(info->gps, 30);
#if HAVE_OSM_GPS_MAP
	info->gps_icon = gtk_button_new_with_label(_("Pick on map"));
	gtk_box_pack_start(GTK_BOX(hbox), info->gps_icon, FALSE, FALSE, 6);
	image = gtk_image_new_from_pixbuf(get_gps_icon());
	gtk_image_set_pixel_size(GTK_IMAGE(image), 128);
	gtk_button_set_image(GTK_BUTTON(info->gps_icon), image);

	location_update.entry = info->gps;
	location_update.dive = dive;
	g_signal_connect(G_OBJECT(info->gps_icon), "clicked", G_CALLBACK(gps_map_callback), NULL);
#endif
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->divemaster = text_entry(hbox, _("Dive master"), people_list, dive->divemaster);
	info->buddy = text_entry(hbox, _("Buddy"), people_list, dive->buddy);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->rating = text_entry(hbox, _("Rating"), star_list, star_strings[dive->rating]);
	info->suit = text_entry(hbox, _("Suit"), suit_list, dive->suit);

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	info->viz = text_entry(hbox, _("Visibility"), star_list, star_strings[dive->visibility]);

	value = get_temp_units(dive->dc.airtemp.mkelvin, &unit);
	snprintf(buffer, sizeof(buffer), _("Air Temp in %s"), unit);
	if (dive->dc.airtemp.mkelvin)
		snprintf(airtemp, sizeof(airtemp), "%.1f", value);
	else
		airtemp[0] = '\0';
	info->airtemp = single_text_entry(hbox, buffer, airtemp);

	/* only show notes if editing a single dive */
	if (multi) {
		info->notes = NULL;
	} else {
		info->notes = text_view(box, _("Notes"), READ_WRITE);
		if (dive->notes && *dive->notes)
			gtk_text_buffer_set_text(gtk_text_view_get_buffer(info->notes), dive->notes, -1);
	}
	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, TRUE, 0);

	/* create a secondary Equipment widget */
	frame = gtk_frame_new(_("Equipment"));
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

/* Empty and NULL compare equal */
static int same_string(const char *a, const char *b)
{
	/* Both NULL or same */
	if (a == b)
		return 1;
	/* Both non-NULL: strcmp */
	if (a && b)
		return !strcmp(a, b);
	/* One non-NULL? Is that one empty? */
	return !*(a ? a : b);
}

static int same_type(cylinder_t *dst, cylinder_t *src)
{
	return	dst->type.size.mliter == src->type.size.mliter &&
		dst->type.workingpressure.mbar == src->type.workingpressure.mbar &&
		same_string(dst->type.description, src->type.description);
}

static void copy_type(cylinder_t *dst, cylinder_t *src)
{
	dst->type.size = src->type.size;
	dst->type.workingpressure = src->type.workingpressure;
	if (dst->type.description)
		free((void *)dst->type.description);
	if (!src->type.description || !*src->type.description)
		dst->type.description = NULL;
	else
		dst->type.description = strdup((char *)src->type.description);
}

static int same_gasmix(cylinder_t *dst, cylinder_t *src)
{
	return !memcmp(&dst->gasmix, &src->gasmix, sizeof(dst->gasmix));
}

static void copy_gasmix(cylinder_t *dst, cylinder_t *src)
{
	memcpy(&dst->gasmix, &src->gasmix, sizeof(dst->gasmix));
}

static int same_press(cylinder_t *dst, cylinder_t *src)
{
	return	dst->start.mbar == src->start.mbar &&
		dst->end.mbar == src->end.mbar;
}

static void copy_press(cylinder_t *dst, cylinder_t *src)
{
	dst->start = src->start;
	dst->end = src->end;
}

/*
 * When we update the cylinder information, we do it individually
 * by type/gasmix/pressure, so that you can change them separately.
 *
 * The rule is: the destination has to be the same as the original
 * field, and the source has to have changed. If so, we change the
 * destination field.
 */
static void update_cylinder(cylinder_t *dst, cylinder_t *src, cylinder_t *orig)
{
	/* Destination type same? Change it */
	if (same_type(dst, orig) && !same_type(src, orig))
		copy_type(dst, src);

	/* Destination gasmix same? Change it */
	if (same_gasmix(dst, orig) && !same_gasmix(src, orig))
		copy_gasmix(dst, src);

	/* Destination pressures the same? */
	if (same_press(dst, orig) && !same_press(src, orig))
		copy_press(dst, src);
}

/* the editing happens on the master dive; we copy the equipment
   data if it has changed in the master dive and the other dive
   either has no entries for the equipment or the same entries
   as the master dive had before it was edited */
static void update_equipment_data(struct dive *dive, struct dive *master)
{
	int i;

	if (dive == master)
		return;
	for (i = 0; i < MAX_CYLINDERS; i++)
		update_cylinder(dive->cylinder+i, master->cylinder+i, remember_cyl+i);
	if (! weightsystems_equal(remember_ws, master->weightsystem) &&
		(no_weightsystems(dive->weightsystem) ||
			weightsystems_equal(dive->weightsystem, remember_ws)))
		memcpy(dive->weightsystem, master->weightsystem, WS_BYTES);
}

gboolean edit_trip(dive_trip_t *trip)
{
	GtkWidget *dialog, *vbox;
	int success;
	gboolean changed = FALSE;
	char *old_text, *new_text;
	struct dive_info info;

	memset(&info, 0, sizeof(struct dive_info));
	dialog = gtk_dialog_new_with_buttons(_("Edit Trip Info"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
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
		if (changed)
			mark_divelist_changed(TRUE);
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

	dialog = gtk_dialog_new_with_buttons(_("Dive Info"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	master = single_dive;
	if (!master)
		master = current_dive;
	if (!master)
		return 0;
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
	location_update.entry = NULL;

	return success;
}

int edit_dive_info(struct dive *dive, gboolean newdive)
{
	if (!dive || (!newdive && !amount_selected))
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

GtkWidget *create_date_time_widget(struct tm *time, GtkWidget **cal, GtkWidget **h, GtkWidget **m)
{
	GtkWidget *dialog;
	GtkWidget *hbox, *vbox;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons(_("Date and Time"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	/* Calendar hbox */
	hbox = frame_box(vbox, _("Date:"));
	*cal = gtk_calendar_new();
	gtk_box_pack_start(GTK_BOX(hbox), *cal, FALSE, TRUE, 0);

	/* Time hbox */
	hbox = frame_box(vbox, _("Time"));

	*h = gtk_spin_button_new_with_range (0.0, 23.0, 1.0);
	*m = gtk_spin_button_new_with_range (0.0, 59.0, 1.0);


	gtk_calendar_select_month(GTK_CALENDAR(*cal), time->tm_mon, time->tm_year + 1900);
	gtk_calendar_select_day(GTK_CALENDAR(*cal), time->tm_mday);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(*h), time->tm_hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(*m), (time->tm_min / 5)*5);

	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(*h), TRUE);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(*m), TRUE);

	gtk_box_pack_end(GTK_BOX(hbox), *m, FALSE, FALSE, 0);
	label = gtk_label_new(":");
	gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), *h, FALSE, FALSE, 0);

	return dialog;
}

static timestamp_t dive_time_widget(struct dive *dive)
{
	GtkWidget *dialog;
	GtkWidget *cal, *vbox, *hbox, *box;
	GtkWidget *h, *m;
	GtkWidget *duration, *depth;
	guint yval, mval, dval;
	struct tm tm, *time;
	int success;
	double depthinterval, val;

	/*
	 * If we have a dive selected, 'add dive' will default
	 * to one hour after the end of that dive. Otherwise,
	 * we'll just take the current time.
	 */
	if (amount_selected == 1) {
		timestamp_t when = current_dive->when;
		when += current_dive->dc.duration.seconds;
		when += 60*60;
		utc_mkdate(when, &tm);
		time = &tm;
	} else {
		time_t now;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		now = tv.tv_sec;
		time = localtime(&now);
	}
	dialog = create_date_time_widget(time, &cal, &h, &m);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	hbox = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Duration hbox */
	box = frame_box(hbox, _("Duration (min)"));
	duration = gtk_spin_button_new_with_range (0.0, 1000.0, 1.0);
	gtk_box_pack_end(GTK_BOX(box), duration, FALSE, FALSE, 0);

	/* Depth box */
	box = frame_box(hbox, _("Depth (%s):"), prefs.units.length == FEET ? _("ft") : _("m"));
	if (prefs.units.length == FEET) {
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
	if (prefs.units.length == FEET) {
		dive->dc.maxdepth.mm = feet_to_mm(val);
	} else {
		dive->dc.maxdepth.mm = val * 1000 + 0.5;
	}

	dive->dc.duration.seconds = gtk_spin_button_get_value(GTK_SPIN_BUTTON(duration))*60;

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

	return edit_dive_info(dive, TRUE);
}

GtkWidget *extended_dive_info_widget(void)
{
	GtkWidget *vbox, *hbox;
	vbox = gtk_vbox_new(FALSE, 6);

	people_list = gtk_list_store_new(1, G_TYPE_STRING);
	location_list = gtk_list_store_new(1, G_TYPE_STRING);
	star_list = gtk_list_store_new(1, G_TYPE_STRING);
	add_string_list_entry(star_strings[0], star_list);
	add_string_list_entry(star_strings[1], star_list);
	add_string_list_entry(star_strings[2], star_list);
	add_string_list_entry(star_strings[3], star_list);
	add_string_list_entry(star_strings[4], star_list);
	add_string_list_entry(star_strings[5], star_list);
	suit_list = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	location = text_value(vbox, _("Location"));

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	divemaster = text_value(hbox, _("Divemaster"));
	buddy = text_value(hbox, _("Buddy"));

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	rating = text_value(hbox, _("Rating"));
	suit = text_value(hbox, _("Suit"));

	notes = text_view(vbox, _("Notes"), READ_ONLY);
	return vbox;
}
