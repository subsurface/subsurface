/* info.c
 *
 * UI toolkit independent logic used for the info frame
 *
 * gboolean gps_changed(struct dive *dive, struct dive *master, const char *gps_text);
 * void print_gps_coordinates(char *buffer, int len, int lat, int lon);
 * void save_equipment_data(struct dive *dive);
 * void update_equipment_data(struct dive *dive, struct dive *master);
 * void update_time_depth(struct dive *dive, struct dive *edited);
 * const char *get_window_title(struct dive *dive);
 * char *evaluate_string_change(const char *newstring, char **textp, const char *master);
 * int text_changed(const char *old, const char *new);
 * gboolean parse_gps_text(const char *gps_text, double *latitude, double *longitude);
 * int divename(char *buf, size_t size, struct dive *dive, char *trailer);
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
#include "divelist.h"

/* old is NULL or a valid string, new is a valid string
 * NOTE: NULL and "" need to be treated as "unchanged" */
int text_changed(const char *old, const char *new)
{
	return (old && strcmp(old,new)) ||
		(!old && strcmp("",new));
}

static const char *skip_space(const char *str)
{
	if (str) {
		while (g_ascii_isspace(*str))
			str++;
		if (!*str)
			str = NULL;
	}
	return str;
}

/*
 * should this string be changed?
 * The "master" string is the string of the current dive - we only consider it
 * changed if the old string is either empty, or matches that master string.
 */
char *evaluate_string_change(const char *newstring, char **textp, const char *master)
{
	char *old = *textp;
	const char *old_text;

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

	while (g_ascii_isspace(*newstring))
		newstring++;
	/* If the master string didn't change, don't change other dives either! */
	if (!text_changed(master, newstring))
		return NULL;
	if (!text_changed(old, newstring))
		return NULL;
	free(old);
	*textp = strdup(newstring);
	return *textp;
}

int divename(char *buf, size_t size, struct dive *dive, char *trailer)
{
	struct tm tm;

	utc_mkdate(dive->when, &tm);
	/*++GETTEXT 80 char buffer: dive nr, weekday, month, day, year, hour, min <trailing text>*/
	return snprintf(buf, size, _("Dive #%1$d - %2$s %3$02d/%4$02d/%5$04d at %6$d:%7$02d %8$s"),
		dive->number,
		weekday(tm.tm_wday),
		tm.tm_mon+1, tm.tm_mday,
		tm.tm_year+1900,
		tm.tm_hour, tm.tm_min,
		trailer);
}

/* caller should free the string returned after it is no longer needed */
const char *get_window_title(struct dive *dive)
{
	const char *text;
	const int maxlen = 128;
	char *basename;
	char *title;
	char *buffer = NULL;
	int len1, len2, sz;

	if (!dive) {
		if (existing_filename) {
			char *basename;
			basename = g_path_get_basename(existing_filename);
			len1 = sizeof("Subsurface: ");
			len2 = g_utf8_strlen(basename, -1);
			sz = (len1 + len2) * sizeof(gunichar);
			title = malloc(sz);
			strncpy(title, "Subsurface: ", len1);
			g_utf8_strncpy(title + len1, basename, len2);
		} else {
			title = strdup("Subsurface");
		}
	} else {
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
			divename(buffer, sz, dive, "");
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
		} else {
			title = strdup(text);
		}
		if (buffer)
			free(buffer);
	}
	return title;
}

/* this is used to skip the cardinal directions (or check if they are
 * present). You pass in the text and a STRING with the direction.
 * This checks for both the standard english text (just one character)
 * and the translated text (possibly longer) and returns 0 if not found
 * and the number of chars to skip otherwise. */
static int string_advance_cardinal(const char *text, const char *look)
{
	char *trans;
	int len = strlen(look);
	if (!strncasecmp(text, look, len))
		return len;
	trans = _(look);
	len = strlen(trans);
	if (!strncasecmp(text, trans, len))
		return len;
	return 0;
}

