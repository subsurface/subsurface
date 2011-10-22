/* dive.c */
/* maintains the internal dive list structure */
#include <string.h>
#include <stdio.h>

#include "dive.h"

void add_event(struct dive *dive, int time, int type, int flags, int value, const char *name)
{
	struct event *ev, **p;
	unsigned int size, len = strlen(name);

	size = sizeof(*ev) + len + 1;
	ev = malloc(size);
	if (!ev)
		return;
	memset(ev, 0, size);
	memcpy(ev->name, name, len);
	ev->time.seconds = time;
	ev->type = type;
	ev->flags = flags;
	ev->value = value;
	ev->next = NULL;

	p = &dive->events;
	while (*p)
		p = &(*p)->next;
	*p = ev;
}

double get_depth_units(unsigned int mm, int *frac, const char **units)
{
	int decimals;
	double d;
	const char *unit;

	switch (output_units.length) {
	case METERS:
		d = mm / 1000.0;
		unit = "m";
		decimals = d < 20;
		break;
	case FEET:
		d = mm_to_feet(mm);
		unit = "ft";
		decimals = 0;
		break;
	}
	if (frac)
		*frac = decimals;
	if (units)
		*units = unit;
	return d;
}

struct dive *alloc_dive(void)
{
	const int initial_samples = 5;
	unsigned int size;
	struct dive *dive;

	size = dive_size(initial_samples);
	dive = malloc(size);
	if (!dive)
		exit(1);
	memset(dive, 0, size);
	dive->alloc_samples = initial_samples;
	return dive;
}

struct sample *prepare_sample(struct dive **divep)
{
	struct dive *dive = *divep;
	if (dive) {
		int nr = dive->samples;
		int alloc_samples = dive->alloc_samples;
		struct sample *sample;
		if (nr >= alloc_samples) {
			unsigned int size;

			alloc_samples = (alloc_samples * 3)/2 + 10;
			size = dive_size(alloc_samples);
			dive = realloc(dive, size);
			if (!dive)
				return NULL;
			dive->alloc_samples = alloc_samples;
			*divep = dive;
		}
		sample = dive->sample + nr;
		memset(sample, 0, sizeof(*sample));
		return sample;
	}
	return NULL;
}

void finish_sample(struct dive *dive, struct sample *sample)
{
	dive->samples++;
}

/*
 * So when we re-calculate maxdepth and meandepth, we will
 * not override the old numbers if they are close to the
 * new ones.
 *
 * Why? Because a dive computer may well actually track the
 * max depth and mean depth at finer granularity than the
 * samples it stores. So it's possible that the max and mean
 * have been reported more correctly originally.
 *
 * Only if the values calculated from the samples are clearly
 * different do we override the normal depth values.
 *
 * This considers 1m to be "clearly different". That's
 * a totally random number.
 */
static void update_depth(depth_t *depth, int new)
{
	if (new) {
		int old = depth->mm;

		if (abs(old - new) > 1000)
			depth->mm = new;
	}
}

static void update_duration(duration_t *duration, int new)
{
	if (new)
		duration->seconds = new;
}

static void update_temperature(temperature_t *temperature, int new)
{
	if (new) {
		int old = temperature->mkelvin;

		if (abs(old - new) > 1000)
			temperature->mkelvin = new;
	}
}

/*
 * If you have more than 32 cylinders, you'd better have a 64-bit build.
 * And if you have more than 64 cylinders, you need to use another tool,
 * or fix this up to do something odd.
 */
static unsigned long fixup_pressure(struct dive *dive, struct sample *sample, unsigned long flags)
{
	unsigned long mask;
	unsigned int pressure, index;
	cylinder_t *cyl;

	pressure = sample->cylinderpressure.mbar;
	if (!pressure)
		return flags;
	index = sample->cylinderindex;
	if (index >= MAX_CYLINDERS)
		return flags;
	cyl = dive->cylinder + index;
	if (!cyl->start.mbar)
		cyl->start.mbar = pressure;
	/*
	 * If we already have an end pressure, without
	 * ever having seen a sample for this cylinder,
	 * that means that somebody set the end pressure
	 * by hand
	 */
	mask = 1ul << index;
	if (cyl->end.mbar) {
		if (!(flags & mask))
			return flags;
	}
	flags |= mask;

	/* we need to handle the user entering beginning and end tank pressures
	 * - maybe even IF we have samples. But for now if we have air pressure
	 * data in the samples, we use that instead of the minimum
	 * if (!cyl->end.mbar || pressure < cyl->end.mbar)
	 */
	cyl->end.mbar = pressure;
	return flags;
}

