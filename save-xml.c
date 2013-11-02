#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "dive.h"
#include "device.h"

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
		show_milli(f, pre, temp.mkelvin - ZERO_C_IN_MKELVIN, " C", post);
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

static void save_depths(FILE *f, struct divecomputer *dc)
{
	/* What's the point of this dive entry again? */
	if (!dc->maxdepth.mm && !dc->meandepth.mm)
		return;

	fputs("  <depth", f);
	show_depth(f, dc->maxdepth, " max='", "'");
	show_depth(f, dc->meandepth, " mean='", "'");
	fputs(" />\n", f);
}

static void save_dive_temperature(FILE *f, struct dive *dive)
{
	if (!dive->airtemp.mkelvin)
		return;
	if (dive->airtemp.mkelvin == dc_airtemp(&dive->dc))
		return;

	fputs("  <divetemperature", f);
	show_temperature(f, dive->airtemp, " air='", "'");
	fputs("/>\n", f);
}

static void save_temperatures(FILE *f, struct divecomputer *dc)
{
	if (!dc->airtemp.mkelvin && !dc->watertemp.mkelvin)
		return;
	fputs("  <temperature", f);
	show_temperature(f, dc->airtemp, " air='", "'");
	show_temperature(f, dc->watertemp, " water='", "'");
	fputs(" />\n", f);
}

static void save_airpressure(FILE *f, struct divecomputer *dc)
{
	if (!dc->surface_pressure.mbar)
		return;
	fputs("  <surface", f);
	show_pressure(f, dc->surface_pressure, " pressure='", "'");
	fputs(" />\n", f);
}

static void save_salinity(FILE *f, struct divecomputer *dc)
{
	/* only save if we have a value that isn't the default of sea water */
	if (!dc->salinity || dc->salinity == SEAWATER_SALINITY)
		return;
	fputs("  <water", f);
	show_salinity(f, dc->salinity, " salinity='", "'");
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
		show_utf8(f, description, " description='", "'", 1);
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
		show_utf8(f, description, " description='", "'", 1);
		fprintf(f, " />\n");
	}
}

static void show_index(FILE *f, int value, const char *pre, const char *post)
{
	if (value)
		fprintf(f, " %s%d%s", pre, value, post);
}