/* this has to be done with UTF8 as people might want to enter the degree symbol */
gboolean parse_gps_text(const char *gps_text, double *latitude, double *longitude)
{
	const char *text = gps_text;
	char *endptr;
	gboolean south = FALSE;
	gboolean west = FALSE;
	double parselat, parselong;
	gunichar degrees = UCS4_DEGREE;
	gunichar c;
	int incr;

	while (g_unichar_isspace(g_utf8_get_char(text)))
		text = g_utf8_next_char(text);

	/* an empty string is interpreted as 0.0,0.0 and therefore "no gps location" */
	if (!*text) {
		*latitude = 0.0;
		*longitude = 0.0;
		return TRUE;
	}
	/* ok, let's parse by hand - first degrees of latitude */
	text += string_advance_cardinal(text, "N");
	if ((incr = string_advance_cardinal(text, "S")) > 0) {
		text += incr;
		south = TRUE;
	}
	parselat = g_ascii_strtod(text, &endptr);
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
	incr = string_advance_cardinal(text, "E") + string_advance_cardinal(text, "W");
	if (!incr && c != ';' && c != ',') {
		parselat += g_ascii_strtod(text, &endptr) / 60.0;
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
	text += string_advance_cardinal(text, "E");
	if ((incr = string_advance_cardinal(text, "W")) > 0) {
		text += incr;
		west = TRUE;
	}
	parselong = g_ascii_strtod(text, &endptr);
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
		parselong += g_ascii_strtod(text, &endptr) / 60.0;
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

gboolean gps_changed(struct dive *dive, struct dive *master, const char *gps_text)
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

	latudeg = rint(1000000 * latitude);
	longudeg = rint(1000000 * longitude);

	/* if dive gps didn't change, nothing changed */
	if (dive->latitude.udeg == latudeg && dive->longitude.udeg == longudeg)
		return FALSE;
	/* ok, update the dive and mark things changed */
	dive->latitude.udeg = latudeg;
	dive->longitude.udeg = longudeg;
	return TRUE;
}

/* take latitude and longitude in udeg and print them in a human readable
 * form, without losing precision */
void print_gps_coordinates(char *buffer, int len, int lat, int lon)
{
	unsigned int latdeg, londeg;
	double latmin, lonmin;
	char *lath, *lonh, dbuf_lat[32], dbuf_lon[32];

	if (!lat && !lon) {
		*buffer = 0;
		return;
	}
	lath = lat >= 0 ? _("N") : _("S");
	lonh = lon >= 0 ? _("E") : _("W");
	lat = abs(lat);
	lon = abs(lon);
	latdeg = lat / 1000000;
	londeg = lon / 1000000;
	latmin = (lat % 1000000) * 60.0 / 1000000.0;
	lonmin = (lon % 1000000) * 60.0 / 1000000.0;
	*dbuf_lat = *dbuf_lon = 0;
	g_ascii_formatd(dbuf_lat, sizeof(dbuf_lat), "%8.5f", latmin);
	g_ascii_formatd(dbuf_lon, sizeof(dbuf_lon), "%8.5f", lonmin);
	if (!*dbuf_lat || !*dbuf_lon) {
		*buffer = 0;
		return;
	}
	snprintf(buffer, len, "%s%u%s %s\' , %s%u%s %s\'",
		lath, latdeg, UTF8_DEGREE, dbuf_lat,
		lonh, londeg, UTF8_DEGREE, dbuf_lon);
}

/* we use these to find out if we edited the cylinder or weightsystem entries */
static cylinder_t remember_cyl[MAX_CYLINDERS];
static weightsystem_t remember_ws[MAX_WEIGHTSYSTEMS];
#define CYL_BYTES sizeof(cylinder_t) * MAX_CYLINDERS
#define WS_BYTES sizeof(weightsystem_t) * MAX_WEIGHTSYSTEMS

void save_equipment_data(struct dive *dive)
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
void update_equipment_data(struct dive *dive, struct dive *master)
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

/* we can simply overwrite these - this only gets called if we edited
 * a single dive and the dive was first copied into edited - so we can
 * just take those values */
void update_time_depth(struct dive *dive, struct dive *edited)
{
	dive->when = edited->when;
	dive->dc.duration.seconds = edited->dc.duration.seconds;
	dive->dc.maxdepth.mm = edited->dc.maxdepth.mm;
	dive->dc.meandepth.mm = edited->dc.meandepth.mm;
}

void add_people(const char *string)
{
	/* add names to the completion list for people */
}
void add_location(const char *string)
{
	/* add names to the completion list for locations */
}
void add_suit(const char *string)
{
	/* add names to the completion list for suits */
}
