/* planner.c
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "dive.h"
#include "divelist.h"
#include "planner.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"

#define TIMESTEP 3 /* second */
#define DECOTIMESTEP 60 /* seconds. Unit of deco stop times */

int decostoplevels[] = { 0, 3000, 6000, 9000, 12000, 15000, 18000, 21000, 24000, 27000,
				  30000, 33000, 36000, 39000, 42000, 45000, 48000, 51000, 54000, 57000,
				  60000, 63000, 66000, 69000, 72000, 75000, 78000, 81000, 84000, 87000,
				  90000, 100000, 110000, 120000, 130000, 140000, 150000, 160000, 170000,
				  180000, 190000, 200000, 220000, 240000, 260000, 280000, 300000,
				  320000, 340000, 360000, 380000 };
double plangflow, plangfhigh;
bool plan_verbatim = false, plan_display_runtime = true, plan_display_duration = false, plan_display_transitions = false;

const char *disclaimer;

#if DEBUG_PLAN
void dump_plan(struct diveplan *diveplan)
{
	struct divedatapoint *dp;
	struct tm tm;

	if (!diveplan) {
		printf("Diveplan NULL\n");
		return;
	}
	utc_mkdate(diveplan->when, &tm);

	printf("\nDiveplan @ %04d-%02d-%02d %02d:%02d:%02d (surfpres %dmbar):\n",
	       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec,
	       diveplan->surface_pressure);
	dp = diveplan->dp;
	while (dp) {
		printf("\t%3u:%02u: %dmm gas: %d o2 %d h2\n", FRACTION(dp->time, 60), dp->depth, get_o2(&dp->gasmix), get_he(&dp->gasmix));
		dp = dp->next;
	}
}
#endif

bool diveplan_empty(struct diveplan *diveplan)
{
	struct divedatapoint *dp;
	if (!diveplan || !diveplan->dp)
		return true;
	dp = diveplan->dp;
	while (dp) {
		if (dp->time)
			return false;
		dp = dp->next;
	}
	return true;
}

void set_last_stop(bool last_stop_6m)
{
	if (last_stop_6m == true)
		decostoplevels[1] = 6000;
	else
		decostoplevels[1] = 3000;
}

void set_verbatim(bool verbatim)
{
	plan_verbatim = verbatim;
}

void set_display_runtime(bool display)
{
	plan_display_runtime = display;
}

void set_display_duration(bool display)
{
	plan_display_duration = display;
}

void set_display_transitions(bool display)
{
	plan_display_transitions = display;
}

/* get the gas at a certain time during the dive */
void get_gas_at_time(struct dive *dive, struct divecomputer *dc, duration_t time, struct gasmix *gas)
{
	// we always start with the first gas, so that's our gas
	// unless an event tells us otherwise
	struct event *event = dc->events;
	*gas = dive->cylinder[0].gasmix;
	while (event && event->time.seconds <= time.seconds) {
		if (!strcmp(event->name, "gaschange")) {
			int cylinder_idx = get_cylinder_index(dive, event);
			*gas = dive->cylinder[cylinder_idx].gasmix;
		}
		event = event->next;
	}
}

int get_gasidx(struct dive *dive, struct gasmix *mix)
{
	int gasidx = -1;

	while (++gasidx < MAX_CYLINDERS)
		if (gasmix_distance(&dive->cylinder[gasidx].gasmix, mix) < 100)
			return gasidx;
	return -1;
}

double interpolate_transition(struct dive *dive, duration_t t0, duration_t t1, depth_t d0, depth_t d1, const struct gasmix *gasmix, o2pressure_t po2)
{
	int j;
	double tissue_tolerance = 0.0;

	for (j = t0.seconds; j < t1.seconds; j++) {
		int depth = interpolate(d0.mm, d1.mm, j - t0.seconds, t1.seconds - t0.seconds);
		tissue_tolerance = add_segment(depth_to_mbar(depth, dive) / 1000.0, gasmix, 1, po2.mbar, dive, prefs.bottomsac);
	}
	return tissue_tolerance;
}

/* returns the tissue tolerance at the end of this (partial) dive */
double tissue_at_end(struct dive *dive, char **cached_datap)
{
	struct divecomputer *dc;
	struct sample *sample, *psample;
	int i;
	depth_t lastdepth = {};
	duration_t t0 = {}, t1 = {};
	double tissue_tolerance;
	struct gasmix gas;

	if (!dive)
		return 0.0;
	if (*cached_datap) {
		tissue_tolerance = restore_deco_state(*cached_datap);
	} else {
		tissue_tolerance = init_decompression(dive);
		cache_deco_state(tissue_tolerance, cached_datap);
	}
	dc = &dive->dc;
	if (!dc->samples)
		return tissue_tolerance;
	psample = sample = dc->sample;

	for (i = 0; i < dc->samples; i++, sample++) {
		t1 = sample->time;
		get_gas_at_time(dive, dc, t0, &gas);
		if (i > 0)
			lastdepth = psample->depth;
		tissue_tolerance = interpolate_transition(dive, t0, t1, lastdepth, sample->depth, &gas, sample->setpoint);
		psample = sample;
		t0 = t1;
	}
	return tissue_tolerance;
}


/* if a default cylinder is set, use that */
void fill_default_cylinder(cylinder_t *cyl)
{
	const char *cyl_name = prefs.default_cylinder;
	struct tank_info_t *ti = tank_info;
	pressure_t pO2 = {.mbar = 1600};

	if (!cyl_name)
		return;
	while (ti->name != NULL) {
		if (strcmp(ti->name, cyl_name) == 0)
			break;
		ti++;
	}
	if (ti->name == NULL)
		/* didn't find it */
		return;
	cyl->type.description = strdup(ti->name);
	if (ti->ml) {
		cyl->type.size.mliter = ti->ml;
		cyl->type.workingpressure.mbar = ti->bar * 1000;
	} else {
		cyl->type.workingpressure.mbar = psi_to_mbar(ti->psi);
		if (ti->psi)
			cyl->type.size.mliter = cuft_to_l(ti->cuft) * 1000 / bar_to_atm(psi_to_bar(ti->psi));
	}
	// MOD of air
	cyl->depth = gas_mod(&cyl->gasmix, pO2, 1);
}

