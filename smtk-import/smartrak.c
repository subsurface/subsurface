// SPDX-License-Identifier: GPL-2.0
/*
 * An .slg file is composed of a main table (Dives), a bunch of tables directly
 * linked to Dives by their indices (Site, Suit, Tank, etc) and another group of
 * independent tables (Activity, Type, Gear, Fish ...) which connect with the dives
 * via a related group of tables (ActivityRelation, TypeRelation ...) that contain
 * just the dive index number and the related table index number.
 * The data stored in the main group of tables are very extensive, going far beyond
 * the actual scope of Subsurface in most of cases; e.g. Dives table keeps
 * information which can be directly uploaded to DAN's database of dives, or Buddy
 * table can include telephones, photo or, even, buddy mother's maiden name.
 *
 * Although these funcs are suposed to work in a standalone tool, will also work
 * on main Subsurface import menu, by simply tweaking file.c and main_window.cpp
 * to call smartrak_import()
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mdbtools.h>
#include <stdarg.h>
#include <locale.h>

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#endif

#include "core/dive.h"
#include "core/subsurface-string.h"
#include "core/gettext.h"
#include "core/divelist.h"
#include "core/event.h"
#include "core/libdivecomputer.h"
#include "core/divesite.h"
#include "core/membuffer.h"
#include "core/tag.h"
#include "core/device.h"

/* SmartTrak version, constant for every single file */
int smtk_version;
int tanks;

/* Freeing temp char arrays utility */
static void smtk_free(char **array, int count)
{
	int n;
	for (n = 0; n < count; n++)
		free(array[n]);
	array = NULL;
}

/*
 * There are AFAIK two versions of Smarttrak. The newer one supports trimix and up
 * to 10 tanks. The other one just 3 tanks and no trimix but only nitrox. This is a
 * problem for an automated parser which has to support both formats.
 * In this solution I made an enum of fields with the same order they would have in
 * a smarttrak db, and a tiny function which returns the number of the column where
 * a field is expected to be, taking into account the different db formats .
 */
enum field_pos {IDX = 0, DIVENUM, _DATE, INTIME, INTVAL, DURATION, OUTTIME, DESATBEFORE, DESATAFTER, NOFLYBEFORE,
		NOFLYAFTER, NOSTOPDECO, MAXDEPTH, VISIBILITY, WEIGHT, O2FRAC, HEFRAC, PSTART, PEND, AIRTEMP,
		MINWATERTEMP, MAXWATERTEMP, SECFACT, ALARMS, MODE, REMARKS, DCNUMBER, DCMODEL, DIVETIMECOUNT, LOG,
		PROFILE, SITEIDX, ALTIDX, SUITIDX, WEATHERIDX, SURFACEIDX, UNDERWATERIDX, TANKIDX};

/*
 * Returns calculated column number depending on smartrak version, as there are more
 * tanks (10) in later versions than in older (3).
 * Older versions also lacks of 3 columns storing he fraction, one for each tank.
 */
static int coln(enum field_pos pos)
{
	int amnd = (smtk_version < 10213) ? 3 : 0;

	if (pos <= O2FRAC)
		return pos;
	if (pos >= AIRTEMP) {
		pos += 4 * (tanks - 1) - amnd;
		return pos;
	}
	switch (pos) {
	case HEFRAC:
		pos = O2FRAC + tanks;
		return pos;
	case PSTART:
		pos = O2FRAC + 2 * tanks - amnd;
		return pos;
	case PEND:
		pos = O2FRAC + 2 * tanks + 1 - amnd;
		return pos;
	}
}

/*
 * Fills the date part of a tm structure with the string data obtained
 * from smartrak in format "DD/MM/YY HH:MM:SS" where time is irrelevant.
 */
static void smtk_date_to_tm(char *d_buffer, struct tm *tm_date)
{
	int n, d, m, y;

	if ((d_buffer) && (!empty_string(d_buffer))) {
		n = sscanf(d_buffer, "%d/%d/%d ", &m, &d, &y);
		y = (y < 70) ? y + 100 : y;
		if (n == 3) {
			tm_date->tm_mday = d;
			tm_date->tm_mon = m - 1;
			tm_date->tm_year = y;
		} else {
			tm_date->tm_mday = tm_date->tm_mon = tm_date->tm_year = 0;
		}
	} else {
		tm_date->tm_mday = tm_date->tm_mon = tm_date->tm_year = 0;
	}
}

/*
 * Fills the time part of a tm structure with the string data obtained
 * from smartrak in format "DD/MM/YY HH:MM:SS" where date is irrelevant.
 */
static void smtk_time_to_tm(char *t_buffer, struct tm *tm_date)
{
	int n, hr, min, sec;

	if ((t_buffer) && (!empty_string(t_buffer))) {
		n = sscanf(t_buffer, "%*[0-9/] %d:%d:%d ", &hr, &min, &sec);
		if (n == 3) {
			tm_date->tm_hour = hr;
			tm_date->tm_min = min;
			tm_date->tm_sec = sec;
		} else {
			tm_date->tm_hour = tm_date->tm_min = tm_date->tm_sec = 0;
		}
	} else {
		tm_date->tm_hour = tm_date->tm_min = tm_date->tm_sec = 0;
	}
	tm_date->tm_isdst = -1;
}

/*
 * Converts to seconds a time lapse returned from smartrak in string format
 * "DD/MM/YY HH:MM:SS" where date is irrelevant.
 * TODO: modify to support times > 24h where date means difference in days
 * from 29/12/99.
 */
static unsigned int smtk_time_to_secs(char *t_buffer)
{
	int n, hr, min, sec;

	if (!empty_string(t_buffer)) {
		n = sscanf(t_buffer, "%*[0-9/] %d:%d:%d ", &hr, &min, &sec);
		return (n == 3) ? (((hr*60)+min)*60)+sec : 0;
	} else {
		return 0;
	}
}

