#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "dive.h"

int verbose;

struct dive_table dive_table;

/*
 * Add a dive into the dive_table array
 */
static void record_dive(struct dive *dive)
{
	int nr = dive_table.nr, allocated = dive_table.allocated;
	struct dive **dives = dive_table.dives;

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		dives = realloc(dives, allocated * sizeof(struct dive *));
		if (!dives)
			exit(1);
		dive_table.dives = dives;
		dive_table.allocated = allocated;
	}
	dives[nr] = dive;
	dive_table.nr = nr+1;
}

static void start_match(const char *type, const char *name, char *buffer)
{
	if (verbose > 2)
		printf("Matching %s '%s' (%s)\n",
			type, name, buffer);
}

static void nonmatch(const char *type, const char *name, char *buffer)
{
	if (verbose > 1)
		printf("Unable to match %s '%s' (%s)\n",
			type, name, buffer);
	free(buffer);
}

typedef void (*matchfn_t)(char *buffer, void *);

static int match(const char *pattern, int plen,
		 const char *name, int nlen,
		 matchfn_t fn, char *buf, void *data)
{
	if (plen > nlen)
		return 0;
	if (memcmp(pattern, name + nlen - plen, plen))
		return 0;
	fn(buf, data);
	return 1;
}

/*
 * We keep our internal data in well-specified units, but
 * the input may come in some random format. This keeps track
 * of the incoming units.
 */
static struct units {
	enum { METERS, FEET } length;
	enum { LITER, CUFT } volume;
	enum { BAR, PSI } pressure;
	enum { CELSIUS, FAHRENHEIT } temperature;
	enum { KG, LBS } weight;
} units;

/* We're going to default to SI units for input */
static const struct units SI_units = {
	.length = METERS,
	.volume = LITER,
	.pressure = BAR,
	.temperature = CELSIUS,
	.weight = KG
};

/*
 * Dive info as it is being built up..
 */
static int alloc_samples;
static struct dive *dive;
static struct sample *sample;
static struct tm tm;
static int suunto, uemis;
static int event_index, gasmix_index;

static time_t utc_mktime(struct tm *tm)
{
	static const int mdays[] = {
	    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	int year = tm->tm_year;
	int month = tm->tm_mon;
	int day = tm->tm_mday;

	/* First normalize relative to 1900 */
	if (year < 70)
		year += 100;
	else if (year > 1900)
		year -= 1900;

	/* Normalized to Jan 1, 1970: unix time */
	year -= 70;

	if (year < 0 || year > 129) /* algo only works for 1970-2099 */
		return -1;
	if (month < 0 || month > 11) /* array bounds */
		return -1;
	if (month < 2 || (year + 2) % 4)
		day--;
	if (tm->tm_hour < 0 || tm->tm_min < 0 || tm->tm_sec < 0)
		return -1;
	return (year * 365 + (year + 1) / 4 + mdays[month] + day) * 24*60*60UL +
		tm->tm_hour * 60*60 + tm->tm_min * 60 + tm->tm_sec;
}

static void divedate(char *buffer, void *_when)
{
	int d,m,y;
	time_t *when = _when;
	int success = 0;

	success = tm.tm_sec | tm.tm_min | tm.tm_hour;
	if (sscanf(buffer, "%d.%d.%d", &d, &m, &y) == 3) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
	} else if (sscanf(buffer, "%d-%d-%d", &y, &m, &d) == 3) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
	} else {
		fprintf(stderr, "Unable to parse date '%s'\n", buffer);
		success = 0;
	}

	if (success)
		*when = utc_mktime(&tm);

	free(buffer);
}

static void divetime(char *buffer, void *_when)
{
	int h,m,s = 0;
	time_t *when = _when;

	if (sscanf(buffer, "%d:%d:%d", &h, &m, &s) >= 2) {
		tm.tm_hour = h;
		tm.tm_min = m;
		tm.tm_sec = s;
		if (tm.tm_year)
			*when = utc_mktime(&tm);
	}
	free(buffer);
}

/* Libdivecomputer: "2011-03-20 10:22:38" */
static void divedatetime(char *buffer, void *_when)
{
	int y,m,d;
	int hr,min,sec;
	time_t *when = _when;

	if (sscanf(buffer, "%d-%d-%d %d:%d:%d",
		&y, &m, &d, &hr, &min, &sec) == 6) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
		tm.tm_hour = hr;
		tm.tm_min = min;
		tm.tm_sec = sec;
		*when = utc_mktime(&tm);
	}
	free(buffer);
}

union int_or_float {
	double fp;
};