/* make sure that the gas we are switching to is represented in our
 * list of cylinders */
static int verify_gas_exists(struct gasmix mix_in)
{
	int i;
	cylinder_t *cyl;

	for (i = 0; i < MAX_CYLINDERS; i++) {
		cyl = displayed_dive.cylinder + i;
		if (cylinder_nodata(cyl))
			continue;
		if (gasmix_distance(&cyl->gasmix, &mix_in) < 100)
			return i;
	}
	fprintf(stderr, "this gas %s should have been on the cylinder list\nThings will fail now\n", gasname(&mix_in));
	return -1;
}

/* calculate the new end pressure of the cylinder, based on its current end pressure and the
 * latest segment. */
static void update_cylinder_pressure(struct dive *d, int old_depth, int new_depth, int duration, int sac, cylinder_t *cyl, bool in_deco)
{
	volume_t gas_used;
	pressure_t delta_p;
	depth_t mean_depth;
	int factor = 1000;

	if (d->dc.divemode == PSCR)
		factor = prefs.pscr_ratio;

	if (!cyl)
		return;
	mean_depth.mm = (old_depth + new_depth) / 2;
	gas_used.mliter = depth_to_atm(mean_depth.mm, d) * sac / 60 * duration * factor / 1000;
	cyl->gas_used.mliter += gas_used.mliter;
	if (in_deco)
		cyl->deco_gas_used.mliter += gas_used.mliter;
	if (cyl->type.size.mliter) {
		delta_p.mbar = gas_used.mliter * 1000.0 / cyl->type.size.mliter;
		cyl->end.mbar -= delta_p.mbar;
	}
}

/* simply overwrite the data in the displayed_dive
 * return false if something goes wrong */
static void create_dive_from_plan(struct diveplan *diveplan, bool track_gas)
{
	struct divedatapoint *dp;
	struct divecomputer *dc;
	struct sample *sample;
	struct gasmix oldgasmix;
	struct event *ev;
	cylinder_t *cyl;
	int oldpo2 = 0;
	int lasttime = 0;
	int lastdepth = 0;
	enum dive_comp_type type = displayed_dive.dc.divemode;

	if (!diveplan || !diveplan->dp)
		return;
#if DEBUG_PLAN & 4
	printf("in create_dive_from_plan\n");
	dump_plan(diveplan);
#endif
	displayed_dive.salinity = diveplan->salinity;
	// reset the cylinders and clear out the samples and events of the
	// displayed dive so we can restart
	reset_cylinders(&displayed_dive, track_gas);
	dc = &displayed_dive.dc;
	dc->when = displayed_dive.when = diveplan->when;
	free(dc->sample);
	dc->sample = NULL;
	dc->samples = 0;
	dc->alloc_samples = 0;
	while ((ev = dc->events)) {
		dc->events = dc->events->next;
		free(ev);
	}
	dp = diveplan->dp;
	cyl = &displayed_dive.cylinder[0];
	oldgasmix = cyl->gasmix;
	sample = prepare_sample(dc);
	sample->setpoint.mbar = dp->setpoint;
	sample->sac.mliter = prefs.bottomsac;
	oldpo2 = dp->setpoint;
	if (track_gas && cyl->type.workingpressure.mbar)
		sample->cylinderpressure.mbar = cyl->end.mbar;
	sample->manually_entered = true;
	finish_sample(dc);
	while (dp) {
		struct gasmix gasmix = dp->gasmix;
		int po2 = dp->setpoint;
		if (dp->setpoint)
			type = CCR;
		int time = dp->time;
		int depth = dp->depth;

		if (time == 0) {
			/* special entries that just inform the algorithm about
			 * additional gases that are available */
			if (verify_gas_exists(gasmix) < 0)
				goto gas_error_exit;
			dp = dp->next;
			continue;
		}

		/* Check for SetPoint change */
		if (oldpo2 != po2) {
			/* this is a bad idea - we should get a different SAMPLE_EVENT type
			 * reserved for this in libdivecomputer... overloading SMAPLE_EVENT_PO2
			 * with a different meaning will only cause confusion elsewhere in the code */
			add_event(dc, lasttime, SAMPLE_EVENT_PO2, 0, po2, "SP change");
			oldpo2 = po2;
		}

		/* Make sure we have the new gas, and create a gas change event */
		if (gasmix_distance(&gasmix, &oldgasmix) > 0) {
			int idx;
			if ((idx = verify_gas_exists(gasmix)) < 0)
				goto gas_error_exit;
			/* need to insert a first sample for the new gas */
			add_gas_switch_event(&displayed_dive, dc, lasttime + 1, idx);
			cyl = &displayed_dive.cylinder[idx];
			sample = prepare_sample(dc);
			sample[-1].setpoint.mbar = po2;
			sample->time.seconds = lasttime + 1;
			sample->depth.mm = lastdepth;
			sample->manually_entered = dp->entered;
			sample->sac.mliter = dp->entered ? prefs.bottomsac : prefs.decosac;
			if (track_gas && cyl->type.workingpressure.mbar)
				sample->cylinderpressure.mbar = cyl->sample_end.mbar;
			finish_sample(dc);
			oldgasmix = gasmix;
		}
		/* Create sample */
		sample = prepare_sample(dc);
		/* set po2 at beginning of this segment */
		/* and keep it valid for last sample - where it likely doesn't matter */
		sample[-1].setpoint.mbar = po2;
		sample->setpoint.mbar = po2;
		sample->time.seconds = lasttime = time;
		sample->depth.mm = lastdepth = depth;
		sample->manually_entered = dp->entered;
		sample->sac.mliter = dp->entered ? prefs.bottomsac : prefs.decosac;
		if (track_gas && !sample[-1].setpoint.mbar) {    /* Don't track gas usage for CCR legs of dive */
			update_cylinder_pressure(&displayed_dive, sample[-1].depth.mm, depth, time - sample[-1].time.seconds,
					dp->entered ? diveplan->bottomsac : diveplan->decosac, cyl, !dp->entered);
			if (cyl->type.workingpressure.mbar)
				sample->cylinderpressure.mbar = cyl->end.mbar;
		}
		finish_sample(dc);
		dp = dp->next;
	}
	dc->divemode = type;
#if DEBUG_PLAN & 32
	save_dive(stdout, &displayed_dive);
#endif
	return;

gas_error_exit:
	report_error(translate("gettextFromC", "Too many gas mixes"));
	return;
}