/*
 * Emulate the non portable timegm() function.
 * Based on timegm man page, changing setenv and unsetenv with putenv,
 * because of portability issues.
 */
static time_t smtk_timegm(struct tm *tm)
{
	time_t ret;
	char *tz, *str;

	tz = getenv("TZ");
	putenv("TZ=");
	tzset();
	ret = mktime(tm);
	if (tz) {
		str = calloc(strlen(tz)+4, 1);
		sprintf(str, "TZ=%s", tz);
		putenv(str);
	} else {
		putenv("TZ");
	}
	tzset();
	return ret;
}

/*
 * Returns an opened table given its name and mdb. outcol and outbounder have to be allocated
 * by the caller and are filled here.
 */
static  MdbTableDef *smtk_open_table(MdbHandle *mdb, char *tablename, char **outbounder, int *outlens[])
{
	MdbCatalogEntry *entry;
	MdbTableDef *table;
	int i;

	entry = mdb_get_catalogentry_by_name(mdb, tablename);
	if (!entry)
		return NULL;
	table = mdb_read_table(entry);
	if (!table)
		return NULL;
	mdb_read_columns(table);
	for (i = 0;  i < table->num_cols; i++) {
		outbounder[i] = (char *) g_malloc(MDB_BIND_SIZE);
		if (outlens) {
			outlens[i] = (int *) g_malloc(sizeof(int));
			mdb_bind_column(table, i+1, outbounder[i], outlens[i]);
		} else {
			mdb_bind_column(table, i+1, outbounder[i], NULL);
		}
	}
	mdb_rewind_table(table);

	return table;
}

/*
 * Utility function which joins three strings, being the second a separator string,
 * usually a "\n". The third is a format string with an argument list.
 * If the original string is NULL, then just returns the third.
 * This is based in add_to_string() and add_to_string_va(), and, as its parents
 * frees the original string.
 */
static char *smtk_concat_str(char *orig, const char *sep, const char *fmt, ...)
{
	char *str;
	va_list args;
	struct membuffer out = { 0 }, in = { 0 };

	va_start(args, fmt);
	put_vformat(&in, fmt, args);
	if (orig != NULL) {
		put_format(&out, "%s%s%s", orig, sep, mb_cstring(&in));
		str = copy_string(mb_cstring(&out));
	} else {
		str = copy_string(mb_cstring(&in));
	}
	va_end(args);

	free_buffer(&out);
	free_buffer(&in);
	free(orig);

	return str;
}

/*
 * A site may be a wreck, which has its own table.
 * Parse this table referred by the site idx. If found, put the different info items in
 * Subsurface's dive_site notes.
 * Wreck format:
 * | Idx | SiteIdx | Text | Built | Sank | SankTime | Reason | ... | Notes | TrakId |
 */
static void smtk_wreck_site(MdbHandle *mdb, char *site_idx, struct dive_site *ds)
{
	MdbTableDef *table;
	char *bound_values[MDB_MAX_COLS];
	char *tmp = NULL, *notes = NULL;
	int rc, i;
	uint32_t d;
	const char *wreck_fields[] = {QT_TRANSLATE_NOOP("gettextFromC", "Built"), QT_TRANSLATE_NOOP("gettextFromC", "Sank"), QT_TRANSLATE_NOOP("gettextFromC", "Sank Time"),
				      QT_TRANSLATE_NOOP("gettextFromC", "Reason"), QT_TRANSLATE_NOOP("gettextFromC", "Nationality"), QT_TRANSLATE_NOOP("gettextFromC", "Shipyard"),
				      QT_TRANSLATE_NOOP("gettextFromC", "ShipType"), QT_TRANSLATE_NOOP("gettextFromC", "Length"), QT_TRANSLATE_NOOP("gettextFromC", "Beam"),
				      QT_TRANSLATE_NOOP("gettextFromC", "Draught"), QT_TRANSLATE_NOOP("gettextFromC", "Displacement"), QT_TRANSLATE_NOOP("gettextFromC", "Cargo"),
				      QT_TRANSLATE_NOOP("gettextFromC", "Notes")};

	table = smtk_open_table(mdb, "Wreck", bound_values, NULL);

	/* Sanity check for table, unlikely but ... */
	if (!table)
		return;

	/* Begin parsing. Write strings to notes only if available.*/
	while (mdb_fetch_row(table)) {
		if (!strcmp(bound_values[1], site_idx)) {
			notes = smtk_concat_str(notes, "\n", translate("gettextFromC", "Wreck Data"));
			for (i = 3; i < 16; i++) {
				switch (i) {
				case 3:
				case 4:
					tmp = copy_string(bound_values[i]);
					if (tmp)
						notes = smtk_concat_str(notes, "\n", "%s: %s", wreck_fields[i - 3], strtok(tmp , " "));
					free(tmp);
					break;
				case 5:
					tmp = copy_string(bound_values[i]);
					if (tmp)
						notes = smtk_concat_str(notes, "\n", "%s: %s", wreck_fields[i - 3], strrchr(tmp, ' '));
					free(tmp);
					break;
				case 6 ... 9:
				case 14:
				case 15:
					tmp = copy_string(bound_values[i]);
					if (tmp)
						notes = smtk_concat_str(notes, "\n", "%s: %s", wreck_fields[i - 3], tmp);
					free(tmp);
					break;
				default:
					d = lrintl(strtold(bound_values[i], NULL));
					if (d)
						notes = smtk_concat_str(notes, "\n", "%s: %d", wreck_fields[i - 3], d);
					break;
				}
			}
			ds->notes = smtk_concat_str(ds->notes, "\n", "%s", notes);
			break;
		}
	}
	/* Clean up and exit */
	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);
	free(notes);
}

