/*
 * An .slg file is composed of a main table (Dives), a bunch of tables directly
 * linked to Dives by their indexes (Site, Suit, Tank, etc) and another group of
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

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mdbtools.h>
#include <stdarg.h>

#include "dive.h"
#include "gettext.h"
#include "divelist.h"
#include "libdivecomputer.h"
#include "divesite.h"
#include "membuffer.h"

/* SmartTrak version, constant for every single file */
int smtk_version;
int tanks;

/*
 * There are AFAIK two versions of Smarttrak. The newer one supports trimix and up
 * to 10 tanks. The other one just 3 tanks and no trimix but only nitrox. This is a
 * problem for an automated parser which has to support both formats.
 * In this solution I made an enum of fields with the same order they would have in
 * a smarttrak db, and a tiny function which returns the number of the column where
 * a field is expected to be, taking into account the different db formats .
 */
enum field_pos {IDX = 0, DIVENUM, DATE, INTIME, INTVAL, DURATION, OUTTIME, DESATBEFORE, DESATAFTER, NOFLYBEFORE,
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
 * TODO: Verify localization.
 */
static void smtk_date_to_tm(char *d_buffer, struct tm *tm_date)
{
	char *temp = NULL;

	temp = copy_string(d_buffer);
	strtok(temp, " ");
	if (temp)
		strptime(temp, "%x", tm_date);
}

/*
 * Fills the time part of a tm structure with the string data obtained
 * from smartrak in format "DD/MM/YY HH:MM:SS" where date is irrelevant.
 * TODO: Verify localization.
 */
static void smtk_time_to_tm(char *t_buffer, struct tm *tm_date)
{
	char *temp = NULL;

	temp = rindex(copy_string(t_buffer), ' ');
	if (temp)
		strptime(temp, "%X", tm_date);
}

/*
 * Converts to seconds a time lapse returned from smartrak in string format
 * "DD/MM/YY HH:MM:SS" where date is irrelevant.
 * TODO: modify to support times > 24h where date means difference in days
 * from 29/12/99.
 */
static unsigned int smtk_time_to_secs(char *t_buffer)
{
	char *temp;
	unsigned int hr, min, sec;

	if (!same_string(t_buffer, "")) {
		temp = rindex(copy_string(t_buffer), ' ');
		hr = atoi(strtok(temp, ":"));
		min = atoi(strtok(NULL, ":"));
		sec = atoi(strtok(NULL, "\0"));
		return((((hr*60)+min)*60)+sec);
	} else {
		return 0;
	}
}

/*
 * Returns an opened table given its name and mdb. outcol and outbounder have to be allocated
 * by the caller and are filled here.
 */
static  MdbTableDef *smtk_open_table(MdbHandle *mdb, char *tablename, MdbColumn **outcol, char **outbounder)
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
		outcol[i] = g_ptr_array_index(table->columns, i);
		outbounder[i] = (char *) g_malloc(MDB_BIND_SIZE);
		mdb_bind_column(table, i+1, outbounder[i], NULL);
	}
	mdb_rewind_table(table);

	return table;
}

/*
 * Utility function which returns the value from a given column in a given table,
 * whose row equals the given idx string.
 * Idx should be a numeric value, but all values obtained from mdbtools are strings,
 * so let's compare strings instead of numbers to avoid unnecessary transforms.
 */
static char *smtk_value_by_idx(MdbHandle *mdb, char *tablename, int colnum, char *idx)
{
	MdbCatalogEntry *entry;
	MdbTableDef *table;
	MdbColumn *idxcol, *valuecol;
	char *bounder[MDB_MAX_COLS], *str = NULL;
	int i = 0;

	entry = mdb_get_catalogentry_by_name(mdb, tablename);
	table = mdb_read_table(entry);
	if (!table) {
		report_error("[Error][smartrak_import]\t%s table doesn't exist\n", tablename);
		return str;
	}
	mdb_read_columns(table);
	idxcol = g_ptr_array_index(table->columns, 0);
	valuecol = g_ptr_array_index(table->columns, colnum);
	for (i = 0; i < table->num_cols; i++) {
		bounder[i] = (char *) g_malloc(MDB_BIND_SIZE);
		mdb_bind_column(table, i+1, bounder[i], NULL);
	}
	mdb_rewind_table(table);
	for (i = 0; i < table->num_rows; i++) {
		mdb_fetch_row(table);
		if (!strcmp(idxcol->bind_ptr, idx)) {
			str = copy_string(valuecol->bind_ptr);
			break;
		}
	}
	for (i = 0; i < table->num_cols; i++)
		free(bounder[i]);
	mdb_free_tabledef(table);
	return str;
}