enum number_type {
	NEITHER,
	FLOAT
};

static enum number_type integer_or_float(char *buffer, union int_or_float *res)
{
	char *end;
	long val;
	double fp;

	/* Integer or floating point? */
	val = strtol(buffer, &end, 10);
	if (val < 0 || end == buffer)
		return NEITHER;

	/* Looks like it might be floating point? */
	if (*end == '.') {
		errno = 0;
		fp = strtod(buffer, &end);
		if (!errno) {
			res->fp = fp;
			return FLOAT;
		}
	}

	res->fp = val;
	return FLOAT;
}

static void pressure(char *buffer, void *_press)
{
	double mbar;
	pressure_t *pressure = _press;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Just ignore zero values */
		if (!val.fp)
			break;
		switch (units.pressure) {
		case BAR:
			/* Assume mbar, but if it's really small, it's bar */
			mbar = val.fp;
			if (mbar < 5000)
				mbar = mbar * 1000;
			break;
		case PSI:
			mbar = val.fp * 68.95;
			break;
		}
		if (mbar > 5 && mbar < 500000) {
			pressure->mbar = mbar + 0.5;
			break;
		}
	/* fallthrough */
	default:
		printf("Strange pressure reading %s\n", buffer);
	}
	free(buffer);
}

static void depth(char *buffer, void *_depth)
{
	depth_t *depth = _depth;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		switch (units.length) {
		case METERS:
			depth->mm = val.fp * 1000 + 0.5;
			break;
		case FEET:
			depth->mm = val.fp * 304.8 + 0.5;
			break;
		}
		break;
	default:
		printf("Strange depth reading %s\n", buffer);
	}
	free(buffer);
}

static void temperature(char *buffer, void *_temperature)
{
	temperature_t *temperature = _temperature;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Ignore zero. It means "none" */
		if (!val.fp)
			break;
		/* Celsius */
		switch (units.temperature) {
		case CELSIUS:
			temperature->mkelvin = (val.fp + 273.15) * 1000 + 0.5;
			break;
		case FAHRENHEIT:
			temperature->mkelvin = (val.fp + 459.67) * 5000/9;
			break;
		}
		break;
	default:
		printf("Strange temperature reading %s\n", buffer);
	}
	free(buffer);
}

static void sampletime(char *buffer, void *_time)
{
	int i;
	int min, sec;
	duration_t *time = _time;

	i = sscanf(buffer, "%d:%d", &min, &sec);
	switch (i) {
	case 1:
		sec = min;
		min = 0;
	/* fallthrough */
	case 2:
		time->seconds = sec + min*60;
		break;
	default:
		printf("Strange sample time reading %s\n", buffer);
	}
	free(buffer);
}

static void duration(char *buffer, void *_time)
{
	sampletime(buffer, _time);
}

static void percent(char *buffer, void *_fraction)
{
	fraction_t *fraction = _fraction;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		if (val.fp <= 100.0)
			fraction->permille = val.fp * 10 + 0.5;
		break;

	default:
		printf("Strange percentage reading %s\n", buffer);
		break;
	}
	free(buffer);
}

static void gasmix(char *buffer, void *_fraction)
{
	/* libdivecomputer does negative percentages. */
	if (*buffer == '-')
		return;
	if (gasmix_index < MAX_MIXES)
		percent(buffer, _fraction);
}

static void gasmix_nitrogen(char *buffer, void *_gasmix)
{
	/* Ignore n2 percentages. There's no value in them. */
}

static void utf8_string(char *buffer, void *_res)
{
	*(char **)_res = buffer;
}

/*
 * Uemis water_pressure. In centibar. And when converting to
 * depth, I'm just going to always use saltwater, because I
 * think "true depth" is just stupid. From a diving standpoint,
 * "true depth" is pretty much completely pointless, unless
 * you're doing some kind of underwater surveying work.
 *
 * So I give water depths in "pressure depth", always assuming
 * salt water. So one atmosphere per 10m.
 */
static void water_pressure(char *buffer, void *_depth)
{
	depth_t *depth = _depth;
        union int_or_float val;
	float atm;

        switch (integer_or_float(buffer, &val)) {
        case FLOAT:
		switch (units.pressure) {
		case BAR:
			/* It's actually centibar! */
			atm = (val.fp / 100) / 1.01325;
			break;
		case PSI:
			/* I think it's centiPSI too. Crazy. */
			atm = (val.fp / 100) * 0.0680459639;
			break;
		}
		/* 10 m per atm */
		depth->mm = 10000 * atm;
		break;
	default:
		fprintf(stderr, "Strange water pressure '%s'\n", buffer);
	}
	free(buffer);
}