/*
 * Smartrak locations db is quite extensive. This builds a string joining some of
 * the data in the style:   "Country, State, Locality, Site"  if this data are
 * available. Uses two tables, Site and Location. Puts Altitude, Depth and Notes
 * in Subsurface's dive site structure's notes if they are available.
 * Site format:
 * | Idx | Text | LocationIdx | WaterIdx | PlatformIdx | BottomIdx | Latitude | Longitude | Altitude | Depth | Notes | TrakId |
 * Location format:
 * | Idx | Text | Province | Country | Depth |
 */
static void smtk_build_location(MdbHandle *mdb, char *idx, struct dive_site **location)
{
	MdbTableDef *table;
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS];
	int i, rc;
	uint32_t d;
	struct dive_site *ds;
	location_t loc;
	char *str = NULL, *loc_idx = NULL, *site = NULL, *notes = NULL;
	const char *site_fields[] = {QT_TRANSLATE_NOOP("gettextFromC", "Altitude"), QT_TRANSLATE_NOOP("gettextFromC", "Depth"),
				     QT_TRANSLATE_NOOP("gettextFromC", "Notes")};

	/* Read data from Site table. Format notes for the dive site if any.*/
	table = smtk_open_table(mdb, "Site", bound_values, NULL);
	if (!table)
		return;
	do {
		rc = mdb_fetch_row(table);
	} while (strcasecmp(bound_values[0], idx) && rc != 0);
	if (rc == 0) {
		smtk_free(bound_values, table->num_cols);
		mdb_free_tabledef(table);
		return;
	}
	loc_idx = copy_string(bound_values[2]);
	site = copy_string(bound_values[1]);
	loc = create_location(strtod(bound_values[6], NULL), strtod(bound_values[7], NULL));

	for (i = 8; i < 11; i++) {
		switch (i) {
		case 8:
		case 9:
			d = lrintl(strtold(bound_values[i], NULL));
			if (d)
				notes = smtk_concat_str(notes, "\n", "%s: %d m", site_fields[i - 8], d);
			break;
		case 10:
			if (memcmp(bound_values[i], "\0", 1))
				notes = smtk_concat_str(notes, "\n", "%s: %s", site_fields[i - 8], bound_values[i]);
			break;
		}
	}
	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);

	/* Read data from Location table, linked to Site by loc_idx */
	table = smtk_open_table(mdb, "Location", bound_values, NULL);
	mdb_rewind_table(table);
	do {
		rc = mdb_fetch_row(table);
	} while (strcasecmp(bound_values[0], loc_idx) && rc != 0);
	if (rc == 0){
		smtk_free(bound_values, table->num_cols);
		mdb_free_tabledef(table);
		if(notes)
			free(notes);
		return;
	}

	/*
	 * Create a string for Subsurface's dive site structure with coordinates
	 * if available, if the site's name doesn't previously exists.
	 */
	if (memcmp(bound_values[3], "\0", 1))
		str = smtk_concat_str(str, ", ", "%s", bound_values[3]); // Country
	if (memcmp(bound_values[2], "\0", 1))
		str = smtk_concat_str(str, ", ", "%s", bound_values[2]); // State - Province
	if (memcmp(bound_values[1], "\0", 1))
		str = smtk_concat_str(str, ", ", "%s", bound_values[1]); // Locality
	str =  smtk_concat_str(str, ", ", "%s", site);

	ds = get_dive_site_by_name(str, &dive_site_table);
	if (!ds) {
		if (!has_location(&loc))
			ds = create_dive_site(str, &dive_site_table);
		else
			ds = create_dive_site_with_gps(str, &loc, &dive_site_table);
	}
	*location = ds;
	smtk_free(bound_values, table->num_cols);

	/* Insert site notes */
	ds->notes = copy_string(notes);
	free(notes);

	/* Check if we have a wreck */
	smtk_wreck_site(mdb, idx, ds);

	/* Clean up and exit */
	mdb_free_tabledef(table);
	free(loc_idx);
	free(site);
	free(str);
}

static void smtk_build_tank_info(MdbHandle *mdb, cylinder_t *tank, char *idx)
{
	MdbTableDef *table;
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS];
	int i;

	table = smtk_open_table(mdb, "Tank", bound_values, NULL);
	if (!table)
		return;

	for (i = 1; i <= atoi(idx); i++)
		mdb_fetch_row(table);
	tank->type.description = copy_string(bound_values[1]);
	tank->type.size.mliter = lrint(strtod(bound_values[2], NULL) * 1000);
	tank->type.workingpressure.mbar = lrint(strtod(bound_values[4], NULL) * 1000);

	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);
}

/*
 * Under some circustances we can get the same tank from DC and from
 * the smartrak DB. Will use this utility to check and clean .
 */
bool is_same_cylinder(cylinder_t *cyl_a, cylinder_t *cyl_b)
{
	// different gasmixes (non zero)
	if (cyl_a->gasmix.o2.permille - cyl_b->gasmix.o2.permille != 0 &&
	    cyl_a->gasmix.o2.permille != 0 &&
	    cyl_b->gasmix.o2.permille != 0)
		return false;
	// different start pressures (possible error 0.1 bar)
	if (!(abs(cyl_a->start.mbar - cyl_b->start.mbar) <= 100))
		return false;
	// different end pressures (possible error 0.1 bar)
	if (!(abs(cyl_a->end.mbar - cyl_b->end.mbar) <= 100))
		return false;
	// different names (none of them null)
	if (!same_string(cyl_a->type.description, "---") &&
	    !same_string(cyl_b->type.description, "---") &&
	    !same_string(cyl_a->type.description, cyl_b->type.description))
		return false;
	// Cylinders are most probably the same
	return true;
}

