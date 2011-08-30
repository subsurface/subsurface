#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static int verbose;

/*
 * Some silly typedefs to make our units very explicit.
 *
 * Also, the units are chosen so that values can be expressible as
 * integers, so that we never have FP rounding issues. And they
 * are small enough that converting to/from imperial units doesn't
 * really matter.
 *
 * We also strive to make '0' a meaningless number saying "not
 * initialized", since many values are things that may not have
 * been reported (eg tank pressure or temperature from dive
 * computers that don't support them). But sometimes -1 is an even
 * more explicit way of saying "not there".
 *
 * Thus "millibar" for pressure, for example, or "millikelvin" for
 * temperatures. Doing temperatures in celsius or fahrenheit would
 * make for loss of precision when converting from one to the other,
 * and using millikelvin is SI-like but also means that a temperature
 * of '0' is clearly just a missing temperature or tank pressure.
 *
 * Also strive to use units that can not possibly be mistaken for a
 * valid value in a "normal" system without conversion. If the max
 * depth of a dive is '20000', you probably didn't convert from mm on
 * output, or if the max depth gets reported as "0.2ft" it was either
 * a really boring dive, or there was some missing input conversion,
 * and a 60-ft dive got recorded as 60mm.
 *
 * Doing these as "structs containing value" means that we always
 * have to explicitly write out those units in order to get at the
 * actual value. So there is hopefully little fear of using a value
 * in millikelvin as Fahrenheit by mistake.
 *
 * We don't actually use these all yet, so maybe they'll change, but
 * I made a number of types as guidelines.
 */
typedef struct {
	int seconds;
} duration_t;

typedef struct {
	int mm;
} depth_t;

typedef struct {
	int mbar;
} pressure_t;

typedef struct {
	int mkelvin;
} temperature_t;

typedef struct {
	int mliter;
} volume_t;

typedef struct {
	int permille;
} fraction_t;

typedef struct {
	int grams;
} weight_t;

typedef struct {
	fraction_t o2;
	fraction_t n2;
	fraction_t he2;
} gasmix_t;

typedef struct {
	volume_t size;
	pressure_t pressure;
} tank_type_t;

static int to_feet(depth_t depth)
{
	return depth.mm * 0.00328084 + 0.5;
}

static int to_C(temperature_t temp)
{
	if (!temp.mkelvin)
		return 0;
	return (temp.mkelvin - 273150) / 1000;
}

static int to_PSI(pressure_t pressure)
{
	return pressure.mbar * 0.0145037738 + 0.5;
}

struct sample {
	duration_t time;
	depth_t depth;
	temperature_t temperature;
	pressure_t tankpressure;
	int tankindex;
};

struct dive {
	time_t when;
	depth_t maxdepth, meandepth;
	duration_t duration, surfacetime;
	depth_t visibility;
	temperature_t airtemp, watertemp;
	pressure_t beginning_pressure, end_pressure;
	int samples;
	struct sample sample[];
};

static void record_dive(struct dive *dive)
{
	int i;
	static int nr;
	struct tm *tm;

	tm = gmtime(&dive->when);

	printf("Dive %d with %d samples at %02d:%02d:%02d %04d-%02d-%02d\n",
		++nr, dive->samples,
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday);
	for (i = 0; i < dive->samples; i++) {
		struct sample *s = dive->sample + i;

		printf("%4d:%02d: %3d ft, %2d C, %4d PSI\n",
			s->time.seconds / 60,
			s->time.seconds % 60,
			to_feet(s->depth),
			to_C(s->temperature),
			to_PSI(s->tankpressure));
	}
}

static void nonmatch(const char *type, const char *fullname, const char *name, char *buffer)
{
	if (verbose)
		printf("Unable to match %s '(%.*s)%s' (%s)\n", type,
			(int) (name - fullname), fullname, name,
			buffer);
	free(buffer);
}

static const char *last_part(const char *name)
{
	const char *p = strrchr(name, '.');
	return p ? p+1 : name;
}

typedef void (*matchfn_t)(char *buffer, void *);

static int match(const char *pattern, const char *name, matchfn_t fn, char *buf, void *data)
{
	if (strcasecmp(pattern, name))
		return 0;
	fn(buf, data);
	return 1;
}

/*
 * Dive info as it is being built up..
 */
static int alloc_samples;
static struct dive *dive;
static struct sample *sample;
static struct tm tm;

