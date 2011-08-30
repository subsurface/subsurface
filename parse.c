#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

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
	static int nr;

	printf("Recording dive %d with %d samples\n", ++nr, dive->samples);
}

static void nonmatch(const char *type, const char *fullname, const char *name, int size, const char *buffer)
{
	printf("Unable to match %s '(%.*s)%s' (%.*s)\n", type,
		(int) (name - fullname), fullname, name,
		size, buffer);
}

static const char *last_part(const char *name)
{
	const char *p = strrchr(name, '.');
	return p ? p+1 : name;
}

/* We're in samples - try to convert the random xml value to something useful */
static void try_to_fill_sample(struct sample *sample, const char *name, int size, const char *buffer)
{
	const char *last = last_part(name);
	nonmatch("sample", name, last, size, buffer);
}

/* We're in the top-level dive xml. Try to convert whatever value to a dive value */
static void try_to_fill_dive(struct dive *dive, const char *name, int size, const char *buffer)
{
	const char *last = last_part(name);
	nonmatch("dive", name, last, size, buffer);
}

/*
 * Dive info as it is being built up..
 */
static int alloc_samples;
static struct dive *dive;
static struct sample *sample;

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

	alloc_samples = 5;
	size = dive_size(alloc_samples);
	dive = malloc(size);
	if (!dive)
		exit(1);
	memset(dive, 0, size);
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
}

static void sample_end(void)
{
	sample = NULL;
	if (!dive)
		return;
	dive->samples++;
}

static void entry(const char *name, int size, const char *buffer)
{
	if (sample) {
		try_to_fill_sample(sample, name, size, buffer);
		return;
	}
	if (dive) {
		try_to_fill_dive(dive, name, size, buffer);
		return;
	}
}

static const char *nodename(xmlNode *node, char *buf, int len)
{
	/* Don't print out the node name if it is "text" */
	if (!strcmp(node->name, "text")) {
		node = node->parent;
		if (!node || !node->name)
			return "root";
	}

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

int main(int argc, char **argv)
{
	int i;

	LIBXML_TEST_VERSION

	for (i = 1; i < argc; i++)
		parse(argv[i]);
	return 0;
}