#define MATCH(pattern, fn, dest) \
	match(pattern, strlen(pattern), name, len, fn, buf, dest)

static int uemis_fill_sample(struct sample *sample, const char *name, int len, char *buf)
{
	return	MATCH(".reading.dive_time", sampletime, &sample->time) ||
		MATCH(".reading.water_pressure", water_pressure, &sample->depth);
}

/* We're in samples - try to convert the random xml value to something useful */
static void try_to_fill_sample(struct sample *sample, const char *name, char *buf)
{
	int len = strlen(name);

	start_match("sample", name, buf);
	if (MATCH(".sample.pressure", pressure, &sample->tankpressure))
		return;
	if (MATCH(".sample.cylpress", pressure, &sample->tankpressure))
		return;
	if (MATCH(".sample.depth", depth, &sample->depth))
		return;
	if (MATCH(".sample.temp", temperature, &sample->temperature))
		return;
	if (MATCH(".sample.temperature", temperature, &sample->temperature))
		return;
	if (MATCH(".sample.sampletime", sampletime, &sample->time))
		return;
	if (MATCH(".sample.time", sampletime, &sample->time))
		return;

	if (uemis) {
		if (uemis_fill_sample(sample, name, len, buf))
			return;
	}

	nonmatch("sample", name, buf);
}

/*
 * Crazy suunto xml. Look at how those o2/he things match up.
 */
static int suunto_dive_match(struct dive *dive, const char *name, int len, char *buf)
{
	return	MATCH(".o2pct", percent, &dive->gasmix[0].o2) ||
		MATCH(".hepct_0", percent, &dive->gasmix[0].he) ||
		MATCH(".o2pct_2", percent, &dive->gasmix[1].o2) ||
		MATCH(".hepct_1", percent, &dive->gasmix[1].he) ||
		MATCH(".o2pct_3", percent, &dive->gasmix[2].o2) ||
		MATCH(".hepct_2", percent, &dive->gasmix[2].he) ||
		MATCH(".o2pct_4", percent, &dive->gasmix[3].o2) ||
		MATCH(".hepct_3", percent, &dive->gasmix[3].he);
}

static int buffer_value(char *buffer)
{
	int val = atoi(buffer);
	free(buffer);
	return val;
}

static void uemis_length_unit(char *buffer, void *_unused)
{
	units.length = buffer_value(buffer) ? FEET : METERS;
}

static void uemis_volume_unit(char *buffer, void *_unused)
{
	units.volume = buffer_value(buffer) ? CUFT : LITER;
}

static void uemis_pressure_unit(char *buffer, void *_unused)
{
	units.pressure = buffer_value(buffer) ? PSI : BAR;
}

static void uemis_temperature_unit(char *buffer, void *_unused)
{
	units.temperature = buffer_value(buffer) ? FAHRENHEIT : CELSIUS;
}

static void uemis_weight_unit(char *buffer, void *_unused)
{
	units.weight = buffer_value(buffer) ? LBS : KG;
}

static void uemis_time_unit(char *buffer, void *_unused)
{
}

static void uemis_date_unit(char *buffer, void *_unused)
{
}

/* Modified julian day, yay! */
static void uemis_date_time(char *buffer, void *_when)
{
	time_t *when = _when;
        union int_or_float val;

        switch (integer_or_float(buffer, &val)) {
        case FLOAT:
		*when = (val.fp - 40587.5) * 86400;
		break;
	default:
		fprintf(stderr, "Strange julian date: %s", buffer);
	}
	free(buffer);
}

/*
 * Uemis doesn't know time zones. You need to do them as
 * minutes, not hours.
 *
 * But that's ok, we don't track timezones yet either. We
 * just turn everything into "localtime expressed as UTC".
 */
static void uemis_time_zone(char *buffer, void *_when)
{
	time_t *when = _when;
	signed char tz = atoi(buffer);

	*when += tz * 3600;
}