void free_dps(struct diveplan *diveplan)
{
	if (!diveplan)
		return;
	struct divedatapoint *dp = diveplan->dp;
	while (dp) {
		struct divedatapoint *ndp = dp->next;
		free(dp);
		dp = ndp;
	}
	diveplan->dp = NULL;
}

struct divedatapoint *create_dp(int time_incr, int depth, struct gasmix gasmix, int po2)
{
	struct divedatapoint *dp;

	dp = malloc(sizeof(struct divedatapoint));
	dp->time = time_incr;
	dp->depth = depth;
	dp->gasmix = gasmix;
	dp->setpoint = po2;
	dp->entered = false;
	dp->next = NULL;
	return dp;
}

void add_to_end_of_diveplan(struct diveplan *diveplan, struct divedatapoint *dp)
{
	struct divedatapoint **lastdp = &diveplan->dp;
	struct divedatapoint *ldp = *lastdp;
	int lasttime = 0;
	while (*lastdp) {
		ldp = *lastdp;
		if (ldp->time > lasttime)
			lasttime = ldp->time;
		lastdp = &(*lastdp)->next;
	}
	*lastdp = dp;
	if (ldp && dp->time != 0)
		dp->time += lasttime;
}

struct divedatapoint *plan_add_segment(struct diveplan *diveplan, int duration, int depth, struct gasmix gasmix, int po2, bool entered)
{
	struct divedatapoint *dp = create_dp(duration, depth, gasmix, po2);
	dp->entered = entered;
	add_to_end_of_diveplan(diveplan, dp);
	return (dp);
}

struct gaschanges {
	int depth;
	int gasidx;
};


static struct gaschanges *analyze_gaslist(struct diveplan *diveplan, int *gaschangenr, int depth, int *asc_cylinder)
{
	struct gasmix gas;
	int nr = 0;
	struct gaschanges *gaschanges = NULL;
	struct divedatapoint *dp = diveplan->dp;
	int best_depth = displayed_dive.cylinder[*asc_cylinder].depth.mm;
	while (dp) {
		if (dp->time == 0) {
			gas = dp->gasmix;
			if (dp->depth <= depth) {
				int i = 0;
				nr++;
				gaschanges = realloc(gaschanges, nr * sizeof(struct gaschanges));
				while (i < nr - 1) {
					if (dp->depth < gaschanges[i].depth) {
						memmove(gaschanges + i + 1, gaschanges + i, (nr - i - 1) * sizeof(struct gaschanges));
						break;
					}
					i++;
				}
				gaschanges[i].depth = dp->depth;
				gaschanges[i].gasidx = get_gasidx(&displayed_dive, &gas);
				assert(gaschanges[i].gasidx != -1);
			} else {
				/* is there a better mix to start deco? */
				if (dp->depth < best_depth) {
					best_depth = dp->depth;
					*asc_cylinder = get_gasidx(&displayed_dive, &gas);
				}
			}
		}
		dp = dp->next;
	}
	*gaschangenr = nr;
#if DEBUG_PLAN & 16
	for (nr = 0; nr < *gaschangenr; nr++) {
		int idx = gaschanges[nr].gasidx;
		printf("gaschange nr %d: @ %5.2lfm gasidx %d (%s)\n", nr, gaschanges[nr].depth / 1000.0,
		       idx, gasname(&displayed_dive.cylinder[idx].gasmix));
	}
#endif
	return gaschanges;
}

/* sort all the stops into one ordered list */
static unsigned int *sort_stops(int *dstops, int dnr, struct gaschanges *gstops, int gnr)
{
	int i, gi, di;
	int total = dnr + gnr;
	unsigned int *stoplevels = malloc(total * sizeof(int));

	/* no gaschanges */
	if (gnr == 0) {
		memcpy(stoplevels, dstops, dnr * sizeof(int));
		return stoplevels;
	}
	i = total - 1;
	gi = gnr - 1;
	di = dnr - 1;
	while (i >= 0) {
		if (dstops[di] > gstops[gi].depth) {
			stoplevels[i] = dstops[di];
			di--;
		} else if (dstops[di] == gstops[gi].depth) {
			stoplevels[i] = dstops[di];
			di--;
			gi--;
		} else {
			stoplevels[i] = gstops[gi].depth;
			gi--;
		}
		i--;
		if (di < 0) {
			while (gi >= 0)
				stoplevels[i--] = gstops[gi--].depth;
			break;
		}
		if (gi < 0) {
			while (di >= 0)
				stoplevels[i--] = dstops[di--];
			break;
		}
	}
	while (i >= 0)
		stoplevels[i--] = 0;

#if DEBUG_PLAN & 16
	int k;
	for (k = gnr + dnr - 1; k >= 0; k--) {
		printf("stoplevel[%d]: %5.2lfm\n", k, stoplevels[k] / 1000.0);
		if (stoplevels[k] == 0)
			break;
	}
#endif
	return stoplevels;
}