static time_t utc_mktime(struct tm *tm)
{
	static const int mdays[] = {
	    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	int year = tm->tm_year;
	int month = tm->tm_mon;
	int day = tm->tm_mday;

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

	if (sscanf(buffer, "%d.%d.%d", &d, &m, &y) == 3) {
		tm.tm_year = y;
		tm.tm_mon = m-1;
		tm.tm_mday = d;
		if (tm.tm_sec | tm.tm_min | tm.tm_hour)
			*when = utc_mktime(&tm);
	}
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

union int_or_float {
	long i;
	double fp;
};

enum number_type {
	NEITHER,
	INTEGER,
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

	res->i = val;
	return INTEGER;
}

static void pressure(char *buffer, void *_press)
{
	pressure_t *pressure = _press;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case FLOAT:
		/* Maybe it's in Bar? */
		if (val.fp < 500.0) {
			pressure->mbar = val.fp * 1000;
			break;
		}
		printf("Unknown fractional pressure reading %s\n", buffer);
		break;

	case INTEGER:
		/*
		 * Random integer? Maybe in PSI? Or millibar already?
		 *
		 * We assume that 5 bar is a ridiculous tank pressure,
		 * so if it's smaller than 5000, it's in PSI..
		 */
		if (val.i < 5000) {
			pressure->mbar = val.i * 68.95;
			break;
		}
		pressure->mbar = val.i;
		break;
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
	/* Integer values are probably in feet */
	case INTEGER:
		depth->mm = 304.8 * val.i;
		break;
	/* Float? Probably meters.. */
	case FLOAT:
		depth->mm = val.fp * 1000;
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
	/* C or F? Who knows? Let's default to Celsius */
	case INTEGER:
		val.fp = val.i;
		/* Fallthrough */
	case FLOAT:
		/* Ignore zero. It means "none" */
		if (!val.fp)
			break;
		/* Celsius */
		if (val.fp < 50.0) {
			temperature->mkelvin = (val.fp + 273.16) * 1000;
			break;
		}
		/* Fahrenheit */
		if (val.fp < 212.0) {
			temperature->mkelvin = (val.fp + 459.67) * 5000/9;
			break;
		}
		/* Kelvin or already millikelvin */
		if (val.fp < 1000.0)
			val.fp *= 1000;
		temperature->mkelvin = val.fp;
		break;
	default:
		printf("Strange temperature reading %s\n", buffer);
	}
	free(buffer);
}

static void sampletime(char *buffer, void *_time)
{
	duration_t *time = _time;
	union int_or_float val;

	switch (integer_or_float(buffer, &val)) {
	case INTEGER:
		time->seconds = val.i;
		break;
	default:
		printf("Strange sample time reading %s\n", buffer);
	}
	free(buffer);
}

/* We're in samples - try to convert the random xml value to something useful */
static void try_to_fill_sample(struct sample *sample, const char *name, char *buf)
{
	const char *last = last_part(name);

	if (match("pressure", last, pressure, buf, &sample->tankpressure))
		return;
	if (match("cylpress", last, pressure, buf, &sample->tankpressure))
		return;
	if (match("depth", last, depth, buf, &sample->depth))
		return;
	if (match("temperature", last, temperature, buf, &sample->temperature))
		return;
	if (match("sampletime", last, sampletime, buf, &sample->time))
		return;
	if (match("time", last, sampletime, buf, &sample->time))
		return;

	nonmatch("sample", name, last, buf);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, char *buf)
{
	const char *last = last_part(name);

	if (match("date", last, divedate, buf, &dive->when))
		return;
	if (match("time", last, divetime, buf, &dive->when))
		return;
	nonmatch("dive", name, last, buf);
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

static void dive_end(void)
{
	if (!dive)
		return;
	record_dive(dive);
	dive = NULL;
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
}

static void sample_end(void)
{
	sample = NULL;
	if (!dive)
		return;
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

static void traverse(xmlNode *node)
{
	xmlNode *n;

	for (n = node; n; n = n->next) {
		/* XML from libdivecomputer: 'dive' per new dive */
		if (!strcmp(n->name, "dive")) {
			dive_start();
			traverse(n->children);
			dive_end();
			continue;
		}

		/*
		 * At least both libdivecomputer and Suunto
		 * agree on "sample".
		 *
		 * Well - almost. Ignore case.
		 */
		if (!strcasecmp(n->name, "sample")) {
			sample_start();
			traverse(n->children);
			sample_end();
			continue;
		}

		/* Anything else - just visit it and recurse */
		visit_one_node(n);
		traverse(n->children);
	}
}

static void parse(const char *filename)
{
	xmlDoc *doc;

	doc = xmlReadFile(filename, NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to parse '%s'.\n", filename);
		return;
	}

	dive_start();
	traverse(xmlDocGetRootElement(doc));
	dive_end();
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

static void parse_argument(const char *arg)
{
	const char *p = arg+1;

	do {
		switch (*p) {
		case 'v':
			verbose++;
			continue;
		default:
			fprintf(stderr, "Bad argument '%s'\n", arg);
			exit(1);
		}
	} while (*++p);
}

int main(int argc, char **argv)
{
	int i;

	LIBXML_TEST_VERSION

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		parse(a);
	}
	return 0;
}