static int uemis_dive_match(struct dive *dive, const char *name, int len, char *buf)
{
	return	MATCH(".units.length", uemis_length_unit, &units) ||
		MATCH(".units.volume", uemis_volume_unit, &units) ||
		MATCH(".units.pressure", uemis_pressure_unit, &units) ||
		MATCH(".units.temperature", uemis_temperature_unit, &units) ||
		MATCH(".units.weight", uemis_weight_unit, &units) ||
		MATCH(".units.time", uemis_time_unit, &units) ||
		MATCH(".units.date", uemis_date_unit, &units) ||
		MATCH(".date_time", uemis_date_time, &dive->when) ||
		MATCH(".time_zone", uemis_time_zone, &dive->when);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, char *buf)
{
	int len = strlen(name);

	start_match("dive", name, buf);
	if (MATCH(".date", divedate, &dive->when))
		return;
	if (MATCH(".time", divetime, &dive->when))
		return;
	if (MATCH(".datetime", divedatetime, &dive->when))
		return;
	if (MATCH(".maxdepth", depth, &dive->maxdepth))
		return;
	if (MATCH(".meandepth", depth, &dive->meandepth))
		return;
	if (MATCH(".duration", duration, &dive->duration))
		return;
	if (MATCH(".divetime", duration, &dive->duration))
		return;
	if (MATCH(".divetimesec", duration, &dive->duration))
		return;
	if (MATCH(".surfacetime", duration, &dive->surfacetime))
		return;
	if (MATCH(".airtemp", temperature, &dive->airtemp))
		return;
	if (MATCH(".watertemp", temperature, &dive->watertemp))
		return;
	if (MATCH(".cylinderstartpressure", pressure, &dive->beginning_pressure))
		return;
	if (MATCH(".cylinderendpressure", pressure, &dive->end_pressure))
		return;
	if (MATCH(".location", utf8_string, &dive->location))
		return;
	if (MATCH(".notes", utf8_string, &dive->notes))
		return;

	if (MATCH(".o2", gasmix, &dive->gasmix[gasmix_index].o2))
		return;
	if (MATCH(".n2", gasmix_nitrogen, &dive->gasmix[gasmix_index]))
		return;
	if (MATCH(".he", gasmix, &dive->gasmix[gasmix_index].he))
		return;

	/* Suunto XML files are some crazy sh*t. */
	if (suunto && suunto_dive_match(dive, name, len, buf))
		return;

	if (uemis && uemis_dive_match(dive, name, len, buf))
		return;

	nonmatch("dive", name, buf);
}

static unsigned int dive_size(int samples)
{
	return sizeof(struct dive) + samples*sizeof(struct sample);
}

/*
 * File boundaries are dive boundaries. But sometimes there are
 * multiple dives per file, so there can be other events too that
 * trigger a "new dive" marker and you may get some nesting due
 * to that. Just ignore nesting levels.
 */
static void dive_start(void)
{
	unsigned int size;

	if (dive)
		return;

	alloc_samples = 5;
	size = dive_size(alloc_samples);
	dive = malloc(size);
	if (!dive)
		exit(1);
	memset(dive, 0, size);
	memset(&tm, 0, sizeof(tm));
}

static char *generate_name(struct dive *dive)
{
	int len;
	struct tm *tm;
	char buffer[256], *p;

	tm = gmtime(&dive->when);

	len = snprintf(buffer, sizeof(buffer),
		"%04d-%02d-%02d "
		"%02d:%02d:%02d "
		"(%d ft, %d min)",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		to_feet(dive->maxdepth), dive->duration.seconds / 60);
	p = malloc(len+1);
	if (!p)
		exit(1);
	memcpy(p, buffer, len+1);
	return p;
}

static void sanitize_gasmix(struct dive *dive)
{
	int i;

	for (i = 0; i < MAX_MIXES; i++) {
		gasmix_t *mix = dive->gasmix+i;
		unsigned int o2, he;

		o2 = mix->o2.permille;
		he = mix->he.permille;

		/* Regular air: leave empty */
		if (!he) {
			if (!o2)
				continue;
			/* 20.9% or 21% O2 is just air */
			if (o2 >= 209 && o2 <= 210) {
				mix->o2.permille = 0;
				continue;
			}
		}

		/* Sane mix? */
		if (o2 <= 1000 && he <= 1000 && o2+he <= 1000)
			continue;
		fprintf(stderr, "Odd gasmix: %d O2 %d He\n", o2, he);
		memset(mix, 0, sizeof(*mix));
	}
}

static void dive_end(void)
{
	if (!dive)
		return;
	if (!dive->name)
		dive->name = generate_name(dive);
	sanitize_gasmix(dive);
	record_dive(dive);
	dive = NULL;
	gasmix_index = 0;
}

static void suunto_start(void)
{
	suunto++;
	units = SI_units;
}

static void suunto_end(void)
{
	suunto--;
}

static void uemis_start(void)
{
	uemis++;
	units = SI_units;
}

static void uemis_end(void)
{
}

static void event_start(void)
{
}

static void event_end(void)
{
	event_index++;
}

static void gasmix_start(void)
{
}

static void gasmix_end(void)
{
	gasmix_index++;
}

