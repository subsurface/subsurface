#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "dive.h"

static void show_milli(FILE *f, const char *pre, int value, const char *unit, const char *post)
{
	int i;
	char buf[4];
	unsigned v;

	fputs(pre, f);
	v = value;
	if (value < 0) {
		putc('-', f);
		v = -value;
	}
	for (i = 2; i >= 0; i--) {
		buf[i] = (v % 10) + '0';
		v /= 10;
	}
	buf[3] = 0;
	if (buf[2] == '0') {
		buf[2] = 0;
		if (buf[1] == '0')
			buf[1] = 0;
	}

	fprintf(f, "%u.%s%s%s", v, buf, unit, post);
}

static void show_temperature(FILE *f, temperature_t temp, const char *pre, const char *post)
{
	if (temp.mkelvin)
		show_milli(f, pre, temp.mkelvin - 273150, " C", post);
}

static void show_depth(FILE *f, depth_t depth, const char *pre, const char *post)
{
	if (depth.mm)
		show_milli(f, pre, depth.mm, " m", post);
}

static void show_duration(FILE *f, duration_t duration, const char *pre, const char *post)
{
	if (duration.seconds)
		fprintf(f, "%s%u:%02u min%s", pre, FRACTION(duration.seconds, 60), post);
}

static void show_pressure(FILE *f, pressure_t pressure, const char *pre, const char *post)
{
	if (pressure.mbar)
		show_milli(f, pre, pressure.mbar, " bar", post);
}

static void show_salinity(FILE *f, int salinity, const char *pre, const char *post)
{
	if (salinity)
		fprintf(f, "%s%.1f kg/l%s", pre, salinity / 10.0, post);
}
/*
 * We're outputting utf8 in xml.
 * We need to quote the characters <, >, &.
 *
 * Technically I don't think we'd necessarily need to quote the control
 * characters, but at least libxml2 doesn't like them. It doesn't even
 * allow them quoted. So we just skip them and replace them with '?'.
 *
 * If we do this for attributes, we need to quote the quotes we use too.
 */
static void quote(FILE *f, const char *text, int is_attribute)
{
	const char *p = text;

	for (;;) {
		const char *escape;

		switch (*p++) {
		default:
			continue;
		case 0:
			escape = NULL;
			break;
		case 1 ... 8:
		case 11: case 12:
		case 14 ... 31:
			escape = "?";
			break;
		case '<':
			escape = "&lt;";
			break;
		case '>':
			escape = "&gt;";
			break;
		case '&':
			escape = "&amp;";
			break;
		case '\'':
			if (!is_attribute)
				continue;
			escape = "&apos;";
			break;
		case '\"':
			if (!is_attribute)
				continue;
			escape = "&quot;";
			break;
		}
		fwrite(text, (p - text - 1), 1, f);
		if (!escape)
			break;
		fputs(escape, f);
		text = p;
	}
}

static void show_utf8(FILE *f, const char *text, const char *pre, const char *post, int is_attribute)
{
	int len;

	if (!text)
		return;
	while (isspace(*text))
		text++;
	len = strlen(text);
	if (!len)
		return;
	while (len && isspace(text[len-1]))
		len--;
	/* FIXME! Quoting! */
	fputs(pre, f);
	quote(f, text, is_attribute);
	fputs(post, f);
}

static void save_depths(FILE *f, struct dive *dive)
{
	/* What's the point of this dive entry again? */
	if (!dive->maxdepth.mm && !dive->meandepth.mm)
		return;

	fputs("  <depth", f);
	show_depth(f, dive->maxdepth, " max='", "'");
	show_depth(f, dive->meandepth, " mean='", "'");
	fputs(" />\n", f);
}

static void save_temperatures(FILE *f, struct dive *dive)
{
	if (!dive->airtemp.mkelvin && !dive->watertemp.mkelvin)
		return;
	fputs("  <temperature", f);
	show_temperature(f, dive->airtemp, " air='", "'");
	show_temperature(f, dive->watertemp, " water='", "'");
	fputs(" />\n", f);
}

