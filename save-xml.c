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
		fprintf(f, "%s%d g/l%s", pre, salinity / 10, post);
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
	/* only save if we have a value that isn't the default of sea water */
	if (!dive->salinity || dive->salinity == 10300)
		return;
	fputs("  <water", f);
	show_salinity(f, dive->salinity, " salinity='", "'");
	fputs(" />\n", f);
}

/*
 * Format degrees to within 6 decimal places. That's about 0.1m
 * on a great circle (ie longitude at equator). And micro-degrees
 * is also enough to fit in a fixed-point 32-bit integer.
 */
static int format_degrees(char *buffer, degrees_t value)
{
	int udeg = value.udeg;
	const char *sign = "";

	if (udeg < 0) {
		sign = "-";
		udeg = -udeg;
	}
	return sprintf(buffer, "%s%u.%06u",
		sign, udeg / 1000000, udeg % 1000000);
}

static int format_location(char *buffer, degrees_t latitude, degrees_t longitude)
{
	int len = sprintf(buffer, "gps='");

	len += format_degrees(buffer+len, latitude);
	buffer[len++] = ' ';
	len += format_degrees(buffer+len, longitude);
	buffer[len++] = '\'';

	return len;
}

static void show_location(FILE *f, struct dive *dive)
{
	char buffer[80];
	const char *prefix = "  <location>";
	degrees_t latitude = dive->latitude;
	degrees_t longitude = dive->longitude;

	/*
	 * Ok, theoretically I guess you could dive at
	 * exactly 0,0. But we don't support that. So
	 * if you do, just fudge it a bit, and say that
	 * you dove a few meters away.
	 */
	if (latitude.udeg || longitude.udeg) {
		int len = sprintf(buffer, "  <location ");

		len += format_location(buffer+len, latitude, longitude);
		if (!dive->location) {
			memcpy(buffer+len, "/>\n", 4);
			fputs(buffer, f);
			return;
		}
		buffer[len++] = '>';
		buffer[len] = 0;
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

static void save_sample(FILE *f, struct sample *sample, const struct sample *prev)
{
	fprintf(f, "  <sample time='%u:%02u min'", FRACTION(sample->time.seconds,60));
	show_milli(f, " depth='", sample->depth.mm, " m", "'");
	show_temperature(f, sample->temperature, " temp='", "'");
	show_pressure(f, sample->cylinderpressure, " pressure='", "'");
	if (sample->cylinderindex)
		fprintf(f, " cylinderindex='%d'", sample->cylinderindex);
	/* the deco/ndl values are stored whenever they change */
	if (sample->ndl.seconds != prev->ndl.seconds)
		fprintf(f, " ndl='%u:%02u min'", FRACTION(sample->ndl.seconds, 60));
	if (sample->stoptime.seconds != prev->stoptime.seconds)
		fprintf(f, " stoptime='%u:%02u min'", FRACTION(sample->stoptime.seconds, 60));
	if (sample->stopdepth.mm != prev->stopdepth.mm)
		show_milli(f, " stopdepth='", sample->stopdepth.mm, " m", "'");
	if (sample->cns != prev->cns)
		fprintf(f, " cns='%u%%'", sample->cns);
	if (sample->po2 != prev->po2)
		fprintf(f, " po2='%u.%2u bar'", FRACTION(sample->po2, 1000));
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

static void show_date(FILE *f, timestamp_t when)
{
	struct tm tm;

	utc_mkdate(when, &tm);

	fprintf(f, " date='%04u-%02u-%02u'",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday);
	fprintf(f, " time='%02u:%02u:%02u'",
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

static void save_trip(FILE *f, dive_trip_t *trip)
{
	fprintf(f, "<trip");
	show_date(f, trip->when);
	if (trip->location)
		show_utf8(f, trip->location, " location=\'","\'", 1);
	fprintf(f, ">\n");
	if (trip->notes)
		show_utf8(f, trip->notes, "<notes>","</notes>\n", 0);
}

static void save_samples(FILE *f, int nr, struct sample *s)
{
	static const struct sample empty_sample;
	const struct sample *prev = &empty_sample;

	while (--nr >= 0) {
		save_sample(f, s, prev);
		prev = s;
		s++;
	}
}

static void save_dc(FILE *f, struct dive *dive, struct divecomputer *dc)
{
	fprintf(f, "  <divecomputer");
	if (dc->model)
		show_utf8(f, dc->model, " model='", "'", 1);
	if (dc->deviceid)
		fprintf(f, " deviceid='%08x'", dc->deviceid);
	if (dc->diveid)
		fprintf(f, " diveid='%08x'", dc->diveid);
	if (dc->when && dc->when != dive->when)
		show_date(f, dc->when);
	fprintf(f, ">\n");
	save_events(f, dc->events);
	save_samples(f, dc->samples, dc->sample);

	fprintf(f, "  </divecomputer>\n");
}

static void save_dive(FILE *f, struct dive *dive)
{
	struct divecomputer *dc;

	fputs("<dive", f);
	if (dive->number)
		fprintf(f, " number='%d'", dive->number);
	if (dive->tripflag == NO_TRIP)
		fprintf(f, " tripflag='NOTRIP'");
	if (dive->rating)
		fprintf(f, " rating='%d'", dive->rating);
	if (dive->visibility)
		fprintf(f, " visibility='%d'", dive->visibility);
	show_date(f, dive->when);
	fprintf(f, " duration='%u:%02u min'>\n",
		FRACTION(dive->duration.seconds, 60));
	save_overview(f, dive);
	save_cylinder_info(f, dive);
	save_weightsystem_info(f, dive);

	/* Save the dive computer data */
	dc = &dive->dc;
	do {
		save_dc(f, dive, dc);
		dc = dc->next;
	} while (dc);

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