static void sample_start(void)
{
	int nr;

	if (!dive)
		return;
	nr = dive->samples;
	if (nr >= alloc_samples) {
		unsigned int size;

		alloc_samples = (alloc_samples * 3)/2 + 10;
		size = dive_size(alloc_samples);
		dive = realloc(dive, size);
		if (!dive)
			return;
	}
	sample = dive->sample + nr;
	memset(sample, 0, sizeof(*sample));
	event_index = 0;
}

static void sample_end(void)
{
	if (!dive)
		return;

	if (sample->time.seconds > dive->duration.seconds) {
		if (sample->depth.mm)
			dive->duration = sample->time;
	}

	if (sample->depth.mm > dive->maxdepth.mm)
		dive->maxdepth.mm = sample->depth.mm;

	if (sample->temperature.mkelvin) {
		if (!dive->watertemp.mkelvin || dive->watertemp.mkelvin > sample->temperature.mkelvin)
			dive->watertemp = sample->temperature;
	}

	sample = NULL;
	dive->samples++;
}

static void entry(const char *name, int size, const char *raw)
{
	char *buf = malloc(size+1);

	if (!buf)
		return;
	memcpy(buf, raw, size);
	buf[size] = 0;
	if (sample) {
		try_to_fill_sample(sample, name, buf);
		return;
	}
	if (dive) {
		try_to_fill_dive(dive, name, buf);
		return;
	}
}

static const char *nodename(xmlNode *node, char *buf, int len)
{
	if (!node || !node->name)
		return "root";

	buf += len;
	*--buf = 0;
	len--;

	for(;;) {
		const char *name = node->name;
		int i = strlen(name);
		while (--i >= 0) {
			unsigned char c = name[i];
			*--buf = tolower(c);
			if (!--len)
				return buf;
		}
		node = node->parent;
		if (!node || !node->name)
			return buf;
		*--buf = '.';
		if (!--len)
			return buf;
	}
}

#define MAXNAME 64

static void visit_one_node(xmlNode *node)
{
	int len;
	const unsigned char *content;
	char buffer[MAXNAME];
	const char *name;

	content = node->content;
	if (!content)
		return;

	/* Trim whitespace at beginning */
	while (isspace(*content))
		content++;

	/* Trim whitespace at end */
	len = strlen(content);
	while (len && isspace(content[len-1]))
		len--;

	if (!len)
		return;

	/* Don't print out the node name if it is "text" */
	if (!strcmp(node->name, "text"))
		node = node->parent;

	name = nodename(node, buffer, sizeof(buffer));

	entry(name, len, content);
}

static void traverse(xmlNode *root);

static void traverse_properties(xmlNode *node)
{
	xmlAttr *p;

	for (p = node->properties; p; p = p->next)
		traverse(p->children);
}

static void visit(xmlNode *n)
{
	visit_one_node(n);
	traverse_properties(n);
	traverse(n->children);
}

/*
 * I'm sure this could be done as some fancy DTD rules.
 * It's just not worth the headache.
 */
static struct nesting {
	const char *name;
	void (*start)(void), (*end)(void);
} nesting[] = {
	{ "dive", dive_start, dive_end },
	{ "SUUNTO", suunto_start, suunto_end },
	{ "sample", sample_start, sample_end },
	{ "SAMPLE", sample_start, sample_end },
	{ "reading", sample_start, sample_end },
	{ "event", event_start, event_end },
	{ "gasmix", gasmix_start, gasmix_end },
	{ "pre_dive", uemis_start, uemis_end },
	{ NULL, }
};

static void traverse(xmlNode *root)
{
	xmlNode *n;

	for (n = root; n; n = n->next) {
		struct nesting *rule = nesting;

		do {
			if (!strcmp(rule->name, n->name))
				break;
			rule++;
		} while (rule->name);

		if (rule->start)
			rule->start();
		visit(n);
		if (rule->end)
			rule->end();
	}
}

/* Per-file reset */
static void reset_all(void)
{
	/*
	 * We reset the units for each file. You'd think it was
	 * a per-dive property, but I'm not going to trust people
	 * to do per-dive setup. If the xml does have per-dive
	 * data within one file, we might have to reset it per
	 * dive for that format.
	 */
	units = SI_units;
	suunto = 0;
	uemis = 0;
}

void parse_xml_file(const char *filename)
{
	xmlDoc *doc;

	doc = xmlReadFile(filename, NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to parse '%s'.\n", filename);
		return;
	}

	reset_all();
	dive_start();
	traverse(xmlDocGetRootElement(doc));
	dive_end();
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

void parse_xml_init(void)
{
	LIBXML_TEST_VERSION
}