/*
 * Next three functions were removed from dive.c just when I was going to use them
 * for this import (see 16276faa). Will tweak them a bit and will use for our needs
 * Macros are copied from dive.c
 */

#define MERGE_MAX(res, a, b, n) res->n = MAX(a->n, b->n)
#define MERGE_MIN(res, a, b, n) res->n = (a->n) ? (b->n) ? MIN(a->n, b->n) : (a->n) : (b->n)

static void merge_cylinder_type(cylinder_type_t *src, cylinder_type_t *dst)
{
	if (!dst->size.mliter)
		dst->size.mliter = src->size.mliter;
	if (!dst->workingpressure.mbar)
		dst->workingpressure.mbar = src->workingpressure.mbar;
	if (!dst->description || same_string(dst->description, "---")) {
		dst->description = src->description;
		src->description = NULL;
	}
}

static void merge_cylinder_mix(struct gasmix src, struct gasmix *dst)
{
	if (!dst->o2.permille)
		*dst = src;
}

static void merge_cylinder_info(cylinder_t *src, cylinder_t *dst)
{
	merge_cylinder_type(&src->type, &dst->type);
	merge_cylinder_mix(src->gasmix, &dst->gasmix);
	MERGE_MAX(dst, dst, src, start.mbar);
	MERGE_MIN(dst, dst, src, end.mbar);
	if (!dst->cylinder_use)
		dst->cylinder_use = src->cylinder_use;
}

/*
 * Remove unused tanks and merge cylinders if there are signs that
 * they might be duplicated. Higher numbers are more prone to be unused,
 * so will make the clean reverse order.
 * When a used cylinder is found, check against previous one; if they are
 * both the same, merge and delete the higher number (as lower numbers are
 * most probably returned by libdivecomputer raw data parse.
 */
static int smtk_clean_cylinders(struct dive *d)
{
	int i = tanks - 1;
	cylinder_t  *cyl, *base = get_cylinder(d, 0);

	cyl = base + tanks - 1;
	while (cyl != base) {
		if (same_string(cyl->type.description, "---") && cyl->start.mbar == 0 && cyl->end.mbar == 0)
			remove_cylinder(d, i);
		else
			if (is_same_cylinder(cyl, cyl - 1)) {
				merge_cylinder_info(cyl, cyl - 1);
				remove_cylinder(d, i);
			}
		cyl--;
		i--;
	}
}

/*
 * List related functions
 */
struct types_list {
	int idx;
	char *text;
	struct types_list *next;
};

/* Head insert types_list items in a list */
static void smtk_head_insert(struct types_list **head, int index, char *txt)
{
	struct types_list *item = (struct types_list *) malloc(sizeof(struct types_list));

	item->next = *head;
	item->idx = index;
	item->text = txt;
	*head = item;
	item = NULL;
}

/* Clean types_list lists */
static void smtk_list_free(struct types_list *head)
{
	struct types_list *p = head;
	while (p) {
		struct types_list *nxt = p->next;
		free(p->text);
		free(p);
		p = nxt;
	}
}

/* Return the number of rows in a given table */
static int get_rows_num(MdbHandle *mdb,  char *table_name)
{
	MdbTableDef *table;
	char *bound_values[MDB_MAX_COLS];
	int n = 0, i = 0;

	table = smtk_open_table(mdb, table_name, bound_values, NULL);
	if (!table)
		return n;

	/* We can get an sparse array (less rows in the table than
	 * index). Ensure we allocate as many strings as greater
	 * index, at least */
	while (mdb_fetch_row(table)) {
		i = atoi(bound_values[0]);
		if (i > n)
			n = i;
	}
	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);
	return n;
}

/*
 * Build a list from a given table_name (Type, Gear, etc)
 * Managed tables formats: Just consider Idx and Text
 * Type:
 * | Idx | Text | Default (bool)
 * Activity:
 * | Idx | Text | Default (bool)
 * Gear:
 * | Idx | Text | Vendor | Type | Typenum | Notes | Default (bool) | TrakId
 * Fish:
 * | Idx | Text | Name | Latin name | Typelength | Maxlength | Picture | Default (bool)| TrakId
 * TODO: Although all divelogs I've seen use *only* the Text field, a concerned diver could
 * be using some other like Vendor (in Gear) or Latin name (in Fish). I'll take a look at this
 * in the future, may be something like Buddy table...
 */
static void smtk_build_list(MdbHandle *mdb,  char *table_name, char *array[])
{
	MdbTableDef *table;
	char *bound_values[MDB_MAX_COLS], *str;

	table = smtk_open_table(mdb, table_name, bound_values, NULL);
	if (!table)
		return;

	/* Read the table items into the array */
	while (mdb_fetch_row(table)) {
		str = bound_values[1];
		if (str && (!strcmp(str, "---") || !strcmp(str, "--")))
	               str = NULL;
		array[atoi(bound_values[0]) - 1] = copy_string(str);
	}

	/* clean up and exit */
	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);
}

/*
 * Parses a relation table and returns a list with the relations for a dive idx.
 * Use types_list items with text set to NULL.
 * Returns a pointer to the list head.
 * Table relation format:
 * | Diveidx | Idx |
 */
static struct types_list *smtk_index_list(MdbHandle *mdb, char *table_name, char *dive_idx)
{
	MdbTableDef *table;
	char *bounders[MDB_MAX_COLS];
	struct types_list *item, *head = NULL;

	table = smtk_open_table(mdb, table_name, bounders, NULL);

	/* Sanity check */
	if (!table)
		return NULL;

	/* Parse the table searching for dive_idx */
	while (mdb_fetch_row(table)) {
		if (!strcmp(dive_idx, bounders[0]))
			smtk_head_insert(&head, atoi(bounders[1]), NULL);
	}

