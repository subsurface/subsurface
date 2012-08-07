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
	remember_event(name);
}

int get_pressure_units(unsigned int mb, const char **units)
{
	int pressure;
	const char* unit;

	switch (output_units.pressure) {
	case PASCAL:
		pressure = mb * 100;
		unit = "pascal";
		break;
	case BAR:
		pressure = (mb + 500) / 1000;
		unit = "bar";
		break;
	case PSI:
		pressure = mbar_to_PSI(mb);
		unit = "psi";
		break;
	}
	if (units)
		*units = unit;
	return pressure;
}

double get_temp_units(unsigned int mk, const char **units)
{
	double deg;
	const char *unit;

	if (output_units.temperature == FAHRENHEIT) {
		deg = mkelvin_to_F(mk);
		unit = UTF8_DEGREE "F";
	} else {
		deg = mkelvin_to_C(mk);
		unit = UTF8_DEGREE "C";
	}
	if (units)
		*units = unit;
	return deg;
}

double get_volume_units(unsigned int ml, int *frac, const char **units)
{
	int decimals;
	double vol;
	const char *unit;

	switch (output_units.volume) {
	case LITER:
		vol = ml / 1000.0;
		unit = "l";
		decimals = 1;
		break;
	case CUFT:
		vol = ml_to_cuft(ml);
		unit = "cuft";
		decimals = 2;
		break;
	}
	if (frac)
		*frac = decimals;
	if (units)
		*units = unit;
	return vol;
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

double get_weight_units(unsigned int grams, int *frac, const char **units)
{
	int decimals;
	double value;
	const char* unit;

	if (output_units.weight == LBS) {
		value = grams_to_lbs(grams);
		unit = "lbs";
		decimals = 0;
	} else {
		value = grams / 1000.0;
		unit = "kg";
		decimals = 1;
	}
	if (frac)
		*frac = decimals;
	if (units)
		*units = unit;
	return value;
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

void finish_sample(struct dive *dive)
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

static void fixup_pressure(struct dive *dive, struct sample *sample)
{
	unsigned int pressure, index;
	cylinder_t *cyl;

	pressure = sample->cylinderpressure.mbar;
	if (!pressure)
		return;
	index = sample->cylinderindex;
	if (index >= MAX_CYLINDERS)
		return;
	cyl = dive->cylinder + index;
	if (!cyl->sample_start.mbar)
		cyl->sample_start.mbar = pressure;
	cyl->sample_end.mbar = pressure;
}

/*
 * If the cylinder tank pressures are within half a bar
 * (about 8 PSI) of the sample pressures, we consider it
 * to be a rounding error, and throw them away as redundant.
 */
static int same_rounded_pressure(pressure_t a, pressure_t b)
{
	return abs(a.mbar - b.mbar) <= 500;
}

static void sanitize_gasmix(struct gasmix *mix)
{
	unsigned int o2, he;

	o2 = mix->o2.permille;
	he = mix->he.permille;

	/* Regular air: leave empty */
	if (!he) {
		if (!o2)
			return;
		/* 20.9% or 21% O2 is just air */
		if (o2 >= 209 && o2 <= 210) {
			mix->o2.permille = 0;
			return;
		}
	}

	/* Sane mix? */
	if (o2 <= 1000 && he <= 1000 && o2+he <= 1000)
		return;
	fprintf(stderr, "Odd gasmix: %d O2 %d He\n", o2, he);
	memset(mix, 0, sizeof(*mix));
}

/*
 * See if the size/workingpressure looks like some standard cylinder
 * size, eg "AL80".
 */
static void match_standard_cylinder(cylinder_type_t *type)
{
	double cuft;
	int psi, len;
	const char *fmt;
	char buffer[20], *p;

	/* Do we already have a cylinder description? */
	if (type->description)
		return;

	cuft = ml_to_cuft(type->size.mliter);
	cuft *= to_ATM(type->workingpressure);
	psi = to_PSI(type->workingpressure);

	switch (psi) {
	case 2300 ... 2500:	/* 2400 psi: LP tank */
		fmt = "LP%d";
		break;
	case 2600 ... 2700:	/* 2640 psi: LP+10% */
		fmt = "LP%d";
		break;
	case 2900 ... 3100:	/* 3000 psi: ALx tank */
		fmt = "AL%d";
		break;
	case 3400 ... 3500:	/* 3442 psi: HP tank */
		fmt = "HP%d";
		break;
	case 3700 ... 3850:	/* HP+10% */
		fmt = "HP%d+";
		break;
	default:
		return;
	}
	len = snprintf(buffer, sizeof(buffer), fmt, (int) (cuft+0.5));
	p = malloc(len+1);
	if (!p)
		return;
	memcpy(p, buffer, len+1);
	type->description = p;
}


/*
 * There are two ways to give cylinder size information:
 *  - total amount of gas in cuft (depends on working pressure and physical size)
 *  - physical size
 *
 * where "physical size" is the one that actually matters and is sane.
 *
 * We internally use physical size only. But we save the workingpressure
 * so that we can do the conversion if required.
 */
static void sanitize_cylinder_type(cylinder_type_t *type)
{
	double volume_of_air, atm, volume;

	/* If we have no working pressure, it had *better* be just a physical size! */
	if (!type->workingpressure.mbar)
		return;

	/* No size either? Nothing to go on */
	if (!type->size.mliter)
		return;

	if (input_units.volume == CUFT) {
		/* confusing - we don't really start from ml but millicuft !*/
		volume_of_air = cuft_to_l(type->size.mliter);
		atm = to_ATM(type->workingpressure);		/* working pressure in atm */
		volume = volume_of_air / atm;			/* milliliters at 1 atm: "true size" */
		type->size.mliter = volume + 0.5;
	}

	/* Ok, we have both size and pressure: try to match a description */
	match_standard_cylinder(type);
}

static void sanitize_cylinder_info(struct dive *dive)
{
	int i;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		sanitize_gasmix(&dive->cylinder[i].gasmix);
		sanitize_cylinder_type(&dive->cylinder[i].type);
	}
}

struct dive *fixup_dive(struct dive *dive)
{
	int i,j;
	double depthtime = 0;
	int lasttime = 0;
	int lastindex = -1;
	int start = -1, end = -1;
	int maxdepth = 0, mintemp = 0;
	int lastdepth = 0;
	int lasttemp = 0, lastpressure = 0;
	int pressure_delta[MAX_CYLINDERS] = {INT_MAX, };

	sanitize_cylinder_info(dive);
	for (i = 0; i < dive->samples; i++) {
		struct sample *sample = dive->sample + i;
		int time = sample->time.seconds;
		int depth = sample->depth.mm;
		int temp = sample->temperature.mkelvin;
		int pressure = sample->cylinderpressure.mbar;
		int index = sample->cylinderindex;

		if (index == lastindex) {
			/* Remove duplicate redundant pressure information */
			if (pressure == lastpressure)
				sample->cylinderpressure.mbar = 0;
			/* check for simply linear data in the samples
			   +INT_MAX means uninitialized, -INT_MAX means not linear */
			if (pressure_delta[index] != -INT_MAX && lastpressure) {
				if (pressure_delta[index] == INT_MAX) {
					pressure_delta[index] = abs(pressure - lastpressure);
				} else {
					int cur_delta = abs(pressure - lastpressure);
					if (cur_delta && abs(cur_delta - pressure_delta[index]) > 150) {
						/* ok the samples aren't just a linearisation
						 * between start and end */
						pressure_delta[index] = -INT_MAX;
					}
				}
			}
		}
		lastindex = index;
		lastpressure = pressure;

		if (lastdepth)
			end = time;

		if (depth) {
			if (start < 0)
				start = lasttime;
			if (depth > maxdepth)
				maxdepth = depth;
		}

		fixup_pressure(dive, sample);

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
	/* if all the samples for a cylinder have pressure data that
	 * is basically equidistant throw out the sample cylinder pressure
	 * information but make sure we still have a valid start and end
	 * pressure
	 * this happens when DivingLog decides to linearalize the
	 * pressure between beginning and end and for strange reasons
	 * decides to put that in the sample data as if it came from
	 * the dive computer; we don't want that (we'll visualize with
	 * constant SAC rate instead)
	 * WARNING WARNING - I have only seen this in single tank dives
	 * --- maybe I should try to create a multi tank dive and see what
	 * --- divinglog does there - but the code right now is only tested
	 * --- for the single tank case */
	for (j = 0; j < MAX_CYLINDERS; j++) {
		if (abs(pressure_delta[j]) != INT_MAX) {
			cylinder_t *cyl = dive->cylinder + j;
			for (i = 0; i < dive->samples; i++)
				if (dive->sample[i].cylinderindex == j)
					dive->sample[i].cylinderpressure.mbar = 0;
			if (! cyl->start.mbar)
				cyl->start.mbar = cyl->sample_start.mbar;
			if (! cyl->end.mbar)
				cyl->end.mbar = cyl->sample_end.mbar;
			cyl->sample_start.mbar = 0;
			cyl->sample_end.mbar = 0;
		}
	}
	if (end < 0)
		return dive;

	update_duration(&dive->duration, end - start);
	if (start != end)
		depthtime /= (end - start);

	update_depth(&dive->meandepth, depthtime);
	update_temperature(&dive->watertemp, mintemp);
	update_depth(&dive->maxdepth, maxdepth);

	add_people(dive->buddy);
	add_people(dive->divemaster);
	add_location(dive->location);
	for (i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = dive->cylinder + i;
		add_cylinder_description(&cyl->type);
		if (same_rounded_pressure(cyl->sample_start, cyl->start))
			cyl->start.mbar = 0;
		if (same_rounded_pressure(cyl->sample_end, cyl->end))
			cyl->end.mbar = 0;
	}
	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		weightsystem_t *ws = dive->weightsystem + i;
		add_weightsystem_description(ws);
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
	finish_sample(dive);
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
	MERGE_MAX(res, a, b, rating);
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