static void save_airpressure(FILE *f, struct dive *dive)
{
	if (!dive->surface_pressure.mbar)
		return;
	fputs("  <surface", f);
	show_pressure(f, dive->surface_pressure, " pressure='", "'");
	fputs(" />\n", f);
}

static void save_salinity(FILE *f, struct dive *dive)
{
	if (!dive->salinity)
		return;
	fputs("  <water ", f);
	show_salinity(f, dive->salinity, " salinity='", "'");
	fputs(" />\n", f);
}


static void show_location(FILE *f, struct dive *dive)
{
	char buffer[80];
	const char *prefix = "  <location>";
	double latitude = dive->latitude;
	double longitude = dive->longitude;

	/*
	 * Ok, theoretically I guess you could dive at
	 * exactly 0,0. But we don't support that. So
	 * if you do, just fudge it a bit, and say that
	 * you dove a few meters away.
	 */
	if (latitude || longitude) {
		int len = snprintf(buffer, sizeof(buffer)-4,
			"  <location gps=");
		char *buf = buffer + len;

		len = strlen(g_ascii_formatd(buf, 80, "'%.12g ", latitude));
		buf += len;
		len = strlen(g_ascii_formatd(buf, 80, "%.12g'>", longitude));
		buf += len;
		len = buf - buffer;
		if (!dive->location) {
			memcpy(&buffer[len-1], "/>\n", 4);
			fputs(buffer, f);
			return;
		}
		prefix = buffer;
	}
	show_utf8(f, dive->location, prefix,"</location>\n", 0);
}

static void save_overview(FILE *f, struct dive *dive)
{
	save_depths(f, dive);
	save_temperatures(f, dive);
	save_airpressure(f, dive);
	save_salinity(f, dive);
	show_duration(f, dive->surfacetime, "  <surfacetime>", "</surfacetime>\n");
	show_location(f, dive);
	show_utf8(f, dive->divemaster, "  <divemaster>","</divemaster>\n", 0);
	show_utf8(f, dive->buddy, "  <buddy>","</buddy>\n", 0);
	show_utf8(f, dive->notes, "  <notes>","</notes>\n", 0);
	show_utf8(f, dive->suit, "  <suit>","</suit>\n", 0);
}

static int nr_cylinders(struct dive *dive)
{
	int nr;

	for (nr = MAX_CYLINDERS; nr; --nr) {
		cylinder_t *cylinder = dive->cylinder+nr-1;
		if (!cylinder_nodata(cylinder))
			break;
	}
	return nr;
}

static void save_cylinder_info(FILE *f, struct dive *dive)
{
	int i, nr;

	nr = nr_cylinders(dive);

	for (i = 0; i < nr; i++) {
		cylinder_t *cylinder = dive->cylinder+i;
		int volume = cylinder->type.size.mliter;
		const char *description = cylinder->type.description;
		int o2 = cylinder->gasmix.o2.permille;
		int he = cylinder->gasmix.he.permille;

		fprintf(f, "  <cylinder");
		if (volume)
			show_milli(f, " size='", volume, " l", "'");
		show_pressure(f, cylinder->type.workingpressure, " workpressure='", "'");
		if (description && *description)
			fprintf(f, " description='%s'", description);
		if (o2) {
			fprintf(f, " o2='%u.%u%%'", FRACTION(o2, 10));
			if (he)
				fprintf(f, " he='%u.%u%%'", FRACTION(he, 10));
		}
		show_pressure(f, cylinder->start, " start='", "'");
		show_pressure(f, cylinder->end, " end='", "'");
		fprintf(f, " />\n");
	}
}

static void save_weightsystem_info(FILE *f, struct dive *dive)
{
	int i;

	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		weightsystem_t *ws = dive->weightsystem+i;
		int grams = ws->weight.grams;
		const char *description = ws->description;

		/* No weight information at all? */
		if (grams == 0)
			return;
		fprintf(f, "  <weightsystem");
		show_milli(f, " weight='", grams, " kg", "'");
		if (description && *description)
			fprintf(f, " description='%s'", description);
		fprintf(f, " />\n");
	}
}