static void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive, bool show_disclaimer, int error)
{
	char buffer[2000000], temp[100000];
	int len, lastdepth = 0, lasttime = 0, lastsetpoint = -1, newdepth = 0, lastprintdepth = 0;
	struct divedatapoint *dp = diveplan->dp;
	bool gaschange = !plan_verbatim, postponed = plan_verbatim;
	struct divedatapoint *nextdp = NULL;

	disclaimer =  translate("gettextFromC", "DISCLAIMER / WARNING: THIS IS A NEW IMPLEMENTATION OF THE BUHLMANN "
				"ALGORITHM AND A DIVE PLANNER IMPLEMENTATION BASED ON THAT WHICH HAS "
				"RECEIVED ONLY A LIMITED AMOUNT OF TESTING. WE STRONGLY RECOMMEND NOT TO "
				"PLAN DIVES SIMPLY BASED ON THE RESULTS GIVEN HERE.");

	if (!dp)
		return;

	if (error) {
		snprintf(temp, sizeof(temp), "%s",
			 translate("gettextFromC", "Decompression calculation aborted due to excessive time"));
		snprintf(buffer, sizeof(buffer), "<span style='color: red;'>%s </span> %s<br>",
				translate("gettextFromC", "Warning:"), temp);
		dive->notes = strdup(buffer);
		return;
	}

	len = show_disclaimer ? snprintf(buffer, sizeof(buffer), "<div><b>%s<b></div><br>", disclaimer) : 0;
	snprintf(temp, sizeof(temp), translate("gettextFromC", "based on GFlow = %d and GFhigh = %d"),
		 diveplan->gflow, diveplan->gfhigh);
	len += snprintf(buffer + len, sizeof(buffer) - len, "<div><b>%s</b><br>%s</div><br>",
			translate("gettextFromC", "Subsurface dive plan"), temp);

	if (!plan_verbatim) {
		len += snprintf(buffer + len, sizeof(buffer) - len, "<div><table><thead><tr><th>%s</th>",
				translate("gettextFromC", "depth"));
		if (plan_display_runtime)
			len += snprintf(buffer + len, sizeof(buffer) - len, "<th style='padding-left: 10px;'>%s</th>",
					translate("gettextFromC", "runtime"));
		if (plan_display_duration)
			len += snprintf(buffer + len, sizeof(buffer) - len, "<th style='padding-left: 10px;'>%s</th>",
					translate("gettextFromC", "duration"));
		len += snprintf(buffer + len, sizeof(buffer) - len,
				"<th style='padding-left: 10px; float: left;'>%s</th></tr></thead><tbody style='float: left;'>",
				translate("gettextFromC", "gas"));
	}
	do {
		struct gasmix gasmix, newgasmix = {};
		const char *depth_unit;
		double depthvalue;
		int decimals;

		nextdp = dp->next;
		if (dp->time == 0)
			continue;
		gasmix = dp->gasmix;
		depthvalue = get_depth_units(dp->depth, &decimals, &depth_unit);
		/* analyze the dive points ahead */
		while (nextdp && nextdp->time == 0)
			nextdp = nextdp->next;
		if (nextdp)
			newgasmix = nextdp->gasmix;
		/* do we want to skip this leg as it is devoid of anything useful? */
		if (!dp->entered &&
		    gasmix_distance(&gasmix, &newgasmix) == 0 &&
		    nextdp &&
		    dp->setpoint == nextdp->setpoint &&
		    dp->depth != lastdepth &&
		    nextdp->depth != dp->depth
		    && !gaschange)
			continue;
		if (dp->time - lasttime < 10 && !(gaschange && dp->next && dp->depth != dp->next->depth))
			continue;

		len = strlen(buffer);
		if (nextdp && (gasmix_distance(&gasmix, &newgasmix) || dp->setpoint != nextdp->setpoint))
			gaschange = true;
		if (plan_verbatim) {
			if (dp->depth != lastprintdepth) {
				if (plan_display_transitions || dp->entered || !dp->next || (gaschange && dp->next && dp->depth != nextdp->depth)) {
					if (dp->setpoint)
						snprintf(temp, sizeof(temp), translate("gettextFromC", "Transition to %.*f %s in %d:%02d min - runtime %d:%02u on %s (SP = %.1fbar)"),
							 decimals, depthvalue, depth_unit,
							 FRACTION(dp->time - lasttime, 60),
							 FRACTION(dp->time, 60),
							 gasname(&gasmix),
							 (double) dp->setpoint / 1000.0);

					else
						snprintf(temp, sizeof(temp), translate("gettextFromC", "Transition to %.*f %s in %d:%02d min - runtime %d:%02u on %s"),
							 decimals, depthvalue, depth_unit,
							 FRACTION(dp->time - lasttime, 60),
							 FRACTION(dp->time, 60),
							 gasname(&gasmix));

					len += snprintf(buffer + len, sizeof(buffer) - len, "%s<br>", temp);
				}
				newdepth = dp->depth;
				lasttime = dp->time;
			} else {
				if (dp->depth != nextdp->depth || gasmix_distance(&gasmix, &newgasmix) != 0 || dp->setpoint != nextdp->setpoint) {
					if (dp->setpoint)
						snprintf(temp, sizeof(temp), translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s (SP = %.1fbar)"),
								decimals, depthvalue, depth_unit,
								FRACTION(dp->time - lasttime, 60),
								FRACTION(dp->time, 60),
								gasname(&gasmix),
								(double) dp->setpoint / 1000.0);
					else
						snprintf(temp, sizeof(temp), translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s"),
								decimals, depthvalue, depth_unit,
								FRACTION(dp->time - lasttime, 60),
								FRACTION(dp->time, 60),
								gasname(&gasmix));

						len += snprintf(buffer + len, sizeof(buffer) - len, "%s<br>", temp);
					newdepth = dp->depth;
					lasttime = dp->time;
				}
			}
		} else {
			if ((dp->depth == lastdepth && dp->depth != nextdp->depth) || plan_display_transitions || dp->entered || !dp->next || (gaschange && dp->next && dp->depth != nextdp->depth)) {
				snprintf(temp, sizeof(temp), translate("gettextFromC", "%3.0f%s"), depthvalue, depth_unit);
				len += snprintf(buffer + len, sizeof(buffer) - len, "<tr><td style='padding-left: 10px; float: right;'>%s</td>", temp);
				if (plan_display_runtime) {
					snprintf(temp, sizeof(temp), translate("gettextFromC", "%3dmin"), (dp->time + 30) / 60);
					len += snprintf(buffer + len, sizeof(buffer) - len, "<td style='padding-left: 10px; float: right;'>%s</td>", temp);
				}
				if (plan_display_duration) {
					snprintf(temp, sizeof(temp), translate("gettextFromC", "%3dmin"), (dp->time - lasttime + 30) / 60);
					len += snprintf(buffer + len, sizeof(buffer) - len, "<td style='padding-left: 10px; float: right;'>%s</td>", temp);
				}

				if (gaschange) {
					if(dp->depth == lastdepth && !postponed) {
						postponed = true;
					} else {
						if (dp->setpoint) {
							snprintf(temp, sizeof(temp), translate("gettextFromC", "(SP = %.1fbar)"), (double) dp->setpoint / 1000.0);
							len += snprintf(buffer + len, sizeof(buffer) - len, "<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>", gasname(&newgasmix),
									temp);
						} else {
							len += snprintf(buffer + len, sizeof(buffer) - len, "<td style='padding-left: 10px; color: red; float: left;'><b>%s</b></td>", gasname(&newgasmix));
						}
						gaschange = false;
						postponed = false;
					}
				} else {
					len += snprintf(buffer + len, sizeof(buffer) - len, "<td>&nbsp;</td>");
				}
				len += snprintf(buffer + len, sizeof(buffer) - len, "</tr>");
				newdepth = dp->depth;
				lasttime = dp->time;
			}
		}
		if (gaschange) {
			if(dp->depth == lastdepth && !postponed) {
				postponed = true;
			} else {
			// gas switch at this waypoint
			if (plan_verbatim) {
				if (lastsetpoint >= 0) {
					if (nextdp && nextdp->setpoint)
						snprintf(temp, sizeof(temp), translate("gettextFromC", "Switch gas to %s (SP = %.1fbar)"), gasname(&newgasmix), (double) nextdp->setpoint / 1000.0);
					else
						snprintf(temp, sizeof(temp), translate("gettextFromC", "Switch gas to %s"), gasname(&newgasmix));

					len += snprintf(buffer + len, sizeof(buffer) - len, "%s<br>", temp);
				}
				gaschange = false;
				postponed = true;
			}
			gasmix = newgasmix;
			}
		}
		lastprintdepth = newdepth;
		lastdepth = dp->depth;
		lastsetpoint = dp->setpoint;
	} while ((dp = nextdp) != NULL);
	len += snprintf(buffer + len, sizeof(buffer) - len, "</tbody></table></div>");

	dive->cns = 0;
	dive->maxcns = 0;
	update_cylinder_related_info(dive);
	snprintf(temp, sizeof(temp), "%s", translate("gettextFromC", "CNS"));
	len += snprintf(buffer + len, sizeof(buffer) - len, "<div><br>%s: %i%%", temp, dive->cns);
	snprintf(temp, sizeof(temp), "%s", translate("gettextFromC", "OTU"));
	len += snprintf(buffer + len, sizeof(buffer) - len, "<br>%s: %i</div>", temp, dive->otu);

	if (dive->dc.divemode == CCR)
		snprintf(temp, sizeof(temp), "%s", translate("gettextFromC", "Gas consumption (CCR legs excluded):"));
	else
		snprintf(temp, sizeof(temp), "%s", translate("gettextFromC", "Gas consumption:"));
	len += snprintf(buffer + len, sizeof(buffer) - len, "<div><br>%s<br>", temp);
	for (int gasidx = 0; gasidx < MAX_CYLINDERS; gasidx++) {
		double volume, pressure, deco_volume, deco_pressure;
		const char *unit, *pressure_unit;
		char warning[1000] = "";
		cylinder_t *cyl = &dive->cylinder[gasidx];
		if (cylinder_none(cyl))
			break;

		volume = get_volume_units(cyl->gas_used.mliter, NULL, &unit);
		deco_volume = get_volume_units(cyl->deco_gas_used.mliter, NULL, &unit);
		if (cyl->type.size.mliter) {
			deco_pressure = get_pressure_units(1000.0 * cyl->deco_gas_used.mliter / cyl->type.size.mliter, &pressure_unit);
			pressure = get_pressure_units(1000.0 * cyl->gas_used.mliter / cyl->type.size.mliter, &pressure_unit);
			/* Warn if the plan uses more gas than is available in a cylinder
			 * This only works if we have working pressure for the cylinder
			 * 10bar is a made up number - but it seemed silly to pretend you could breathe cylinder down to 0 */
			if (cyl->end.mbar < 10000)
				snprintf(warning, sizeof(warning), " &mdash; <span style='color: red;'>%s </span> %s",
					translate("gettextFromC", "Warning:"),
					translate("gettextFromC", "this is more gas than available in the specified cylinder!"));
			else
				if ((float) cyl->end.mbar * cyl->type.size.mliter / 1000.0 < (float) cyl->deco_gas_used.mliter)
					snprintf(warning, sizeof(warning), " &mdash; <span style='color: red;'>%s </span> %s",
						translate("gettextFromC", "Warning:"),
						translate("gettextFromC", "not enough reserve for gas sharing on ascent!"));

			snprintf(temp, sizeof(temp), translate("gettextFromC", "%.0f%s/%.0f%s of %s (%.0f%s/%.0f%s in planned ascent)"), volume, unit, pressure, pressure_unit, gasname(&cyl->gasmix), deco_volume, unit, deco_pressure, pressure_unit);
		} else {
			snprintf(temp, sizeof(temp), translate("gettextFromC", "%.0f%s (%.0f%s during planned ascent) of %s"), volume, unit, deco_volume, unit, gasname(&cyl->gasmix));
		}
		len += snprintf(buffer + len, sizeof(buffer) - len, "%s%s<br>", temp, warning);
	}
	dp = diveplan->dp;
	if (dive->dc.divemode != CCR) {
		while (dp) {
			if (dp->time != 0) {
				struct gas_pressures pressures;
				fill_pressures(&pressures, depth_to_atm(dp->depth, dive), &dp->gasmix, 0.0, dive->dc.divemode);

				if (pressures.o2 > (dp->entered ? prefs.bottompo2 : prefs.decopo2) / 1000.0) {
					const char *depth_unit;
					int decimals;
					double depth_value = get_depth_units(dp->depth, &decimals, &depth_unit);
					len = strlen(buffer);
					snprintf(temp, sizeof(temp),
						 translate("gettextFromC", "high pO₂ value %.2f at %d:%02u with gas %s at depth %.*f %s"),
						 pressures.o2, FRACTION(dp->time, 60), gasname(&dp->gasmix), decimals, depth_value, depth_unit);
					len += snprintf(buffer + len, sizeof(buffer) - len, "<span style='color: red;'>%s </span> %s<br>",
							translate("gettextFromC", "Warning:"), temp);
				} else if (pressures.o2 < 0.16) {
					const char *depth_unit;
					int decimals;
					double depth_value = get_depth_units(dp->depth, &decimals, &depth_unit);
					len = strlen(buffer);
					snprintf(temp, sizeof(temp),
						 translate("gettextFromC", "low pO₂ value %.2f at %d:%02u with gas %s at depth %.*f %s"),
						 pressures.o2, FRACTION(dp->time, 60), gasname(&dp->gasmix), decimals, depth_value, depth_unit);
					len += snprintf(buffer + len, sizeof(buffer) - len, "<span style='color: red;'>%s </span> %s<br>",
							translate("gettextFromC", "Warning:"), temp);

				}
			}
			dp = dp->next;
		}
	}
	snprintf(buffer + len, sizeof(buffer) - len, "</div>");
	dive->notes = strdup(buffer);
}