	/* Clean up and exit */
	smtk_free(bounders, table->num_cols);
	mdb_free_tabledef(table);
	return head;
}

/*
 * "Buddy" is a bit special table that needs some extra work, so we can't just use smtk_build_list.
 * "Buddy" table is a buddies relation with lots and lots and lots of data (even buddy mother's
 * maiden name ;-) ) most of them useless for a dive log. Let's just consider the nickname as main
 * field and the full name if this exists and its construction is different from the nickname.
 * Buddy table format:
 * | Idx | Text (nickname) | Name | Firstname | Middlename | Title | Picture | Phone | ...
 */
static void smtk_build_buddies(MdbHandle *mdb, char *array[])
{
	MdbTableDef *table;
	char *bound_values[MDB_MAX_COLS], *fullname = NULL, *str = NULL;

	table = smtk_open_table(mdb, "Buddy", bound_values, NULL);
	if (!table)
		return;

	while (mdb_fetch_row(table)) {
		if (!empty_string(bound_values[3]))
			fullname = smtk_concat_str(fullname, " ", "%s", bound_values[3]);
		if (!empty_string(bound_values[4]))
			fullname = smtk_concat_str(fullname, " ", "%s", bound_values[4]);
		if (!empty_string(bound_values[2]))
			fullname = smtk_concat_str(fullname, " ", "%s", bound_values[2]);
		if (fullname && !same_string(bound_values[1], fullname))
			array[atoi(bound_values[0]) - 1] = smtk_concat_str(str, "", "%s (%s)", bound_values[1], fullname);
		else
			array[atoi(bound_values[0]) - 1] = smtk_concat_str(str, "", "%s", bound_values[1]);
		free(fullname);
		fullname = NULL;
	}

	free(str);
	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);
}

/*
 * Returns string with buddies names as registered in smartrak (may be a nickname).
 */
static char *smtk_locate_buddy(MdbHandle *mdb, char *dive_idx, char *buddies_list[])
{
	char *str = NULL;
	struct types_list *rel, *rel_head;

	rel_head = smtk_index_list(mdb, "BuddyRelation", dive_idx);
	if (!rel_head)
		return str;

	for (rel = rel_head; rel; rel = rel->next)
		str = smtk_concat_str(str, ", ", "%s", buddies_list[rel->idx - 1]);

	/* Clean up and exit */
	smtk_list_free(rel_head);
	return str;
}

/* Parses the dive_type mdb tables and import the data into dive's
 * taglist structure or notes.  If there are tags that affects dive's dive_mode
 * (SCR, CCR or so), set the dive mode too.
 * The "tag" parameter is used to mark if we want this table to be imported
 * into tags or into notes.
 */
static void smtk_parse_relations(MdbHandle *mdb, struct dive *dive, char *dive_idx, char *table_name, char *rel_table_name, char *list[], bool tag)
{
	char *tmp = NULL;
	struct types_list *diverel_head, *d_runner;

	diverel_head = smtk_index_list(mdb, rel_table_name, dive_idx);
	if (!diverel_head)
		return;

	/* Get the text associated with the relations */
	for (d_runner = diverel_head; d_runner; d_runner = d_runner->next) {
		if (!list[d_runner->idx - 1])
			continue;
		if (tag)
			taglist_add_tag(&dive->tag_list, list[d_runner->idx - 1]);
		else
			tmp = smtk_concat_str(tmp, ", ", "%s", list[d_runner->idx - 1]);
		if (strstr(list[d_runner->idx - 1], "SCR"))
			dive->dc.divemode = PSCR;
		else if (strstr(list[d_runner->idx -1], "CCR"))
			dive->dc.divemode = CCR;
	}
	if (tmp)
		dive->notes = smtk_concat_str(dive->notes, "\n", "Smartrak %s: %s", table_name, tmp);
	free(tmp);
	smtk_list_free(diverel_head);
}

/*
 * Add data from tables related in Dives table which are not directly supported
 * in Subsurface. Write them as tags or dive notes by setting true or false the
 * boolean parameter "tag".
 */
static void smtk_parse_other(struct dive *dive, char *list[], char *data_name, char *idx, bool tag)
{
       int i = atoi(idx) - 1;
       char *str = NULL;

       str = list[i];
       if (str) {
               if (tag)
                       taglist_add_tag(&dive->tag_list, str);
               else
                       dive->notes = smtk_concat_str(dive->notes, "\n", "Smartrak %s: %s", data_name, str);
       }
}

/*
 * Returns a pointer to a bookmark event in an event list if it exists for
 * a given time. Return NULL otherwise.
 */
static struct event *find_bookmark(struct event *orig, unsigned int t)
{
	struct event *ev = orig;

	while (ev) {
		if ((ev->time.seconds == t) && (ev->type == SAMPLE_EVENT_BOOKMARK))
			return ev;
		ev = ev->next;
	}
	return NULL;
}

/*
 * Marker table is a mix between Type tables and Relations tables. Its format is
 * | Dive Idx | Idx | Text | Type | XPos | YPos | XConnect | YConnect
 * Type may be one of 1 = Text; 2 = DC; 3 = Tissue Data; 4 = Photo (0 most of time??)
 * XPos is time in minutes during the dive (long int)
 * YPos irelevant
 * XConnect irelevant
 * YConnect irelevant
 */
