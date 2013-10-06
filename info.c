/* info.c
 *
 * UI toolkit independent logic used for the info frame
 *
 * bool gps_changed(struct dive *dive, struct dive *master, const char *gps_text);
 * void print_gps_coordinates(char *buffer, int len, int lat, int lon);
 * void save_equipment_data(struct dive *dive);
 * void update_equipment_data(struct dive *dive, struct dive *master);
 * void update_time_depth(struct dive *dive, struct dive *edited);
 * const char *get_window_title(struct dive *dive);
 * char *evaluate_string_change(const char *newstring, char **textp, const char *master);
 * int text_changed(const char *old, const char *new);
 * bool parse_gps_text(const char *gps_text, double *latitude, double *longitude);
 * int divename(char *buf, size_t size, struct dive *dive, char *trailer);
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include "gettext.h"
#include "dive.h"
#include "display.h"
#include "divelist.h"

/* take latitude and longitude in udeg and print them in a human readable
 * form, without losing precision */
void print_gps_coordinates(char *buffer, int len, int lat, int lon)
{
	unsigned int latdeg, londeg;
	const char *lath, *lonh;
	char dbuf_lat[32], dbuf_lon[32];

	if (!lat && !lon) {
		*buffer = 0;
		return;
	}
	lath = lat >= 0 ? tr("N") : tr("S");
	lonh = lon >= 0 ? tr("E") : tr("W");
	lat = abs(lat);
	lon = abs(lon);
	latdeg = lat / 1000000;
	londeg = lon / 1000000;
	int ilatmin = (lat % 1000000) * 60;
	int ilonmin = (lon % 1000000) * 60;
	snprintf(dbuf_lat, sizeof(dbuf_lat), "%2d.%05d", ilatmin / 1000000, (ilatmin % 1000000) / 10);
	snprintf(dbuf_lon, sizeof(dbuf_lon), "%2d.%05d", ilonmin / 1000000, (ilonmin % 1000000) / 10);
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