int ascent_velocity(int depth, int avg_depth, int bottom_time)
{
	/* We need to make this configurable */

	/* As an example (and possibly reasonable default) this is the Tech 1 provedure according
	 * to http://www.globalunderwaterexplorers.org/files/Standards_and_Procedures/SOP_Manual_Ver2.0.2.pdf */

	if (depth * 4 > avg_depth * 3) {
		return prefs.ascrate75;
	} else {
		if (depth * 2 > avg_depth) {
			return prefs.ascrate50;
		} else {
			if (depth > 6000)
				return prefs.ascratestops;
			else
				return prefs.ascratelast6m;
		}
	}
}

void track_ascent_gas(int depth, cylinder_t *cylinder, int avg_depth, int bottom_time, bool safety_stop)
{
	while (depth > 0) {
		int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
		if (deltad > depth)
			deltad = depth;
		update_cylinder_pressure(&displayed_dive, depth, depth - deltad, TIMESTEP, prefs.decosac, cylinder, true);
		if (depth <= 5000 && depth >= (5000 - deltad) && safety_stop) {
			update_cylinder_pressure(&displayed_dive, 5000, 5000, 180, prefs.decosac, cylinder, true);
			safety_stop = false;
		}
		depth -= deltad;
	}
}

bool trial_ascent(int trial_depth, int stoplevel, int avg_depth, int bottom_time, double tissue_tolerance, struct gasmix *gasmix, int po2, double surface_pressure)
{

	bool clear_to_ascend = true;
	char *trial_cache = NULL;

	cache_deco_state(tissue_tolerance, &trial_cache);
	while (trial_depth > stoplevel) {
		int deltad = ascent_velocity(trial_depth, avg_depth, bottom_time) * TIMESTEP;
		if (deltad > trial_depth) /* don't test against depth above surface */
			deltad = trial_depth;
		tissue_tolerance = add_segment(depth_to_mbar(trial_depth, &displayed_dive) / 1000.0,
					       gasmix,
					       TIMESTEP, po2, &displayed_dive, prefs.decosac);
		if (deco_allowed_depth(tissue_tolerance, surface_pressure, &displayed_dive, 1) > trial_depth - deltad) {
			/* We should have stopped */
			clear_to_ascend = false;
			break;
		}
		trial_depth -= deltad;
	}
	restore_deco_state(trial_cache);
	return clear_to_ascend;
}

