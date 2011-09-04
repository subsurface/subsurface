#include <string.h>
#include <stdio.h>

#include "dive.h"

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
	int old = depth->mm;

	if (abs(old - new) > 1000)
		depth->mm = new;
}

struct dive *fixup_dive(struct dive *dive)
{
	int i;
	double depthtime = 0;
	int lasttime = 0;
	int start = -1, end = -1;
	int startpress = 0, endpress = 0;
	int maxdepth = 0, mintemp = 0;
	int lastdepth = 0;

	for (i = 0; i < dive->samples; i++) {
		struct sample *sample = dive->sample + i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;
		int press = sample->cylinderpressure.mbar;
		int temp = sample->temperature.mkelvin;

		if (lastdepth)
			end = time;

		if (depth) {
			if (start < 0)
				start = lasttime;
			if (depth > maxdepth)
				maxdepth = depth;
		}
		if (press) {
			endpress = press;
			if (!startpress)
				startpress = press;
		}
		if (temp) {
			if (!mintemp || temp < mintemp)
				mintemp = temp;
		}
		depthtime += (time - lasttime) * (lastdepth + depth) / 2;
		lastdepth = depth;
		lasttime = time;
	}
	if (end < 0)
		return dive;
	dive->duration.seconds = end - start;
	if (start != end)
		update_depth(&dive->meandepth, depthtime / (end - start));
	if (startpress)
		dive->beginning_pressure.mbar = startpress;
	if (endpress)
		dive->end_pressure.mbar = endpress;
	if (mintemp)
		dive->watertemp.mkelvin = mintemp;

	if (maxdepth)
		update_depth(&dive->maxdepth, maxdepth);

	return dive;
}

/* Don't pick a zero for MERGE_MIN() */
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MERGE_MAX(res, a, b, n) res->n = MAX(a->n, b->n)
#define MERGE_MIN(res, a, b, n) res->n = (a->n)?(b->n)?MIN(a->n, b->n):(a->n):(b->n)

static int alloc_samples;

static struct dive *add_sample(struct sample *sample, int time, struct dive *dive)
{
	int nr = dive->samples;
	struct sample *d;

	if (nr >= alloc_samples) {
		alloc_samples = (alloc_samples + 64) * 3 / 2;
		dive = realloc(dive, dive_size(alloc_samples));
		if (!dive)
			return NULL;
	}
	dive->samples = nr+1;
	d = dive->sample + nr;

	*d = *sample;
	d->time.seconds = time;
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

/* Pick whichever has any info (if either). Prefer 'a' */
static void merge_cylinder_type(cylinder_type_t *res, cylinder_type_t *a, cylinder_type_t *b)
{
	if (a->size.mliter)
		b = a;
	*res = *b;
}

static void merge_cylinder_mix(gasmix_t *res, gasmix_t *a, gasmix_t *b)
{
	if (a->o2.permille)
		b = a;
	*res = *b;
}

static void merge_cylinder_info(cylinder_t *res, cylinder_t *a, cylinder_t *b)
{
	merge_cylinder_type(&res->type, &a->type, &b->type);
	merge_cylinder_mix(&res->gasmix, &a->gasmix, &b->gasmix);
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

	if (a->when != b->when)
		return NULL;

	alloc_samples = 5;
	res = malloc(dive_size(alloc_samples));
	if (!res)
		return NULL;
	memset(res, 0, dive_size(alloc_samples));

	res->when = a->when;
	res->name = merge_text(a->name, b->name);
	res->location = merge_text(a->location, b->location);
	res->notes = merge_text(a->notes, b->notes);
	MERGE_MAX(res, a, b, maxdepth.mm);
	res->meandepth.mm = 0;
	MERGE_MAX(res, a, b, duration.seconds);
	MERGE_MAX(res, a, b, surfacetime.seconds);
	MERGE_MAX(res, a, b, airtemp.mkelvin);
	MERGE_MIN(res, a, b, watertemp.mkelvin);
	MERGE_MAX(res, a, b, beginning_pressure.mbar);
	MERGE_MAX(res, a, b, end_pressure.mbar);
	for (i = 0; i < MAX_CYLINDERS; i++)
		merge_cylinder_info(res->cylinder+i, a->cylinder + i, b->cylinder + i);

	return merge_samples(res, a, b, 0);
}
