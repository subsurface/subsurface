#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "dive.h"

#define FRACTION(n,x) ((unsigned)(n)/(x)),((unsigned)(n)%(x))

static void show_milli(FILE *f, const char *pre, int value, const char *unit, const char *post)
{
	fputs(pre, f);
	if (value < 0) {
		putc('-', f);
		value = -value;
	}
	fprintf(f, "%u.%03u%s%s", FRACTION(value, 1000), unit, post);
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

/*
 * We're outputting utf8 in xml.
 * We need to quote the characters <, >, &.
 *
 * Technically I don't think we'd necessarily need to quote the control
 * characters, but at least libxml2 doesn't like them. It doesn't even
 * allow them quoted. So we just skip them and replace them with '?'.
 *
 * Nothing else (and if we ever do this using attributes, we'd need to
 * quote the quotes we use too).
 */
static void quote(FILE *f, const char *text)
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
		}
		fwrite(text, (p - text - 1), 1, f);
		if (!escape)
			break;
		fputs(escape, f);
		text = p;
	}
}

static void show_utf8(FILE *f, const char *text, const char *pre, const char *post)
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
	quote(f, text);
	fputs(post, f);
}

static void save_overview(FILE *f, struct dive *dive)
{
	show_depth(f, dive->maxdepth, "  <maxdepth>", "</maxdepth>\n");
	show_depth(f, dive->meandepth, "  <meandepth>", "</meandepth>\n");
	show_temperature(f, dive->airtemp, "  <airtemp>", "</airtemp>\n");
	show_temperature(f, dive->watertemp, "  <watertemp>", "</watertemp>\n");
	show_duration(f, dive->duration, "  <duration>", "</duration>\n");
	show_duration(f, dive->surfacetime, "  <surfacetime>", "</surfacetime>\n");
	show_pressure(f, dive->beginning_pressure, "  <cylinderstartpressure>", "</cylinderstartpressure>\n");
	show_pressure(f, dive->end_pressure, "  <cylinderendpressure>", "</cylinderendpressure>\n");
	show_utf8(f, dive->location, "  <location>","</location>\n");
	show_utf8(f, dive->notes, "  <notes>","</notes>\n");
}

static void save_cylinder_info(FILE *f, struct dive *dive)
{
	int i;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cylinder = dive->cylinder+i;
		int volume = cylinder->type.size.mliter;
		const char *description = cylinder->type.description;
		int o2 = cylinder->gasmix.o2.permille;
		int he = cylinder->gasmix.he.permille;

		/* No cylinder information at all? */
		if (!o2 && !volume)
			return;
		fprintf(f, "  <cylinder");
		if (o2) {
			fprintf(f, " o2='%u.%u%%'", FRACTION(o2, 10));
			if (he)
				fprintf(f, " he='%u.%u%%'", FRACTION(he, 10));
		}
		if (volume)
			show_milli(f, " size='", volume, " l", "'");
		if (description)
			fprintf(f, " description='%s'", description);
		fprintf(f, " />\n");
	}
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

static void save_dive(FILE *f, struct dive *dive)
{
	int i;
	struct tm *tm = gmtime(&dive->when);

	fprintf(f, "<dive date='%04u-%02u-%02u' time='%02u:%02u:%02u'>\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	save_overview(f, dive);
	save_cylinder_info(f, dive);
	for (i = 0; i < dive->samples; i++)
		save_sample(f, dive->sample+i);
	fprintf(f, "</dive>\n");
}

#define VERSION 1

void save_dives(const char *filename)
{
	int i;
	FILE *f = fopen(filename, "w");

	if (!f)
		return;

	/* Flush any edits of current dives back to the dives! */
	flush_dive_info_changes();

	fprintf(f, "<dives>\n<program name='diveclog' version='%d'></program>\n", VERSION);
	for (i = 0; i < dive_table.nr; i++)
		save_dive(f, get_dive(i));
	fprintf(f, "</dives>\n");
	fclose(f);
}