bool enough_gas(int current_cylinder)
{
	cylinder_t *cyl;
	cyl = &displayed_dive.cylinder[current_cylinder];

	if (!cyl->start.mbar)
		return true;
	if (cyl->type.size.mliter)
		return (float) (cyl->end.mbar - prefs.reserve_gas)	 * cyl->type.size.mliter / 1000.0 > (float) cyl->deco_gas_used.mliter;
	else
		return true;
}

// Work out the stops. Return value is if there were any mandatory stops.

bool plan(struct diveplan *diveplan, char **cached_datap, bool is_planner, bool show_disclaimer)
{
	struct sample *sample;
	int po2;
	int transitiontime, gi;
	int current_cylinder;
	unsigned int stopidx;
	int depth;
	double tissue_tolerance = 0.0;
	struct gaschanges *gaschanges = NULL;
	int gaschangenr;
	unsigned int *stoplevels = NULL;
	bool stopping = false;
	bool clear_to_ascend;
	int clock, previous_point_time;
	int avg_depth, max_depth, bottom_time = 0;
	int last_ascend_rate;
	int best_first_ascend_cylinder;
	struct gasmix gas;
	int o2time = 0;
	int breaktime = -1;
	int breakcylinder = 0;
	int error = 0;
	bool decodive = false;

	set_gf(diveplan->gflow, diveplan->gfhigh, prefs.gf_low_at_maxdepth);
	if (!diveplan->surface_pressure)
		diveplan->surface_pressure = SURFACE_PRESSURE;
	create_dive_from_plan(diveplan, is_planner);

	if (prefs.verbatim_plan)
		plan_verbatim = true;
	if (prefs.display_runtime)
		plan_display_runtime = true;
	if (prefs.display_duration)
		plan_display_duration = true;
	if (prefs.display_transitions)
		plan_display_transitions = true;
	if (prefs.last_stop)
		decostoplevels[1] = 6000;

	/* Let's start at the last 'sample', i.e. the last manually entered waypoint. */
	sample = &displayed_dive.dc.sample[displayed_dive.dc.samples - 1];

	get_gas_at_time(&displayed_dive, &displayed_dive.dc, sample->time, &gas);

	po2 = sample->setpoint.mbar;
	if ((current_cylinder = get_gasidx(&displayed_dive, &gas)) == -1) {
		report_error(translate("gettextFromC", "Can't find gas %s"), gasname(&gas));
		current_cylinder = 0;
	}
	depth = displayed_dive.dc.sample[displayed_dive.dc.samples - 1].depth.mm;
	average_max_depth(diveplan, &avg_depth, &max_depth);
	last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);

	/* if all we wanted was the dive just get us back to the surface */
	if (!is_planner) {
		transitiontime = depth / 75; /* this still needs to be made configurable */
		plan_add_segment(diveplan, transitiontime, 0, gas, po2, false);
		create_dive_from_plan(diveplan, is_planner);
		return(false);
	}
	tissue_tolerance = tissue_at_end(&displayed_dive, cached_datap);

