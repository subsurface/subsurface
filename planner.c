/* planner.c
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */

#include "dive.h"
#include "divelist.h"

int stoplevels[] = { 3000, 6000, 9000, 12000, 15000, 21000, 30000, 42000, 60000, 90000 };

struct divedatapoint {
	int time;
	int depth;
	int o2;
	int he;
	struct divedatapoint *next;
};

struct diveplan {
	timestamp_t when;
	int surface_pressure;
	struct divedatapoint *dp;
};

/* returns the tissue tolerance at the end of this (partial) dive */
double tissue_at_end(struct dive *dive)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i, j, t0, t1;
	double tissue_tolerance;

	if (!dive)
		return 0.0;
	tissue_tolerance = init_decompression(dive);

	dc = &dive->dc;
	if (!dc->samples)
		return 0.0;
	psample = sample = dc->sample;
	t0 = 0;
	for (i = 0; i < dc->samples; i++, sample++) {
		t1 = sample->time.seconds;
		for (j = t0; j < t1; j++) {
			int depth = psample->depth.mm + (j - t0) * (sample->depth.mm - psample->depth.mm) / (t1 - t0);
			tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
						&dive->cylinder[sample->sensor].gasmix, 1, sample->po2);
		}
		psample = sample;
		t0 = t1;
	}
	return tissue_tolerance;
}

/* how many seconds until we can ascend to the next stop? */
int time_at_last_depth(struct dive *dive, int next_stop)
{
	int depth;
	double surface_pressure, tissue_tolerance;
	int wait = 0;
	struct sample *sample;

	if (!dive)
		return 0;
	surface_pressure = dive->surface_pressure.mbar / 1000.0;
	tissue_tolerance = tissue_at_end(dive);
	sample = &dive->dc.sample[dive->dc.samples - 1];
	depth = sample->depth.mm;
	while (deco_allowed_depth(tissue_tolerance, surface_pressure, dive, 1) > next_stop) {
		wait++;
		tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0,
					&dive->cylinder[sample->sensor].gasmix, 1, sample->po2);
	}
	return wait;
}

int add_gas(struct dive *dive, int o2, int he)
{
	int i;
	struct gasmix *mix;
	cylinder_t *cyl;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cyl = dive->cylinder + i;
		if (cylinder_nodata(cyl))
			break;
		mix = &cyl->gasmix;
		if (o2 == mix->o2.permille && he == mix->he.permille)
			return i;
	}
	if (i == MAX_CYLINDERS) {
		printf("too many cylinders\n");
		return -1;
	}
	mix = &cyl->gasmix;
	mix->o2.permille = o2;
	mix->he.permille = he;
	return i;
}

struct dive *create_dive_from_plan(struct diveplan *diveplan)
{
	struct dive *dive;
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	int gasused = 0;
	int t = 0;
	int lastdepth = 0;

	if (!diveplan || !diveplan->dp)
		return NULL;
	dive = alloc_dive();
	dive->when = diveplan->when;
	dive->surface_pressure.mbar = diveplan->surface_pressure;
	dc = &dive->dc;
	dc->model = "Simulated Dive";
	dp = diveplan->dp;
	while (dp) {
		int i, depth;

		if (dp->o2 != dive->cylinder[gasused].gasmix.o2.permille ||
		    dp->he != dive->cylinder[gasused].gasmix.he.permille)
			gasused = add_gas(dive, dp->o2, dp->he);

		for (i = t; i < dp->time; i += 10) {
			depth = lastdepth + (i - t) * (dp->depth - lastdepth) / (dp->time - t);
			sample = prepare_sample(dc);
			sample->time.seconds = i;
			sample->depth.mm = depth;
			sample->sensor = gasused;
			dc->samples++;
		}
		sample = prepare_sample(dc);
		sample->time.seconds = dp->time;
		sample->depth.mm = dp->depth;
		sample->sensor = gasused;
		lastdepth = dp->depth;
		t = dp->time;
		dp = dp->next;
		dc->samples++;
	}
	return dive;
}