struct dive *fixup_dive(struct dive *dive)
{
	int i;
	double depthtime = 0;
	int lasttime = 0;
	int start = -1, end = -1;
	int maxdepth = 0, mintemp = 0;
	int lastdepth = 0;
	int lasttemp = 0;
	unsigned long flags = 0;

	for (i = 0; i < dive->samples; i++) {
		struct sample *sample = dive->sample + i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;
		int temp = sample->temperature.mkelvin;

		if (lastdepth)
			end = time;

		if (depth) {
			if (start < 0)
				start = lasttime;
			if (depth > maxdepth)
				maxdepth = depth;
		}

		flags = fixup_pressure(dive, sample, flags);

		if (temp) {
			/*
			 * If we have consecutive identical
			 * temperature readings, throw away
			 * the redundant ones.
			 */
			if (lasttemp == temp)
				sample->temperature.mkelvin = 0;
			else
				lasttemp = temp;

			if (!mintemp || temp < mintemp)
				mintemp = temp;
		}
		depthtime += (time - lasttime) * (lastdepth + depth) / 2;
		lastdepth = depth;
		lasttime = time;
	}
	if (end < 0)
		return dive;

	update_duration(&dive->duration, end - start);
	if (start != end)
		depthtime /= (end - start);

	update_depth(&dive->meandepth, depthtime);
	update_temperature(&dive->watertemp, mintemp);
	update_depth(&dive->maxdepth, maxdepth);

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_type_t *type = &dive->cylinder[i].type;
		add_cylinder_description(type);
	}

	return dive;
}

/* Don't pick a zero for MERGE_MIN() */
#define MERGE_MAX(res, a, b, n) res->n = MAX(a->n, b->n)
#define MERGE_MIN(res, a, b, n) res->n = (a->n)?(b->n)?MIN(a->n, b->n):(a->n):(b->n)
#define MERGE_TXT(res, a, b, n) res->n = merge_text(a->n, b->n)
#define MERGE_NONZERO(res, a, b, n) res->n = a->n ? a->n : b->n

static struct dive *add_sample(struct sample *sample, int time, struct dive *dive)
{
	struct sample *p = prepare_sample(&dive);

	if (!p)
		return NULL;
	*p = *sample;
	p->time.seconds = time;
	finish_sample(dive, p);
	return dive;
}

/*
 * Merge samples. Dive 'a' is "offset" seconds before Dive 'b'
 */
static struct dive *merge_samples(struct dive *res, struct dive *a, struct dive *b, int offset)
{
	int asamples = a->samples;
	int bsamples = b->samples;
	struct sample *as = a->sample;
	struct sample *bs = b->sample;

	for (;;) {
		int at, bt;
		struct sample sample;

		if (!res)
			return NULL;

		at = asamples ? as->time.seconds : -1;
		bt = bsamples ? bs->time.seconds + offset : -1;

		/* No samples? All done! */
		if (at < 0 && bt < 0)
			return fixup_dive(res);

		/* Only samples from a? */
		if (bt < 0) {
add_sample_a:
			res = add_sample(as, at, res);
			as++;
			asamples--;
			continue;
		}

		/* Only samples from b? */
		if (at < 0) {
add_sample_b:
			res = add_sample(bs, bt, res);
			bs++;
			bsamples--;
			continue;
		}

		if (at < bt)
			goto add_sample_a;
		if (at > bt)
			goto add_sample_b;

		/* same-time sample: add a merged sample. Take the non-zero ones */
		sample = *bs;
		if (as->depth.mm)
			sample.depth = as->depth;
		if (as->temperature.mkelvin)
			sample.temperature = as->temperature;
		if (as->cylinderpressure.mbar)
			sample.cylinderpressure = as->cylinderpressure;
		if (as->cylinderindex)
			sample.cylinderindex = as->cylinderindex;

		res = add_sample(&sample, at, res);

		as++;
		bs++;
		asamples--;
		bsamples--;
	}
}