#if DEBUG_PLAN & 4
	printf("gas %s\n", gasname(&gas));
	printf("depth %5.2lfm \n", depth / 1000.0);
#endif

	best_first_ascend_cylinder = current_cylinder;
	/* Find the gases available for deco */

	if (po2) {	// Don't change gas in CCR mode
		gaschanges = NULL;
		gaschangenr = 0;
	} else {
		gaschanges = analyze_gaslist(diveplan, &gaschangenr, depth, &best_first_ascend_cylinder);
	}
	/* Find the first potential decostopdepth above current depth */
	for (stopidx = 0; stopidx < sizeof(decostoplevels) / sizeof(int); stopidx++)
		if (decostoplevels[stopidx] >= depth)
			break;
	if (stopidx > 0)
		stopidx--;
	/* Stoplevels are either depths of gas changes or potential deco stop depths. */
	stoplevels = sort_stops(decostoplevels, stopidx + 1, gaschanges, gaschangenr);
	stopidx += gaschangenr;

	/* Keep time during the ascend */
	bottom_time = clock = previous_point_time = displayed_dive.dc.sample[displayed_dive.dc.samples - 1].time.seconds;
	gi = gaschangenr - 1;
	if(prefs.recreational_mode) {
		bool safety_stop = prefs.safetystop && max_depth >= 10000;
		track_ascent_gas(depth, &displayed_dive.cylinder[current_cylinder], avg_depth, bottom_time, safety_stop);
		// How long can we stay at the current depth and still directly ascent to the surface?
		while (trial_ascent(depth, 0, avg_depth, bottom_time, tissue_tolerance, &displayed_dive.cylinder[current_cylinder].gasmix,
				  po2, diveplan->surface_pressure / 1000.0) &&
		       enough_gas(current_cylinder)) {
			tissue_tolerance = add_segment(depth_to_mbar(depth, &displayed_dive) / 1000.0,
						       &displayed_dive.cylinder[current_cylinder].gasmix,
						       DECOTIMESTEP, po2, &displayed_dive, prefs.bottomsac);
			update_cylinder_pressure(&displayed_dive, depth, depth, DECOTIMESTEP, prefs.bottomsac, &displayed_dive.cylinder[current_cylinder], false);
			clock += DECOTIMESTEP;
		}
		clock -= DECOTIMESTEP;
		plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, true);
		previous_point_time = clock;
		do {
			/* Ascend to surface */
			int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
			if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
				plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
				previous_point_time = clock;
				last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
			}
			if (depth - deltad < 0)
				deltad = depth;

			tissue_tolerance = add_segment(depth_to_mbar(depth, &displayed_dive) / 1000.0,
						       &displayed_dive.cylinder[current_cylinder].gasmix,
						       TIMESTEP, po2, &displayed_dive, prefs.decosac);
			clock += TIMESTEP;
			depth -= deltad;
			if (depth <= 5000 && depth >= (5000 - deltad) && safety_stop) {
				plan_add_segment(diveplan, clock - previous_point_time, 5000, gas, po2, false);
				previous_point_time = clock;
				clock += 180;
				plan_add_segment(diveplan, clock - previous_point_time, 5000, gas, po2, false);
				previous_point_time = clock;
				safety_stop = false;
			}
		} while (depth > 0);
		plan_add_segment(diveplan, clock - previous_point_time, 0, gas, po2, false);
		create_dive_from_plan(diveplan, is_planner);
		add_plan_to_notes(diveplan, &displayed_dive, show_disclaimer, error);
		fixup_dc_duration(&displayed_dive.dc);

		free(stoplevels);
		free(gaschanges);

		return(false);
	}

	if (best_first_ascend_cylinder != current_cylinder) {
		stopping = true;

		current_cylinder = best_first_ascend_cylinder;
		gas = displayed_dive.cylinder[current_cylinder].gasmix;

#if DEBUG_PLAN & 16
		printf("switch to gas %d (%d/%d) @ %5.2lfm\n", best_first_ascend_cylinder,
		       (get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[best_first_ascend_cylinder].depth / 1000.0);
#endif
	}
	while (1) {
		/* We will break out when we hit the surface */
		do {
			/* Ascend to next stop depth */
			int deltad = ascent_velocity(depth, avg_depth, bottom_time) * TIMESTEP;
			if (ascent_velocity(depth, avg_depth, bottom_time) != last_ascend_rate) {
				plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
				previous_point_time = clock;
				stopping = false;
				last_ascend_rate = ascent_velocity(depth, avg_depth, bottom_time);
			}
			if (depth - deltad < stoplevels[stopidx])
				deltad = depth - stoplevels[stopidx];

			tissue_tolerance = add_segment(depth_to_mbar(depth, &displayed_dive) / 1000.0,
						       &displayed_dive.cylinder[current_cylinder].gasmix,
						       TIMESTEP, po2, &displayed_dive, prefs.decosac);
			clock += TIMESTEP;
			depth -= deltad;
		} while (depth > 0 && depth > stoplevels[stopidx]);

		if (depth <= 0)
			break; /* We are at the surface */

		if (gi >= 0 && stoplevels[stopidx] == gaschanges[gi].depth) {
			/* We have reached a gas change.
			 * Record this in the dive plan */
			plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
			previous_point_time = clock;
			stopping = true;

			current_cylinder = gaschanges[gi].gasidx;
			gas = displayed_dive.cylinder[current_cylinder].gasmix;
#if DEBUG_PLAN & 16
			printf("switch to gas %d (%d/%d) @ %5.2lfm\n", gaschanges[gi].gasidx,
			       (get_o2(&gas) + 5) / 10, (get_he(&gas) + 5) / 10, gaschanges[gi].depth / 1000.0);
#endif
			gi--;
		}

		--stopidx;

		/* Save the current state and try to ascend to the next stopdepth */
		while (1) {
			/* Check if ascending to next stop is clear, go back and wait if we hit the ceiling on the way */
			if (trial_ascent(depth, stoplevels[stopidx], avg_depth, bottom_time, tissue_tolerance,
					 &displayed_dive.cylinder[current_cylinder].gasmix, po2, diveplan->surface_pressure / 1000.0))
				break; /* We did not hit the ceiling */

			/* Add a minute of deco time and then try again */
			decodive = true;
			if (!stopping) {
				/* The last segment was an ascend segment.
				 * Add a waypoint for start of this deco stop */
				plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
				previous_point_time = clock;
				stopping = true;
			}
			tissue_tolerance = add_segment(depth_to_mbar(depth, &displayed_dive) / 1000.0,
						       &displayed_dive.cylinder[current_cylinder].gasmix,
						       DECOTIMESTEP, po2, &displayed_dive, prefs.decosac);
			clock += DECOTIMESTEP;
			/* Finish infinite deco */
			if(clock >= 48 * 3600 && depth >= 6000) {
				error = LONGDECO;
				break;
			}
			if (prefs.doo2breaks) {
				if (get_o2(&displayed_dive.cylinder[current_cylinder].gasmix) == 1000) {
					o2time += DECOTIMESTEP;
					if (o2time >= 12 * 60) {
						breaktime = 0;
						breakcylinder = current_cylinder;
						plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
						previous_point_time = clock;
						current_cylinder = 0;
						gas = displayed_dive.cylinder[current_cylinder].gasmix;
					}
				} else {
					if (breaktime >= 0) {
						breaktime += DECOTIMESTEP;
						if (breaktime >= 6 * 60) {
							o2time = 0;
							plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
							previous_point_time = clock;
							current_cylinder = breakcylinder;
							gas = displayed_dive.cylinder[current_cylinder].gasmix;
							breaktime = -1;
						}
					}
				}
			}
		}
		if (stopping) {
			/* Next we will ascend again. Add a waypoint if we have spend deco time */
			plan_add_segment(diveplan, clock - previous_point_time, depth, gas, po2, false);
			previous_point_time = clock;
			stopping = false;
		}
	}

	/* We made it to the surface
	 * Create the final dive, add the plan to the notes and fixup some internal
	 * data that we need to be there when plotting the dive */
	plan_add_segment(diveplan, clock - previous_point_time, 0, gas, po2, false);
	create_dive_from_plan(diveplan, is_planner);
	add_plan_to_notes(diveplan, &displayed_dive, show_disclaimer, error);
	fixup_dc_duration(&displayed_dive.dc);

	free(stoplevels);
	free(gaschanges);
	return decodive;
}