static void smtk_parse_bookmarks(MdbHandle *mdb, struct dive *d, char *dive_idx)
{
	MdbTableDef *table;
	char *bound_values[MDB_MAX_COLS], *tmp = NULL;
	unsigned int time;
	struct event *ev;

	table = smtk_open_table(mdb, "Marker", bound_values, NULL);
	if (!table) {
		report_error("[smtk-import] Error - Couldn't open table 'Marker', dive %d", d->number);
		return;
	}
	while (mdb_fetch_row(table)) {
		if (same_string(bound_values[0], dive_idx)) {
			time = lrint(strtod(bound_values[4], NULL) * 60);
			tmp = strdup(bound_values[2]);
			ev = find_bookmark(d->dc.events, time);
			if (ev)
				update_event_name(d, ev, tmp);
			else
				if (!add_event(&d->dc, time, SAMPLE_EVENT_BOOKMARK, 0, 0, tmp))
					report_error("[smtk-import] Error - Couldn't add bookmark, dive %d, Name = %s",
						     d->number, tmp);
		}
	}
	free(tmp);
	smtk_free(bound_values, table->num_cols);
	mdb_free_tabledef(table);
}



/*
 * Returns a dc_descriptor_t structure based on dc  model's number.
 * This ensures the model pased to libdc_buffer_parser() is a supported model and avoids
 * problems with shared model num devices by taking the family into account.  The family
 * is estimated elsewhere based in dive header length.
 */
static dc_descriptor_t *get_data_descriptor(int data_model, dc_family_t data_fam)
{
	dc_descriptor_t *descriptor = NULL, *current = NULL;
	dc_iterator_t *iterator = NULL;
	dc_status_t rc;

	rc = dc_descriptor_iterator(&iterator);
	if (rc != DC_STATUS_SUCCESS) {
		report_error("[Error][libdc]\t\t\tCreating the device descriptor iterator.\n");
		return current;
	}
	while ((dc_iterator_next(iterator, &descriptor)) == DC_STATUS_SUCCESS) {
		int desc_model = dc_descriptor_get_model(descriptor);
		dc_family_t desc_fam = dc_descriptor_get_type(descriptor);

		if (data_fam == DC_FAMILY_UWATEC_ALADIN) {
			if (data_model == desc_model && data_fam == desc_fam) {
				current = descriptor;
				break;
			}
		} else if (data_model == desc_model && (desc_fam == DC_FAMILY_UWATEC_SMART ||
							desc_fam == DC_FAMILY_UWATEC_MERIDIAN)) {
			current = descriptor;
			break;
		}
		dc_descriptor_free(descriptor);
	}
	dc_iterator_free(iterator);
	return current;
}

/*
 * Fills a device_data_t structure with known dc data.
 * Completes a dc_descriptor_t structure with libdc's g_descriptors[] table so
 * we ensure that data passed for parsing to libdc have a supported and known
 * DC.  dc_family_t is certainly known *only* if it is Aladin/Memomouse family
 * otherwise it will be known after get_data_descriptor call.
 */
static int prepare_data(int data_model, char *serial, dc_family_t dc_fam, device_data_t *dev_data)
{
	dev_data->device = NULL;
	dev_data->context = NULL;
	if (!data_model){
		dev_data->model = copy_string("manually added dive");
		dev_data->descriptor = NULL;
		dev_data->deviceid = 0;
		return DC_STATUS_NODEVICE;
	}
	dev_data->descriptor = get_data_descriptor(data_model, dc_fam);
	if (dev_data->descriptor) {
		dev_data->vendor = dc_descriptor_get_vendor(dev_data->descriptor);
		dev_data->product = dc_descriptor_get_product(dev_data->descriptor);
		dev_data->model = smtk_concat_str(dev_data->model, "", "%s %s", dev_data->vendor, dev_data->product);
		dev_data->deviceid = (uint32_t) lrint(strtod(serial, NULL));
		return DC_STATUS_SUCCESS;
	} else {
		dev_data->model = copy_string("unsupported dive computer");
		dev_data->deviceid = (uint32_t) lrint(strtod(serial, NULL));
		return DC_STATUS_UNSUPPORTED;
	}
}

static void device_data_free(device_data_t *dev_data)
{
	free((void *) dev_data->model);
	dc_descriptor_free(dev_data->descriptor);
	free(dev_data);
}

/*
 * Returns a buffer prepared for libdc parsing.
 * Aladin and memomouse dives were imported from datatrak, so they lack of a
 * header. Creates a fake header for them and inserts the dc model where libdc
 * expects it to be.
 */
static dc_status_t libdc_buffer_complete(device_data_t *dev_data, unsigned char *hdr_buffer, int hdr_length,
				     unsigned char *prf_buffer, int prf_length, unsigned char *compl_buf)
{
	switch (dc_descriptor_get_type(dev_data->descriptor)) {
	case DC_FAMILY_UWATEC_ALADIN:
	case DC_FAMILY_UWATEC_MEMOMOUSE:
		compl_buf[3] = (unsigned char) dc_descriptor_get_model(dev_data->descriptor);
		memcpy(compl_buf+hdr_length, prf_buffer, prf_length);
		break;
	case DC_FAMILY_UWATEC_SMART:
	case DC_FAMILY_UWATEC_MERIDIAN:
		memcpy(compl_buf, hdr_buffer, hdr_length);
		memcpy(compl_buf+hdr_length, prf_buffer, prf_length);
		break;
	default:
		return -2;
	}
	return 0;
}

/*
 * Main function.
 * Two looping levels using a database for the first level ("Dives" table)
 * and a clone of the first DB for the second level (each dive items). Using
 * a DB clone is necessary as calling mdb_fetch_row() over different tables in
 * a single DB breaks binded row data, and so would break the top loop.
 */