/*
 * Utility function which joins three strings, being the second a separator string,
 * usually a "\n". The third is a format string with an argument list.
 * If the original string is NULL, then just returns the third.
 * This is based in add_to_string() and add_to_string_va(), and, as its parents
 * frees the original string.
 */
static char *smtk_concat_str(const char *orig, const char *sep, const char *fmt, ...)
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
	free((void *)orig);

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
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS];
	char *tmp = NULL, *notes = NULL;
	int rc, i;
	uint32_t d;
	const char *wreck_fields[] = {QT_TRANSLATE_NOOP("gettextFromC", "Built"), QT_TRANSLATE_NOOP("gettextFromC", "Sank"), QT_TRANSLATE_NOOP("gettextFromC", "SankTime"),
				      QT_TRANSLATE_NOOP("gettextFromC", "Reason"), QT_TRANSLATE_NOOP("gettextFromC", "Nationality"), QT_TRANSLATE_NOOP("gettextFromC", "Shipyard"),
				      QT_TRANSLATE_NOOP("gettextFromC", "ShipType"), QT_TRANSLATE_NOOP("gettextFromC", "Length"), QT_TRANSLATE_NOOP("gettextFromC", "Beam"),
				      QT_TRANSLATE_NOOP("gettextFromC", "Draught"), QT_TRANSLATE_NOOP("gettextFromC", "Displacement"), QT_TRANSLATE_NOOP("gettextFromC", "Cargo"),
				      QT_TRANSLATE_NOOP("gettextFromC", "Notes")};

	table = smtk_open_table(mdb, "Wreck", col, bound_values);

	/* Sanity check for table, unlikely but ... */
	if (!table)
		return;

	/* Begin parsing. Write strings to notes only if available.*/
	while (mdb_fetch_row(table)) {
		if (!strcmp(col[1]->bind_ptr, site_idx)) {
			tmp = copy_string(col[1]->bind_ptr);
			notes = smtk_concat_str(notes, "\n", translate("gettextFromC", "Wreck Data"));
			for (i = 3; i < 16; i++) {
				switch (i) {
				case 3:
				case 4:
					if (memcmp(col[i]->bind_ptr, "\0", 1))
						notes = smtk_concat_str(notes, "\n", "%s: %s", wreck_fields[i - 3], strtok(copy_string(col[i]->bind_ptr), " "));
					break;
				case 5:
					if (strcmp(rindex(copy_string(col[i]->bind_ptr), ' '), "\0"))
						notes = smtk_concat_str(notes, "\n", "%s: %s", wreck_fields[i - 3], rindex(col[i]->bind_ptr, ' '));
					break;
				case 6 ... 9:
				case 14:
				case 15:
					if (memcmp(col[i]->bind_ptr, "\0", 1))
						notes = smtk_concat_str(notes, "\n", "%s: %s", wreck_fields[i - 3], col[i]->bind_ptr);
					break;
				default:
					d = strtold(col[i]->bind_ptr, NULL);
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
	for (i = 0;  i < table->num_cols; i++)
		free(bound_values[i]);
	mdb_free_tabledef(table);
	free(notes);
	free(tmp);
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
static void smtk_build_location(MdbHandle *mdb, char *idx, timestamp_t when, uint32_t *location)
{
	MdbTableDef *table;
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS];
	int i;
	uint32_t d;
	struct dive_site *ds;
	degrees_t lat, lon;
	char *str = NULL, *loc_idx = NULL, *site = NULL, *notes = NULL;
	const char *site_fields[] = {QT_TRANSLATE_NOOP("gettextFromC", "Altitude"), QT_TRANSLATE_NOOP("gettextFromC", "Depth"),
				     QT_TRANSLATE_NOOP("gettextFromC", "Notes")};

	/* Read data from Site table. Format notes for the dive site if any.*/
	table = smtk_open_table(mdb, "Site", col, bound_values);
	if (!table)
		return;

	for (i = 1; i <= atoi(idx); i++)
		mdb_fetch_row(table);
	loc_idx = copy_string(col[2]->bind_ptr);
	site = copy_string(col[1]->bind_ptr);
	lat.udeg = lrint(strtod(copy_string(col[6]->bind_ptr), NULL) * 1000000);
	lon.udeg = lrint(strtod(copy_string(col[7]->bind_ptr), NULL) * 1000000);

	for (i = 8; i < 11; i++) {
		switch (i) {
		case 8:
		case 9:
			d = strtold(col[i]->bind_ptr, NULL);
			if (d)
				notes = smtk_concat_str(notes, "\n", "%s: %d m", site_fields[i - 8], d);
			break;
		case 10:
			if (memcmp(col[i]->bind_ptr, "\0", 1))
				notes = smtk_concat_str(notes, "\n", "%s: %s", site_fields[i - 8], col[i]->bind_ptr);
			break;
		}
	}
	for (i = 0;  i < table->num_cols; i++) {
		bound_values[i] = NULL;
		col[i] = NULL;
	}

	/* Read data from Location table, linked to Site by loc_idx */
	table = smtk_open_table(mdb, "Location", col, bound_values);
	mdb_rewind_table(table);
	for (i = 1; i <= atoi(loc_idx); i++)
		mdb_fetch_row(table);
	/*
	 * Create a string for Subsurface's dive site structure with coordinates
	 * if available, if the site's name doesn't previously exists.
	 */
	if (memcmp(col[3]->bind_ptr, "\0", 1))
		str = smtk_concat_str(str, ", ", "%s", col[3]->bind_ptr); // Country
	if (memcmp(col[2]->bind_ptr, "\0", 1))
		str = smtk_concat_str(str, ", ", "%s", col[2]->bind_ptr); // State - Province
	if (memcmp(col[1]->bind_ptr, "\0", 1))
		str = smtk_concat_str(str, ", ", "%s", col[1]->bind_ptr); // Locality
	str =  smtk_concat_str(str, ", ", "%s", site);

	*location = get_dive_site_uuid_by_name(str, NULL);
	if (*location == 0) {
		if (lat.udeg == 0 && lon.udeg == 0)
			*location = create_dive_site(str, when);
		else
			*location = create_dive_site_with_gps(str, lat, lon, when);
	}
	for (i = 0;  i < table->num_cols; i++) {
		bound_values[i] = NULL;
		col[i] = NULL;
	}

	/* Insert site notes */
	ds = get_dive_site_by_uuid(*location);
	ds->notes = copy_string(notes);
	free(notes);

	/* Check if we have a wreck */
	smtk_wreck_site(mdb, idx, ds);

	/* Clean up and exit */
	for (i = 0;  i < table->num_cols; i++)
		free(bound_values[i]);
	mdb_free_tabledef(table);
	free(loc_idx);
	free(site);
	free(str);
}

static void smtk_build_tank_info(MdbHandle *mdb, struct dive *dive, int tanknum, char *idx)
{
	MdbTableDef *table;
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS];
	int i;

	table = smtk_open_table(mdb, "Tank", col, bound_values);
	if (!table)
		return;

	for (i = 1; i <= atoi(idx); i++)
		mdb_fetch_row(table);
	dive->cylinder[tanknum].type.description = copy_string(col[1]->bind_ptr);
	dive->cylinder[tanknum].type.size.mliter = strtod(col[2]->bind_ptr, NULL) * 1000;
	dive->cylinder[tanknum].type.workingpressure.mbar = strtod(col[4]->bind_ptr, NULL) * 1000;

	for (i = 0;  i < table->num_cols; i++)
		free(bound_values[i]);
	mdb_free_tabledef(table);
}

/*
 * Parses a relation table and fills a list with the relations for a dive idx.
 * Returns the number of relations found for a given dive idx.
 * Table relation format:
 * | Diveidx | Idx |
 */
static int smtk_index_list(MdbHandle *mdb, char *table_name, char *dive_idx, int idx_list[])
{
	int n = 0, i = 0;
	MdbTableDef *table;
	MdbColumn *cols[MDB_MAX_COLS];
	char *bounders[MDB_MAX_COLS];

	table = smtk_open_table(mdb, table_name, cols, bounders);

	/* Sanity check */
	if (!table)
		return 0;

	/* Parse the table searching for dive_idx */
	while (mdb_fetch_row(table)) {
		if (!strcmp(dive_idx, cols[0]->bind_ptr)) {
			idx_list[n] = atoi(cols[1]->bind_ptr);
			n++;
		}
	}

	/* Clean up and exit */
	for (i = 0; i < table->num_cols; i++)
		free(bounders[i]);
	mdb_free_tabledef(table);
	return n;
}

/*
 * Returns string with buddies names as registered in smartrak (may be a nickname).
 * "Buddy" table is a buddies relation with lots and lots and lots of data (even buddy mother's
 * maiden name ;-) ) most of them useless for a dive log. Let's just consider the nickname as main
 * field and the full name if this exists and its construction is different from the nickname.
 * Buddy table format:
 * | Idx | Text (nickname) | Name | Firstname | Middlename | Title | Picture | Phone | ...
 */
static char *smtk_locate_buddy(MdbHandle *mdb, char *dive_idx)
{
	char *str = NULL, *fullname = NULL, *bounder[MDB_MAX_COLS] = { NULL }, *buddies[256] = { NULL };
	MdbTableDef *table;
	MdbColumn *col[MDB_MAX_COLS];
	int i, n, rel[256] = { 0 };

	n = smtk_index_list(mdb, "BuddyRelation", dive_idx, rel);
	if (!n)
		return str;
	table = smtk_open_table(mdb, "Buddy", col, bounder);
	if (!table)
		return str;
	/*
	 * Buddies in a single dive aren't (usually) a big number, so probably
	 * it's not a good idea to use a complex data structure and algorithm.
	 */
	while (mdb_fetch_row(table)) {
		if (!same_string(col[3]->bind_ptr, ""))
			fullname = smtk_concat_str(fullname, " ", "%s", col[3]->bind_ptr);
		if (!same_string(col[4]->bind_ptr, ""))
			fullname = smtk_concat_str(fullname, " ", "%s", col[4]->bind_ptr);
		if (!same_string(col[2]->bind_ptr, ""))
			fullname = smtk_concat_str(fullname, " ", "%s", col[2]->bind_ptr);
		if (fullname && !same_string(col[1]->bind_ptr, fullname))
			buddies[atoi(col[0]->bind_ptr)] = smtk_concat_str(buddies[atoi(col[0]->bind_ptr)], "", "%s (%s)", col[1]->bind_ptr, fullname);
		else
			buddies[atoi(col[0]->bind_ptr)] = smtk_concat_str(buddies[atoi(col[0]->bind_ptr)], "", "%s", col[1]->bind_ptr);
		fullname = NULL;
	}
	free(fullname);
	for (i = 0; i < n; i++)
		str = smtk_concat_str(str, ", ", "%s", buddies[rel[i]]);

	/* Clean up and exit */
	for (i = 0; i < table->num_rows; i++)
		free(buddies[i]);
	for (i = 0; i < table->num_cols; i++)
		free(bounder[i]);
	mdb_free_tabledef(table);
	return str;
}

/* Parses the dive_type mdb tables and import the data into dive's
 * taglist structure or notes.  If there are tags that affects dive's dive_mode
 * (SCR, CCR or so), set the dive mode too.
 * The "tag" parameter is used to mark if we want this table to be imported
 * into tags or into notes.
 * Managed tables formats: Just consider Idx and Text
 * Type:
 * | Idx | Text | Default (bool)
 * Activity:
 * | Idx | Text | Default (bool)
 * Gear:
 * | Idx | Text | Vendor | Type | Typenum | Notes | Default (bool) | TrakId
 * Fish:
 * | Idx | Text | Name | Latin name | Typelength | Maxlength | Picture | Default (bool)| TrakId
 */
static void smtk_parse_relations(MdbHandle *mdb, struct dive *dive, char *dive_idx, char *table_name, char *rel_table_name, bool tag)
{
	MdbTableDef *table;
	MdbColumn *col[MDB_MAX_COLS];
	char *bound_values[MDB_MAX_COLS], *tmp = NULL, *types[64] = { NULL };
	int i = 0, n = 0, rels[256] = { 0 };

	n = smtk_index_list(mdb, rel_table_name, dive_idx, rels);
	if (!n)
		return;
	table = smtk_open_table(mdb, table_name, col, bound_values);
	if (!table)
		return;
	while (mdb_fetch_row(table))
		types[atoi(col[0]->bind_ptr)] = copy_string(col[1]->bind_ptr);

	for (i = 0; i < n; i++) {
		if (tag)
			taglist_add_tag(&dive->tag_list, copy_string(types[rels[i]]));
		else
			tmp = smtk_concat_str(tmp, ", ", "%s", types[rels[i]]);
		if (strstr(types[rels[i]], "SCR"))
			dive->dc.divemode = PSCR;
		else if (strstr(types[rels[i]], "CCR"))
			dive->dc.divemode = CCR;
	}
	if (tmp)
		dive->notes = smtk_concat_str(dive->notes, "\n", "Smartrak %s: %s", table_name, tmp);
	free(tmp);

	/* clean up and exit */
	for (i = 1; i < 64; i++)
		free(types[i]);
	for (i = 0; i < table->num_cols; i++)
		free(bound_values[i]);
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
static int prepare_data(int data_model, dc_family_t dc_fam, device_data_t *dev_data)
{
	dev_data->device = NULL;
	dev_data->context = NULL;
	dev_data->descriptor = get_data_descriptor(data_model, dc_fam);
	if (dev_data->descriptor) {
		dev_data->vendor = copy_string(dev_data->descriptor->vendor);
		dev_data->product = copy_string(dev_data->descriptor->product);
		dev_data->model = smtk_concat_str(dev_data->model, "", "%s %s", dev_data->vendor, dev_data->product);
		return DC_STATUS_SUCCESS;
	} else {
		return DC_STATUS_UNSUPPORTED;
	}
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
	switch (dev_data->descriptor->type) {
	case DC_FAMILY_UWATEC_ALADIN:
	case DC_FAMILY_UWATEC_MEMOMOUSE:
		compl_buf[3] = (unsigned char) dev_data->descriptor->model;
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
	int i, dc_model;

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

	/* Check Smarttrak version (different number of supported tanks, mixes and so) */
	smtk_version = atoi(smtk_value_by_idx(mdb_clon, "SmartTrak", 1, "1"));
	tanks = (smtk_version < 10213) ? 3 : 10;

	mdb_table = smtk_open_table(mdb, "Dives", col, bound_values);
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

		smtkdive->number = strtod(col[1]->bind_ptr, NULL);
		/*
		 * If there is a DC model (no zero) try to create a buffer for the
		 * dive and parse it with libdivecomputer
		 */
		dc_model = (int) strtod(col[coln(DCMODEL)]->bind_ptr, NULL) & 0xFF;
		if (dc_model) {
			hdr_buffer = mdb_ole_read_full(mdb, col[coln(LOG)], &hdr_length);
			if (hdr_length > 0 && hdr_length < 20)	// We have a profile but it's imported from datatrak
				dc_fam = DC_FAMILY_UWATEC_ALADIN;
			rc = prepare_data(dc_model, dc_fam, devdata);
		} else {
			rc = DC_STATUS_NODEVICE;
		}
		smtkdive->dc.model = devdata->model;
		smtkdive->dc.serial = copy_string(col[coln(DCNUMBER)]->bind_ptr);
		if (rc == DC_STATUS_SUCCESS) {
			prf_buffer = mdb_ole_read_full(mdb, col[coln(PROFILE)], &prf_length);
			if (prf_length > 0) {
				if (devdata->descriptor->type == DC_FAMILY_UWATEC_ALADIN || devdata->descriptor->type == DC_FAMILY_UWATEC_MEMOMOUSE)
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
				smtkdive->dc.maxdepth.mm = smtkdive->maxdepth.mm = strtod(col[coln(MAXDEPTH)]->bind_ptr, NULL) * 1000;
			}
			free(prf_buffer);
		} else {
			/* Manual dives or unknown DCs */
			report_error("[Warning][smartrak_import]\t Manual or unknown dive computer for dive %d", smtkdive->number);
			smtkdive->dc.duration.seconds = smtkdive->duration.seconds = smtk_time_to_secs(col[coln(DURATION)]->bind_ptr);
			smtkdive->dc.maxdepth.mm = smtkdive->maxdepth.mm = strtod(col[coln(MAXDEPTH)]->bind_ptr, NULL) * 1000;
		}
		free(hdr_buffer);
		/*
		 * Cylinder and gasmixes completion.
		 * Revisit data under some circunstances, e.g. a start pressure = 0 may mean
		 * that dc don't support gas control, in this situation let's look into mdb data
		 */
		int numtanks = (tanks == 10) ? 8 : 3; // Subsurface supports up to 8 tanks
		int pstartcol = coln(PSTART);
		int o2fraccol = coln(O2FRAC);
		int hefraccol = coln(HEFRAC);
		int tankidxcol = coln(TANKIDX);

		for (i = 0; i < numtanks; i++) {
			if (smtkdive->cylinder[i].start.mbar == 0) {
				smtkdive->cylinder[i].start.mbar = strtod(col[(i*2)+pstartcol]->bind_ptr, NULL) * 1000;
				smtkdive->cylinder[i].gasmix.o2.permille = strtod(col[i+o2fraccol]->bind_ptr, NULL) * 10;
				if (smtk_version == 10213)
					smtkdive->cylinder[i].gasmix.he.permille = strtod(col[i+hefraccol]->bind_ptr, NULL) * 10;
				else
					smtkdive->cylinder[i].gasmix.he.permille = 0;
			}
			/*
			 * If there is a start pressure ensure that end pressure is not zero as
			 * will be registered in DCs which only keep track of differential pressures,
			 * and collect the data registered  by the user in mdb
			 */
			if (smtkdive->cylinder[i].start.mbar != 0) {
				if (smtkdive->cylinder[i].end.mbar == 0)
					smtkdive->cylinder[i].end.mbar = strtod(col[(i * 2) + 1 + pstartcol]->bind_ptr, NULL) * 1000 ? : 1000;
				smtk_build_tank_info(mdb_clon, smtkdive, i, col[i + tankidxcol]->bind_ptr);
			}
		}

		/* Date issues with libdc parser - Take date time from mdb */
		smtk_date_to_tm(col[coln(DATE)]->bind_ptr, tm_date);
		smtk_time_to_tm(col[coln(INTIME)]->bind_ptr, tm_date);
		smtkdive->dc.when = smtkdive->when = mktime(tm_date);
		free(tm_date);
		smtkdive->dc.surfacetime.seconds = smtk_time_to_secs(col[coln(INTVAL)]->bind_ptr);

		/* Data that user may have registered manually if not supported by DC, or not parsed */
		if (!smtkdive->airtemp.mkelvin)
			smtkdive->airtemp.mkelvin = C_to_mkelvin(strtod(col[coln(AIRTEMP)]->bind_ptr, NULL));
		if (!smtkdive->watertemp.mkelvin)
			smtkdive->watertemp.mkelvin = smtkdive->mintemp.mkelvin = C_to_mkelvin(strtod(col[coln(MINWATERTEMP)]->bind_ptr, NULL));
		if (!smtkdive->maxtemp.mkelvin)
			smtkdive->maxtemp.mkelvin = C_to_mkelvin(strtod(col[coln(MAXWATERTEMP)]->bind_ptr, NULL));

		/* No DC related data */
		smtkdive->visibility = strtod(col[coln(VISIBILITY)]->bind_ptr, NULL) > 25 ? 5 : strtod(col[13]->bind_ptr, NULL) / 5;
		smtkdive->weightsystem[0].weight.grams = strtod(col[coln(WEIGHT)]->bind_ptr, NULL) * 1000;
		smtkdive->suit = smtk_value_by_idx(mdb_clon, "Suit", 1, col[coln(SUITIDX)]->bind_ptr);
		smtk_build_location(mdb_clon, col[coln(SITEIDX)]->bind_ptr, smtkdive->when, &smtkdive->dive_site_uuid);
		smtkdive->buddy = smtk_locate_buddy(mdb_clon, col[0]->bind_ptr);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Type", "TypeRelation", true);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Activity", "ActivityRelation", false);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Gear", "GearRelation", false);
		smtk_parse_relations(mdb_clon, smtkdive, col[0]->bind_ptr, "Fish", "FishRelation", false);
		smtkdive->notes = smtk_concat_str(smtkdive->notes, "\n", "%s", col[coln(REMARKS)]->bind_ptr);

		record_dive_to_table(smtkdive, divetable);
		free(devdata);
	}
	for (i = 0;  i < mdb_table->num_cols; i++)
		free(bound_values[i]);
	mdb_free_tabledef(mdb_table);
	mdb_free_catalog(mdb_clon);
	mdb->catalog = NULL;
	mdb_close(mdb_clon);
	mdb_close(mdb);
	sort_table(divetable);
}