/*
 * Get a value in tenths (so "10.2" == 102, "9" = 90)
 *
 * Return negative for errors.
 */
static int get_tenths(const char *begin, const char **endp)
{
	char *end;
	int value = strtol(begin, &end, 10);

	if (begin == end)
		return -1;
	value *= 10;

	/* Fraction? We only look at the first digit */
	if (*end == '.') {
		end++;
		if (!isdigit(*end))
			return -1;
		value += *end - '0';
		do {
			end++;
		} while (isdigit(*end));
	}
	*endp = end;
	return value;
}

static int get_permille(const char *begin, const char **end)
{
	int value = get_tenths(begin, end);
	if (value >= 0) {
		/* Allow a percentage sign */
		if (**end == '%')
			++*end;
	}
	return value;
}

int validate_gas(const char *text, struct gasmix *gas)
{
	int o2, he;

	if (!text)
		return 0;

	while (isspace(*text))
		text++;

	if (!*text)
		return 0;

	if (!strcasecmp(text, translate("gettextFromC", "air"))) {
		o2 = O2_IN_AIR;
		he = 0;
		text += strlen(translate("gettextFromC", "air"));
	} else if (!strcasecmp(text, translate("gettextFromC", "oxygen"))) {
			o2 = 1000;
			he = 0;
			text += strlen(translate("gettextFromC", "oxygen"));
	} else if (!strncasecmp(text, translate("gettextFromC", "ean"), 3)) {
		o2 = get_permille(text + 3, &text);
		he = 0;
	} else {
		o2 = get_permille(text, &text);
		he = 0;
		if (*text == '/')
			he = get_permille(text + 1, &text);
	}

	/* We don't want any extra crud */
	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	/* Validate the gas mix */
	if (*text || o2 < 1 || o2 > 1000 || he < 0 || o2 + he > 1000)
		return 0;

	/* Let it rip */
	gas->o2.permille = o2;
	gas->he.permille = he;
	return 1;
}

int validate_po2(const char *text, int *mbar_po2)
{
	int po2;

	if (!text)
		return 0;

	po2 = get_tenths(text, &text);
	if (po2 < 0)
		return 0;

	while (isspace(*text))
		text++;

	while (isspace(*text))
		text++;
	if (*text)
		return 0;

	*mbar_po2 = po2 * 100;
	return 1;
}