struct divedatapoint *create_dp(int time_incr, int depth, int o2, int he)
{
	struct divedatapoint *dp;

	dp = malloc(sizeof(struct divedatapoint));
	dp->time = time_incr;
	dp->depth = depth;
	dp->o2 = o2;
	dp->he = he;
	dp->next = NULL;
	return dp;
}

void add_to_end_of_diveplan(struct diveplan *diveplan, struct divedatapoint *dp)
{
	struct divedatapoint **lastdp = &diveplan->dp;
	struct divedatapoint *ldp = *lastdp;
	while(*lastdp) {
		ldp = *lastdp;
		lastdp = &(*lastdp)->next;
	}
	*lastdp = dp;
	if (ldp)
		dp->time += ldp->time;
}

void plan_add_segment(struct diveplan *diveplan, int duration, int depth, int o2, int he)
{
	struct divedatapoint *dp = create_dp(duration, depth, o2, he);
	add_to_end_of_diveplan(diveplan, dp);
}

void plan(struct diveplan *diveplan)
{
	struct dive *dive;
	struct sample *sample;
	int wait_time, o2, he;
	int ceiling, depth, transitiontime;
	int stopidx;
	double tissue_tolerance;

	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = 1013;
	dive = create_dive_from_plan(diveplan);
	record_dive(dive);

	sample = &dive->dc.sample[dive->dc.samples - 1];
	o2 = dive->cylinder[sample->sensor].gasmix.o2.permille;
	he = dive->cylinder[sample->sensor].gasmix.he.permille;

	tissue_tolerance = tissue_at_end(dive);
	ceiling = deco_allowed_depth(tissue_tolerance, diveplan->surface_pressure / 1000.0, dive, 1);

	for (stopidx = 0; stopidx < sizeof(stoplevels) / sizeof(int); stopidx++)
		if (stoplevels[stopidx] >= ceiling)
			break;

	while (stopidx >= 0) {
		depth = dive->dc.sample[dive->dc.samples - 1].depth.mm;
		if (depth > stoplevels[stopidx]) {
			transitiontime = (depth - stoplevels[stopidx]) / 150;
			plan_add_segment(diveplan, transitiontime, stoplevels[stopidx], o2, he);
			/* re-create the dive */
			delete_single_dive(dive_table.nr - 1);
			dive = create_dive_from_plan(diveplan);
			record_dive(dive);
		}
		wait_time = time_at_last_depth(dive, stoplevels[stopidx - 1]);
		if (wait_time)
			plan_add_segment(diveplan, wait_time, stoplevels[stopidx], o2, he);
		transitiontime = (stoplevels[stopidx] - stoplevels[stopidx - 1]) / 150;
		plan_add_segment(diveplan, transitiontime, stoplevels[stopidx - 1], o2, he);
		/* re-create the dive */
		delete_single_dive(dive_table.nr - 1);
		dive = create_dive_from_plan(diveplan);
		record_dive(dive);
		stopidx--;
	}
	/* now make the dive visible as last dive of the dive list */
	report_dives(FALSE, FALSE);
}

void test_planner()
{
	struct diveplan diveplan = {};
	int end_of_last_dive = dive_table.dives[dive_table.nr - 1]->when + dive_table.dives[dive_table.nr -1]->duration.seconds;
	diveplan.when = end_of_last_dive + 50 * 3600; /* don't take previous dives into account for deco calculation */
	diveplan.surface_pressure = 1013;
	plan_add_segment(&diveplan, 120, 36000, 209, 0);
	plan_add_segment(&diveplan, 1800, 36000, 209, 0);
	plan_add_segment(&diveplan, 59, 27000, 209, 0);
	plan_add_segment(&diveplan, 1, 27000, 400, 0);

	plan(&diveplan);
}