static void show_index(FILE *f, int value, const char *pre, const char *post)
{
	if (value)
		fprintf(f, " %s%d%s", pre, value, post);
}

static void save_sample(FILE *f, struct sample *sample)
{
	fprintf(f, "  <sample time='%u:%02u min'", FRACTION(sample->time.seconds,60));
	show_milli(f, " depth='", sample->depth.mm, " m", "'");
	show_temperature(f, sample->temperature, " temp='", "'");
	show_pressure(f, sample->cylinderpressure, " pressure='", "'");
	if (sample->cylinderindex)
		fprintf(f, " cylinderindex='%d'", sample->cylinderindex);
	fprintf(f, " />\n");
}

static void save_one_event(FILE *f, struct event *ev)
{
	fprintf(f, "  <event time='%d:%02d min'", FRACTION(ev->time.seconds,60));
	show_index(f, ev->type, "type='", "'");
	show_index(f, ev->flags, "flags='", "'");
	show_index(f, ev->value, "value='", "'");
	show_utf8(f, ev->name, " name='", "'", 1);
	fprintf(f, " />\n");
}


static void save_events(FILE *f, struct event *ev)
{
	while (ev) {
		save_one_event(f, ev);
		ev = ev->next;
	}
}

static void save_trip(FILE *f, dive_trip_t *trip)
{
	struct tm tm;

	utc_mkdate(trip->when, &tm);

	fprintf(f, "<trip");
	fprintf(f, " date='%04u-%02u-%02u'",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
	fprintf(f, " time='%02u:%02u:%02u'",
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (trip->location)
		show_utf8(f, trip->location, " location=\'","\'", 1);
	fprintf(f, ">\n");
	if (trip->notes)
		show_utf8(f, trip->notes, "<notes>","</notes>\n", 0);
}

static void save_dive(FILE *f, struct dive *dive)
{
	int i;
	struct tm tm;

	utc_mkdate(dive->when, &tm);

	fputs("<dive", f);
	if (dive->number)
		fprintf(f, " number='%d'", dive->number);
	/*
	 * TF_NONE is the default for dives with no trips
	 * IN_TRIP is the default for dives with trips
	 * ASSIGNED_TRIP is an in-memory thing and gets converted
	 *    to IN_TRIP by the save code.
	 */
	if (dive->tripflag != TF_NONE && dive->tripflag != IN_TRIP && dive->tripflag != ASSIGNED_TRIP)
		fprintf(f, " tripflag='%s'", tripflag_names[dive->tripflag]);
	if (dive->rating)
		fprintf(f, " rating='%d'", dive->rating);
	if (dive->visibility)
		fprintf(f, " visibility='%d'", dive->visibility);
	fprintf(f, " date='%04u-%02u-%02u'",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
	fprintf(f, " time='%02u:%02u:%02u'",
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(f, " duration='%u:%02u min'>\n",
		FRACTION(dive->duration.seconds, 60));
	save_overview(f, dive);
	save_cylinder_info(f, dive);
	save_weightsystem_info(f, dive);
	save_events(f, dive->events);
	for (i = 0; i < dive->samples; i++)
		save_sample(f, dive->sample+i);
	fprintf(f, "</dive>\n");
}

#define VERSION 1

void save_dives(const char *filename)
{
	int i;
	struct dive *dive;
	dive_trip_t *trip = NULL;

	FILE *f = g_fopen(filename, "w");

	if (!f)
		return;

	/* Flush any edits of current dives back to the dives! */
	update_dive(current_dive);

	fprintf(f, "<dives>\n<program name='subsurface' version='%d'></program>\n", VERSION);

	/* save the dives */
	for_each_dive(i, dive) {
		dive_trip_t *thistrip = dive->divetrip;
		if (trip != thistrip) {
			/* Close the old trip? */
			if (trip)
				fprintf(f, "</trip>\n");
			/* Open the new one */
			if (thistrip)
				save_trip(f, thistrip);
			trip = thistrip;
		}
		save_dive(f, get_dive(i));
	}
	if (trip)
		fprintf(f, "</trip>\n");
	fprintf(f, "</dives>\n");
	fclose(f);
}
