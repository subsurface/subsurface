#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "dive.h"

#define FRACTION(n,x) ((unsigned)(n)/(x)),((unsigned)(n)%(x))

static void show_temperature(FILE *f, temperature_t temp, const char *pre, const char *post)
{
	if (temp.mkelvin) {
		int mcelsius = temp.mkelvin - 273150;
		const char *sign ="";
		if (mcelsius < 0) {
			sign = "-";
			mcelsius = - mcelsius;
		}
		fprintf(f, "%s%s%u.%03u%s", pre, sign, FRACTION(mcelsius, 1000), post);
	}
}

static void show_depth(FILE *f, depth_t depth, const char *pre, const char *post)
{
	if (depth.mm)
		fprintf(f, "%s%u.%03u%s", pre, FRACTION(depth.mm, 1000), post);
}

static void show_duration(FILE *f, duration_t duration, const char *pre, const char *post)
{
	if (duration.seconds)
		fprintf(f, "%s%u:%03u%s", pre, FRACTION(duration.seconds, 60), post);
}

static void show_pressure(FILE *f, pressure_t pressure, const char *pre, const char *post)
{
	if (pressure.mbar)
		fprintf(f, "%s%u.%03u%s", pre, FRACTION(pressure.mbar, 1000), post);
}

static void save_overview(FILE *f, struct dive *dive)
{
	show_depth(f, dive->maxdepth, "  <maxdepth>", " m</maxdepth>\n");
	show_depth(f, dive->meandepth, "  <meandepth>", " m</meandepth>\n");
	show_temperature(f, dive->airtemp, "  <airtemp>", " C</airtemp>\n");
	show_temperature(f, dive->watertemp, "  <watertemp>", " C</airtemp>\n");
	show_duration(f, dive->duration, "  <duration>", " min</duration>\n");
	show_duration(f, dive->surfacetime, "  <surfacetime>", " min</surfacetime>\n");
	show_pressure(f, dive->beginning_pressure, "  <cylinderstartpressure>", " bar</cylinderstartpressure>\n");
	show_pressure(f, dive->end_pressure, "  <cylinderendpressure>", " bar</cylinderendpressure>\n");
}

static void save_gasmix(FILE *f, struct dive *dive)
{
	int i;

	for (i = 0; i < MAX_MIXES; i++) {
		gasmix_t *mix = dive->gasmix+i;
		int o2 = mix->o2.permille, he = mix->he.permille;
		int n2 = 1000 - o2 - he;

		if (!mix->o2.permille)
			return;
		fprintf(f, "  <gasmix o2='%u.%u%%'", FRACTION(o2, 10));
		if (mix->he.permille)
			fprintf(f, " he='%u.%u%%'", FRACTION(he, 10));
		fprintf(f, " n2='%u.%u%%'></gasmix>\n", FRACTION(n2, 10));
	}
}

static void save_sample(FILE *f, struct sample *sample)
{
	fprintf(f, "  <sample time='%u:%02u' depth='%u.%03u'",
		FRACTION(sample->time.seconds,60),
		FRACTION(sample->depth.mm, 1000));
	show_temperature(f, sample->temperature, " temp='", " C'");
	show_pressure(f, sample->tankpressure, " pressure='", " bar'");
	if (sample->tankindex)
		fprintf(f, " tankindex='%d'", sample->tankindex);
	fprintf(f, "></sample>\n");
}

static void save_dive(FILE *f, struct dive *dive)
{
	int i;
	struct tm *tm = gmtime(&dive->when);

	fprintf(f, "<dive date='%02u.%02u.%u' time='%02u:%02u:%02u'>\n",
		tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	save_overview(f, dive);
	save_gasmix(f, dive);
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
	fprintf(f, "<dives>\n<program name='diveclog' version='%d'></program>\n", VERSION);
	for (i = 0; i < dive_table.nr; i++)
		save_dive(f, get_dive(i));
	fprintf(f, "</dives>\n");
}