static char *merge_text(const char *a, const char *b)
{
	char *res;

	if (!a || !*a)
		return (char *)b;
	if (!b || !*b)
		return (char *)a;
	if (!strcmp(a,b))
		return (char *)a;
	res = malloc(strlen(a) + strlen(b) + 9);
	if (!res)
		return (char *)a;
	sprintf(res, "(%s) or (%s)", a, b);
	return res;
}

#define SORT(a,b,field) \
	if (a->field != b->field) return a->field < b->field ? -1 : 1

static int sort_event(struct event *a, struct event *b)
{
	SORT(a,b,time.seconds);
	SORT(a,b,type);
	SORT(a,b,flags);
	SORT(a,b,value);
	return strcmp(a->name, b->name);
}

static void merge_events(struct dive *res, struct dive *src1, struct dive *src2, int offset)
{
	struct event *a, *b;
	struct event **p = &res->events;

	a = src1->events;
	b = src2->events;
	while (b) {
		b->time.seconds += offset;
		b = b->next;
	}
	b = src2->events;

	while (a || b) {
		int s;
		if (!b) {
			*p = a;
			break;
		}
		if (!a) {
			*p = b;
			break;
		}
		s = sort_event(a, b);
		/* Pick b */
		if (s > 0) {
			*p = b;
			p = &b->next;
			b = b->next;
			continue;
		}
		/* Pick 'a' or neither */
		if (s < 0) {
			*p = a;
			p = &a->next;
		}
		a = a->next;
		continue;
	}
}

/* Pick whichever has any info (if either). Prefer 'a' */
static void merge_cylinder_type(cylinder_type_t *res, cylinder_type_t *a, cylinder_type_t *b)
{
	if (a->size.mliter)
		b = a;
	*res = *b;
}

static void merge_cylinder_mix(struct gasmix *res, struct gasmix *a, struct gasmix *b)
{
	if (a->o2.permille)
		b = a;
	*res = *b;
}

static void merge_cylinder_info(cylinder_t *res, cylinder_t *a, cylinder_t *b)
{
	merge_cylinder_type(&res->type, &a->type, &b->type);
	merge_cylinder_mix(&res->gasmix, &a->gasmix, &b->gasmix);
	MERGE_MAX(res, a, b, start.mbar);
	MERGE_MIN(res, a, b, end.mbar);
}

/*
 * This could do a lot more merging. Right now it really only
 * merges almost exact duplicates - something that happens easily
 * with overlapping dive downloads.
 */
struct dive *try_to_merge(struct dive *a, struct dive *b)
{
	int i;
	struct dive *res;

	if ((a->when >= b->when + 60) || (a->when <= b->when - 60))
		return NULL;

	res = alloc_dive();

	res->when = a->when;
	MERGE_NONZERO(res, a, b, latitude);
	MERGE_NONZERO(res, a, b, longitude);
	MERGE_TXT(res, a, b, location);
	MERGE_TXT(res, a, b, notes);
	MERGE_TXT(res, a, b, buddy);
	MERGE_TXT(res, a, b, divemaster);
	MERGE_MAX(res, a, b, number);
	MERGE_MAX(res, a, b, maxdepth.mm);
	res->meandepth.mm = 0;
	MERGE_MAX(res, a, b, duration.seconds);
	MERGE_MAX(res, a, b, surfacetime.seconds);
	MERGE_MAX(res, a, b, airtemp.mkelvin);
	MERGE_MIN(res, a, b, watertemp.mkelvin);
	for (i = 0; i < MAX_CYLINDERS; i++)
		merge_cylinder_info(res->cylinder+i, a->cylinder + i, b->cylinder + i);
	merge_events(res, a, b, 0);
	return merge_samples(res, a, b, 0);
}