void smartrak_import(const char *file, struct dive_table *divetable)
{
	MdbHandle *mdb, *mdb_clon;
	MdbTableDef *mdb_table;
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS];
	int i, dc_model, *bound_lens[MDB_MAX_COLS];
	struct device_table *devices = alloc_device_table();

	// Set an european style locale to work date/time conversion
	setlocale(LC_TIME, "POSIX");
	mdb = mdb_open(file, MDB_NOFLAGS);
	if (!mdb) {
		report_error("[Error][smartrak_import]\tFile %s does not seem to be an Access database.", file);
		return;
	}
	if (!mdb_read_catalog(mdb, MDB_TABLE)) {
		report_error("[Error][smartrak_import]\tFile %s does not seem to be an Access database.", file);
		return;
	}
	mdb_clon = mdb_clone_handle(mdb);
	mdb_read_catalog(mdb_clon, MDB_TABLE);

	/* Define arrays for the tables */
	int	type_num = get_rows_num(mdb_clon, "Type"),
		activity_num = get_rows_num(mdb_clon, "Activity"),
		gear_num = get_rows_num(mdb_clon, "Gear"),
		fish_num = get_rows_num(mdb_clon, "Fish"),
		suit_num = get_rows_num(mdb_clon, "Suit"),
		weather_num = get_rows_num(mdb_clon, "Weather"),
		underwater_num = get_rows_num(mdb_clon, "Underwater"),
		surface_num = get_rows_num(mdb_clon, "Surface"),
		buddy_num = get_rows_num(mdb_clon, "Buddy");

	char	*type_list[type_num], *activity_list[activity_num], *gear_list[gear_num],
		*fish_list[fish_num], *buddy_list[buddy_num], *suit_list[suit_num],
		*weather_list[weather_num], *underwater_list[underwater_num],
		*surface_list[surface_num], *smtk_ver[1];

	/* Load auxiliary tables */
	smtk_build_list(mdb_clon, "Type", type_list);
	smtk_build_list(mdb_clon, "Activity", activity_list);
	smtk_build_list(mdb_clon, "Gear", gear_list);
	smtk_build_list(mdb_clon, "Fish", fish_list);
	smtk_build_list(mdb_clon, "SmartTrak", smtk_ver);
	smtk_build_list(mdb_clon, "Suit", suit_list);
	smtk_build_list(mdb_clon, "Weather", weather_list);
	smtk_build_list(mdb_clon, "Underwater", underwater_list);
	smtk_build_list(mdb_clon, "Surface", surface_list);
	smtk_build_buddies(mdb_clon, buddy_list);

	/* Check Smarttrak version (different number of supported tanks, mixes and so) */
	smtk_version = atoi(smtk_ver[0]);
	tanks = (smtk_version < 10213) ? 3 : 10;

	mdb_table = smtk_open_table(mdb, "Dives", bound_values, bound_lens);
	if (!mdb_table) {
		report_error("[Error][smartrak_import]\tFile %s does not seem to be an SmartTrak file.", file);
		return;
	}
	while (mdb_fetch_row(mdb_table)) {
		device_data_t *devdata = calloc(1, sizeof(device_data_t));
		dc_family_t dc_fam = DC_FAMILY_NULL;
		unsigned char *prf_buffer, *hdr_buffer, *compl_buffer;
		struct dive *smtkdive = alloc_dive();
		struct tm *tm_date = malloc(sizeof(struct tm));
		size_t hdr_length, prf_length;
		dc_status_t rc = 0;

		for (int j = 0; j < mdb_table->num_cols; j++)
			col[j] = g_ptr_array_index(mdb_table->columns, j);
		smtkdive->number = lrint(strtod(bound_values[1], NULL));
		/*
		 * If there is a DC model (no zero) try to create a buffer for the
		 * dive and parse it with libdivecomputer
		 */
		dc_model = lrint(strtod(bound_values[coln(DCMODEL)], NULL)) & 0xFF;
		if (*bound_lens[coln(LOG)]) {
			hdr_buffer = mdb_ole_read_full(mdb, col[coln(LOG)], &hdr_length);
			if (hdr_length > 0 && hdr_length < 20)	// We have a profile but it's imported from datatrak
				dc_fam = DC_FAMILY_UWATEC_ALADIN;
		}
		rc = prepare_data(dc_model, copy_string(col[coln(DCNUMBER)]->bind_ptr), dc_fam, devdata);
		smtkdive->dc.deviceid = devdata->deviceid;
		smtkdive->dc.model = copy_string(devdata->model);
		if (rc == DC_STATUS_SUCCESS && *bound_lens[coln(PROFILE)]) {
			prf_buffer = mdb_ole_read_full(mdb, col[coln(PROFILE)], &prf_length);
			if (prf_length > 0) {
				if (dc_descriptor_get_type(devdata->descriptor) == DC_FAMILY_UWATEC_ALADIN || dc_descriptor_get_type(devdata->descriptor) == DC_FAMILY_UWATEC_MEMOMOUSE)
					hdr_length = 18;
				compl_buffer = calloc(hdr_length+prf_length, sizeof(char));
				rc = libdc_buffer_complete(devdata, hdr_buffer, hdr_length, prf_buffer, prf_length, compl_buffer);
				if (rc != DC_STATUS_SUCCESS) {
					report_error("[Error][smartrak_import]\t- %s - for dive %d", errmsg(rc), smtkdive->number);
				} else {
					rc = libdc_buffer_parser(smtkdive, devdata, compl_buffer, hdr_length + prf_length);
					if (rc != DC_STATUS_SUCCESS)
						report_error("[Error][libdc]\t\t- %s - for dive %d", errmsg(rc), smtkdive->number);
				}
				free(compl_buffer);
			} else {
				/* Dives without profile samples (usual in older aladin series) */
				report_error("[Warning][smartrak_import]\t No profile for dive %d", smtkdive->number);
				smtkdive->dc.duration.seconds = smtkdive->duration.seconds = smtk_time_to_secs(col[coln(DURATION)]->bind_ptr);
				smtkdive->dc.maxdepth.mm = smtkdive->maxdepth.mm = lrint(strtod(col[coln(MAXDEPTH)]->bind_ptr, NULL) * 1000);
			}
			free(hdr_buffer);
			free(prf_buffer);
		} else {
			/* Manual dives or unknown DCs */
			report_error("[Warning][smartrak_import]\t Manual or unknown dive computer for dive %d", smtkdive->number);
			smtkdive->dc.duration.seconds = smtkdive->duration.seconds = smtk_time_to_secs(col[coln(DURATION)]->bind_ptr);
			smtkdive->dc.maxdepth.mm = smtkdive->maxdepth.mm = lrint(strtod(col[coln(MAXDEPTH)]->bind_ptr, NULL) * 1000);
		}
		/*
		 * Cylinder and gasmixes completion.
		 * Revisit data under some circunstances, e.g. a start pressure = 0 may mean
		 * that dc doesn't support gas control, in this situation let's look into mdb data
		 */
		int pstartcol = coln(PSTART);
		int o2fraccol = coln(O2FRAC);
		int hefraccol = coln(HEFRAC);
		int tankidxcol = coln(TANKIDX);

		for (i = 0; i < tanks; i++) {
			cylinder_t *tmptank = get_or_create_cylinder(smtkdive, i);
			if (!tmptank)
				break;
			if (tmptank->start.mbar == 0)
				tmptank->start.mbar = lrint(strtod(col[(i * 2) + pstartcol]->bind_ptr, NULL) * 1000);
			/*
			 * If there is a start pressure ensure that end pressure is not zero as
			 * will be registered in DCs which only keep track of differential pressures,
			 * and collect the data registered  by the user in mdb
			 */
			if (tmptank->end.mbar == 0 && tmptank->start.mbar != 0)
				tmptank->end.mbar = lrint(strtod(col[(i * 2) + 1 + pstartcol]->bind_ptr, NULL) * 1000 ? : 1000);
			if (tmptank->gasmix.o2.permille == 0)
				tmptank->gasmix.o2.permille = lrint(strtod(col[i + o2fraccol]->bind_ptr, NULL) * 10);
			if (smtk_version == 10213) {
				if (tmptank->gasmix.he.permille == 0)
					tmptank->gasmix.he.permille = lrint(strtod(col[i + hefraccol]->bind_ptr, NULL) * 10);
			} else {
				tmptank->gasmix.he.permille = 0;
			}
			smtk_build_tank_info(mdb_clon, tmptank, col[i + tankidxcol]->bind_ptr);
		}
		/* Check for duplicated cylinders and clean them */
		smtk_clean_cylinders(smtkdive);

		/* Date issues with libdc parser - Take date time from mdb */
		smtk_date_to_tm(col[coln(_DATE)]->bind_ptr, tm_date);
		smtk_time_to_tm(col[coln(INTIME)]->bind_ptr, tm_date);
		smtkdive->dc.when = smtkdive->when = smtk_timegm(tm_date);
		free(tm_date);
		smtkdive->dc.surfacetime.seconds = smtk_time_to_secs(col[coln(INTVAL)]->bind_ptr);

		/* Data that user may have registered manually if not supported by DC, or not parsed */
		if (!smtkdive->airtemp.mkelvin)
			smtkdive->airtemp.mkelvin = C_to_mkelvin(lrint(strtod(col[coln(AIRTEMP)]->bind_ptr, NULL)));
		if (!smtkdive->watertemp.mkelvin)
			smtkdive->watertemp.mkelvin = smtkdive->mintemp.mkelvin = C_to_mkelvin(lrint(strtod(col[coln(MINWATERTEMP)]->bind_ptr, NULL)));
		if (!smtkdive->maxtemp.mkelvin)
			smtkdive->maxtemp.mkelvin = C_to_mkelvin(lrint(strtod(col[coln(MAXWATERTEMP)]->bind_ptr, NULL)));

		/* No DC related data */
		smtkdive->visibility = strtod(col[coln(VISIBILITY)]->bind_ptr, NULL) > 25 ? 5 : lrint(strtod(col[13]->bind_ptr, NULL) / 5);
		weightsystem_t ws = { {lrint(strtod(col[coln(WEIGHT)]->bind_ptr, NULL) * 1000)}, "" };
		add_cloned_weightsystem(&smtkdive->weightsystems, ws);
		smtkdive->suit = copy_string(suit_list[atoi(col[coln(SUITIDX)]->bind_ptr) - 1]);
		smtk_build_location(mdb_clon, col[coln(SITEIDX)]->bind_ptr, &smtkdive->dive_site);
		smtkdive->buddy = smtk_locate_buddy(mdb_clon, col[0]->bind_ptr, buddy_list);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Type", "TypeRelation", type_list, true);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Activity", "ActivityRelation", activity_list, false);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Gear", "GearRelation", gear_list, false);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Fish", "FishRelation", fish_list, false);
		smtk_parse_other(smtkdive, weather_list, "Weather", col[coln(WEATHERIDX)]->bind_ptr, false);
		smtk_parse_other(smtkdive, underwater_list, "Underwater", col[coln(UNDERWATERIDX)]->bind_ptr, false);
		smtk_parse_other(smtkdive, surface_list, "Surface", col[coln(SURFACEIDX)]->bind_ptr, false);
		smtk_parse_bookmarks(mdb_clon, smtkdive, col[0]->bind_ptr);
		smtkdive->notes = smtk_concat_str(smtkdive->notes, "\n", "%s", col[coln(REMARKS)]->bind_ptr);

		record_dive_to_table(smtkdive, divetable);
		add_devices_of_dive(smtkdive, devices);
		device_data_free(devdata);
	}
	mdb_free_tabledef(mdb_table);
	mdb_free_catalog(mdb_clon);
	mdb->catalog = NULL;
	mdb_close(mdb_clon);
	mdb_close(mdb);
	sort_dive_table(divetable);
}