static void save_sample(FILE *f, struct sample *sample, struct sample *old)
{
	fprintf(f, "  <sample time='%u:%02u min'", FRACTION(sample->time.seconds,60));
	show_milli(f, " depth='", sample->depth.mm, " m", "'");
	show_temperature(f, sample->temperature, " temp='", "'");
	show_pressure(f, sample->cylinderpressure, " pressure='", "'");

	/*
	 * We only show sensor information for samples with pressure, and only if it
	 * changed from the previous sensor we showed.
	 */
	if (sample->cylinderpressure.mbar && sample->sensor != old->sensor) {
		fprintf(f, " sensor='%d'", sample->sensor);
		old->sensor = sample->sensor;
	}

	/* the deco/ndl values are stored whenever they change */
	if (sample->ndl.seconds != old->ndl.seconds) {
		fprintf(f, " ndl='%u:%02u min'", FRACTION(sample->ndl.seconds, 60));
		old->ndl = sample->ndl;
	}
	if (sample->in_deco != old->in_deco) {
		fprintf(f, " in_deco='%d'", sample->in_deco ? 1 : 0);
		old->in_deco = sample->in_deco;
	}
	if (sample->stoptime.seconds != old->stoptime.seconds) {
		fprintf(f, " stoptime='%u:%02u min'", FRACTION(sample->stoptime.seconds, 60));
		old->stoptime = sample->stoptime;
	}

	if (sample->stopdepth.mm != old->stopdepth.mm) {
		show_milli(f, " stopdepth='", sample->stopdepth.mm, " m", "'");
		old->stopdepth = sample->stopdepth;
	}

	if (sample->cns != old->cns) {
		fprintf(f, " cns='%u%%'", sample->cns);
		old->cns = sample->cns;
	}

	if (sample->po2 != old->po2) {
		show_milli(f, " po2='", sample->po2, " bar", "'");
		old->po2 = sample->po2;
	}
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

static void save_tags(FILE *f, struct tag_entry *tag_list)
{
	int more = 0;
	struct tag_entry *tmp = tag_list->next;

	/* Only write tag attribute if the list contains at least one item  */
	if (tmp != NULL) {
		fprintf(f, " tags='");

		while (tmp != NULL) {
			if (more)
				fprintf(f, ", ");
			/* If the tag has been translated, write the source to the xml file */
			if (tmp->tag->source != NULL)
				fprintf(f, "%s", tmp->tag->source);
			else
				fprintf(f, "%s", tmp->tag->name);
			tmp = tmp->next;
			more = 1;
		}
		fprintf(f, "'");
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

static void save_samples(FILE *f, int nr, struct sample *s)
{
	struct sample dummy = { };

	while (--nr >= 0) {
		save_sample(f, s, &dummy);
		s++;
	}
}

static void save_dc(FILE *f, struct dive *dive, struct divecomputer *dc)
{
	fprintf(f, "  <divecomputer");
	show_utf8(f, dc->model, " model='", "'", 1);
	if (dc->deviceid)
		fprintf(f, " deviceid='%08x'", dc->deviceid);
	if (dc->diveid)
		fprintf(f, " diveid='%08x'", dc->diveid);
	if (dc->when && dc->when != dive->when)
		show_date(f, dc->when);
	if (dc->duration.seconds && dc->duration.seconds != dive->dc.duration.seconds)
		show_duration(f, dc->duration, " duration='", "'");
	fprintf(f, ">\n");
	save_depths(f, dc);
	save_temperatures(f, dc);
	save_airpressure(f, dc);
	save_salinity(f, dc);
	show_duration(f, dc->surfacetime, "  <surfacetime>", "</surfacetime>\n");

	save_events(f, dc->events);
	save_samples(f, dc->samples, dc->sample);

	fprintf(f, "  </divecomputer>\n");
}

void save_dive(FILE *f, struct dive *dive)
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
	if (dive->tag_list != NULL)
		save_tags(f, dive->tag_list);

	show_date(f, dive->when);
	fprintf(f, " duration='%u:%02u min'>\n",
		FRACTION(dive->dc.duration.seconds, 60));
	save_overview(f, dive);
	save_cylinder_info(f, dive);
	save_weightsystem_info(f, dive);
	save_dive_temperature(f, dive);
	/* Save the dive computer data */
	dc = &dive->dc;
	do {
		save_dc(f, dive, dc);
		dc = dc->next;
	} while (dc);

	fprintf(f, "</dive>\n");
}

static void save_trip(FILE *f, dive_trip_t *trip)
{
	int i;
	struct dive *dive;

	fprintf(f, "<trip");
	show_date(f, trip->when);
	show_utf8(f, trip->location, " location=\'","\'", 1);
	fprintf(f, ">\n");
	show_utf8(f, trip->notes, "<notes>","</notes>\n", 0);

	/*
	 * Incredibly cheesy: we want to save the dives sorted, and they
	 * are sorted in the dive array.. So instead of using the dive
	 * list in the trip, we just traverse the global dive array and
	 * check the divetrip pointer..
	 */
	for_each_dive(i, dive) {
		if (dive->divetrip == trip)
			save_dive(f, dive);
	}

	fprintf(f, "</trip>\n");
}

static void save_one_device(FILE *f, const char * model, uint32_t deviceid,
			    const char *nickname, const char *serial_nr, const char *firmware)
{
	/* Nicknames that are empty or the same as the device model are not interesting */
	if (nickname) {
		if (!*nickname || !strcmp(model, nickname))
			nickname = NULL;
	}

	/* Serial numbers that are empty are not interesting */
	if (serial_nr && !*serial_nr)
		serial_nr = NULL;

	/* Firmware strings that are empty are not interesting */
	if (firmware && !*firmware)
		firmware = NULL;

	/* Do we have anything interesting about this dive computer to save? */
	if (!serial_nr && !nickname && !firmware)
		return;

	fprintf(f, "<divecomputerid");
	show_utf8(f, model, " model='", "'", 1);
	fprintf(f, " deviceid='%08x'", deviceid);
	show_utf8(f, serial_nr, " serial='", "'", 1);
	show_utf8(f, firmware, " firmware='", "'", 1);
	show_utf8(f, nickname, " nickname='", "'", 1);
	fprintf(f, "/>\n");
}

#define VERSION 2

void save_dives(const char *filename)
{
	save_dives_logic(filename, FALSE);
}

void save_dives_logic(const char *filename, const bool select_only)
{
	int i;
	struct dive *dive;
	dive_trip_t *trip;

	FILE *f = fopen(filename, "w");

	if (!f)
		return;

	fprintf(f, "<divelog program='subsurface' version='%d'>\n<settings>\n", VERSION);

	/* save the dive computer nicknames, if any */
	call_for_each_dc(f, save_one_device);
	if (autogroup)
		fprintf(f, "<autogroup state='1' />\n");
	fprintf(f, "</settings>\n<dives>\n");

	for (trip = dive_trip_list; trip != NULL; trip = trip->next)
		trip->index = 0;

	/* save the dives */
	for_each_dive(i, dive) {

		if (select_only) {

			if(!dive->selected)
				continue;
			save_dive(f, dive);

		} else {
			trip = dive->divetrip;

			/* Bare dive without a trip? */
			if (!trip) {
				save_dive(f, dive);
				continue;
			}

			/* Have we already seen this trip (and thus saved this dive?) */
			if (trip->index)
				continue;

			/* We haven't seen this trip before - save it and all dives */
			trip->index = 1;
			save_trip(f, trip);
		}
	}
	fprintf(f, "</dives>\n</divelog>\n");
	fclose(f);
}

void export_dives_uddf(const char *filename, const bool selected)
{
	FILE *f;
	size_t streamsize;
	char *membuf;
	xmlDoc *doc;
	xsltStylesheetPtr xslt = NULL;
	xmlDoc *transformed;

	if (!filename)
		return;

	/* Save XML to file and convert it into a memory buffer */
	save_dives_logic(filename, selected);
	f = fopen(filename, "r");
	fseek(f, 0, SEEK_END);
	streamsize = ftell(f);
	rewind(f);

	membuf = malloc(streamsize + 1);
	if (!membuf || !fread(membuf, streamsize, 1, f)) {
		fprintf(stderr, "Failed to read memory buffer\n");
		return;
	}
	membuf[streamsize] = 0;
	fclose(f);
	unlink(filename);

	/*
	 * Parse the memory buffer into XML document and
	 * transform it to UDDF format, finally dumping
	 * the XML into a character buffer.
	 */
	doc = xmlReadMemory(membuf, strlen(membuf), "divelog", NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to read XML memory\n");
		return;
	}
	free((void *)membuf);

	/* Convert to UDDF format */
	xslt = get_stylesheet("uddf-export.xslt");

	if (!xslt) {
		fprintf(stderr, "Failed to open UDDF conversion stylesheet\n");
		return;
	}
	transformed = xsltApplyStylesheet(xslt, doc, NULL);
	xsltFreeStylesheet(xslt);
	xmlFreeDoc(doc);

	/* Write the transformed XML to file */
	f = fopen(filename, "w");
	xmlDocFormatDump(f, transformed, 1);
	xmlFreeDoc(transformed);

	fclose(f);
}
